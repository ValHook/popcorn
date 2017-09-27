#  Imports
import re
import sys
import lxml.html as LH
import requests
from pprint import pprint

from lxml.cssselect import CSSSelector
from lxml.etree import tostring as htmlstring


from pyspark import SparkContext, SparkConf
from pyspark.sql import SQLContext
from pyspark.sql.types import (
    StructField,
    StringType,
    ArrayType,
    FloatType,
    StructType
    )

conf = SparkConf().setAppName("Allocine")
sc = SparkContext(conf=conf)
sqlContext = SQLContext(sc)


#  Constants
N_MOVIE_DIRECTORIES = 7085
ALLOCINE_URL_PREFIX = 'http://www.allocine.fr'
ALLOCINE_MOVIE_DIRECTORY_PREFIX = '/films/?page='
INFLUXDB_DB_NAME = 'allocine'
INFLUXDB_TABLE_NAME = 'allocine'
INFLUXDB_WRITE_URL = "http://localhost:8086/write?db=" + INFLUXDB_DB_NAME
INFLUXDB_QUERY_URL = "http://localhost:8086/query?db=" + INFLUXDB_DB_NAME


#  Globals
global current_chunk
current_chunk = 0


#  Functions
def count_in_a_partition(iterator):
    yield sum(1 for _ in iterator)


def repartitionate(rdd, tag):
    print("\tRepartitioning " + str(rdd.count()) + " " + str(tag) + " ...")
    rdd_partitions = rdd.getNumPartitions()
    rdd = rdd.partitionBy(rdd_partitions)
    print("\tNow parsing " + str(rdd.count()) + " repartitioned " + str(tag) + " ...")
    return rdd


def log_influxdb(tag):
	'''
	Logs a tag to a local influx DB database'
	tag: The tag string to log
	'''
	try:
		requests.post(INFLUXDB_WRITE_URL, INFLUXDB_TABLE_NAME + " " + str(tag) + "=1")
	except:
		a = 1+1

def clean_influxdb():
    '''
    Cleans the influxdb database
    '''
    requests.post(INFLUXDB_QUERY_URL, params={"q": "DELETE FROM " + INFLUXDB_TABLE_NAME + ";"}).text


def css_select(selector, dom):
    '''
    css selector
    '''
    from lxml.cssselect import CSSSelector as CS
    sel = CS(selector)
    return sel(dom)
    
def stripslashes(s):
	'''
	Strips Slashes
	'''
	r = re.sub(r"\\(n|r)", "\n", s)
	r = re.sub(r"\\", "", r)
	return r
	
def to_ascii(data):
	'''
	UTF-8 to ascii
	'''
	return data
	udata=data.decode("utf-8")
	asciidata=udata.encode("ascii","ignore")
	return asciidata
    
    
def directories_to_movies(url):
	'''
	Receives an Allocine directory page url and extracts all movies URL on that page
	'''
	try:
		content = requests.get(url, headers={'Accept-Encoding': 'identity'}).text
		log_influxdb("DIRECTORIES")
	except:
		return []
	dom = LH.fromstring(content)
	sel = CSSSelector('a.meta-title-link')
	movies = sel(dom)
	movies = [ALLOCINE_URL_PREFIX + result.get('href') for result in movies]
	return movies
	
def movie_details(url):
	'''
	Extracts movie info from URL
	'''
	try:
		content = requests.get(url, headers={'Accept-Encoding': 'identity'}).text
		dom = LH.fromstring(content)
		status = "RELEASED"
		rank = -1
		try:
			rank = float(re.findall(r'\d+', url)[0])
		except:
			log_influxdb("COULDNT_RANK")
			rank = -1.0
		try:
			length = re.findall('\(([0-9]+)h ([0-9]+)min\)', content)
			hours = float(length[0][0])
			minutes = float(length[0][1])
		except:
			status = "UNRELEASED"
			hours = minutes = float(-1)
			log_influxdb("UNRELEASED_MOVIES")
		try:
			score_press = float(css_select(".rating-holder .rating-item:nth-child(1) .stareval-note", dom)[0].text.replace(",", "."))
			reviews_press = float(css_select(".rating-holder .rating-item:nth-child(1) .stareval-review", dom)[0].text)
		except:
			score_press = reviews_press = float(-1)
		try:
			score_viewers = float(css_select(".rating-holder .rating-item:nth-child(2) .stareval-note", dom)[0].text.replace(",", "."))
			reviews_viewers = float(css_select('.rating-holder .rating-item:nth-child(2) .stareval-review [itemprop="ratingCount"]', dom)[0].text)
		except:
			score_viewers = reviews_viewers = float(-1)
		try:
			date = to_ascii(css_select(".date.blue-link", dom)[0].text)
		except:
			status = "NO RELEASE DATE"
			date = ""
			log_influxdb("NO RELEASE DATE")
		try:
			synopsis = to_ascii(css_select(".synopsis-txt", dom)[0].text)
		except:
			status = "NO SYNOPSIS"
			synopsis = ""
			log_influxdb("NO SYNOPSIS")
			
		title = to_ascii(css_select(".titlebar-title.titlebar-title-lg", dom)[0].text)
		cover = to_ascii(css_select(".card-movie-overview .thumbnail-img", dom)[0].get("src"))
		director = to_ascii(css_select('[itemprop="director"] [itemprop="name"]', dom)[0].text)
		genre = [to_ascii(result.text) for result in css_select('[itemprop="genre"]', dom)]
		nationalities = [to_ascii(result.text) for result in css_select(".blue-link.nationality", dom)]
		pictures = [to_ascii(result.get("data-src")) for result in css_select(".shot-img", dom)]
		actors = [to_ascii(result.text) for result in css_select(".card-movie-overview .meta-body .meta-body-item:nth-child(3) span.blue-link:not(.more)", dom)]
		misc = to_ascii(htmlstring(css_select(".ovw-synopsis-info", dom)[0]))
		
		try:
			trailer = ALLOCINE_URL_PREFIX + css_select(".trailer", dom)[0].get("href").replace("&amp;", "&");
			trailer = requests.get(trailer, headers={'Accept-Encoding': 'identity'}).text
			trailer_hd = re.findall("([\.\\\/0-9a-zA-Z_]+hd[\\/0-9a-zA-Z_]+\.mp4)", trailer)
			if len(trailer_hd):
				trailer = to_ascii(u'http:' + stripslashes(trailer_hd[0]))
			else:
				trailer = to_ascii(u'http:' + stripslashes(re.findall("([\.\\\/0-9a-zA-Z_]+[^k]\.mp4)", trailer)[0]))
		except:
			trailer = ""
			status = "MISSING TRAILER"
			log_influxdb("FAILED_TRAILERS")
			
		log_influxdb("MOVIES")
		return [status, hours, minutes, title, date, cover, director, genre, nationalities, score_press, reviews_press, score_viewers, reviews_viewers,
				pictures, actors, synopsis, misc, trailer, rank]
	except Exception as e:
		log_influxdb("FAILED_MOVIES")
		return []
		
#print(movie_details("http://www.allocine.fr/film/fichefilm_gen_cfilm=235200.html"))
#exit()

dirs = [(i, ALLOCINE_URL_PREFIX+ALLOCINE_MOVIE_DIRECTORY_PREFIX+str(i)) for i in range(1, N_MOVIE_DIRECTORIES + 1)]
dirs = sc.parallelize(dirs)
dirs = repartitionate(dirs, "directories")
movies = dirs.flatMapValues(directories_to_movies)
movies = repartitionate(movies, "movies")
movies = movies.mapValues(movie_details)
movies = movies.map(lambda c: c[1])
movies = movies.filter(lambda c: len(c) > 0)

schema = StructType([
    StructField("status", StringType(), True),
    StructField("hours", FloatType(), True),
    StructField("minutes", FloatType(), True),
    StructField("title", StringType(), True),
    StructField("date", StringType(), True),
    StructField("cover", StringType(), True),
    StructField("director", StringType(), True),
    StructField("genre", ArrayType(StringType(), True), True),
    StructField("nationalities", ArrayType(StringType(), True), True),
    StructField("score_press", FloatType(), True),
    StructField("reviews_press", FloatType(), True),
    StructField("score_viewers", FloatType(), True),
    StructField("reviews_viewers", FloatType(), True),
    StructField("pictures", ArrayType(StringType(), True), True),
    StructField("actors", ArrayType(StringType(), True), True),
    StructField("synopsis", StringType(), True),
    StructField("misc", StringType(), True),
    StructField("trailer", StringType(), True),
    StructField("rank", FloatType(), True)
])
df = sqlContext.createDataFrame(movies, schema)
if '--dry-run' not in sys.argv:
	df.write.json('dump')
else:
	movies.collect()


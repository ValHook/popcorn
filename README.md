# Popcorn DB
PopCorn DB [http://popcorn-db.net](http://popcorn-db.net) is a personal project which aims at recreating **from scrath** an IMDB like website with a machine learning layer on top of it.
**Therefore it includes the following features:**

### A fast & scalable web crawler.
*I used Apache Spark for parallel computing, and InfluxDB for logging in live its activity.
To reuse the CPU idle time when waiting for network responses I configured Spark to create 8 times more executors than CPU cores for each machine of the cluster*

### A blazingly fast custom built search-engine with fuzzy search and autocompletion.
*The average query time for 100K movies is 0.03ms. The speed is obtained by indexing every possible ngram of each movie title. The fuzzy search is done by building & exploring Levenstein automata on the go*

### A movie genre & nationality predictor
*I used a naive bayes network approach as it seemed after experimentation to be the best Machine Learning model adapted to this case*

### A web-server, socket-server and front-end
* The search engine and machine learning layers are written in C++. So I decided to build a web-server also in my C++ program. No need of Apache or nginx, less overhead = more speed*

# Presentation Slides
[https://www.hutworks.net/PopcornValentinMercierFinalProjectSlides.pdf](https://www.hutworks.net/PopcornValentinMercierFinalProjectSlides.pdf)

# Run
1. Install [https://github.com/uNetworking/uWebSockets](https://github.com/uNetworking/uWebSockets) and its dependencies
2. ```mkdir tmp && cd tmp && cmake .. && make && cd ..```
3. ```./AllocineBackend```
4. Visit [http://localhost:2200](http://localhost:2200)
5. Feel free to daeomonize the backend with an ```init.d``` service or even proxy/change the port to 80.

*You will need to re-crawl the movies because I could not upload the database and the images to this git repository since GitHub enforces a file/repo size limit. You might additionally want to disable the InfluxDB logging featured inside the crawler if you do not want to install InfluxDB*

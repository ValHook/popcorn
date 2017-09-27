#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <list>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <ctime>
#include <thread>
#include <regex>
#include <uWS/uWS.h>
#include "movie.hpp"
#include "levenshtein.hpp"

using namespace std;
using json = nlohmann::json;

inline void log_query_and_speed(string query, clock_t begin) {
    clock_t end = clock();
    double elapsed_ms = double(end - begin) / CLOCKS_PER_SEC * 1000;
    cout << "Processed query: " << query << " in " << elapsed_ms << " ms" << endl;
}


int main() {
    /* Variables and files for stats */
    unordered_map<string, vector<movie*>> ngrams;
    unordered_map<string, unordered_map<string, double>> genre_freq, nationality_freq;
    stack<movie, list<movie>> movies;
    ifstream jsonfile("db/fulldump.json");
    string line;
    int i = 0, number_of_movies = 88000;


    /* Load DB */
    if (!jsonfile.is_open()) {
        cerr << "Could not open db/fulldump.json" << endl;
        return EXIT_FAILURE;
    }
    while (getline(jsonfile, line)) {
        /* Progress */
        if (i++%2000 == 0) {
            cout << "\rLoading movies... " << i * 100 / number_of_movies << "% " << flush;
        }

        /* Parse and create movie */
        movies.push(json::parse(line));
        movie& m = movies.top();

        /* Compute and index n-grams of the titles */
        string title = m.title;
        stem(title);
        unordered_set<string> localngrams;
        for (auto j = title.begin(); j != title.end(); ++j) {
            for (auto k = j; k != title.end(); ++k) {
                string ngram = string(j, k+1);
                if (localngrams.find(ngram) != localngrams.end()) // Avoid mapping two similar n-grams to the same movie
                    continue;
                localngrams.insert(ngram);
                ngrams[ngram].push_back(&m); // Index movie to each n-gram of the title
            }
        }

        /* Extract words */
        istringstream title_ss(title);
        vector<string> words{istream_iterator<string>{title_ss}, istream_iterator<string>{}};
        words.erase (remove_if (words.begin (), words.end (), isACommonWord), words.end ());
        for (auto w: words) {
            for (auto g: m.genre)
                genre_freq[w][g] += (m.rank + 600000)/((double) 60000)/((double) words.size());
            for (auto g: m.nationalities)
                nationality_freq[w][g] += (m.rank + 600000)/((double) 60000)/((double) words.size());
        }
    }


    /* Pre-rank movies according to each n-gram */
    i = 0;
    cout << endl;
    size_t sz = ngrams.size();
    for (auto& e: ngrams) {
        // Progress
        if (i++%600000 == 0) {
            cout << "\rRanking movies... " << i * 100 / (sz-600000) << "% " << flush;
        }
        // Rank movies
        sort(e.second.begin(), e.second.end(), moviecomp_max());
    }
    cout << endl << "Normalizing frequencies...";
    /* Normalize frequencies */
    for (auto& gf: genre_freq) {
        double total = 0;
        for (auto &f: gf.second) total += f.second;
        for (auto &f: gf.second) f.second /= total;
    }
    for (auto& nf: nationality_freq) {
        double total = 0;
        for (auto &f: nf.second) total += f.second;
        for (auto &f: nf.second) f.second /= total;
    }
    cout << " ok" << endl;



    /* Close DB */
    jsonfile.close();
    /* Print aggregated stats */
    cout << endl << movies.size() << " movies";
    cout << endl << sz << " ngrams";
    cout << endl << "Server started !" << endl;



    /* START SERVER */
    uWS::Hub h;

    /* Web Socket Controller */
    h.onMessage([&ngrams, &genre_freq, &nationality_freq](uWS::WebSocket<uWS::SERVER> *ws, char *message, size_t length, uWS::OpCode opCode) {
        if (length > 80) {ws->send("", 0, opCode); return;}
        clock_t begin = clock();
        message[length] = '\0';
        char feature = message[0];
        string response;
        string input(message+1);
        stem(input);

        // Full Movie Detail
        if (feature == 'f') {
            if (ngrams[input].size()) {
                json j = *(ngrams[input][0]);
                response = j.dump();
            }
        }

        // Discover Feature
        else if (feature == 'd') {
            vector<movie *> &exact_movies = ngrams[input];
            // Extact Match
            if (ngrams[input].size() >= 6) {
                for (int i = 0; i < 6; ++i) {
                    movie *e = exact_movies[i];
                    response += e->title + "§" + e->local_cover + "§" + e->director + "§" + e->date + "\n";
                }
            }
            // Fuzzy Search
            else {
                fuzzy_search_context<movie *, moviecomp_min> ctx(input, &ngrams);
                vector<movie *> fuzzy_movies = ctx.best_matches(6, 1); // 6 results, Max levenstein distance = 1
                size_t fuzzy_count = fuzzy_movies.size() > 6 ? 6 : fuzzy_movies.size();
                for (int i = 0; i < fuzzy_count; ++i) {
                    movie *e = fuzzy_movies[i];
                    response += e->title + "§" + e->local_cover + "§" + e->director + "§" + e->date + "\n";
                }
            }
        }

        // Analyze feature
        else if (feature == 'a') {
            istringstream title_ss(input);
            vector<string> words{istream_iterator<string>{title_ss}, istream_iterator<string>{}};
            unordered_map<string, double> genre_proba, nationality_proba;
            vector<pair<double, string>> genre_res, nationality_res;
            double total;

            for (string& word: words) {
                if (isACommonWord(word))
                    continue;
                // genre analysis
                auto genre_freq_it = genre_freq.find(word);
                if (genre_freq_it == genre_freq.end())
                    continue;
                for (auto& a_genre_freq: (*genre_freq_it).second) {
                    genre_proba[a_genre_freq.first] += a_genre_freq.second;
                }

                // Nationality analysis
                auto nationality_freq_it = nationality_freq.find(word);
                if (nationality_freq_it == nationality_freq.end())
                    continue;
                for (auto& a_nationality_freq: (*nationality_freq_it).second) {
                    nationality_proba[a_nationality_freq.first] += a_nationality_freq.second;
                }
            }

            // Sort genre results
            total = 0;
            for (auto& gp: genre_proba) total += gp.second;
            for (auto& gp: genre_proba) genre_res.push_back(make_pair(gp.second/total, gp.first));
            sort(genre_res.rbegin(), genre_res.rend());
            genre_res.resize(min((size_t)4, genre_res.size())); // Keep 4 best genres

            // Sort nationality results
            total = 0;
            for (auto& np: nationality_proba) total += np.second;
            for (auto& np: nationality_proba) nationality_res.push_back(make_pair(np.second/total, np.first));
            sort(nationality_res.rbegin(), nationality_res.rend());
            nationality_res.resize(min((size_t)2, nationality_res.size())); // Keep 2 best nationalities

            // JSON output
            json j;
            j["query"] = string(message+1);
            j["genre"] = {};
            j["nationality"] = {};
            for (auto& gr: genre_res) if (gr.first > 0.01) j["genre"].push_back({{gr.second, (int) (gr.first*100)}});
            for (auto& nr: nationality_res) if (nr.first > 0.01) j["nationality"].push_back({{nr.second,(int) (nr.first*100)}});
            response = j.dump();
        }

        ws->send(response.data(), response.length(), opCode);
        log_query_and_speed(("WS "+string(message)).data(), begin);
    });




    /* HTTP Server Controller */
    h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t length, size_t remainingBytes) {
        // Parse request and discard malformed requests
        if (length > 80) {res->end(); return;}
        clock_t begin = clock();
        string url = req.getUrl().toString();
        string error404 = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nContent-type: text/plain\r\n\r\n";
        if (url == "/")  url = "/index.html";
        if (!regex_match(url, regex("/[a-zA-Z0-9]{1,15}\\.[a-z]{2,4}"))) {
            res->write(error404.data(), error404.length());
            res->end();
            log_query_and_speed(("HTTP "+url).data(), begin);
            return;
        }
        if (url.substr(url.size() - 3) == "jpg" || url.substr(url.size() - 3) == "png" || url.substr(url.size() - 3) == "ico") url = "/img"+url;
        if (url.substr(url.size() - 3) == "txt") url = "./stats"+url;
        else url = "./web"+url;
        ifstream in(url);

        // If requested file exists
        if (in.is_open()) {
            // Build HTTP headers
            string cache = "max-age=172800, public, must-revalidate";
            string content_type;
            if (url.substr(url.size() - 3) == "jpg") {
                content_type = "image/jpeg";
                cache = "max-age=31536000, public";
            } else if (url.substr(url.size() - 3) == "png" || url.substr(url.size() - 3) == "ico") {
                content_type = "image/png";
                cache = "max-age=31536000, public";
            } else if (url.substr(url.size() - 3) == "css")
                content_type = "text/css";
            else if (url.substr(url.size() - 4) == "html")
                content_type = "text/html; charset=utf-8";
            else if (url.substr(url.size() - 2) == "js")
                content_type = "application/javascript";
            else
                content_type = "text/plain; charset=utf-8";

            // Read file
            string str(static_cast<stringstream const&>(stringstream() << in.rdbuf()).str());
            string ok200 = "HTTP/1.1 200 OK\r\n"
                            "Content-Length: "+to_string(str.length())+"\r\n"
                            "Content-Type: "+content_type+"\r\n"
                            "Cache-Control: "+cache+"\r\n\r\n";

            // Send file contents
            res->write((ok200 + str).data(), ok200.length() + str.length());
            res->end();
            log_query_and_speed(("HTTP "+url).data(), begin);
            return;
        }
        // Or redirect to home page
        else {
            res->write(error404.data(), error404.length());
            res->end();
            log_query_and_speed(("HTTP "+url).data(), begin);
            return;
        }
    });

    if (!h.listen(2200))
        cout << "Failed to listen" << endl << "Server stopped" << endl;
    h.run();
    return 0;
}

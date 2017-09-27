//
// Created by Val on 7/28/17.
//

#ifndef ALLOCINEBACKEND_LEVENSHTEIN_HPP
#define ALLOCINEBACKEND_LEVENSHTEIN_HPP

#include <unordered_map>
#include <string>
#include <vector>
#include <queue>


/* Stemming */
bool isACommonWord(std::string& word) {
    return word == "and" || word == "the" || word == "of" || word == "de" || word == "l" || word == "l'" || word == "avec"
    || word == "for" || word == "pour" || word == "in" || word == "sur" || word == "on" || word == "under"
    || word == "sous" || word == "la" || word == "le" || word == "les" || word == "all" || word == "with"
    || word == "a" || word == "un" || word == "une" || word == "to" || word == "an" || word == "from";
}
bool BothAreSpaces(char lhs, char rhs) { return (lhs == rhs) && (lhs == ' '); }
void stem(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower); // Lowercase
    s.erase (std::remove_if (s.begin (), s.end (), [](const char c){
        return ispunct(c) || c == '\'' || c == '"'; // Remove punctuation & noise
    }), s.end ());
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end()); // Trim
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace)))); // Trim
    std::string::iterator new_end = std::unique(s.begin(), s.end(), BothAreSpaces); // Remove whitespaces
    s.erase(new_end, s.end());
}
std::string stem(const char*c) {
    std::string s(c);
    stem(s);
    return s;
}


/* Fuzzy search helper */
template <typename T, class C>
class fuzzy_search_context {
private:
    std::string original;
    std::unordered_map<std::string, std::vector<T>>* ngrams;
    std::vector<char> alphabet;
    C comp;

public:
    fuzzy_search_context(std::string original_p, std::unordered_map<std::string, std::vector<T>>* ngrams_p) {
        this->original = original_p;
        this->ngrams = ngrams_p;
    }
    std::vector<T> best_matches(int max_res, int max_dist) {
        char* placeholder = (char *)calloc(original.length()+5, sizeof(char));
        std::priority_queue<T, std::vector<T>, C> pq;
        std::unordered_set<T> seen;
        build_levenshtein_automaton(placeholder, 0, 0, max_dist, pq, max_res, seen);
        std::vector<T> results;
        while (!pq.empty()) {
            results.push_back(pq.top());
            pq.pop();
        }
        free(placeholder);
        return results;
    }

private:
    void build_levenshtein_automaton(char* placeholder, int original_pos, int placeholder_pos,
                                     int dist, std::priority_queue<T, std::vector<T>, C>&pq, int max_res,
                                     std::unordered_set<T>& seen) {
        // Exit condition + Indexation of results in priority queue
        if (original_pos == original.length()) {
            std::vector<T>& movies = (*ngrams)[std::string(placeholder)];
            size_t size = movies.size();
            for (int i = 0; i < size; ++i) {
                if (seen.find(movies[i]) != seen.end()) // Avoid likely doublons
                    continue;
                seen.insert(movies[i]);
                if (pq.size() < max_res) {
                    pq.push(movies[i]);
                } else if (comp(pq.top(), movies[i])) {
                    pq.pop();
                    pq.push(movies[i]);
                }
            }
            return;
        }

        // Recursive automaton builder
        placeholder[placeholder_pos] = original[original_pos];
        build_levenshtein_automaton(placeholder, original_pos+1, placeholder_pos+1, dist, pq, max_res, seen);
        if (dist > 0) {
            placeholder[placeholder_pos] = ' ';
            build_levenshtein_automaton(placeholder, original_pos+1, placeholder_pos+1, dist-1, pq, max_res, seen);
            build_levenshtein_automaton(placeholder, original_pos, placeholder_pos+1, dist-1, pq, max_res, seen);
            for (int i = 0; i < 26; ++i) {
                placeholder[placeholder_pos] = (char) ('a' + i);
                build_levenshtein_automaton(placeholder, original_pos+1, placeholder_pos+1, dist-1, pq, max_res, seen);
                build_levenshtein_automaton(placeholder, original_pos, placeholder_pos+1, dist-1, pq, max_res, seen);
            }
            placeholder[placeholder_pos] = '\0';
            build_levenshtein_automaton(placeholder, original_pos+1, placeholder_pos, dist-1, pq, max_res, seen);
        }
        placeholder[placeholder_pos] = '\0';
    }
};

#endif //ALLOCINEBACKEND_LEVENSHTEIN_HPP

//
// Created by Val on 7/27/17.
//

#ifndef ALLOCINEBACKEND_MOVIE_HPP
#define ALLOCINEBACKEND_MOVIE_HPP

#include <iostream>
#include <vector>
#include "json.hpp"


struct movie {
    int rank;
    float hours, minutes, reviews_press, score_press, reviews_viewers, score_viewers;
    std::string status, title, cover, local_cover, trailer, director, synopsis, misc, date;
    std::vector<std::string> pictures, actors, nationalities, genre;
};
struct moviecomp_max {
    bool operator() (const movie* lhs, const movie *rhs) const {
        return lhs->rank > rhs->rank;
    }
};
struct moviecomp_min {
    bool operator() (const movie* lhs, const movie *rhs) const {
        return lhs->rank < rhs->rank;
    }
};

void from_json(const nlohmann::json& j, movie& m) {
    m.hours = j.at("hours").get<float>();
    m.minutes = j.at("minutes").get<float>();
    m.reviews_press = j.at("reviews_press").get<float>();
    m.score_press = j.at("score_press").get<float>();
    m.reviews_viewers = j.at("reviews_viewers").get<float>();
    m.score_viewers = j.at("score_viewers").get<float>();

    m.status = j.at("status").get<std::string>();
    m.title = j.at("title").get<std::string>();
    m.date = j.at("date").get<std::string>();
    if (m.date == "") m.date = "Date de sortie inconnue";
    m.cover = j.at("cover").get<std::string>();
    m.local_cover = "/" + std::to_string((int) (j.at("rank").get<float>())) + ".jpg";
    m.trailer = j.at("trailer").get<std::string>();
    m.director = j.at("director").get<std::string>();
    m.synopsis = j.at("synopsis").get<std::string>();
    m.misc = j.at("misc").get<std::string>();

    m.rank = (int) (j.at("rank").get<float>() - (m.trailer == "" ? 600000 : 0) + m.reviews_viewers * 10 + m.score_viewers *10000);

    auto pictures = j.at("pictures");
    auto actors = j.at("actors");
    auto nationalities = j.at("nationalities");
    auto genre = j.at("genre");
    for (nlohmann::json::iterator it = pictures.begin(); it != pictures.end(); ++it)
        m.pictures.push_back(it->get<std::string>());
    for (nlohmann::json::iterator it = actors.begin(); it != actors.end(); ++it)
        try {m.actors.push_back(it->get<std::string>());} catch (std::domain_error e){}
    for (nlohmann::json::iterator it = nationalities.begin(); it != nationalities.end(); ++it)
        m.nationalities.push_back(std::string(it->get<std::string>().c_str()+1)); // The filedump nationalities have a ' ' at the begginning
    for (nlohmann::json::iterator it = genre.begin(); it != genre.end(); ++it)
        m.genre.push_back(it->get<std::string>());
}


void to_json(nlohmann::json& j, const movie& m) {
    j["trailer"] = m.trailer;
    j["cover"] = m.local_cover;
    j["synopsis"] = m.synopsis;
    j["misc"] = m.misc;
    j["status"] = m.status;
    j["hours"] = m.hours;
    j["minutes"] = m.minutes;
    j["reviews_viewers"] = m.reviews_viewers;
    j["score_viewers"] = m.score_viewers;
    j["director"] = m.director;
    j["actors"] = m.actors;
    j["title"] = m.title;
    j["nationalities"] = m.nationalities;
    j["genre"] = m.genre;
    j["pictures"] = m.pictures;
    j["date"] = m.date;
}

#endif //ALLOCINEBACKEND_MOVIE_HPP

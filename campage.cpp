#include "campage.h"
#include "image.h"
#include <iostream>

#include <cpr/cpr.h>

Campage::Campage() {
}

Campage::~Campage() {
}

void Campage::setRootPage(std::string const &url) {
    rootUrl_ = url;
}

void Campage::setImageUrlMatch(std::string const &url) {
    imagePrefix_ = url;
}

void Campage::addImageUrlReplace(std::string const &from, std::string const &to) {
    if (!from.empty()) {
        replacements_.push_back({from, to});
    }
}

size_t Campage::fetchSome() {
    if (rootUrl_.empty() || imagePrefix_.empty()) {
        return 0;
    }
    imageUrls.resize(0);
    cpr::Response r = cpr::Get(cpr::Url{rootUrl_}, cpr::Header{{"accept", "text/html"}});
    if (r.status_code != 200) {
        std::cerr << rootUrl_ << " status " << r.status_code << std::endl;
        return 0;
    }
    size_t pos = 0;
    std::string before(imagePrefix_);
    size_t star = before.find("*");
    if (star != std::string::npos) {
        before = before.substr(0, star);
    }
    while (true) {
        pos = r.text.find(before, pos);
        if (pos == std::string::npos) {
            break;
        }
        size_t p2 = pos + before.size();
        size_t sz = r.text.size();
        while (p2 != sz && r.text[p2] != '"' && r.text[p2] != '\'' && r.text[p2] != '>' && r.text[p2] != ' ') {
            ++p2;
        }
        std::string url = r.text.substr(pos, p2-pos);
        if (pattern_match(imagePrefix_, url)) {
            for (auto const &rep : replacements_) {
                size_t pos = 0;
                while (((pos = url.find(rep.first, pos))) != std::string::npos) {
                    url.replace(pos, pos+rep.first.size(), rep.second);
                    pos += rep.second.size();
                }
            }
            imageUrls.push_back(url);
        }
        pos = p2;
    }
    return imageUrls.size();
}

Image *Campage::fetch_image(std::string const &url) {
    cpr::Response r = cpr::Get(cpr::Url{url}, cpr::Header{{"accept", "image/*"}});
    if (r.status_code != 200) {
        std::cerr << url << " status " << r.status_code << std::endl;
        return 0;
    }
    Image *ret = new Image();
    if (!ret->load(&r.text[0], r.text.size())) {
        delete ret;
        return nullptr;
    }
    return ret;
}

bool Campage::pattern_match(std::string const &pattern, std::string const &data) {
    //  eek! TODO: FIXME: make this real
    return true;
}


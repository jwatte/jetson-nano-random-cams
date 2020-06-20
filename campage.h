#if !defined(campage_h)
#define campage_h

#include <string>
#include <vector>
#include <stdlib.h>


class Image;

//  This is a Cam Page. It's not some weird "camp"-age.
class Campage {
    public:
        Campage();
        ~Campage();

        void setRootPage(std::string const &url);
        void setImageUrlMatch(std::string const &url);
        void addImageUrlReplace(std::string const &from, std::string const &to);

        size_t fetchSome();
        std::vector<std::string> imageUrls;

        static Image *fetch_image(std::string const &url);
        static bool pattern_match(std::string const &pattern, std::string const &data);

        std::string rootUrl_;
        std::string imagePrefix_;
        std::vector<std::pair<std::string, std::string>> replacements_;

    private:
        Campage(Campage const &) = delete;
        Campage &operator=(Campage const &) = delete;
};

#endif

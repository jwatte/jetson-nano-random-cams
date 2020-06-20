#if !defined(image_h)
#define image_h

#include <stdlib.h>
#include <string>

class Image {
    public:
        Image();
        Image(std::string const &label);
        ~Image();
        bool load(void const *data, size_t filesize);
        void setLabel(std::string const &label);

        unsigned char const *data() const { return (unsigned char const *)data_; };
        int width() const { return width_; };
        int height() const { return height_; };
        int components() const { return components_; };
        char const *label() const { return label_.empty() ? nullptr : label_.c_str(); }

        void *data_ = nullptr;
        int width_ = 0;
        int height_ = 0;
        int components_ = 0;
        std::string label_;

    private:
        Image(Image const &) = delete;
        Image &operator=(Image const &) = delete;
};

bool fixupImage(Image *im, float strength = 0.5f);

#endif  //  image_h

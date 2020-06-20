#if !defined(image_h)
#define image_h

#include <stdlib.h>

class Image {
    public:
        Image();
        ~Image();
        bool load(void const *data, size_t filesize);

        unsigned char const *data() const { return (unsigned char const *)data_; };
        int width() const { return width_; };
        int height() const { return height_; };
        int components() const { return components_; };

        void *data_ = nullptr;
        int width_ = 0;
        int height_ = 0;
        int components_ = 0;

    private:
        Image(Image const &) = delete;
        Image &operator=(Image const &) = delete;
};

#endif  //  image_h

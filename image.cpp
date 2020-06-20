#include "image.h"
#include "stb_image.h"
#include <iostream>

Image::Image() {
}

Image::Image(std::string const &lab) {
    label_ = lab;
}

Image::~Image() {
    if (data_ != nullptr) {
        ::free(data_);
    }
}

void Image::setLabel(std::string const &l) {
    label_ = l;
}

bool Image::load(void const *data, size_t size) {
    if (data_ != nullptr) {
        ::free(data_);
    }
    components_ = 0;
    data_ = stbi_load_from_memory((stbi_uc const *)data, (int)size, &width_, &height_, &components_, 0);
    return data_ != nullptr;
}

static inline unsigned char clamp(float f) {
    if (f < 0) return 0;
    if (f > 255) return 255;
    return (unsigned char)(int(f) & 0xff);
}


bool fixupImage(Image *fi, float strength) {
    unsigned char const *data = fi->data();
    int comp = fi->components();
    int wid = fi->width();
    int hei = fi->height();

    int64_t sum_low = 0;
    int64_t count_low = 0;
    int64_t sum_high = 0;
    int64_t count_high = 0;
    int minval = 0xff;
    int maxval = 0;

#define COLLECT(x) \
    if (x < 0x80) { \
        sum_low += x; \
        count_low++; \
    } else { \
        sum_high += x; \
        count_high++; \
    } \
    if (x < minval) { \
        minval = x; \
    } \
    if (x > maxval) { \
        maxval = x; \
    }

    for (int row = 0; row != hei; ++row) {
        unsigned char const *ptr = data + row * wid * comp;
        switch (comp) {
            case 1:
                for (int col = 0; col != wid; ++col) {
                    COLLECT(ptr[0]);
                    ptr += 1;
                }
                break;
            case 2:
                for (int col = 0; col != wid; ++col) {
                    COLLECT(ptr[0]);
                    ptr += 2;
                }
                break;
            case 3:
                for (int col = 0; col != wid; ++col) {
                    COLLECT(ptr[0]);
                    COLLECT(ptr[1]);
                    COLLECT(ptr[2]);
                    ptr += 3;
                }
                break;
            case 4:
                for (int col = 0; col != wid; ++col) {
                    COLLECT(ptr[0]);
                    COLLECT(ptr[1]);
                    COLLECT(ptr[2]);
                    ptr += 4;
                }
                break;
        }
    }
    if (int(minval) + 40 > int(maxval)) {
        std::cerr << "minval " << minval << " maxval " << maxval << " bad contrast" << std::endl;
        return false;
    }

    float mul = 1.0f;
    float add = 0.0f;
    if (count_low > 3 * count_high) {   //  dark
        float avglo = double(sum_low) / double(count_low);
        if (avglo <= 20) {
            std::cerr << "minval " << minval << " maxval " << maxval << " count_low " << count_low << " count_high " << count_high << "avglo " << avglo << " too dark" << std::endl;
            return false;
        }
    }
    if (count_high > 10 * count_low) {    //  light
        float avghi = double(sum_high) / double(count_high);
        if (avghi >= 235) {
            std::cerr << "minval " << minval << " maxval " << maxval << " count_low " << count_low << " count_high " << count_high << " avghi " << avghi << " too light" << std::endl;
            return false;
        }
    }

    float avghi = double(sum_high) / double(count_high);
    float avglo = double(sum_low) / double(count_low);
    add = (64.0f - avglo) * strength;
    mul = (2.0f + 128.0f / (avghi - avglo)) * strength;
    /*
    std::cerr << "count_low " << count_low << " sum_low " << sum_low << " avglo " << avglo <<
        " count_high " << count_high << " sum_high " << sum_high << " avghi " << avghi <<
        " adjust: mul " << mul << " add " << add << std::endl;
    */

#define ADJUST(x) \
    x = clamp(float(x + add) * mul)

    for (int row = 0; row != hei; ++row) {
        unsigned char *ptr = (unsigned char *)data + row * wid * comp;
        switch (comp) {
            case 1:
                for (int col = 0; col != wid; ++col) {
                    ADJUST(ptr[0]);
                    ptr += 1;
                }
                break;
            case 2:
                for (int col = 0; col != wid; ++col) {
                    ADJUST(ptr[0]);
                    ptr += 2;
                }
                break;
            case 3:
                for (int col = 0; col != wid; ++col) {
                    ADJUST(ptr[0]);
                    ADJUST(ptr[1]);
                    ADJUST(ptr[2]);
                    ptr += 3;
                }
                break;
            case 4:
                for (int col = 0; col != wid; ++col) {
                    ADJUST(ptr[0]);
                    ADJUST(ptr[1]);
                    ADJUST(ptr[2]);
                    ptr += 4;
                }
                break;
        }
    }

    return true;
}



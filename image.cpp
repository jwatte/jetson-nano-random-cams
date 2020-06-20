#include "image.h"
#include "stb_image.h"

Image::Image() {
}

Image::~Image() {
    if (data_ != nullptr) {
        ::free(data_);
    }
}

bool Image::load(void const *data, size_t size) {
    if (data_ != nullptr) {
        ::free(data_);
    }
    components_ = 0;
    data_ = stbi_load_from_memory((stbi_uc const *)data, (int)size, &width_, &height_, &components_, 0);
    return data_ != nullptr;
}


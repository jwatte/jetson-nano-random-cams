#if !defined(recognize_h)
#define recognize_h

#include <stdlib.h>
#include <vector>
#include <string>

class Recognize {
    public:
        Recognize();
        ~Recognize();

        bool copyInput(void const *data, int width, int height, int components);

        float *inDataRGBA = nullptr;
        size_t inDataSize = 0;
        int inWidth = 0;
        int inHeight = 0;

    private:
        Recognize(Recognize const &) = delete;
        Recognize &operator=(Recognize const &) = delete;
};

class detectNet;

struct DetectBox {
    float left_;
    float right_;
    float top_;
    float bottom_;
    float confidence_;
    std::string className_;
};

class DetectModel {
    public:
        DetectModel();
        ~DetectModel();

        bool load(std::string const &s);

        detectNet *net = nullptr;

        size_t detect(Recognize const &r);
        std::vector<DetectBox> detections;

    private:
        DetectModel(DetectModel const &) = delete;
        DetectModel &operator=(DetectModel const &) = delete;
};

#endif  //  recognize_h

#include "recognize.h"
#include <jetson-inference/imageNet.h>
#include <jetson-utils/cudaMappedMemory.h>
#include <iostream>
#include <cuda.h>
#include <cuda_runtime.h>
#include <jetson-inference/detectNet.h>


Recognize::Recognize() {
}

Recognize::~Recognize() {
    if (inDataRGBA != nullptr) {
        cudaFreeHost(inDataRGBA);
        inDataRGBA = nullptr;
    }
}

//  packed -- no rowbytes
bool Recognize::copyInput(void const *data, int width, int height, int components) {
    if (inDataRGBA != nullptr) {
        cudaFreeHost(inDataRGBA);
        inDataRGBA = nullptr;
    }
    if (!cudaAllocMapped((void **)&inDataRGBA, 16*width*height)) {
        std::cerr << "cudaAllocMapped(" << 16*width*height << ") failed" << std::endl;
        return false;
    }
    inWidth = width;
    inHeight = height;
    float *d = inDataRGBA;
    for (int row = 0; row != height; ++row) {
        unsigned char const *src = (unsigned char const *)data + row * width * components;
        switch (components) {
            case 1:
                for (int col = 0; col != width; ++col) {
                    float f = float(*(src++));
                    *d++ = f;
                    *d++ = f;
                    *d++ = f;
                    *d++ = 1.0f;
                }
                break;
            case 2:
                for (int col = 0; col != width; ++col) {
                    float f = float(*(src++));
                    *d++ = f;
                    *d++ = f;
                    *d++ = f;
                    float a = float(*(src++));
                    *d++ = a;
                }
                break;
            case 3:
                for (int col = 0; col != width; ++col) {
                    *d++ = float(*(src++));
                    *d++ = float(*(src++));
                    *d++ = float(*(src++));
                    *d++ = 1.0f;
                }
                break;
            case 4:
                for (int col = 0; col != width; ++col) {
                    *d++ = float(*(src++));
                    *d++ = float(*(src++));
                    *d++ = float(*(src++));
                    *d++ = float(*(src++));
                }
                break;
            default:
                std::cerr << "number of components " << components << " is not a supported format" << std::endl;
                return false;
        }
    }
    return true;
}


DetectModel::DetectModel() {
}

DetectModel::~DetectModel() {
    if (net != nullptr) {
        delete net;
        net = nullptr;
    }
}

bool DetectModel::load(std::string const &model) {
    if (net != nullptr) {
        delete net;
        net = nullptr;
    }
    detectNet::NetworkType type = detectNet::NetworkTypeFromStr(model.c_str());
    if (type == detectNet::CUSTOM) {
        std::cerr << "model type '" << model << "' is not recognized" << std::endl;
        return false;
    }
    net = detectNet::Create(type);
    if (net == nullptr) {
        std::cerr << "could not load network '" << model << "'" << std::endl;
        return false;
    }
    return true;
}

size_t DetectModel::detect(Recognize const &r) {
    detections.resize(0);
    if (net == nullptr) {
        return 0;
    }
    detectNet::Detection *det = nullptr;
    int n = net->Detect(r.inDataRGBA, r.inWidth, r.inHeight, &det, 0);
    for (int i = 0; i != n; ++i) {
        detections.push_back(DetectBox{
                det[i].Left,
                det[i].Right,
                det[i].Top,
                det[i].Bottom,
                det[i].Confidence,
                net->GetClassDesc(det[i].ClassID)
        });
    }
    return size_t(n);
}

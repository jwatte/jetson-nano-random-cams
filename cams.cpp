#include "optlong.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "image.h"
#include "recognize.h"
#include "campage.h"
#include "flboximage.h"

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl.H>


Fl_Double_Window *window;
Fl_BoxImage *picturebox;
Fl_Multiline_Output *textoutput;
std::vector<std::string> textlines;
std::vector<DetectBox> indetections;

Options opt;
volatile bool progressChanged;
Image *nextImage;
volatile bool imageChanged;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

#define IMAGE_TIME 15

void progress(std::string prog) {
    std::cerr << "progress: " << prog << std::endl;
    pthread_mutex_lock(&mutex);
    textlines.push_back(prog);
    progressChanged = true;
    pthread_mutex_unlock(&mutex);
    Fl::awake();
}

void showImage(Image *im, std::vector<DetectBox> const &d) {
    pthread_mutex_lock(&mutex);
    nextImage = im;
    imageChanged = true;
    indetections = d;
    pthread_mutex_unlock(&mutex);
    Fl::awake();
}


static inline unsigned char clamp(float f) {
    if (f < 0) return 0;
    if (f > 255) return 255;
    return (unsigned char)(int(f) & 0xff);
}

static bool fixupImage(Image *fi) {
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
    add = (64.0f - avglo) * 0.33f;
    mul = (2.0f + 128.0f / (avghi - avglo)) * 0.33f;
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

static void *load_images(void *) {

    DetectModel m;

    if (!m.load(opt.get_string("network"))) {
        progress("must specify a valid network type, not " + opt.get_string("network"));
        return 0;
    }

    Campage cp;
    // cp.setRootPage("http://www.opentopia.com/hiddencam.php?showmode=standard&country=*&seewhat=highlyrated");
    cp.setImageUrlMatch("http://images.opentopia.com/cams/*/medium.jpg");
    cp.addImageUrlReplace("/medium.jpg", "/big.jpg");
    cp.addImageUrlReplace("/tiny.jpg", "/big.jpg");

    // http://cam6284208.miemasu.net/snapshotJPEG?Resolution=640x480&amp;Quality=Clarity&time=1592641287555&dummy=image.jpg

    int page = 1;
    while (true) {
        page++;
        if (page == 6) {
            page = 1;
        }
        std::string pstr("");
        if (page != 1) {
            pstr = "&p=" + std::to_string(page);
        }
        time_t t;
        time(&t);
        struct tm lt;
        lt = *gmtime(&t);
        if (lt.tm_hour >= 8 && lt.tm_hour < 16) {
            switch (page) {
                case 1:
                    cp.setRootPage("http://www.opentopia.com/hiddencam.php?showmode=standard&country=United+Kingdom&seewhat=oftenviewed");
                    break;
                case 2:
                    cp.setRootPage("http://www.opentopia.com/hiddencam.php?showmode=standard&country=Germany&seewhat=oftenviewed");
                    break;
                case 3:
                    cp.setRootPage("http://www.opentopia.com/hiddencam.php?showmode=standard&country=Austria&seewhat=oftenviewed");
                    break;
                case 4:
                    cp.setRootPage("http://www.opentopia.com/hiddencam.php?showmode=standard&country=Norway&seewhat=oftenviewed");
                    break;
                default:
                    cp.setRootPage("http://www.opentopia.com/hiddencam.php?showmode=standard&country=Sweden&seewhat=oftenviewed");
                    break;
            }
        } else if (lt.tm_hour >= 0 && lt.tm_hour < 8) {
            cp.setRootPage("http://www.opentopia.com/hiddencam.php?showmode=standard&country=Japan&seewhat=oftenviewed" + pstr);
        } else {
            cp.setRootPage("http://www.opentopia.com/hiddencam.php?showmode=standard&country=United+States&seewhat=oftenviewed" + pstr);
        }
        progress("fetching page: " + cp.rootUrl_);
        size_t n = cp.fetchSome();
        progress("got " + std::to_string(n)  + " urls from " + cp.rootUrl_);
        for (size_t i = 0; i != n; ++i) {
            progress("fetching image: " + cp.imageUrls[i]);
            Image *fi = cp.fetch_image(cp.imageUrls[i]);
            if (!fi) {
                progress("error downloading image: " + cp.imageUrls[i]);
                continue;
            } else {
                if (fixupImage(fi)) {
                    Recognize r;
                    if (!r.copyInput(fi->data(), fi->width(), fi->height(), fi->components())) {
                        progress("error preparing image: " + cp.imageUrls[i]);
                        delete fi;
                        continue;
                    }
                    size_t ndet = m.detect(r);
                    if (ndet == 0) {
                        progress("image " + cp.imageUrls[i] + " has no discernible objects");
                    } else {
                        progress("image " + cp.imageUrls[i] + " has " + std::to_string(ndet) + " objects");
                        std::string prog;
                        for (size_t j = 0; j != ndet; ++j) {
                            auto const &q = m.detections[j];
                            if (!prog.empty()) {
                                prog = prog + ", ";
                            }
                            prog = prog + q.className_;
                        }
                        progress(prog);
                    }
                    showImage(fi, m.detections);
                    sleep(IMAGE_TIME);
                } else {
                    progress("skipping image " + cp.imageUrls[i] + " because of bad quality");
                }
            }
        }
    }
    return nullptr;
}

void timeout_callback(void *) {
    std::string v;
    Image *newImg = nullptr;
    std::vector<DetectBox> dets;
    pthread_mutex_lock(&mutex);
    if (progressChanged) {
        progressChanged = false;
        while (textlines.size() > 30) {
            textlines.erase(textlines.begin());
        }
        for (auto const &a : textlines) {
            v += a;
            v += "\n";
        }
    }
    if (imageChanged) {
        imageChanged = false;
        dets = indetections;
        indetections.resize(0);
        newImg = nextImage;
        nextImage = nullptr;
    }
    pthread_mutex_unlock(&mutex);
    if (!v.empty()) {
        textoutput->value(v.c_str());
    }
    if (newImg != nullptr) {
        picturebox->boxes(dets);
        picturebox->image(newImg);
    }
    Fl::repeat_timeout(0.5, timeout_callback);
}

int main(int argc, char const **argv) {
    opt.define_string('n', "network", "ssd-mobilenet", "the network model name to use (multiped, facenet, ssd-inception, ssd-mobilenet, etc)");
    if (!opt.parse(&argc, argv)) {
        for (auto const & e : opt.errors) {
            std::cerr << e << std::endl;
        }
        return 1;
    }
    if (!opt.files.empty()) {
        std::cerr << "this program does not support inputs files" << std::endl;
        return 1;
    }

    window = new Fl_Double_Window(64, 400, 1800, 640, "Observations");
    window->color(FL_DARK3);

    picturebox = new Fl_BoxImage(10, 10, 880, 620, 0);

    textoutput = new Fl_Multiline_Output(900, 10, 890, 620, 0);
    textoutput->box(FL_FLAT_BOX);
    textoutput->color(FL_DARK3);
    textoutput->textcolor(FL_YELLOW);
    textoutput->textfont(FL_COURIER);
    textoutput->textsize(16);
    textoutput->value("Loading ML model ...");

    window->end();
    window->show();

    pthread_t pt;
    pthread_create(&pt, nullptr, load_images, nullptr);
    Fl::add_timeout(0.5, timeout_callback);
    Fl::run();

    return 0;
}


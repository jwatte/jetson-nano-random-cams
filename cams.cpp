#include "optlong.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <pthread.h>
#include <unistd.h>
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
std::string progressStr;
volatile bool progressChanged;
Image *nextImage;
volatile bool imageChanged;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

#define IMAGE_TIME 5

void progress(std::string prog) {
    pthread_mutex_lock(&mutex);
    progressStr = prog;
    progressChanged = true;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);
    Fl::awake();
}

void showImage(Image *im, std::vector<DetectBox> const &d) {
    pthread_mutex_lock(&mutex);
    nextImage = im;
    imageChanged = true;
    indetections = d;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);
    Fl::awake();
}


static void *load_images(void *) {

    DetectModel m;

    if (!m.load(opt.get_string("network"))) {
        progress("must specify a valid network type, not " + opt.get_string("network"));
        return 0;
    }

    Campage cp;
    cp.setRootPage("http://www.opentopia.com/hiddencam.php?showmode=standard&country=*&seewhat=newest");
    cp.setImageUrlMatch("http://images.opentopia.com/cams/*/medium.jpg");
    cp.addImageUrlReplace("/medium.jpg", "/big.jpg");
    cp.addImageUrlReplace("/tiny.jpg", "/big.jpg");

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
            Recognize r;
            if (!r.copyInput(fi->data(), fi->width(), fi->height(), fi->components())) {
                progress("error preparing image: " + cp.imageUrls[i]);
                delete fi;
                continue;
            }
            size_t ndet = m.detect(r);
            std::string prog;
            for (size_t j = 0; j != ndet; ++j) {
                auto const &q = m.detections[i];
                if (!prog.empty()) {
                    prog = prog + ", ";
                }
                prog = prog + q.className_;
            }
            if (prog.empty()) {
                progress("image " + cp.imageUrls[i] + " has no discernible objects");
            } else {
                progress(prog);
            }
            showImage(fi, m.detections);
            sleep(IMAGE_TIME);
        }
    }
    return nullptr;
}

void timeout_callback(void *) {
    std::string newProg;
    Image *newImg = nullptr;
    std::vector<DetectBox> dets;
    pthread_mutex_lock(&mutex);
    if (progressChanged) {
        progressChanged = false;
        newProg = progressStr;
        progressStr = "";
    }
    if (imageChanged) {
        imageChanged = false;
        dets = indetections;
        indetections.resize(0);
        newImg = nextImage;
        nextImage = nullptr;
    }
    pthread_mutex_unlock(&mutex);
    if (!newProg.empty()) {
        textlines.push_back(newProg);
        if (textlines.size() > 15) {
            textlines.erase(textlines.begin());
        }
        std::string v;
        for (auto const &a : textlines) {
            v += a;
            v += "\n";
        }
        textoutput->value(v.c_str());
    }
    if (newImg != nullptr) {
        picturebox->boxes(dets);
        picturebox->image(newImg);
    }
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

    window = new Fl_Double_Window(64, 320, 1800, 720, "Observations");

    picturebox = new Fl_BoxImage(10, 10, 880, 700, 0);

    textoutput = new Fl_Multiline_Output(900, 10, 890, 700, 0);
    textoutput->box(FL_FLAT_BOX);
    textoutput->color(FL_DARK3);
    textoutput->textcolor(FL_YELLOW);
    textoutput->textfont(FL_COURIER);
    textoutput->textsize(16);

    window->end();
    window->show();

    pthread_t pt;
    pthread_create(&pt, nullptr, load_images, nullptr);
    Fl::repeat_timeout(0.5, timeout_callback);
    Fl::run();

    return 0;
}


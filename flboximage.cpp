#include "flboximage.h"
#include <FL/fl_draw.H>

Fl_BoxImage::Fl_BoxImage(int x, int y, int w, int h, char const *label) :
    Fl_Widget(x, y, w, h, label),
    img_(nullptr),
    flimg_(nullptr),
    drawimg_(nullptr)
{
    box(FL_FLAT_BOX);
    color(FL_DARK3);
    align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    labelcolor(FL_GREEN);
    boxescolor(FL_GREEN);
}

Fl_BoxImage::~Fl_BoxImage()
{
    if (flimg_ != nullptr) {
        delete flimg_;
    }
    if (img_ != nullptr) {
        delete img_;
    }
    if (drawimg_ != nullptr) {
        delete drawimg_;
    }
}

void Fl_BoxImage::boxescolor(Fl_Color c)
{
    boxesColor_ = c;
    redraw();
}

void Fl_BoxImage::boxes(std::vector<DetectBox> const &d)
{
    boxes_ = d;
    redraw();
}

void Fl_BoxImage::image(Image *img)
{
    if (img_ != nullptr) {
        delete img_;
    }
    img_ = img;
    if (flimg_ != nullptr) {
        delete flimg_;
    }
    flimg_ = new Fl_RGB_Image(img->data(), img->width(), img->height(), img->components());
    if (drawimg_ != nullptr) {
        delete drawimg_;
        drawimg_ = nullptr;
    }
    redraw();
}

void Fl_BoxImage::draw()
{
    draw_box();
    if (flimg_ == nullptr) {
        return;
    }
    int ww = 0;
    int wh = 0;
    int gotw = w();
    int goth = h();
    int imgw = flimg_->w();
    int imgh = flimg_->h();
    wh = imgh * gotw / imgw;
    if (wh > goth) {
        wh = goth;
        ww = imgw * goth / imgh;
    } else {
        ww = imgw;
    }
    if (drawimg_ != nullptr && (drawimg_->w() != ww || drawimg_->h() != wh)) {
        delete drawimg_;
        drawimg_ = nullptr;
    }
    if (drawimg_ == nullptr) {
        drawimg_ = flimg_->copy(ww, wh);
    }
    int xpos = (gotw - ww) / 2 + x();
    int ypos = (goth - wh) / 2 + y();
    drawimg_->draw(xpos, ypos);
    fl_font(FL_HELVETICA, 16);
    fl_color(boxesColor_);
    for (auto const &p : boxes_) {
        int l = p.left_ * gotw / imgw + xpos;
        int t = p.top_ * goth / imgh + ypos;
        int w = (p.right_ - p.left_) * gotw / imgw;
        int h = (p.bottom_ - p.top_) * goth / imgh;
        fl_rect(l, t, w, h);
        fl_draw(p.className_.c_str(), l+2, t+2);
    }
}

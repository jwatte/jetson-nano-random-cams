#if !defined(Fl_BoxImage_H)
#define Fl_BoxImage_H

#include <Fl/Fl_Image.H>
#include <Fl/Fl_Widget.H>
#include "image.h"
#include "recognize.h"

class Fl_BoxImage : public Fl_Widget {
    public:
        Fl_BoxImage(int x, int y, int w, int h, char const *label);
        virtual ~Fl_BoxImage();

        void boxes(std::vector<DetectBox> const &d);
        void boxescolor(Fl_Color c);
        void image(Image *img);
        void draw() override;

        Image *img_;
        Fl_Image *flimg_ = nullptr;
        Fl_Image *drawimg_ = nullptr;
        std::vector<DetectBox> boxes_;
        Fl_Color boxesColor_;
};


#endif  //  Fl_BoxImage_H

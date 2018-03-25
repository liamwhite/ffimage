#include "ruby.h"

extern void Init_libjpeg();
extern void Init_librsvg();
extern void Init_libffmpeg();
extern void Init_libgif();
extern void Init_libpng();

void Init_ffimage()
{
    VALUE rb_cFFImage = rb_define_module("FFImage");
    Init_libjpeg(rb_cFFImage);
    Init_librsvg(rb_cFFImage);
    Init_libffmpeg(rb_cFFImage);
    Init_libgif(rb_cFFImage);
    Init_libpng(rb_cFFImage);
}

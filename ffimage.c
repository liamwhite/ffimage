#include "ruby.h"

VALUE rb_cFFImage;

extern void Init_libjpeg();
extern void Init_librsvg();

void Init_ffimage()
{
    rb_cFFImage = rb_define_module("FFImage");
    Init_libjpeg();
    Init_librsvg();
}

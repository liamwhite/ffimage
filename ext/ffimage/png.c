#include "ruby.h"
#include <libpng/png.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct libpng_object
{
    size_t png_size;
    void *png_buf; // original
    void *png_ptr; // streamed to end
    png_structp png;
    png_infop info;
};

static void png_input_func(png_structp png, png_bytep bytes, png_size_t len)
{
    struct libpng_object *obj = (struct libpng_object *) png_get_io_ptr(png);
    size_t real_len = MIN(len, obj->png_size);
    memcpy(bytes, obj->png_ptr, real_len);
    obj->png_ptr  += real_len;
    obj->png_size -= real_len;
}

static void png_error_func(png_structp png, const char *msg)
{
    rb_raise(rb_eRuntimeError, "%s", msg);
}

static void png_warning_func(png_structp png, const char *msg)
{
}

static void rb_libpng_deallocate(VALUE ptr)
{
    struct libpng_object *obj = (struct libpng_object *) ptr;

    if (obj->png) {
        png_destroy_read_struct(&obj->png, &obj->info, NULL);
    }
    xfree(obj->png_buf);
    xfree(obj);
}

static VALUE rb_libpng_allocate(VALUE klass)
{
    struct libpng_object *obj = ALLOC(struct libpng_object);
    return Data_Wrap_Struct(klass, NULL, rb_libpng_deallocate, obj);
}

static VALUE rb_libpng_initialize(VALUE self, VALUE buf)
{
    // Return code
    int rc = 0;

    // Data struct
    struct libpng_object *obj;
    Data_Get_Struct(self, struct libpng_object, obj);

    // Create our own copy of the image
    Check_Type(buf, T_STRING);
    obj->png_size   = RSTRING_LEN(buf);
    obj->png_buf    = xmalloc(obj->png_size);
    obj->png_ptr    = obj->png_buf;
    memcpy(obj->png_buf, RSTRING_PTR(buf), obj->png_size);

    obj->png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!obj->png) {
        rb_raise(rb_eRuntimeError, "Unknown error reading PNG");
    }

    obj->info = png_create_info_struct(obj->png);
    if (!obj->info) {
        rb_raise(rb_eRuntimeError, "Unknown error reading PNG");
    }

    // just in case there's one stuck in there
    if (setjmp(png_jmpbuf(obj->png))) {
        rb_raise(rb_eRuntimeError, "Error reading PNG");
    }

    png_set_error_fn(obj->png, NULL, png_error_func, png_warning_func);
    png_set_read_fn(obj->png, obj, png_input_func);
    png_read_info(obj->png, obj->info);

    return Qnil;
}

static VALUE rb_libpng_dimensions(VALUE self)
{
    struct libpng_object *obj;
    Data_Get_Struct(self, struct libpng_object, obj);

    size_t width  = png_get_image_width(obj->png, obj->info);
    size_t height = png_get_image_height(obj->png, obj->info);

    return rb_ary_new3(2, INT2FIX(width), INT2FIX(height));
}

extern void Init_libpng(VALUE rb_cFFImage)
{
    VALUE LibPNG = rb_define_class_under(rb_cFFImage, "LibPNG", rb_cObject);
    rb_define_alloc_func(LibPNG, rb_libpng_allocate);
    rb_define_method(LibPNG, "initialize", rb_libpng_initialize, 1);
    rb_define_method(LibPNG, "dimensions", rb_libpng_dimensions, 0);
}

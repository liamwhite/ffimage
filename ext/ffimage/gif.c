#include "ruby.h"
#include <stdlib.h>
#include <gif_lib.h>

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct libgif_object
{
    size_t gif_size;
    void *gif_buf; // original
    void *gif_ptr; // streamed to end
    GifFileType *file_handle;
};

static int gif_input_func(GifFileType *file_type, GifByteType *byte_type, int length)
{
    struct libgif_object *obj = (struct libgif_object *) file_type->UserData;
    size_t real_len = MIN(length, obj->gif_size);
    memcpy(byte_type, obj->gif_ptr, real_len);
    obj->gif_ptr  += real_len;
    obj->gif_size -= real_len;
    return real_len;
}

static void rb_libgif_deallocate(VALUE ptr)
{
    struct libgif_object *obj = (struct libgif_object *) ptr;

    xfree(obj->gif_buf);
    xfree(obj);
}

static VALUE rb_libgif_allocate(VALUE klass)
{
    struct libgif_object *obj = ALLOC(struct libgif_object);
    return Data_Wrap_Struct(klass, NULL, rb_libgif_deallocate, obj);
}

static VALUE rb_libgif_initialize(VALUE self, VALUE buf)
{
    // Return code
    int rc = 0;

    // Data struct
    struct libgif_object *obj;
    Data_Get_Struct(self, struct libgif_object, obj);

    // Create our own copy of the image
    Check_Type(buf, T_STRING);
    obj->gif_size   = RSTRING_LEN(buf);
    obj->gif_buf    = xmalloc(obj->gif_size);
    obj->gif_ptr    = obj->gif_buf;
    memcpy(obj->gif_buf, RSTRING_PTR(buf), obj->gif_size);

    obj->file_handle = DGifOpen(obj, gif_input_func, &rc);
    if (!obj->file_handle) {
        rb_raise(rb_eRuntimeError, "Error reading GIF: %s", GifErrorString(rc));
    }

    return Qnil;
}

static VALUE rb_libgif_dimensions(VALUE self)
{
    struct libgif_object *obj;
    Data_Get_Struct(self, struct libgif_object, obj);

    size_t width  = obj->file_handle->SWidth;
    size_t height = obj->file_handle->SHeight;

    return rb_ary_new3(2, INT2FIX(width), INT2FIX(height));
}

extern void Init_libgif(VALUE rb_cFFImage)
{
    VALUE LibGIF = rb_define_class_under(rb_cFFImage, "LibGIF", rb_cObject);
    rb_define_alloc_func(LibGIF, rb_libgif_allocate);
    rb_define_method(LibGIF, "initialize", rb_libgif_initialize, 1);
    rb_define_method(LibGIF, "dimensions", rb_libgif_dimensions, 0);
}

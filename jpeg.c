#include "ruby.h"
#include <jpeglib.h>

extern VALUE rb_cFFImage;

struct libjpeg_object
{
    size_t jpg_size;
    void *jpg_buf;
    struct jpeg_error_mgr jerr;
    struct jpeg_decompress_struct cinfo;
};

static void rb_libjpeg_deallocate(VALUE ptr)
{
    struct libjpeg_object *obj = (struct libjpeg_object *) ptr;

    jpeg_destroy_decompress(&obj->cinfo);
    xfree(obj->jpg_buf);
    xfree(obj);
}

static VALUE rb_libjpeg_allocate(VALUE klass)
{
    struct libjpeg_object *obj = ALLOC(struct libjpeg_object);
    return Data_Wrap_Struct(klass, NULL, rb_libjpeg_deallocate, obj);
}

static void rb_libjpeg_error(j_common_ptr cinfo)
{
    char last_error[JMSG_LENGTH_MAX] = { 0 };
    cinfo->err->format_message(cinfo, last_error);

    rb_raise(rb_eException, "%s", last_error);
}

static VALUE rb_libjpeg_initialize(VALUE self, VALUE buf)
{
    // Return code
    int rc;

    // Data struct
    struct libjpeg_object *obj;
    Data_Get_Struct(self, struct libjpeg_object, obj);

    // Create our own copy of the image
    Check_Type(buf, T_STRING);
    obj->jpg_size   = RSTRING_LEN(buf);
    obj->jpg_buf    = xmalloc(obj->jpg_size);
    memcpy(obj->jpg_buf, RSTRING_PTR(buf), obj->jpg_size);

    // Set up error handler
    obj->cinfo.err = jpeg_std_error(&obj->jerr);
    obj->jerr.error_exit = rb_libjpeg_error;

    // Read the JPEG
    jpeg_create_decompress(&obj->cinfo);
    jpeg_mem_src(&obj->cinfo, obj->jpg_buf, obj->jpg_size);

    rc = jpeg_read_header(&obj->cinfo, TRUE);
    if (rc != 1) {
        jpeg_destroy_decompress(&obj->cinfo);
        rb_raise(rb_eException, "Failed to read JPEG");
    }

    jpeg_start_decompress(&obj->cinfo);

    return Qnil;
}

static VALUE rb_libjpeg_dimensions(VALUE self)
{
    struct libjpeg_object *obj;
    Data_Get_Struct(self, struct libjpeg_object, obj);

    size_t width  = obj->cinfo.output_width;
    size_t height = obj->cinfo.output_height;

    return rb_ary_new3(2, INT2FIX(width), INT2FIX(height));
}

extern void Init_libjpeg()
{
    VALUE LibJPEG = rb_define_class_under(rb_cFFImage, "LibJPEG", rb_cObject);
    rb_define_alloc_func(LibJPEG, rb_libjpeg_allocate);
    rb_define_method(LibJPEG, "initialize", rb_libjpeg_initialize, 1);
    rb_define_method(LibJPEG, "dimensions", rb_libjpeg_dimensions, 0);
}

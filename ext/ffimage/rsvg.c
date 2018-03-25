#include "ruby.h"
#include <librsvg/rsvg.h>

struct librsvg_object
{
    size_t svg_size;
    void *svg_buf;
    RsvgHandle *handle;
};

static void rb_librsvg_deallocate(VALUE ptr)
{
    struct librsvg_object *obj = (struct librsvg_object *) ptr;

    if (obj->handle) {
        g_object_unref(obj->handle);
    }

    xfree(obj->svg_buf);
    xfree(obj);
}

static VALUE rb_librsvg_allocate(VALUE klass)
{
    struct librsvg_object *obj = ALLOC(struct librsvg_object);
    return Data_Wrap_Struct(klass, NULL, rb_librsvg_deallocate, obj);
}

static VALUE rb_librsvg_initialize(VALUE self, VALUE buf)
{
    // Error
    GError *error = NULL;

    // Data struct
    struct librsvg_object *obj;
    Data_Get_Struct(self, struct librsvg_object, obj);

    // Create our own copy of the image
    Check_Type(buf, T_STRING);
    obj->svg_size   = RSTRING_LEN(buf);
    obj->svg_buf    = xmalloc(obj->svg_size);
    memcpy(obj->svg_buf, RSTRING_PTR(buf), obj->svg_size);

    // Read the SVG
    obj->handle = rsvg_handle_new_with_flags(RSVG_HANDLE_FLAG_UNLIMITED);
    rsvg_handle_write(obj->handle, obj->svg_buf, obj->svg_size, &error);

    if (error) {
        rb_raise(rb_eRuntimeError, "%s", error->message);
    }

    error = NULL;
    rsvg_handle_close(obj->handle, &error);

    if (error) {
        rb_raise(rb_eRuntimeError, "%s", error->message);
    }

    return Qnil;
}

static VALUE rb_librsvg_dimensions(VALUE self)
{
    struct librsvg_object *obj;
    Data_Get_Struct(self, struct librsvg_object, obj);

    RsvgDimensionData dimensions;
    rsvg_handle_get_dimensions(obj->handle, &dimensions);

    size_t width  = dimensions.width;
    size_t height = dimensions.height;

    return rb_ary_new3(2, INT2FIX(width), INT2FIX(height));
}

extern void Init_librsvg(VALUE rb_cFFImage)
{
    VALUE LibRSVG = rb_define_class_under(rb_cFFImage, "LibRSVG", rb_cObject);
    rb_define_alloc_func(LibRSVG, rb_librsvg_allocate);
    rb_define_method(LibRSVG, "initialize", rb_librsvg_initialize, 1);
    rb_define_method(LibRSVG, "dimensions", rb_librsvg_dimensions, 0);
}

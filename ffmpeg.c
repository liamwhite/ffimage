#include "ruby.h"
#include <libavformat/avformat.h>

extern VALUE rb_cFFImage;

struct libavformat_object
{
    size_t vid_size;
    void *vid_buf; // original
    void *vid_ptr; // streamed to end
    AVFormatContext *format;
    AVIOContext *avio;
    uint8_t *avio_ctx_buffer;
};

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    struct libavformat_object *obj = (struct libavformat_object *)opaque;
    buf_size = FFMIN(buf_size, obj->vid_size);
    memcpy(buf, obj->vid_ptr, buf_size);
    obj->vid_ptr  += buf_size;
    obj->vid_size -= buf_size;
    return buf_size;
}

static void rb_libavformat_deallocate(VALUE ptr)
{
    struct libavformat_object *obj = (struct libavformat_object *) ptr;

    avformat_close_input(&obj->format);
    av_freep(&obj->avio->buffer);
    av_freep(&obj->avio);
    av_freep(&obj->vid_buf);
    xfree(obj);
}

static VALUE rb_libavformat_allocate(VALUE klass)
{
    struct libavformat_object *obj = ALLOC(struct libavformat_object);
    return Data_Wrap_Struct(klass, NULL, rb_libavformat_deallocate, obj);
}

static VALUE rb_libavformat_initialize(VALUE self, VALUE buf)
{
    // Data struct
    struct libavformat_object *obj;
    Data_Get_Struct(self, struct libavformat_object, obj);

    // Create our own copy of the video
    Check_Type(buf, T_STRING);
    obj->vid_size   = RSTRING_LEN(buf);
    obj->vid_buf    = av_malloc(obj->vid_size);
    obj->vid_ptr    = obj->vid_buf;
    memcpy(obj->vid_buf, RSTRING_PTR(buf), obj->vid_size);

    // Read the video
    obj->format = avformat_alloc_context();
    obj->avio_ctx_buffer = av_malloc(4096);
    obj->avio   = avio_alloc_context(obj->avio_ctx_buffer, 4096, 0, obj, &read_packet, NULL, NULL);
    obj->format->pb = obj->avio;

    int ret;

    if ((ret = avformat_open_input(&obj->format, NULL, NULL, NULL))) {
        rb_raise(rb_eException, "%s", av_err2str(ret));
    }

    return Qnil;
}

static VALUE rb_libavformat_dimensions(VALUE self)
{
    struct libavformat_object *obj;
    Data_Get_Struct(self, struct libavformat_object, obj);

    int ret;

    if ((ret = avformat_find_stream_info(obj->format, NULL))) {
        rb_raise(rb_eException, "%s", av_err2str(ret));
    }

    int stream_index = -1;

    for (int i = 0; i < obj->format->nb_streams; ++i) {
        if (obj->format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            stream_index = i;
            break;
        }
    }

    if (stream_index == -1) {
        rb_raise(rb_eException, "Could not find suitable video stream");
    }

    AVStream* stream = obj->format->streams[stream_index];
    AVCodecParameters* codec = stream->codecpar;

    size_t width  = codec->width;
    size_t height = codec->height;

    return rb_ary_new3(2, INT2FIX(width), INT2FIX(height));
}

extern void Init_libffmpeg()
{
    av_register_all();
    VALUE LibFFMpeg = rb_define_class_under(rb_cFFImage, "LibFFMpeg", rb_cObject);
    rb_define_alloc_func(LibFFMpeg, rb_libavformat_allocate);
    rb_define_method(LibFFMpeg, "initialize", rb_libavformat_initialize, 1);
    rb_define_method(LibFFMpeg, "dimensions", rb_libavformat_dimensions, 0);
}

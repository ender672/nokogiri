#include <html_sax_native_parser.h>

static VALUE
initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE _filename, encoding, doc;
    const char *filename = NULL;
    xmlSAXHandlerPtr sax;
    htmlParserCtxtPtr ctx;
    xmlCharEncoding enc;

    rb_scan_args(argc, argv, "12", &doc, &_filename, &encoding);

    if (!NIL_P(_filename))
	filename = StringValuePtr(_filename);

    if (!NIL_P(encoding))
	enc = (xmlCharEncoding)NUM2INT(encoding);

    //FIXME: make sure to free the existing DATA_PTR first, if exists.
    sax = create_sax_handler_callbacks(doc);

    ctx = htmlCreatePushParserCtxt(sax, NULL, NULL, 0, filename, enc);
    if (!ctx)
      rb_raise(rb_eRuntimeError, "Could not create a parser context");

    ctx->sax2 = 1;
    DATA_PTR(self) = ctx;

    return self;
}

static void
write2(xmlParserCtxtPtr ctx, const char *chunk, int size, int last_chunk)
{
    if (htmlParseChunk(ctx, chunk, size, last_chunk)) {
	if (!(ctx->options & XML_PARSE_RECOVER)) {
	    xmlErrorPtr e = xmlCtxtGetLastError(ctx);
	    Nokogiri_error_raise(NULL, e);
	}
    }
}

/*
 * call-seq:
 *  native_write(chunk, last_chunk)
 *
 * Write +chunk+ to PushParser. +last_chunk+ triggers the end_document handle
 */
static VALUE
write(int argc, VALUE *argv, VALUE self)
{
    VALUE _chunk, _last_chunk;
    xmlParserCtxtPtr ctx;
    const char *chunk = NULL;
    int size = 0;

    Data_Get_Struct(self, xmlParserCtxt, ctx);

    rb_scan_args(argc, argv, "11", &_chunk, &_last_chunk);

    if (!NIL_P(_chunk)) {
	chunk = StringValuePtr(_chunk);
	size = (int)RSTRING_LEN(_chunk);
    }

    write2(ctx, chunk, size, !NIL_P(_last_chunk));

    return self;
}

static VALUE
finish(VALUE self)
{
    xmlParserCtxtPtr ctx;
    Data_Get_Struct(self, xmlParserCtxt, ctx);
    write2(ctx, NULL, 0, 1);
    return self;
}

VALUE cNokogiriHtmlSaxNativeParser;

void
init_html_sax_native_parser()
{
    VALUE nokogiri = rb_define_module("Nokogiri");
    VALUE html = rb_define_module_under(nokogiri, "HTML");
    VALUE sax = rb_define_module_under(html, "SAX");
    VALUE klass = rb_define_class_under(sax, "NativeParser",
					cNokogiriXmlSaxNativeParser);

    cNokogiriHtmlSaxNativeParser = klass;

    rb_define_method(klass, "initialize", initialize, -1);
    rb_define_method(klass, "write", write, -1);
    rb_define_method(klass, "finish", finish, 0);
}

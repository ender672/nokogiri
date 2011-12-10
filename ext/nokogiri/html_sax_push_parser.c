#include <html_sax_push_parser.h>

static VALUE
initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE _filename, encoding, doc;
    const char *filename = NULL;
    htmlParserCtxtPtr ctx;
    xmlCharEncoding enc = XML_CHAR_ENCODING_NONE;
    struct xml_sax_parser_data *data;

    rb_scan_args(argc, argv, "12", &doc, &_filename, &encoding);

    if (!NIL_P(_filename))
	filename = StringValuePtr(_filename);

    if (!NIL_P(encoding)) {
	enc = xmlParseCharEncoding(StringValuePtr(encoding));
	if (enc == XML_CHAR_ENCODING_ERROR)
	    rb_raise(rb_eArgError, "Unsupported encoding");
    }

    Data_Get_Struct(self, struct xml_sax_parser_data, data);

    ctx = htmlCreatePushParserCtxt(&nokogiriSAXHandlerPrototype, (void *)data,
				   NULL, 0, filename, enc);
    if (!ctx)
	rb_raise(rb_eRuntimeError, "Could not create a parser context");

    if (data->ctx)
	xmlFreeParserCtxt(data->ctx);

    ctx->sax2 = 1;
    data->handler = doc;
    data->ctx = (xmlParserCtxtPtr)ctx;

    return self;
}

static VALUE
write2(VALUE self, const char *chunk, int size, int last_chunk)
{
    struct xml_sax_parser_data *data;
    int status, ret;
    Data_Get_Struct(self, struct xml_sax_parser_data, data);

    if (!data->ctx)
      rb_raise(rb_eRuntimeError, "Parser is Uninitialized.");

    ret = htmlParseChunk(data->ctx, chunk, size, last_chunk);

    if (data->status) {
	status = data->status;
        data->status = 0;
	rb_jump_tag(status);
    }

    if (ret && !(data->ctx->options & XML_PARSE_RECOVER)) {
	xmlErrorPtr e = xmlCtxtGetLastError(data->ctx);
	Nokogiri_error_raise(NULL, e);
    }

    return self;
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
    const char *chunk = NULL;
    int size = 0;

    rb_scan_args(argc, argv, "11", &_chunk, &_last_chunk);

    if (!NIL_P(_chunk)) {
	chunk = StringValuePtr(_chunk);
	size = (int)RSTRING_LEN(_chunk);
    }

    return write2(self, chunk, size, !NIL_P(_last_chunk));
}

static VALUE
finish(VALUE self)
{
    return write2(self, NULL, 0, 1);
}

VALUE cNokogiriHtmlSaxPushParser;

void
init_html_sax_push_parser()
{
    VALUE nokogiri = rb_define_module("Nokogiri");
    VALUE html = rb_define_module_under(nokogiri, "HTML");
    VALUE sax = rb_define_module_under(html, "SAX");
    VALUE klass = rb_define_class_under(sax, "PushParser",
					cNokogiriXmlSaxPushParser);

    cNokogiriHtmlSaxPushParser = klass;

    rb_define_method(klass, "initialize", initialize, -1);
    rb_define_method(klass, "write", write, -1);
    rb_define_method(klass, "finish", finish, 0);
}

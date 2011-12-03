#include <html_sax_push_parser.h>
#include <xml_sax_push_parser.h>

/*
 * call-seq:
 *  native_write(chunk, last_chunk)
 *
 * Write +chunk+ to PushParser. +last_chunk+ triggers the end_document handle
 */
static VALUE
native_write(VALUE self, VALUE _chunk, VALUE _last_chunk)
{
    htmlParserCtxtPtr ctx;
    const char * chunk  = NULL;
    int size            = 0;

    Data_Get_Struct(self, htmlParserCtxt, ctx);

    if(Qnil != _chunk) {
	chunk = StringValuePtr(_chunk);
	size = (int)RSTRING_LEN(_chunk);
    }

    if (htmlParseChunk(ctx, chunk, size, Qtrue == _last_chunk ? 1 : 0)) {
	if (!(ctx->options & XML_PARSE_RECOVER)) {
	    xmlErrorPtr e = xmlCtxtGetLastError(ctx);
	    Nokogiri_error_raise(NULL, e);
	}
    }

    return self;
}

/*
 * call-seq:
 *  initialize_native(xml_sax, filename)
 *
 * Initialize the push parser with +xml_sax+ using +filename+
 */
static VALUE
initialize_native(VALUE self, VALUE _xml_sax, VALUE _filename, VALUE encoding)
{
    xmlSAXHandlerPtr sax;
    const char * filename = NULL;
    htmlParserCtxtPtr ctx;
    xmlCharEncoding enc = (xmlCharEncoding)NUM2INT(encoding);

    Data_Get_Struct(_xml_sax, xmlSAXHandler, sax);

    if (_filename != Qnil)
	filename = StringValuePtr(_filename);

    ctx = htmlCreatePushParserCtxt(sax, NULL, NULL, 0, filename, enc);

    if (ctx == NULL)
	rb_raise(rb_eRuntimeError, "Could not create a parser context");

    ctx->userData = NOKOGIRI_SAX_TUPLE_NEW(ctx, self);

    ctx->sax2 = 1;
    DATA_PTR(self) = ctx;
    return self;
}

VALUE cNokogiriHtmlSaxPushParser;

void
init_html_sax_push_parser()
{
    VALUE nokogiri = rb_define_module("Nokogiri");
    VALUE html = rb_define_module_under(nokogiri, "HTML");
    VALUE sax = rb_define_module_under(html, "SAX");
    VALUE klass = rb_define_class_under(sax, "PushParser", cNokogiriXmlSaxPushParser);

    cNokogiriHtmlSaxPushParser = klass;

    rb_define_private_method(klass, "initialize_native", initialize_native, 3);
    rb_define_private_method(klass, "native_write", native_write, 2);
}

#include <xml_sax_push_parser.h>

static void
mark(xmlParserCtxtPtr ctx)
{
    if (ctx)
	rb_gc_mark((VALUE)ctx->sax->_private);
}

static void
deallocate(xmlParserCtxtPtr ctx)
{
    NOKOGIRI_DEBUG_START(ctx);
    if (ctx)
	xmlFreeParserCtxt(ctx);
    NOKOGIRI_DEBUG_END(ctx);
}

static VALUE
allocate(VALUE klass)
{
    return Data_Wrap_Struct(klass, mark, deallocate, NULL);
}

static VALUE
initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE _filename, doc;
    const char *filename = NULL;
    xmlSAXHandlerPtr sax;
    xmlParserCtxtPtr ctx;

    rb_scan_args(argc, argv, "11", &doc, &_filename);

    if (!NIL_P(_filename))
	filename = StringValuePtr(_filename);

    //FIXME: make sure to free the existing DATA_PTR first, if exists.
    sax = create_sax_handler_callbacks(doc);

    ctx = xmlCreatePushParserCtxt(sax, NULL, NULL, 0, filename);
    free(sax);
    if (!ctx)
      rb_raise(rb_eRuntimeError, "Could not create a parser context");

    ctx->sax2 = 1;
    DATA_PTR(self) = ctx;

    return self;
}

static void
write2(xmlParserCtxtPtr ctx, const char *chunk, int size, int last_chunk)
{
    if (xmlParseChunk(ctx, chunk, size, last_chunk)) {
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

static VALUE get_options(VALUE self)
{
  xmlParserCtxtPtr ctx;
  Data_Get_Struct(self, xmlParserCtxt, ctx);

  return INT2NUM(ctx->options);
}

static VALUE set_options(VALUE self, VALUE options)
{
  xmlParserCtxtPtr ctx;
  Data_Get_Struct(self, xmlParserCtxt, ctx);

  if (xmlCtxtUseOptions(ctx, (int)NUM2INT(options)) != 0)
    rb_raise(rb_eRuntimeError, "Cannot set XML parser context options");

  return Qnil;
}

/*
 * call-seq:
 *  replace_entities=(boolean)
 *
 * Should this parser replace entities?  &amp; will get converted to '&' if
 * set to true
 */
static VALUE set_replace_entities(VALUE self, VALUE value)
{
  xmlParserCtxtPtr ctxt;
  Data_Get_Struct(self, xmlParserCtxt, ctxt);

  if(Qfalse == value)
    ctxt->replaceEntities = 0;
  else
    ctxt->replaceEntities = 1;

  return value;
}

/*
 * call-seq:
 *  replace_entities
 *
 * Should this parser replace entities?  &amp; will get converted to '&' if
 * set to true
 */
static VALUE get_replace_entities(VALUE self)
{
  xmlParserCtxtPtr ctxt;
  Data_Get_Struct(self, xmlParserCtxt, ctxt);

  if(0 == ctxt->replaceEntities)
    return Qfalse;
  else
    return Qtrue;
}

/*
 * call-seq: line
 *
 * Get the current line the parser context is processing.
 */
static VALUE line(VALUE self)
{
  xmlParserCtxtPtr ctxt;
  xmlParserInputPtr io;

  Data_Get_Struct(self, xmlParserCtxt, ctxt);

  io = ctxt->input;
  if(io)
    return INT2NUM(io->line);

  return Qnil;
}

/*
 * call-seq: column
 *
 * Get the current column the parser context is processing.
 */
static VALUE column(VALUE self)
{
  xmlParserCtxtPtr ctxt;
  xmlParserInputPtr io;

  Data_Get_Struct(self, xmlParserCtxt, ctxt);

  io = ctxt->input;
  if(io)
    return INT2NUM(io->col);

  return Qnil;
}

static VALUE
document(VALUE self)
{
    xmlParserCtxtPtr ctxt;
    Data_Get_Struct(self, xmlParserCtxt, ctxt);
    return (VALUE)ctxt->sax->_private;
}

static VALUE
force_encoding(VALUE self, VALUE encoding)
{
    xmlParserCtxtPtr ctxt;
    xmlCharEncodingHandlerPtr enc_handler;
    Data_Get_Struct(self, xmlParserCtxt, ctxt);
    enc_handler = xmlFindCharEncodingHandler(StringValuePtr(encoding));

    if (xmlSwitchToEncoding(ctxt, enc_handler))
	rb_raise(rb_eRuntimeError, "Unsupported encoding");
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

VALUE cNokogiriXmlSaxPushParser;

void
init_xml_sax_push_parser()
{
    VALUE nokogiri  = rb_define_module("Nokogiri");
    VALUE xml       = rb_define_module_under(nokogiri, "XML");
    VALUE sax       = rb_define_module_under(xml, "SAX");
    VALUE klass     = rb_define_class_under(sax, "PushParser", rb_cObject);

    cNokogiriXmlSaxPushParser = klass;

    rb_define_alloc_func(klass, allocate);
    rb_define_method(klass, "write", write, -1);
    rb_define_method(klass, "initialize", initialize, -1);
    rb_define_method(klass, "options", get_options, 0);
    rb_define_method(klass, "options=", set_options, 1);
    rb_define_method(klass, "replace_entities=", set_replace_entities, 1);
    rb_define_method(klass, "replace_entities", get_replace_entities, 0);
    rb_define_method(klass, "line", line, 0);
    rb_define_method(klass, "column", column, 0);
    rb_define_method(klass, "document", document, 0);
    rb_define_method(klass, "force_encoding", force_encoding, 1);
    rb_define_method(klass, "finish", finish, 0);

    rb_define_alias(klass, "<<", "write");

    xmlSetStructuredErrorFunc(NULL, NULL);
}

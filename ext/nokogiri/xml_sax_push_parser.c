#include <xml_sax_push_parser.h>

static void
mark(struct xml_sax_parser_data *data)
{
    if (data->handler)
	rb_gc_mark(data->handler);
}

static void
deallocate(struct xml_sax_parser_data *data)
{
    NOKOGIRI_DEBUG_START(data->ctx);
    if (data->ctx) {
	    if (data->ctx->myDoc)
		xmlFreeDoc(data->ctx->myDoc);
	    xmlFreeParserCtxt(data->ctx);
    }
    NOKOGIRI_DEBUG_END(data->ctx);
    free(data);
}

static VALUE
allocate(VALUE klass)
{
    struct xml_sax_parser_data *data;
    VALUE self;

    self = Data_Make_Struct(klass, struct xml_sax_parser_data, mark, deallocate,
			    data);
    data->handler = Qnil;

    return self;
}

static VALUE
initialize(int argc, VALUE *argv, VALUE self)
{
    VALUE _filename, doc;
    const char *filename = NULL;
    xmlParserCtxtPtr ctx;
    struct xml_sax_parser_data *data;

    rb_scan_args(argc, argv, "11", &doc, &_filename);

    if (!NIL_P(_filename))
	filename = StringValuePtr(_filename);

    Data_Get_Struct(self, struct xml_sax_parser_data, data);

    ctx = xmlCreatePushParserCtxt(&nokogiriSAXHandlerPrototype, (void*)data,
				  NULL, 0, filename);
    if (!ctx)
	rb_raise(rb_eRuntimeError, "Could not create a parser context");

    if (data->ctx)
	xmlFreeParserCtxt(data->ctx);

    ctx->sax2 = 1;
    data->handler = doc;
    data->ctx = ctx;

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

    ret = xmlParseChunk(data->ctx, chunk, size, last_chunk);

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

static VALUE get_options(VALUE self)
{
  struct xml_sax_parser_data *data;
  Data_Get_Struct(self, struct xml_sax_parser_data, data);
  if (!data->ctx)
    rb_raise(rb_eRuntimeError, "Parser is Uninitialized.");
  return INT2NUM(data->ctx->options);
}

static VALUE set_options(VALUE self, VALUE options)
{
  struct xml_sax_parser_data *data;
  Data_Get_Struct(self, struct xml_sax_parser_data, data);
  if (!data->ctx)
    rb_raise(rb_eRuntimeError, "Parser is Uninitialized.");

  if (xmlCtxtUseOptions(data->ctx, (int)NUM2INT(options)) != 0)
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
  struct xml_sax_parser_data *data;
  Data_Get_Struct(self, struct xml_sax_parser_data, data);
  if (!data->ctx)
    rb_raise(rb_eRuntimeError, "Parser is Uninitialized.");

  if(Qfalse == value)
    data->ctx->replaceEntities = 0;
  else
    data->ctx->replaceEntities = 1;

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
  struct xml_sax_parser_data *data;
  Data_Get_Struct(self, struct xml_sax_parser_data, data);
  if (!data->ctx)
    rb_raise(rb_eRuntimeError, "Parser is Uninitialized.");

  if(0 == data->ctx->replaceEntities)
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
  xmlParserInputPtr io;
  struct xml_sax_parser_data *data;
  Data_Get_Struct(self, struct xml_sax_parser_data, data);
  if (!data->ctx)
    rb_raise(rb_eRuntimeError, "Parser is Uninitialized.");

  io = data->ctx->input;
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
  xmlParserInputPtr io;
  struct xml_sax_parser_data *data;
  Data_Get_Struct(self, struct xml_sax_parser_data, data);
  if (!data->ctx)
    rb_raise(rb_eRuntimeError, "Parser is Uninitialized.");

  io = data->ctx->input;
  if(io)
    return INT2NUM(io->col);

  return Qnil;
}

static VALUE
document(VALUE self)
{
    struct xml_sax_parser_data *data;
    Data_Get_Struct(self, struct xml_sax_parser_data, data);
    return data->handler;
}

static VALUE
force_encoding(VALUE self, VALUE encoding)
{
    xmlCharEncodingHandlerPtr enc_handler;
    struct xml_sax_parser_data *data;
    Data_Get_Struct(self, struct xml_sax_parser_data, data);
    if (!data->ctx)
      rb_raise(rb_eRuntimeError, "Parser is Uninitialized.");

    enc_handler = xmlFindCharEncodingHandler(StringValuePtr(encoding));

    if (xmlSwitchToEncoding(data->ctx, enc_handler))
	rb_raise(rb_eRuntimeError, "Unsupported encoding");

    return self;
}

static VALUE
finish(VALUE self)
{
    return write2(self, NULL, 0, 1);
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

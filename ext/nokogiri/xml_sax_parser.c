#include <xml_sax_parser.h>

int vasprintf (char **strp, const char *fmt, va_list ap);
void vasprintf_free (void *p);

static ID id_start_document, id_end_document, id_start_element, id_end_element;
static ID id_start_element_namespace, id_end_element_namespace;
static ID id_comment, id_characters, id_xmldecl, id_error, id_warning;
static ID id_cdata_block, id_cAttribute;

static int
rb_protect2(VALUE (*proc) (VALUE), VALUE args, struct xml_sax_parser_data *data)
{
    int status;
    rb_protect(proc, args, &status);
    if (status) {
      xmlStopParser(data->ctx);
      data->status = status;
    }
    return status;
}

static VALUE
prot_xmldecl(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_xmldecl, 3, args[1], args[2], args[3]);
}

static VALUE
prot_start_document(VALUE doc)
{
    return rb_funcall(doc, id_start_document, 0);
}

static void start_document(void *user_data)
{
  struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;
  xmlParserCtxtPtr ctxt = data->ctx;
  VALUE args[4];

  if(NULL != ctxt && ctxt->html != 1) {
    if(ctxt->standalone != -1) {  /* -1 means there was no declaration */
      VALUE encoding = ctxt->encoding ?
        NOKOGIRI_STR_NEW2(ctxt->encoding) :
        Qnil;

      VALUE version = ctxt->version ?
        NOKOGIRI_STR_NEW2(ctxt->version) :
        Qnil;

      VALUE standalone = Qnil;

      switch(ctxt->standalone)
      {
        case 0:
          standalone = NOKOGIRI_STR_NEW2("no");
          break;
        case 1:
          standalone = NOKOGIRI_STR_NEW2("yes");
          break;
      }

      args[0] = data->handler;
      args[1] = version;
      args[2] = encoding;
      args[3] = standalone;
      if (rb_protect2(prot_xmldecl, (VALUE)args, data))
	  return;
    }
  }
  rb_protect2(prot_start_document, data->handler, data);
}

static VALUE
prot_end_document(VALUE doc)
{
    return rb_funcall(doc, id_end_document, 0);
}

static void end_document(void *user_data)
{
    struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;
    rb_protect2(prot_end_document, data->handler, data);
}

static VALUE
prot_start_element(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_start_element, 2, args[1], args[2]);
}

static void start_element(void *user_data, const xmlChar *name,
			  const xmlChar **atts)
{
  struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;
  VALUE attributes = rb_ary_new();
  VALUE args[3];
  const xmlChar * attr;
  int i = 0;
  if(atts) {
    while((attr = atts[i]) != NULL) {
      const xmlChar * val = atts[i+1];
      VALUE value = val != NULL ? NOKOGIRI_STR_NEW2(val) : Qnil;
      rb_ary_push(attributes, rb_ary_new3(2, NOKOGIRI_STR_NEW2(attr), value));
      i+=2;
    }
  }

  args[0] = data->handler;
  args[1] = NOKOGIRI_STR_NEW2(name);
  args[2] = attributes;
  rb_protect2(prot_start_element, (VALUE)args, data);
}

static VALUE
prot_end_element(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_end_element, 1, args[1]);
}

static void end_element(void *user_data, const xmlChar *name)
{
    struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;
    VALUE args[2];
    args[0] = data->handler;
    args[1] = NOKOGIRI_STR_NEW2(name);
    rb_protect2(prot_end_element, (VALUE)args, data);
}

static VALUE attributes_as_list(
  int nb_attributes,
  const xmlChar ** attributes)
{
  VALUE list = rb_ary_new2((long)nb_attributes);

  VALUE attr_klass = rb_const_get(cNokogiriXmlSaxParser, id_cAttribute);
  if (attributes) {
    /* Each attribute is an array of [localname, prefix, URI, value, end] */
    int i;
    for (i = 0; i < nb_attributes * 5; i += 5) {
      VALUE argv[4], attribute;

      argv[0] = RBSTR_OR_QNIL(attributes[i + 0]); /* localname */
      argv[1] = RBSTR_OR_QNIL(attributes[i + 1]); /* prefix */
      argv[2] = RBSTR_OR_QNIL(attributes[i + 2]); /* URI */

      /* value */
      argv[3] = NOKOGIRI_STR_NEW((const char*)attributes[i+3],
          (attributes[i+4] - attributes[i+3]));

      attribute = rb_class_new_instance(4, argv, attr_klass);
      rb_ary_push(list, attribute);
    }
  }

  return list;
}

static VALUE
prot_start_element_ns(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_start_element_namespace, 5, args[1], args[2],
		      args[3], args[4], args[5]);
}

static void
start_element_ns (
  void *user_data,
  const xmlChar * localname,
  const xmlChar * prefix,
  const xmlChar * uri,
  int nb_namespaces,
  const xmlChar ** namespaces,
  int nb_attributes,
  int nb_defaulted,
  const xmlChar ** attributes)
{
  VALUE args[6];
  struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;

  VALUE attribute_list = attributes_as_list(nb_attributes, attributes);

  VALUE ns_list = rb_ary_new2((long)nb_namespaces);

  if (namespaces) {
    int i;
    for (i = 0; i < nb_namespaces * 2; i += 2)
    {
      rb_ary_push(ns_list,
        rb_ary_new3((long)2,
          RBSTR_OR_QNIL(namespaces[i + 0]),
          RBSTR_OR_QNIL(namespaces[i + 1])
        )
      );
    }
  }

  args[0] = data->handler;
  args[1] = NOKOGIRI_STR_NEW2(localname);
  args[2] = attribute_list;
  args[3] = RBSTR_OR_QNIL(prefix);
  args[4] = RBSTR_OR_QNIL(uri);
  args[5] = ns_list;
  rb_protect2(prot_start_element_ns, (VALUE)args, data);
}

static VALUE
prot_end_element_ns(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_end_element_namespace, 3, args[1], args[2],
		      args[3]);
}

/**
 * end_element_ns was borrowed heavily from libxml-ruby.
 */
static void
end_element_ns (
  void *user_data,
  const xmlChar * localname,
  const xmlChar * prefix,
  const xmlChar * uri)
{
    VALUE args[4];
    struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;
    args[0] = data->handler;
    args[1] = NOKOGIRI_STR_NEW2(localname);
    args[2] = RBSTR_OR_QNIL(prefix);
    args[3] = RBSTR_OR_QNIL(uri);
    rb_protect2(prot_end_element_ns, (VALUE)args, data);
}

static VALUE
prot_characters(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_characters, 1, args[1]);
}

static void characters_func(void *user_data, const xmlChar * ch, int len)
{
    struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;
    VALUE args[2];
    VALUE doc = data->handler;
    args[0] = doc;
    args[1] = NOKOGIRI_STR_NEW(ch, len);
    rb_protect2(prot_characters, (VALUE)args, data);
}

static VALUE
prot_comment(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_comment, 1, args[1]);
}

static void comment_func(void *user_data, const xmlChar * value)
{
    struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;
    VALUE args[2];
    VALUE doc = data->handler;
    args[0] = doc;
    args[1] = NOKOGIRI_STR_NEW2(value);
    rb_protect2(prot_comment, (VALUE)args, data);
}

static VALUE
prot_warning(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_warning, 1, args[1]);
}

static void warning_func(void *user_data, const char *msg, ...)
{
  struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;
  VALUE prot_args[2];
  char * message;

  va_list args;
  va_start(args, msg);
  vasprintf(&message, msg, args);
  va_end(args);

  prot_args[0] = data->handler;
  prot_args[1] = NOKOGIRI_STR_NEW2(message);
  vasprintf_free(message);
  rb_protect2(prot_warning, (VALUE)prot_args, data);
}

static VALUE
prot_error(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_error, 1, args[1]);
}

static void error_func(void *user_data, const char *msg, ...)
{
  struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;
  VALUE prot_args[2];
  char * message;

  va_list args;
  va_start(args, msg);
  vasprintf(&message, msg, args);
  va_end(args);

  prot_args[0] = data->handler;
  prot_args[1] = NOKOGIRI_STR_NEW2(message);
  vasprintf_free(message);
  rb_protect2(prot_error, (VALUE)prot_args, data);
}

static VALUE
prot_cdata_block(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_cdata_block, 1, args[1]);
}

static void cdata_block(void *user_data, const xmlChar *value, int len)
{
    struct xml_sax_parser_data *data = (struct xml_sax_parser_data *)user_data;
    VALUE args[2];
    args[0] = data->handler;
    args[1] = NOKOGIRI_STR_NEW(value, len);
    rb_protect2(prot_cdata_block, (VALUE)args, data);
}

xmlSAXHandler nokogiriSAXHandlerPrototype =
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    start_document,
    end_document,
    start_element,
    end_element,
    NULL,
    characters_func,
    NULL,
    NULL,
    comment_func,
    warning_func,
    error_func,
    NULL,
    NULL,
    cdata_block,
    NULL,
    XML_SAX2_MAGIC,
    NULL,
    start_element_ns,
    end_element_ns,
    NULL
};

VALUE cNokogiriXmlSaxParser ;
void init_xml_sax_parser()
{
  VALUE nokogiri  = rb_define_module("Nokogiri");
  VALUE xml       = rb_define_module_under(nokogiri, "XML");
  VALUE sax       = rb_define_module_under(xml, "SAX");
  VALUE klass     = rb_define_class_under(sax, "Parser", rb_cObject);

  cNokogiriXmlSaxParser = klass;
  id_start_document = rb_intern("start_document");
  id_end_document   = rb_intern("end_document");
  id_start_element  = rb_intern("start_element");
  id_end_element    = rb_intern("end_element");
  id_comment        = rb_intern("comment");
  id_characters     = rb_intern("characters");
  id_xmldecl        = rb_intern("xmldecl");
  id_error          = rb_intern("error");
  id_warning        = rb_intern("warning");
  id_cdata_block    = rb_intern("cdata_block");
  id_cAttribute     = rb_intern("Attribute");
  id_start_element_namespace = rb_intern("start_element_namespace");
  id_end_element_namespace = rb_intern("end_element_namespace");

  xmlSetStructuredErrorFunc(NULL, NULL);
}

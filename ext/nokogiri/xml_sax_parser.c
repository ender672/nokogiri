#include <xml_sax_parser.h>

int vasprintf (char **strp, const char *fmt, va_list ap);
void vasprintf_free (void *p);

static ID id_start_document, id_end_document, id_start_element, id_end_element;
static ID id_start_element_namespace, id_end_element_namespace;
static ID id_comment, id_characters, id_xmldecl, id_error, id_warning;
static ID id_cdata_block, id_cAttribute;

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

static void start_document(void * ctx)
{
  xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
  VALUE doc = (VALUE)ctxt->sax->_private;
  int status;
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

      args[0] = doc;
      args[1] = version;
      args[2] = encoding;
      args[3] = standalone;
      rb_protect(prot_xmldecl, (VALUE)args, &status);
      if (status) {
	  xmlStopParser(ctxt);
	  return;
      }
    }
  }
  rb_protect(prot_start_document, doc, &status);
  if (status)
	  xmlStopParser(ctxt);
}

static VALUE
prot_end_document(VALUE doc)
{
    return rb_funcall(doc, id_end_document, 0);
}

static void end_document(void * ctx)
{
    int status;
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    rb_protect(prot_end_document, (VALUE)ctxt->sax->_private, &status);
    if (status)
  	  xmlStopParser(ctxt);
}

static VALUE
prot_start_element(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_start_element, 2, args[1], args[2]);
}

static void start_element(void * ctx, const xmlChar *name, const xmlChar **atts)
{
  xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
  VALUE doc = (VALUE)ctxt->sax->_private;
  VALUE attributes = rb_ary_new();
  VALUE args[3];
  const xmlChar * attr;
  int status, i = 0;
  if(atts) {
    while((attr = atts[i]) != NULL) {
      const xmlChar * val = atts[i+1];
      VALUE value = val != NULL ? NOKOGIRI_STR_NEW2(val) : Qnil;
      rb_ary_push(attributes, rb_ary_new3(2, NOKOGIRI_STR_NEW2(attr), value));
      i+=2;
    }
  }

  args[0] = doc;
  args[1] = NOKOGIRI_STR_NEW2(name);
  args[2] = attributes;
  rb_protect(prot_start_element, (VALUE)args, &status);
  if (status)
	  xmlStopParser(ctxt);
}

static VALUE
prot_end_element(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_end_element, 1, args[1]);
}

static void end_element(void * ctx, const xmlChar *name)
{
    int status;
    VALUE args[2];
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    args[0] = (VALUE)ctxt->sax->_private;
    args[1] = NOKOGIRI_STR_NEW2(name);
    rb_protect(prot_end_element, (VALUE)args, &status);
    if (status)
  	  xmlStopParser(ctxt);
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
  void * ctx,
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
  int status;
  xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
  VALUE doc = (VALUE)ctxt->sax->_private;

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

  args[0] = doc;
  args[1] = NOKOGIRI_STR_NEW2(localname);
  args[2] = attribute_list;
  args[3] = RBSTR_OR_QNIL(prefix);
  args[4] = RBSTR_OR_QNIL(uri);
  args[5] = ns_list;
  rb_protect(prot_start_element_ns, (VALUE)args, &status);
  if (status)
	  xmlStopParser(ctxt);
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
  void * ctx,
  const xmlChar * localname,
  const xmlChar * prefix,
  const xmlChar * uri)
{
    int status;
    VALUE args[4];
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    args[0] = (VALUE)ctxt->sax->_private;
    args[1] = NOKOGIRI_STR_NEW2(localname);
    args[2] = RBSTR_OR_QNIL(prefix);
    args[3] = RBSTR_OR_QNIL(uri);
    rb_protect(prot_end_element_ns, (VALUE)args, &status);
    if (status)
  	  xmlStopParser(ctxt);
}

static VALUE
prot_characters(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_characters, 1, args[1]);
}

static void characters_func(void * ctx, const xmlChar * ch, int len)
{
    int status;
    VALUE args[2];
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    args[0] = (VALUE)ctxt->sax->_private;
    args[1] = NOKOGIRI_STR_NEW(ch, len);
    rb_protect(prot_characters, (VALUE)args, &status);
    if (status)
  	  xmlStopParser(ctxt);
}

static VALUE
prot_comment(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_comment, 1, args[1]);
}

static void comment_func(void * ctx, const xmlChar * value)
{
    int status;
    VALUE args[2];
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    args[0] = (VALUE)ctxt->sax->_private;
    args[1] = NOKOGIRI_STR_NEW2(value);
    rb_protect(prot_comment, (VALUE)args, &status);
    if (status)
  	  xmlStopParser(ctxt);
}

static VALUE
prot_warning(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_warning, 1, args[1]);
}

static void warning_func(void * ctx, const char *msg, ...)
{
  int status;
  VALUE prot_args[2];
  xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
  char * message;

  va_list args;
  va_start(args, msg);
  vasprintf(&message, msg, args);
  va_end(args);

  prot_args[0] = (VALUE)ctxt->sax->_private;
  prot_args[1] = NOKOGIRI_STR_NEW2(message);
  vasprintf_free(message);
  rb_protect(prot_warning, (VALUE)prot_args, &status);
  if (status)
	  xmlStopParser(ctxt);
}

static VALUE
prot_error(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_error, 1, args[1]);
}

static void error_func(void * ctx, const char *msg, ...)
{
  int status;
  VALUE prot_args[2];
  xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
  char * message;

  va_list args;
  va_start(args, msg);
  vasprintf(&message, msg, args);
  va_end(args);

  prot_args[0] = (VALUE)ctxt->sax->_private;
  prot_args[1] = NOKOGIRI_STR_NEW2(message);
  vasprintf_free(message);
  rb_protect(prot_error, (VALUE)prot_args, &status);
  if (status)
      xmlStopParser(ctxt);
}

static VALUE
prot_cdata_block(VALUE _args)
{
    VALUE *args = (VALUE *)_args;
    return rb_funcall(args[0], id_cdata_block, 1, args[1]);
}

static void cdata_block(void * ctx, const xmlChar *value, int len)
{
    int status;
    VALUE args[2];
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    args[0] = (VALUE)ctxt->sax->_private;
    args[1] = NOKOGIRI_STR_NEW(value, len);
    rb_protect(prot_cdata_block, (VALUE)args, &status);
    if (status)
        xmlStopParser(ctxt);
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

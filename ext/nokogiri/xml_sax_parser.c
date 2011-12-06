#include <xml_sax_parser.h>

int vasprintf (char **strp, const char *fmt, va_list ap);
void vasprintf_free (void *p);

static ID id_start_document, id_end_document, id_start_element, id_end_element;
static ID id_start_element_namespace, id_end_element_namespace;
static ID id_comment, id_characters, id_xmldecl, id_error, id_warning;
static ID id_cdata_block, id_cAttribute;

static void start_document(void * ctx)
{
  xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
  VALUE doc = (VALUE)ctxt->sax->_private;

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

      rb_funcall(doc, id_xmldecl, 3, version, encoding, standalone);
    }
  }

  rb_funcall(doc, id_start_document, 0);
}

static void end_document(void * ctx)
{
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    rb_funcall((VALUE)ctxt->sax->_private, id_end_document, 0);
}

static void start_element(void * ctx, const xmlChar *name, const xmlChar **atts)
{
  xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
  VALUE doc = (VALUE)ctxt->sax->_private;
  VALUE attributes = rb_ary_new();
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

  rb_funcall( doc,
              id_start_element,
              2,
              NOKOGIRI_STR_NEW2(name),
              attributes
  );
}

static void end_element(void * ctx, const xmlChar *name)
{
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    rb_funcall((VALUE)ctxt->sax->_private, id_end_element, 1,
	       NOKOGIRI_STR_NEW2(name));
}

static VALUE attributes_as_list(
  int nb_attributes,
  const xmlChar ** attributes)
{
  VALUE list = rb_ary_new2((long)nb_attributes);

  VALUE attr_klass = rb_const_get(cNokogiriXmlSaxPushParser, id_cAttribute);
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

  rb_funcall( doc,
              id_start_element_namespace,
              5,
              NOKOGIRI_STR_NEW2(localname),
              attribute_list,
              RBSTR_OR_QNIL(prefix),
              RBSTR_OR_QNIL(uri),
              ns_list
  );
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
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    rb_funcall((VALUE)ctxt->sax->_private, id_end_element_namespace, 3,
	       NOKOGIRI_STR_NEW2(localname), RBSTR_OR_QNIL(prefix),
	       RBSTR_OR_QNIL(uri));
}

static void characters_func(void * ctx, const xmlChar * ch, int len)
{
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    VALUE str = NOKOGIRI_STR_NEW(ch, len);
    rb_funcall((VALUE)ctxt->sax->_private, id_characters, 1, str);
}

static void comment_func(void * ctx, const xmlChar * value)
{
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    VALUE str = NOKOGIRI_STR_NEW2(value);
    rb_funcall((VALUE)ctxt->sax->_private, id_comment, 1, str);
}

static void warning_func(void * ctx, const char *msg, ...)
{
  xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
  char * message;
  VALUE ruby_message;

  va_list args;
  va_start(args, msg);
  vasprintf(&message, msg, args);
  va_end(args);

  ruby_message = NOKOGIRI_STR_NEW2(message);
  vasprintf_free(message);
  rb_funcall((VALUE)ctxt->sax->_private, id_warning, 1, ruby_message);
}

static void error_func(void * ctx, const char *msg, ...)
{
  xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
  char * message;
  VALUE ruby_message;

  va_list args;
  va_start(args, msg);
  vasprintf(&message, msg, args);
  va_end(args);

  ruby_message = NOKOGIRI_STR_NEW2(message);
  vasprintf_free(message);
  rb_funcall((VALUE)ctxt->sax->_private, id_error, 1, ruby_message);
}

static void cdata_block(void * ctx, const xmlChar *value, int len)
{
    xmlParserCtxtPtr ctxt = (xmlParserCtxtPtr)ctx;
    VALUE string = NOKOGIRI_STR_NEW(value, len);
    rb_funcall((VALUE)ctxt->sax->_private, id_cdata_block, 1, string);
}

xmlSAXHandlerPtr
create_sax_handler_callbacks(VALUE document)
{
  xmlSAXHandlerPtr handler = calloc((size_t)1, sizeof(xmlSAXHandler));

  xmlSetStructuredErrorFunc(NULL, NULL);

  handler->startDocument = start_document;
  handler->endDocument = end_document;
  handler->startElement = start_element;
  handler->endElement = end_element;
  handler->startElementNs = start_element_ns;
  handler->endElementNs = end_element_ns;
  handler->characters = characters_func;
  handler->comment = comment_func;
  handler->warning = warning_func;
  handler->error = error_func;
  handler->cdataBlock = cdata_block;
  handler->initialized = XML_SAX2_MAGIC;
  handler->_private = document;
  return handler;
}

void init_xml_sax_parser()
{
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
}

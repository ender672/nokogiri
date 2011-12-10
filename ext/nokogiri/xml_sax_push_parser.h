#ifndef NOKOGIRI_XML_SAX_PUSH_PARSER
#define NOKOGIRI_XML_SAX_PUSH_PARSER

#include <nokogiri.h>

void init_xml_sax_push_parser();

extern VALUE cNokogiriXmlSaxPushParser;

struct xml_sax_parser_data {
    xmlParserCtxtPtr ctx;
    VALUE handler;
    int status;
};

#endif


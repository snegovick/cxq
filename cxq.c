// Based on xpath1 sample from libxml2 repository
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define STRING_BUFFER_SIZE 256


static void
usage(void) {
  fprintf(stderr, "cxq - commandline tool for running XPath queries on XML data\n\n");
  fprintf(stderr, "Usage: cxq -f <xml file path> -x <xpath expression>\n\n");
  fprintf(stderr, "If xml file path arg (-f) is missing, then XML data is expected on stdin.\n\n");
  fprintf(stderr, "Example:\n\n");
  fprintf(stderr, "$ echo '<apn user=\"test\"/>' | ./cxq -x /apn/@user\n");
  fprintf(stderr, "test\n\n");
  fprintf(stderr, "Command options:\n");
  fprintf(stderr, "\t-h\t\t\tShow this help\n");
  fprintf(stderr, "\t-f\t\t\tXML file path\n");
#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED)
  fprintf(stderr, "\t-x\t\t\tXPath expression\n");
#else
  fprintf(stderr, "\t-x\t\t\tXPath expression support is not enabled in libxml2\n");
#endif
}

xmlDocPtr
load_xml(const char* filename, const char *inputBuffer, size_t inputBufferSize) {
  xmlDocPtr doc;

  /* Load XML document */
  if (filename != NULL) {
    doc = xmlParseFile(filename);
    if (doc == NULL) {
      fprintf(stderr, "Error: unable to parse file \"%s\"\n", filename);
      return NULL;
    }
  } else if (inputBuffer != NULL) {
    doc = xmlReadMemory(inputBuffer, inputBufferSize, "stdin", NULL, 0);
    if (doc == NULL) {
      fprintf(stderr, "Error: unable to parse input\n");
      return NULL;
    }
  }

  return doc;
}

int
pretty_print_xml(xmlDocPtr doc, FILE* output) {
  xmlBufferPtr buffer = xmlBufferCreate();
  if (buffer == NULL) {
    fprintf(stderr, "Error creating the xml buffer\n");
    return -1;
  }

  xmlOutputBuffer *outputBuffer = xmlOutputBufferCreateBuffer(buffer, NULL);

  xmlSaveFormatFileTo(outputBuffer, doc, "utf-8", 1);
  fprintf(output, "%s\n", buffer->content);
  xmlBufferFree(buffer);
  return 0;
}

int
pretty_print_xml_node(xmlNodePtr cur, FILE *output) {
  size_t alloc_size = 4096;
  char *outstr = realloc(NULL, alloc_size);
  int offt = sprintf(outstr, "<%s", cur->name);
  xmlAttrPtr attr = cur->properties;
  while (attr != NULL) {
    xmlChar* value = xmlNodeListGetString(cur->doc, attr->children, 1);
    // space + 2*quotes + '=' + '/' + '>' + \0 -> 7
    size_t combined_len = xmlStrlen(attr->name) + xmlStrlen(value) + 7;
    if (offt + combined_len > alloc_size) {
      outstr = realloc(outstr, alloc_size + combined_len);
      alloc_size += combined_len;
    }
    offt += sprintf(outstr+offt, " %s=\"%s\"", attr->name, value);
    xmlFree(value);
    attr = attr->next;
  }
  sprintf(outstr+offt, "/>");
  fprintf(output, "%s\n", outstr);
  free(outstr);

  return 0;
}

#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED)
int
register_namespaces(xmlXPathContextPtr xpathCtx, const xmlChar* nsList) {
  xmlChar* nsListDup;
  xmlChar* prefix;
  xmlChar* href;
  xmlChar* next;

  assert(xpathCtx);
  assert(nsList);

  nsListDup = xmlStrdup(nsList);
  if (nsListDup == NULL) {
    fprintf(stderr, "Error: unable to strdup namespaces list\n");
    return -1;
  }

  next = nsListDup;
  while(next != NULL) {
    /* skip spaces */
    while((*next) == ' ') next++;
    if ((*next) == '\0') break;

    /* find prefix */
    prefix = next;
    next = (xmlChar*)xmlStrchr(next, '=');
    if (next == NULL) {
      fprintf(stderr,"Error: invalid namespaces list format\n");
      xmlFree(nsListDup);
      return -1;
    }
    *(next++) = '\0';

    /* find href */
    href = next;
    next = (xmlChar*)xmlStrchr(next, ' ');
    if (next != NULL) {
      *(next++) = '\0';
    }

    /* do register namespace */
    if (xmlXPathRegisterNs(xpathCtx, prefix, href) != 0) {
      fprintf(stderr,"Error: unable to register NS with prefix=\"%s\" and href=\"%s\"\n", prefix, href);
      xmlFree(nsListDup);
      return -1;
    }
  }

  xmlFree(nsListDup);
  return(0);
}

void
print_xpath_nodes(xmlNodeSetPtr nodes, FILE* output) {
  xmlNodePtr cur;
  int size;
  int i;

  assert(output);
  size = (nodes) ? nodes->nodeNr : 0;

  for(i = 0; i < size; ++i) {
    assert(nodes->nodeTab[i]);

    if (nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
      xmlNsPtr ns;

      ns = (xmlNsPtr)nodes->nodeTab[i];
      cur = (xmlNodePtr)ns->next;
      if (cur->ns) {
        fprintf(output, "= namespace \"%s\"=\"%s\" for node %s:%s\n",
                ns->prefix, ns->href, cur->ns->href, cur->name);
      } else {
        fprintf(output, "= namespace \"%s\"=\"%s\" for node %s\n",
                ns->prefix, ns->href, cur->name);
      }
    } else if (nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
      cur = nodes->nodeTab[i];
      if (cur->ns) {
        fprintf(output, "= element node \"%s:%s\"\n",
                cur->ns->href, cur->name);
      } else {
        pretty_print_xml_node(cur, stdout);
      }
    } else {
      cur = nodes->nodeTab[i];
      xmlChar* value = xmlNodeListGetString(cur->doc, cur->children, 1);
      fprintf(output, "%s\n", value);
      xmlFree(value);
    }
  }
}

int
execute_xpath_expression(xmlDocPtr doc, const xmlChar* xpathExpr, const xmlChar* nsList) {
  xmlXPathContextPtr xpathCtx;
  xmlXPathObjectPtr xpathObj;

  assert(xpathExpr);

  /* Create xpath evaluation context */
  xpathCtx = xmlXPathNewContext(doc);
  if (xpathCtx == NULL) {
    fprintf(stderr, "Error: unable to create new XPath context\n");
    xmlFreeDoc(doc);
    return -1;
  }

  /* Register namespaces from list (if any) */
  if ((nsList != NULL) && (register_namespaces(xpathCtx, nsList) < 0)) {
    fprintf(stderr, "Error: failed to register namespaces list \"%s\"\n", nsList);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    return -1;
  }

  /* Evaluate xpath expression */
  xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
  if (xpathObj == NULL) {
    fprintf(stderr, "Error: unable to evaluate xpath expression \"%s\"\n", xpathExpr);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    return -1;
  }

  /* Print results */
  print_xpath_nodes(xpathObj->nodesetval, stdout);

  /* Cleanup */
  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx);
  xmlFreeDoc(doc);

  return(0);
}
#endif

int
main(int argc, char **argv) {
  /* Parse command line and process file */
  char *filePath = NULL;
  char *xpathExpression = NULL;
  char *nsList = NULL;
  char *inputBuffer = NULL;
  size_t inputBufferSize = 0;
  char stringBuffer[STRING_BUFFER_SIZE] = {0};
  int c;

  opterr = 0;

  while ((c = getopt (argc, argv, "hf:x:n:")) != -1) {
    switch (c) {
    case 'h':
      usage();
      return 0;
    case 'f':
      filePath = optarg;
      break;
#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED)
    case 'x':
      xpathExpression = optarg;
      break;
#endif
    case 'n':
      nsList = optarg;
      break;
    default:
      usage();
      return -1;
    }
  }

  if (filePath == NULL) {
    while(fgets(stringBuffer, STRING_BUFFER_SIZE, stdin) != NULL) {
      size_t sbSize = strlen(stringBuffer);
      inputBuffer = realloc(inputBuffer, inputBufferSize + sbSize);
      memcpy(inputBuffer + inputBufferSize, stringBuffer, sbSize);
      inputBufferSize += sbSize;
    }
  }

  /* Init libxml */
  xmlInitParser();
  LIBXML_TEST_VERSION

    xmlDocPtr doc = load_xml(filePath, inputBuffer, inputBufferSize);
  if (doc == NULL) {
    return -1;
  }

  /* Do the main job */
  if (xpathExpression == NULL) {
    return pretty_print_xml(doc, stdout);
  }
#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED)
  else if (execute_xpath_expression(doc, BAD_CAST xpathExpression, (nsList != NULL) ? BAD_CAST nsList : NULL) < 0) {
    usage();
    return -1;
  }
#endif

  return 0;
}

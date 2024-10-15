#ifndef COFD_COMMON_HEADER_FILES_H
#define COFD_COMMON_HEADER_FILES_H

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/xml/cfx_xmlparser.h"
#include "core/fxcrt/xml/cfx_xmldocument.h"
#include "core/fxcrt/xml/cfx_xmlelement.h"

//
/*
* OFD层，使用PDF层相同的基础单位， 1/72inch
* 
*/
static inline float ofd_mm_to_one_72_point(float val) {
  return val * 72 / 25.4f;
}

static inline float ofd_one_72_point_to_mm(float val) {
  return val * 25.4f / 72;
}
#endif  // !COFD_COMMON_HEADER_FILES_H

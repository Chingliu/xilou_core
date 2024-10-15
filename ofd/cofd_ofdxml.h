#ifndef COFD_PARSE_OFD_XML_H_HEADER
#define COFD_PARSE_OFD_XML_H_HEADER
#include "ofd/cofd_common.h"
namespace ofd {

  class COFD_DocInfo {
 public:
    WideString doc_root;
    WideString doc_id;
    WideString doc_title;
    WideString doc_author;
    WideString doc_create_date;
    WideString doc_modify_date;
    WideString doc_creator;
    WideString doc_creator_version;
    WideString signature_root;
  };
class COFD_ParseOfdXml {
 public:
   explicit COFD_ParseOfdXml(const CFX_XMLDocument* xml);

 public:
  COFD_ParseOfdXml(const COFD_ParseOfdXml&) = delete;
  COFD_ParseOfdXml(COFD_ParseOfdXml&&) = delete;
  COFD_ParseOfdXml& operator=(const COFD_ParseOfdXml&) = delete;
  COFD_ParseOfdXml& operator=(COFD_ParseOfdXml&&) = delete;
  ~COFD_ParseOfdXml();

public:
  COFD_DocInfo* GetDocInfo(size_t idx);
  size_t DocNum() { return doc_info_.size(); }
 private:
  const CFX_XMLDocument* xml_;
  std::vector<std::unique_ptr<COFD_DocInfo> > doc_info_;
        };
}
#endif  // !COFD_PACKAGE_H_HEADER

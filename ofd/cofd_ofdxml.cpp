#include "ofd/cofd_common.h"
#include "ofd/cofd_ofdxml.h"
using namespace ofd;

COFD_ParseOfdXml::COFD_ParseOfdXml(const CFX_XMLDocument* xml) : xml_(xml) {
  CFX_XMLElement* root = xml_->GetRoot();
  if (!root)
    return;
  CFX_XMLElement* ofd_node = root->GetFirstChildNamed_NoPrefix(L"OFD");
  if (!ofd_node)
    return;
  CFX_XMLElement* doc_body = ofd_node->GetFirstChildNamed_NoPrefix(L"DocBody");
  while (doc_body) {
    CFX_XMLElement *doc_root = doc_body->GetFirstChildNamed_NoPrefix(L"DocRoot");
    if (!doc_root)
      continue;
    std::unique_ptr<COFD_DocInfo> doc_info(new COFD_DocInfo());
    doc_info->doc_root = doc_root->GetTextData();
    doc_info->doc_root.Trim();
    auto doc_sign = doc_body->GetFirstChildNamed_NoPrefix(L"Signatures");
    if (doc_sign) {
      doc_info->signature_root = doc_sign->GetTextData();
      doc_info->signature_root.Trim();
    }
    CFX_XMLElement* info_node = doc_body->GetFirstChildNamed_NoPrefix(L"DocInfo");
    if (info_node) {
      CFX_XMLElement* id_node =
          info_node->GetFirstChildNamed_NoPrefix(L"DocID");
      if (id_node) {
        doc_info->doc_id = id_node->GetTextData();
      }
      id_node =
          id_node->GetFirstChildNamed_NoPrefix(L"Title");
      if (id_node) {
        doc_info->doc_title = id_node->GetTextData();
      }
      id_node =
          info_node->GetFirstChildNamed_NoPrefix(L"Author");
      if (id_node) {
        doc_info->doc_author = id_node->GetTextData();
      }
      id_node =
          info_node->GetFirstChildNamed_NoPrefix(L"CreationDate");
      if (id_node) {
        doc_info->doc_create_date = id_node->GetTextData();
      }
      id_node =
          info_node->GetFirstChildNamed_NoPrefix(L"ModDate");
      if (id_node) {
        doc_info->doc_modify_date = id_node->GetTextData();
      }

      id_node =
          info_node->GetFirstChildNamed_NoPrefix(L"Creator");
      if (id_node) {
        doc_info->doc_creator = id_node->GetTextData();
      }
      id_node =
          info_node->GetFirstChildNamed_NoPrefix(L"CreatorVersion");
      if (id_node) {
        doc_info->doc_creator_version = id_node->GetTextData();
      }
    }
    doc_info_.push_back(std::move(doc_info));
    doc_body = doc_body->GetNextSiblingNamed_NoPrefix(L"DocBody");
  }
}

COFD_ParseOfdXml::~COFD_ParseOfdXml() {

}

COFD_DocInfo* COFD_ParseOfdXml::GetDocInfo(size_t idx) {
  if (idx < doc_info_.size()) {
    return doc_info_.at(idx).get();
  }
  return nullptr;
}
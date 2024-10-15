/**
 * Copyright (C) 2024 chingliu.  All rights reserved.
 *
 * @file		xilou_ofdmerge
 * @author		余清留(chingliu)
 * @date		2024-6-12
 * @brief		实现ofd合并功能
 */
#ifndef XILOU_IMPL_OFD_MERGE_H
#define XILOU_IMPL_OFD_MERGE_H
#include "xilou_public/xilou_view.h"
#include "core/fxcrt/xml/cfx_xmldocument.h"
#include "core/fxcrt/widestring.h"
#include <memory>
#include <vector>
namespace ofd {
class COFD_ZipEntry;
}
namespace xilou {
class COfd_Merge {
 public:
  explicit COfd_Merge(XILOU_UTF8STRING merge2ofd_path,
                      XILOU_UTF8STRING been_merged_ofd_path);

 public:
  COfd_Merge(const COfd_Merge&) = delete;
  COfd_Merge(COfd_Merge&&) = delete;
  COfd_Merge& operator=(const COfd_Merge&) = delete;
  COfd_Merge& operator=(COfd_Merge&&) = delete;
  ~COfd_Merge();

 public:
  int do_merge();

 private:
  int update_id(std::vector<uint8_t>* buf);
  void merge_doc();
  std::unique_ptr<CFX_XMLDocument> getDoc(ofd::COFD_ZipEntry* zip,
                                          WideString* baseurl);

  void merge_commondata(CFX_XMLElement* root, WideString baseurl);
  void merge_pages(CFX_XMLElement* root, WideString baseurl);
  void merge_bytag(CFX_XMLElement* root, WideString baseurl, WideString tag);
  void update(std::string& content);
  void update_annotations(std::string& content);
  void update_resource(std::string& content);
  void modify_location_node(CFX_XMLDocument* doc,
                            CFX_XMLElement* parent,
                            WideString tag,
                            WideString child_tag);

 private:
  std::unique_ptr<ofd::COFD_ZipEntry> m_target_zip;
  std::unique_ptr<ofd::COFD_ZipEntry> m_been_merged_zip;
  std::unique_ptr<CFX_XMLDocument> m_document_xml;
  UnownedPtr<CFX_XMLElement> m_doc_node;
  WideString m_doc_root;                 // 没有最后的/
  WideString m_target_documentxml_path;  //
  WideString m_second_file_base_url;
  int m_base_id;
};
}  // namespace xilou
#endif
#include "xilou_impl/xilou_ofdmerge.h"
#include "ofd/cofd_common.h"
#include<unordered_set>
#include "xilou_public/xilou_errcode.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/widestring.h"
#include "ofd/cofd_zipentry.h"
#include "ofd/cofd_common.h"
#include "ofd/cofd_ofdxml.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/xml/cfx_xmlparser.h"
#include "core/fxcrt/xml/cfx_xmldocument.h"
#include "core/fxcrt/xml/cfx_xmlelement.h"
#include "core/fxcrt/xml/cfx_xmltext.h"
#include "core/fxcrt/cfx_readonlymemorystream.h"
#include "xilou_impl/xilou_xmlpage.h"
#include "third_party/spdlog/include/spdlog/spdlog.h"
#include <regex>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
using namespace xilou;
using namespace ofd;
COfd_Merge::COfd_Merge(XILOU_UTF8STRING merge2ofd_path,
                       XILOU_UTF8STRING been_merged_ofd_path)
    : m_doc_node(nullptr), m_base_id(10000) {
  {
    std::unique_ptr<ofd::COFD_ZipEntry> zip(new COFD_ZipEntry(merge2ofd_path));
    if (!zip) {
      spdlog::error("[COfd_Merge] open doc failed:{}", merge2ofd_path);
      return;  //需要先读出document.xml;
    }
    if (zip->Open('r')) {
      m_document_xml = getDoc(zip.get(), &m_target_documentxml_path);
      if (!m_document_xml) {
        spdlog::error("[COfd_Merge] parse document.xml failed from:{}",
                      merge2ofd_path);
        return;
      }
      CFX_XMLElement* root = m_document_xml->GetRoot();
      if (!root) {
        spdlog::error("[COfd_Merge] xml root is null.\n");
        return;
      }
      CFX_XMLElement* document_node =
          root->GetFirstChildNamed_NoPrefix(L"Document");
      if (!document_node) {
        spdlog::error("[COfd_Merge] xml document_node is null.\n");
        return;
      }
      m_doc_node = document_node;
      auto target_common =
          m_doc_node->GetFirstChildNamed_NoPrefix(L"CommonData");
      if (target_common) {
        auto max_id = target_common->GetFirstChildNamed_NoPrefix(L"MaxUnitID");
        if (max_id) {
          auto id = max_id->GetTextData().GetInteger();
          if (id > m_base_id) {
            m_base_id = ((id / m_base_id) + 1) * m_base_id;
          }
        }
      }
    } else {
      spdlog::error("[COfd_Merge] open zip failed:{}.\n", merge2ofd_path);
      return;
    }
  }
  {//TODO:最好是删除ofd.xml重写ofd.xml这样对于非封闭章还能保证章有效
    std::unique_ptr<ofd::COFD_ZipEntry> zip(new COFD_ZipEntry(merge2ofd_path));
    if (!zip) {
      spdlog::error("[COfd_Merge] open doc failed:{}", merge2ofd_path);
      return;  //需要删除原有document.xml;
    }
    if (zip->Open('d')) {
      std::vector<WideString> entries;
      entries.push_back(m_target_documentxml_path);
      zip->DeleteEntries(entries);
    } else {
      spdlog::error("[COfd_Merge] open zip failed for delete document:{}.\n", merge2ofd_path);
      return;
    }
  }
  m_target_zip.reset(new COFD_ZipEntry(merge2ofd_path));
  m_target_zip->Open('a');//a和w没有办法读出已有条目
  m_been_merged_zip.reset(new COFD_ZipEntry(been_merged_ofd_path));
  m_been_merged_zip->Open('r');
}
COfd_Merge::~COfd_Merge() {
  
}

int COfd_Merge::do_merge() {
  if (!m_target_zip || !m_been_merged_zip)
    return XILOU_E_FAILEDOPENZIP;
  auto t =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::stringstream ss;
  ss << std::put_time(std::localtime(&t), "%Y%m%d%H%M%S");  
  ByteString doc2 = ByteString::Format("xilou_doc_%s", ss.str().c_str());
  m_doc_root = WideString::FromASCII(doc2.AsStringView());
  merge_doc();//只能在这，需要m_doc_root, m_second_file_base_url完成初始化
  for (auto entry : m_been_merged_zip->entries()) {
    std::vector<uint8_t> buf;
    if (m_been_merged_zip->ReadBinary(entry.AsStringView(), &buf) > 0) {
      ByteString newdoc;
      newdoc = doc2 + "/" + entry;
      update_id(&buf);
      if (0 != m_target_zip->WriteBinary(newdoc.AsStringView(), &buf)) {
        spdlog::error("[COfd_Merge::do_merge] failed write entry :{}  to:{}",
                      entry.c_str(), newdoc.c_str());
      }
    } else {
      spdlog::error("[COfd_Merge::do_merge] read entry failed:{}",
                    entry.c_str());
    }
  }
  
  return XILOU_E_SUC;
}
std::unique_ptr<CFX_XMLDocument> COfd_Merge::getDoc(ofd::COFD_ZipEntry* zip,
                                                    WideString* baseurl) {
  CFX_XMLDocument* ofd_xml = zip->ReadAndCacheXml("ofd.xml");
  if (!ofd_xml) {
    spdlog::error("[getDoc] read ofd.xml failed from");
    return nullptr;
  }
  std::unique_ptr<COFD_ParseOfdXml> ofd_xml_;
  ofd_xml_.reset(new COFD_ParseOfdXml(ofd_xml));
  if (ofd_xml_->DocNum() <= 0)
    return nullptr;
  auto doc_info = ofd_xml_->GetDocInfo(0);
  if (!doc_info) {
    spdlog::error("[COfd_Merge] get doc info failed ");
    return nullptr;
  }
  if (baseurl) {
    *baseurl = doc_info->doc_root;
  }
  std::vector<uint8_t> buf;
  zip->ReadBinary(doc_info->doc_root.ToUTF8().AsStringView(), &buf);
  CFX_XMLParser parser(pdfium::MakeRetain<CFX_ReadOnlyMemoryStream>(buf));
  return parser.Parse();
}
int COfd_Merge::update_id(std::vector<uint8_t>* buf) {

  std::string check_xml((char*)&(*buf)[0],
                        buf->size() > 100 ? 100 : buf->size());
  if (check_xml.find("<?xml") == std::string::npos &&
      check_xml.find("<ofd:") == std::string::npos) {
    return XILOU_E_FAILEREPLACEID;
  }
  std::string content((char *) & (*buf)[0], buf->size());

  std::regex id_regex(R"!((ID|Relative|DrawParam|TemplateID|ResourceID|PageID|RefId|PageRef|Font)="(\d+)")!");
  std::string updated_content;
  std::sregex_iterator begin(content.begin(), content.end(), id_regex);
  std::sregex_iterator end;
  std::size_t last_pos = 0;

  //TODO:大纲要调整：//<od:ObjectRef PageRef="1">16</od:ObjectRef> PageRef是页id, 16是对象id
  //TODO:<ofd:Res xmlns:ofd="http://www.ofdspec.org/2016" BaseLoc="Res">//BaseLoc要调整，绿页显示不了
  for (std::sregex_iterator i = begin; i != end; ++i) {
    std::smatch match = *i;
    updated_content.append(content, last_pos, match.position() - last_pos);
    int id = std::stoi(match[2].str());
    updated_content.append(match[1].str() + "=\"" + std::to_string(id + m_base_id) +
                           "\"");
    last_pos = match.position() + match.length();
  }
  updated_content.append(content, last_pos);
  if (!updated_content.empty()) {
    update(updated_content);
    buf->clear();
    buf->resize(updated_content.length());
    buf->assign(updated_content.begin(), updated_content.end());
    return XILOU_E_SUC;
  }
  return XILOU_E_FAILEREPLACEID;
}

void COfd_Merge::merge_commondata(CFX_XMLElement* root, WideString baseurl) {
  auto target_common = m_doc_node->GetFirstChildNamed_NoPrefix(L"CommonData");
  if (!target_common) {
    target_common =
        m_document_xml->CreateNode<CFX_XMLElement>(L"ofd:CommonData");
    m_doc_node->AppendFirstChild(target_common);
  }
  CFX_XMLElement* common = root->GetFirstChildNamed_NoPrefix(L"CommonData");
  if (!common)
    return;
  auto max_id_node = common->GetFirstChildNamed_NoPrefix(L"MaxUnitID");
  if (max_id_node) {
    auto data = max_id_node->GetTextData().GetInteger();
    if (data > 0) {
      data = data +  m_base_id + 1000;//TODO:要遍历所有对象才能保证max_id的有效性
      auto target_max_node =
          target_common->GetFirstChildNamed_NoPrefix(L"MaxUnitID");
      if (target_max_node) {
        target_max_node->RemoveAllChildren();
        auto text = m_document_xml->CreateNode<CFX_XMLText>(WideString::Format(L"%d", data));
        target_max_node->AppendFirstChild(text);
      }
    }
  }

  for (auto* child = common->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (!elem)
      continue;
    if (elem->GetLocalTagName() == L"PublicRes" ||
        elem->GetLocalTagName() == L"DocumentRes") {
      auto res_url = elem->GetTextData();
      WideString new_res;
      if (res_url[0] == L'/') {
        new_res = L"/" + m_doc_root + res_url;
      } else {
        new_res = L"/" + m_doc_root
                  + (baseurl[0] == L'/' ? L"" : L"/") + baseurl
                  + L"/" + res_url;
      }
      auto res_node =
          m_document_xml->CreateNode<CFX_XMLElement>(L"ofd:PublicRes");
      target_common->AppendLastChild(res_node);
      auto text = m_document_xml->CreateNode<CFX_XMLText>(new_res);
      res_node->AppendFirstChild(text);
    }  // end of PublicRes/DocumentRes
    if (elem->GetLocalTagName() == L"TemplatePage") {
      auto template_id = elem->GetAttribute(L"ID").GetInteger();
      if (template_id > 0) {
        auto template_url = elem->GetAttribute(L"BaseLoc");
        WideString new_template;
        if (template_url[0] == L'/') {
          new_template = L"/" + m_doc_root + template_url;
        } else {
          new_template = L"/" + m_doc_root
                       + (baseurl[0] == L'/' ? L"" : L"/") + baseurl
                       + L"/" + template_url;
        }
        auto tmpl_node =
            m_document_xml->CreateNode<CFX_XMLElement>(L"ofd:TemplatePage");
        target_common->AppendLastChild(tmpl_node);
        tmpl_node->SetAttribute(L"BaseLoc", new_template);
        auto zorder = elem->GetAttribute(L"ZOrder");
        if (zorder.GetStringLength() > 0)
          tmpl_node->SetAttribute(L"ZOrder", zorder);
        template_id += m_base_id;
        tmpl_node->SetAttribute(L"ID", WideString::Format(L"%d", template_id));
      }
    }  // end of TemplatePage
  }
}

void COfd_Merge::merge_doc() {
  WideString baseurl;
  auto doc = getDoc(m_been_merged_zip.get(), &baseurl);
  if (!doc)
    return;
  CFX_XMLElement* root = doc->GetRoot();
  if (!root) {
    spdlog::error("[merge_doc] xml root is null.\n");
    return;
  }
  CFX_XMLElement* document_node =
      root->GetFirstChildNamed_NoPrefix(L"Document");
  if (!document_node) {
    spdlog::error("[merge_doc] xml document_node is null.\n");
    return;
  }
  if (baseurl.GetStringLength() > 0) {
    auto len = baseurl.GetLength();
    if (len > 4 && 0 == baseurl.Last(4).CompareNoCase(L".xml")) {
      len -= 5;
      while (len >= 0) {
        if (baseurl[len] == L'\\' || baseurl[len] == L'/')
          break;
        len--;
      }
      if (len >= 0)
        baseurl = baseurl.Substr(0, len);
    }
  }
  m_second_file_base_url = baseurl;
  merge_commondata(document_node, baseurl);
  merge_pages(document_node, baseurl);
  merge_bytag(document_node, baseurl, L"Annotations");
  merge_bytag(document_node, baseurl, L"CustomTags");
  auto stream = pdfium::MakeRetain<BinaryWriteStream>();
  stream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  m_doc_node->Save(stream);
  if (0 != m_target_zip->WriteBinary(
               m_target_documentxml_path.ToUTF8().AsStringView(),
                                     stream->getBufAddr())) {
    spdlog::error("[merge_doc] failed write entry :{}",
                  m_target_documentxml_path.ToUTF8().c_str());
  }
}

void COfd_Merge::merge_pages(CFX_XMLElement* root, WideString baseurl) {
  CFX_XMLElement* pages = root->GetFirstChildNamed_NoPrefix(L"Pages");
  if (!pages) {
    spdlog::error("[merge_pages()] xml pages is null.\n");
    return;
  }
  CFX_XMLElement* page = pages->GetFirstChildNamed_NoPrefix(L"Page");
  auto target_pages = m_doc_node->GetFirstChildNamed_NoPrefix(L"Pages");
  while (page) {
    if (page->HasAttribute(L"ID") && page->HasAttribute(L"BaseLoc")) {
      WideString base_loc = page->GetAttribute(L"BaseLoc");
      if (!base_loc.IsEmpty()) {
        int pageid = page->GetAttribute(L"ID").GetInteger();
        WideString new_loc;
        if (base_loc[0] == L'/') {
          new_loc = L"/" + m_doc_root + base_loc;
        } else {
          new_loc = L"/" + m_doc_root
                  + (baseurl[0] == L'/' ? L"" : L"/") + baseurl
                  + L"/" + base_loc;
        }
        auto page_node =
            m_document_xml->CreateNode<CFX_XMLElement>(L"ofd:Page");
        target_pages->AppendLastChild(page_node);  // TODO:还要考虑插入的需求
        page_node->SetAttribute(L"BaseLoc", new_loc);
        pageid += m_base_id;
        page_node->SetAttribute(L"ID", WideString::Format(L"%d", pageid));
      }
    }
    page = page->GetNextSiblingNamed_NoPrefix(L"Page");
  }
}

void COfd_Merge::merge_bytag(CFX_XMLElement* root,
                             WideString baseurl,
                             WideString tag) {

  auto annot = root->GetFirstChildNamed_NoPrefix(tag.AsStringView());
  while (annot) {
    auto annot_url = annot->GetTextData();
    WideString new_annot;
    if (annot_url[0] == L'/') {
      new_annot = L"/" + m_doc_root + annot_url;
    } else {
      new_annot = L"/" + m_doc_root
                  + (baseurl[0] == L'/' ? L"" : L"/") + baseurl
                  + L"/" + annot_url;
    }
    WideString with_tag = L"ofd:" + tag;
    auto annot_node =
        m_document_xml->CreateNode<CFX_XMLElement>(with_tag);
    m_doc_node->AppendLastChild(annot_node);
    auto text = m_document_xml->CreateNode<CFX_XMLText>(new_annot);
    annot_node->AppendFirstChild(text);
    annot = annot->GetNextSiblingNamed_NoPrefix(tag.AsStringView());
  }
}

void COfd_Merge::update(std::string& content) {
  std::string check = content.substr(0, 150);
  if (check.find("<ofd:Annotations") != std::string::npos) {
    update_annotations(content);
  }
  if (check.find("<ofd:Res") != std::string::npos) {
    update_resource(content);
  }
}

void COfd_Merge::modify_location_node(CFX_XMLDocument* doc,
  CFX_XMLElement* parent,
  WideString tag,
  WideString child_tag) {
  CFX_XMLElement* page = parent->GetFirstChildNamed_NoPrefix(tag.AsStringView());
  while (page) {
    auto fileloc_node = page->GetFirstChildNamed_NoPrefix(child_tag.AsStringView());
    if (fileloc_node) {
      auto file_url = fileloc_node->GetTextData();
      WideString new_file_url;
      if (file_url[0] == L'/') {
        new_file_url = L"/" + m_doc_root + file_url;
      } else {
        new_file_url =
            L"/" + m_doc_root + L"/" + m_second_file_base_url + L"/" + file_url;
      }
      fileloc_node->RemoveAllChildren();
      auto text = doc->CreateNode<CFX_XMLText>(new_file_url);
      fileloc_node->AppendFirstChild(text);
    }
    page = page->GetNextSiblingNamed_NoPrefix(tag.AsStringView());
  }
}
void COfd_Merge::update_annotations(std::string& content) {
  pdfium::span<const uint8_t> data((const unsigned char*)content.c_str(),
                                   content.length());
  CFX_XMLParser parser(pdfium::MakeRetain<CFX_ReadOnlyMemoryStream>(data));
  auto doc = parser.Parse();
  CFX_XMLElement* root = doc->GetRoot();
  if (!root)
    return;
  auto annotations_node = root->GetFirstChildNamed_NoPrefix(L"Annotations");
  if (!annotations_node)
    return;
  modify_location_node(doc.get(), annotations_node, L"Page", L"FileLoc");
  auto stream = pdfium::MakeRetain<BinaryWriteStream>();
  stream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  annotations_node->Save(stream);
  auto buf = stream->getBufAddr();
  if (buf)
    content.assign((char*)&(*buf)[0], buf->size());
}

void COfd_Merge::update_resource(std::string& content) {
  pdfium::span<const uint8_t> data((const unsigned char*)content.c_str(),
                                   content.length());
  CFX_XMLParser parser(pdfium::MakeRetain<CFX_ReadOnlyMemoryStream>(data));
  auto doc = parser.Parse();
  CFX_XMLElement* root = doc->GetRoot();
  if (!root)
    return;
  auto res_node = root->GetFirstChildNamed_NoPrefix(L"Res");
  if (!res_node)
    return;
  WideString base_loc = L"";
  if (res_node->HasAttribute(L"BaseLoc")) {
    base_loc = res_node->GetAttribute(L"BaseLoc");
  }
  WideString new_base_loc;
  if (base_loc[0] == L'/') {
    new_base_loc = L"/" + m_doc_root + base_loc;
  } else {
    WideString tmp_base_loc;
    if (m_second_file_base_url[0] != L'/')
      tmp_base_loc = L"/" + m_second_file_base_url;
    else
      tmp_base_loc = m_second_file_base_url;
    new_base_loc = L"/" + m_doc_root + tmp_base_loc + L"/" + base_loc;
  }
  res_node->SetAttribute(L"BaseLoc", new_base_loc);
  auto stream = pdfium::MakeRetain<BinaryWriteStream>();
  stream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  res_node->Save(stream);
  auto buf = stream->getBufAddr();
  if (buf)
    content.assign((char*)&(*buf)[0], buf->size());
}
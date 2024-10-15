#include "ofd/cofd_common.h"
#include "xilou_impl/xilou_xmlpackage.h"
#include "xilou_impl/xilou_xmlpage.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/widestring.h"
#include "ofd/cofd_zipentry.h"
#include "xilou_impl/xilou_xmldocument.h"
#include <iostream>
using namespace ofd;
namespace xilou {
Cxml_package::Cxml_package() : m_id(5856) {
  if (!m_curdoc) {
    if (!m_ofdxml) {
      create_ofdxml();
    }
    auto doc = std::make_unique<Cxml_Document>(m_ofdxml.get(), this);
    m_curdoc = doc.get();
    m_docs.push_back(std::move(doc));
    m_curdoc->create();
  }
}
Cxml_package::~Cxml_package() {
  
}

void Cxml_package::appendPage(std::unique_ptr<Cxml_page> page) {
  m_pages.push_back(std::move(page));
}

uint32_t Cxml_package::getUniqueID() {
  return ++m_id;
}

uint32_t Cxml_package::getFontID(const WideString& wsFontName) {
  return m_curdoc->getFontID(wsFontName);
}

uint32_t Cxml_package::getFontID(const CPDF_Font* font) {
  return m_curdoc->getFontID(font);
}

bool Cxml_package::save_ofdxml() {
  if (m_ofdxml && m_ofdxml->GetRoot() && m_ofdxml->GetRoot()->GetFirstChild()) {
    auto stream = pdfium::MakeRetain<BinaryWriteStream>();
    stream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    m_ofdxml->GetRoot()->GetFirstChild()->Save(stream);
    if (0 != m_zip->WriteBinary(ByteString("OFD.xml").AsStringView(),
                                stream->getBufAddr())) {
      return false;
    }
    return true;
  }
  return false;
}
void Cxml_package::save_page_annot(Cxml_page* page,
                                   int page_index,
                                   uint32_t pageID) {
  auto annot_count = page->annotCount();
  auto stream = pdfium::MakeRetain<BinaryWriteStream>();
  stream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
  stream->WriteString("<ofd:PageAnnot xmlns:ofd=\"http://www.ofdspec.org/2016\">");
  for (size_t i = 0; i < annot_count; i++) {
    auto annot = page->getAnnot(i);
    annot->annot2stream(stream);
  }
  stream->WriteString("</ofd:PageAnnot>");
  WideString baseurl(m_curdoc->getDocBaseUrl());
  WideString page_annot_url =
      L"/" + baseurl + WideString::Format(L"Pages_%d/Annotation.xml", page_index);

  if (0 != m_zip->WriteBinary(page_annot_url.ToUTF8().AsStringView(),
                              stream->getBufAddr())) {
    return ;
  }
  //update Annotations.xml
  m_curdoc->appendAnnot(pageID, page_annot_url.AsStringView());
}
bool Cxml_package::save_page(Cxml_page* page, int page_index) {
  page->genPageXml();
  auto page_xml = page->getOfdPageNode();
  auto stream = pdfium::MakeRetain<BinaryWriteStream>();
  auto pageID = getUniqueID();
  if (page_xml->GetRoot() && page_xml->GetRoot()->GetFirstChild()) {
    stream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    page_xml->GetRoot()->GetFirstChild()->Save(stream);
    WideString baseurl(m_curdoc->getDocBaseUrl());
    WideString pageurl =
        L"/" + baseurl + WideString::Format(L"Pages_%d/Content.xml", page_index);

    if (0 != m_zip->WriteBinary(pageurl.ToUTF8().AsStringView(),
                                stream->getBufAddr())) {
      return false;
    }
    m_curdoc->appendPage(pageurl.AsStringView(), pageID);
  }
  save_page_annot(page, page_index, pageID);
  return true;
}
bool Cxml_package::save() {
  if (!m_zip->Opened())
    m_zip->Open('w');
  /*if (!m_curdoc) {
    if (!m_ofdxml) {
      create_ofdxml();
    }
    auto doc = std::make_unique<Cxml_Document>(m_ofdxml.get(), this);
    m_curdoc = doc.get();
    m_docs.push_back(std::move(doc));
    m_curdoc->create();
  }*/
  int page_index = 0;
  for (auto& page : m_pages) {
    if (!save_page(page.get(), page_index++))
      return false;
  }
  m_curdoc->save(m_zip.get());
  save_ofdxml();
  return true;
}

bool Cxml_package::create_ofdxml() {
  m_ofdxml = std::make_unique<CFX_XMLDocument>();
  auto ofd_node = m_ofdxml->CreateNode<CFX_XMLElement>(L"ofd:OFD");
  m_ofdxml->GetRoot()->AppendFirstChild(ofd_node);
  ofd_node->SetAttribute(L"Version", L"1.0");
  ofd_node->SetAttribute(L"DocType", L"OFD");
  ofd_node->SetAttribute(L"xmlns:ofd", L"http://www.ofdspec.org/2016");
  return true;
}
void Cxml_package::setpath(const char* utf8_path) {
#if !defined(OS_WIN)
  m_ofd_path = FX_UTF8Decode(utf8_path);
#else
  ByteString utf16_byte = FX_UTF8Decode(utf8_path).ToUTF16LE();
  m_ofd_path = WideString::FromUTF16LE(
      reinterpret_cast<const unsigned short*>(utf16_byte.raw_span().data()),
      utf16_byte.GetLength());
#endif
  m_zip.reset(new COFD_ZipEntry(utf8_path));
  m_zip->Open('w');
}

uint32_t Cxml_package::appendImageRes(WideStringView image_name,
                        std::vector<uint8_t>* img_data) {

  return m_curdoc->appendImageRes(image_name, img_data);
}

void Cxml_package::updatePageArea(const CFX_RectF& pagearea) {
  m_curdoc->updatePageArea(pagearea);
}
}// end of namespace xilou
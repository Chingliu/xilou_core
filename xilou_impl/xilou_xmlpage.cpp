#include "xilou_impl/xilou_xmlpage.h"
#include "xilou_impl/xilou_xmlpackage.h"
#include "ofd/cofd_common.h"
#include "core/fxcrt/bytestring.h"
#include "core/fxcrt/widestring.h"
#include "third_party/base/check_op.h"
#include "core/fxcrt/xml/cfx_xmltext.h"
#include <cmath>
namespace xilou {

  Cxml_pageobj::Cxml_pageobj(Cxml_page* page) : m_page(page) {
  m_root = m_page->getPageXmlRoot();
    m_hasParentForm = false;
  }
Cxml_pageobj::~Cxml_pageobj() {
}

CFX_XMLElement* Cxml_pageobj::getObjectRoot() {
  return m_objectroot;
}


CFX_Matrix Cxml_pageobj::transformOfdCtm(CFX_Matrix& ctm,
                                         CFX_Matrix& annotCtm,
                                         float& scale) {
  CFX_Matrix ofdCtm;
  float angle = 0.f;
  ctm.Concat(annotCtm);
  ofdCtm.Scale(1, -1);
  ctm.Concat(ofdCtm);
  ofdCtm = ctm;
  ofdCtm.b = -ofdCtm.b;
  ofdCtm.c = -ofdCtm.c;

  if (fabs(ofdCtm.d) > 0.f) {
    angle = atan(ofdCtm.c / ofdCtm.d);
    scale = fabs(ofdCtm.d / cos(angle));
  } else if (fabs(ofdCtm.c) > 0.f) {
    // angle = 90.f;
    scale = fabs(ofdCtm.c);
  } else {
    // assert(0);
  }
  ofdCtm.a /= scale;
  ofdCtm.b /= scale;
  ofdCtm.c /= scale;
  ofdCtm.d /= scale;
  ofdCtm.e = 0;
  ofdCtm.f = 0;

  return ofdCtm;
}

Cxml_page::Cxml_page(Cxml_package* package) : m_package(package) {
  m_pagexml = std::make_unique<CFX_XMLDocument>();
  m_fpdfpage = nullptr;
  m_fpdfdoc = nullptr;
}
Cxml_page::~Cxml_page() {

}

void Cxml_page::appendObj(std::unique_ptr<Cxml_pageobj> xml_pageobj) {
  //要是碰到超多xml节点的文件，怎么办
  //工程文件，CAD,超多path
  // m_pagenode->AppendLastChild(xml_pageobj->getObjectRoot());
  m_objects.push_back(std::move(xml_pageobj));
}

void Cxml_page::genPageXml() {
  m_pagenode = m_pagexml->CreateNode<CFX_XMLElement>(L"ofd:Page");
  m_pagenode->SetAttribute(L"xmlns:ofd", L"http://www.ofdspec.org/2016");
  m_pagexml->GetRoot()->AppendFirstChild(m_pagenode);
  CFX_XMLElement* layernode = nullptr;
  auto area_node = m_pagenode->GetFirstChildNamed_NoPrefix(L"Area");
  if (!area_node) {
    area_node = m_pagexml->CreateNode<CFX_XMLElement>(L"ofd:Area");
    m_pagenode->AppendFirstChild(area_node);
    auto pbox = m_pagexml->CreateNode<CFX_XMLElement>(L"ofd:PhysicalBox");
    area_node->AppendLastChild(pbox);
    auto area_rect =
        WideString::Format(L"%.3f %.3f %.3f %.3f",
                                        ofd_one_72_point_to_mm(m_pdfbox.Left()),
                                        ofd_one_72_point_to_mm(m_pdfbox.Top()),
                                        ofd_one_72_point_to_mm(m_pdfbox.Width()), 
                                        ofd_one_72_point_to_mm(m_pdfbox.Height()));
    auto text = m_pagexml->CreateNode<CFX_XMLText>(area_rect);
    pbox->AppendLastChild(text);
  }
  auto content_node = m_pagenode->GetFirstChildNamed_NoPrefix(L"Content");
  if (!content_node) {
    content_node = m_pagexml->CreateNode<CFX_XMLElement>(L"ofd:Content");
    m_pagenode->AppendLastChild(content_node);
  }
  if (!content_node)
    return;
  layernode = content_node->GetFirstChildNamed_NoPrefix(L"Layer");
  if (!layernode) {
    layernode = m_pagexml->CreateNode<CFX_XMLElement>(L"ofd:Layer");
    content_node->AppendLastChild(layernode);
    layernode->SetAttribute(
        L"ID", WideString::Format(L"%d", m_package->getUniqueID()));
    layernode->SetAttribute(L"Type", L"Body");
  }
  if (!layernode)
    return;

  for (auto& object : m_objects) {
    if (object && object->getObjectRoot())
      layernode->AppendLastChild(object->getObjectRoot());
  }
}


uint32_t Cxml_page::getUniqueID() {
  return m_package->getUniqueID();
}

uint32_t Cxml_page::getFontID(const WideString& wsFontName) {
  return m_package->getFontID(wsFontName);
}

uint32_t Cxml_page::appendImageRes(WideStringView image_name,
                                   std::vector<uint8_t>* img_data) {
  return m_package->appendImageRes(image_name, img_data);
}
uint32_t Cxml_page::getFontID(const CPDF_Font* font) {
  return m_package->getFontID(font);
}

void Cxml_page::appendPageAnnot(std::unique_ptr<Cxml_pageAnnot> pageAnnot) {
  m_pageAnnots.push_back(std::move(pageAnnot));
}


StringWriteStream::StringWriteStream() = default;

StringWriteStream::~StringWriteStream() = default;

bool StringWriteStream::WriteBlock(const void* pData, size_t size) {
  stream_.write(static_cast<const char*>(pData), size);
  return true;
}

BinaryWriteStream::BinaryWriteStream() = default;

BinaryWriteStream::~BinaryWriteStream() = default;

bool BinaryWriteStream::WriteBlock(const void* pData, size_t size) {
  //stream_.write(static_cast<const char*>(pData), size);
  m_buf.insert(m_buf.end(), static_cast<const uint8_t*>(pData),
               static_cast<const uint8_t*>(pData) + size);
  return true;
}


Cxml_pageAnnot::Cxml_pageAnnot() : m_bAnnot(false) {
}
Cxml_pageAnnot::~Cxml_pageAnnot() {
}

void Cxml_page::annot2stream(
    const RetainPtr<IFX_RetainableWriteStream>& pXMLStream) {

  auto annot_node = m_pagexml->CreateNode<CFX_XMLElement>(L"ofd:Annot");
  annot_node->SetAttribute(L"ID", WideString::Format(L"%u", getUniqueID()));
  auto ap_node = m_pagexml->CreateNode<CFX_XMLElement>(L"ofd:Appearance");
  CFX_FloatRect annot_rect = getAnnotRect();
  ap_node->SetAttribute(L"Boundary", WideString::Format(L"%.3f %.3f %.3f %.3f",
                         ofd_one_72_point_to_mm(annot_rect.Left()),
                         ofd_one_72_point_to_mm(m_pdfbox.Height() - annot_rect.Top()),
                         ofd_one_72_point_to_mm(annot_rect.Width()),
                         ofd_one_72_point_to_mm(annot_rect.Height())));
  annot_node->AppendLastChild(ap_node);
  for (auto& object : m_objects) {
    if (object && object->getObjectRoot())
      ap_node->AppendLastChild(object->getObjectRoot());
  }
  annot_node->Save(pXMLStream);
}
}
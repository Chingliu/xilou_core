#include "ofd/cofd_common.h"
#include "xilou_impl/xilou_xmlpackage.h"
#include "xilou_impl/xilou_xmlpage.h"
#include "xilou_impl/xilou_xmldocument.h"
#include "xilou_impl/xilou_xmlres.h"
#include "core/fxcrt/xml/cfx_xmltext.h"
#include "ofd/cofd_zipentry.h"
using namespace ofd;
namespace xilou {
Cxml_Document::Cxml_Document(CFX_XMLDocument* ofdxml, Cxml_package* pkg)
    : m_ofdxml(ofdxml), m_pkg(pkg), m_pagearea(0.0f, 0.0f, 210.0f, 297.0f) {
  
}

Cxml_Document::~Cxml_Document() {

}

void Cxml_Document::create(int index) {
  createDocBody(index);
  m_doc = std::make_unique<CFX_XMLDocument>();
  auto doc_node = m_doc->CreateNode<CFX_XMLElement>(L"ofd:Document");
  m_doc->GetRoot()->AppendFirstChild(doc_node);
  doc_node->SetAttribute(L"xmlns:ofd", L"http://www.ofdspec.org/2016");
  //TODO common data

}
void Cxml_Document::createDocBody(int index) {
  auto docbody = m_ofdxml->CreateNode<CFX_XMLElement>(L"ofd:DocBody");
  m_ofdxml->GetRoot()->GetFirstChild()->AppendFirstChild(docbody);
  auto docinfo = m_ofdxml->CreateNode<CFX_XMLElement>(L"ofd:DocInfo");
  docbody->AppendFirstChild(docinfo);

  auto docid = m_ofdxml->CreateNode<CFX_XMLElement>(L"ofd:DocID");
  docinfo->AppendFirstChild(docid);
  auto text = m_ofdxml->CreateNode<CFX_XMLText>(L"Y2hpbmdsaXV5dTE1MjA3OTM1ODU2");
  docid->AppendFirstChild(text);

  auto doccreator = m_ofdxml->CreateNode<CFX_XMLElement>(L"ofd:Creator");
  docinfo->AppendFirstChild(doccreator);
  text = m_ofdxml->CreateNode<CFX_XMLText>(L"xilou reader created");
  doccreator->AppendFirstChild(text);

  auto doccreatorversion = m_ofdxml->CreateNode<CFX_XMLElement>(L"ofd:CreatorVersion");
  docinfo->AppendFirstChild(doccreatorversion);
  text = m_ofdxml->CreateNode<CFX_XMLText>(L"1.0");
  doccreatorversion->AppendFirstChild(text);


  auto doccreatedate = m_ofdxml->CreateNode<CFX_XMLElement>(L"ofd:CreationDate");
  docinfo->AppendFirstChild(doccreatedate);
  text = m_ofdxml->CreateNode<CFX_XMLText>(L"2023-12-22");
  doccreatedate->AppendFirstChild(text);

  //docinfo end
  m_docBaseUrl = WideString::Format(L"Doc_%d/", index);
  auto docroot = m_ofdxml->CreateNode<CFX_XMLElement>(L"ofd:DocRoot");
  m_docroot = m_docBaseUrl + L"Document.xml";
  text = m_ofdxml->CreateNode<CFX_XMLText>(m_docroot);
  docroot->AppendFirstChild(text);
  docbody->AppendLastChild(docroot);

}
void Cxml_Document::createCommonData() {
  auto doc_node = ToXMLElement(m_doc->GetRoot()->GetFirstChild());
  auto cd_node = doc_node->GetFirstChildNamed_NoPrefix(L"CommonData");
  if (!cd_node) {
    cd_node = m_doc->CreateNode<CFX_XMLElement>(L"ofd:CommonData");
    doc_node->AppendFirstChild(cd_node);
  }
  auto maxid_node = m_doc->CreateNode<CFX_XMLElement>(L"ofd:MaxUnitID");
  cd_node->AppendLastChild(maxid_node);
  auto text = m_doc->CreateNode<CFX_XMLText>(
      WideString::Format(L"%d", m_pkg->getUniqueID()));
  maxid_node->AppendFirstChild(text);

  auto pagearea = m_doc->CreateNode<CFX_XMLElement>(L"ofd:PageArea");
  cd_node->AppendLastChild(pagearea);
  auto pbox = m_doc->CreateNode<CFX_XMLElement>(L"ofd:PhysicalBox");
  pagearea->AppendLastChild(pbox);
  auto doc_area =
      WideString::Format(L"%f %f %f %f", m_pagearea.Left(), m_pagearea.Top(),
                         m_pagearea.Width(), m_pagearea.Height());
  text = m_doc->CreateNode<CFX_XMLText>(doc_area);
  pbox->AppendLastChild(text);

}

bool Cxml_Document::saveRes(ofd::COFD_ZipEntry* pzip) {
  bool result = false;
  for (auto& res : m_res) {
    result = res->save(pzip);
  }
  return result;
}
bool Cxml_Document::saveAnnots(ofd::COFD_ZipEntry* pzip) {
  if (m_annots&& m_annots->GetRoot() && m_annots->GetRoot()->GetFirstChild()) {
    auto stream = pdfium::MakeRetain<BinaryWriteStream>();
    stream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    m_annots->GetRoot()->GetFirstChild()->Save(stream);
    auto annots_url = m_docBaseUrl + L"Annotations.xml";
    if (0 != pzip->WriteBinary(annots_url.ToUTF8().AsStringView(),
                               stream->getBufAddr())) {
      return false;
    }
  }
  return true;
}

//此函数需要在页面及相关资源已完成save后再调用
//因为MaxId，PageArea需在最后阶段更新
bool Cxml_Document::save(ofd::COFD_ZipEntry* pzip) {
  createCommonData();
  if (m_doc && m_doc->GetRoot() && m_doc->GetRoot()->GetFirstChild()) {
    auto stream = pdfium::MakeRetain<BinaryWriteStream>();
    stream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    m_doc->GetRoot()->GetFirstChild()->Save(stream);
    if (0 != pzip->WriteBinary(m_docroot.ToUTF8().AsStringView(),
                                stream->getBufAddr())) {
      return false;
    }
    bool bret = true;
    bret &= saveRes(pzip);
    bret &= saveAnnots(pzip);
    return bret;
  }
  return false;
}
void Cxml_Document::appendPage(WideStringView pageurl, uint32_t pageid) {
  auto doc_node = ToXMLElement(m_doc->GetRoot()->GetFirstChild());
  auto pages_node = doc_node->GetFirstChildNamed_NoPrefix(L"Pages");
  if (!pages_node) {
    pages_node = m_doc->CreateNode<CFX_XMLElement>(L"ofd:Pages");
    doc_node->AppendFirstChild(pages_node);
  }
  auto page_node = m_doc->CreateNode<CFX_XMLElement>(L"ofd:Page");
  pages_node->AppendLastChild(page_node);
  page_node->SetAttribute(L"ID", WideString::Format(L"%d", pageid));
  page_node->SetAttribute(L"BaseLoc", WideString(pageurl));
}

uint32_t Cxml_Document::getUniqueID() {
  return m_pkg->getUniqueID();
}

Cxml_Res* Cxml_Document::createPublicRes() {
  auto publicRes = std::make_unique<Cxml_Res>(this);
  publicRes->create();
  auto pres = publicRes.get();
  WideString wsFileName = publicRes->getName();
  addPublicResNode(wsFileName);
  m_res.push_back(std::move(publicRes));
  return pres;
}
uint32_t Cxml_Document::getFontID(const WideString& wsFontName) {

  for (auto& res : m_res) {
    WideString wsName = res->getName();
    if ((size_t)-1 != wsName.Find(L"PublicRes")) {
      return res->getFontID(wsFontName);
    }
  }
  auto pRes = createPublicRes();
  return pRes->getFontID(wsFontName);
}

uint32_t Cxml_Document::appendImageRes(WideStringView image_name,
                                       std::vector<uint8_t>* img_data) {
  auto pzip = m_pkg->getZipEntry();
  for (auto& res : m_res) {
    WideString wsName = res->getName();
    if ((size_t)-1 != wsName.Find(L"PublicRes")) {
      // TODO 图像要去重
      auto image_url = res->getResBaseUrl() + image_name;
      if (0 != pzip->WriteBinary(image_url.ToUTF8().AsStringView(), img_data)) {
        return 0;
      }      
      return res->appendImageRes(image_name);
    }
  }
  auto pRes = createPublicRes();
  // TODO 图像要去重
  auto image_url = pRes->getResBaseUrl() + image_name;
  if (0 != pzip->WriteBinary(image_url.ToUTF8().AsStringView(), img_data)) {
    return 0;
  }      
  return pRes->appendImageRes(image_name);
}

uint32_t Cxml_Document::getFontID(const CPDF_Font* font) {
  uint32_t uID = 0;
  for (auto& res : m_res) {
    WideString wsName = res->getName();
    if ((size_t)-1 != wsName.Find(L"PublicRes")) {
      return res->getFontID(font);
    }
  }
  auto publicRes = std::make_unique<Cxml_Res>(this);
  publicRes->create();
  uID = publicRes->getFontID(font);
  WideString wsFileName = publicRes->getName();
  addPublicResNode(wsFileName);
  m_res.push_back(std::move(publicRes));
  return uID;
}

bool Cxml_Document::addPublicResNode(const WideString& wsFileName) {
  auto doc_node = ToXMLElement(m_doc->GetRoot()->GetFirstChild());
  auto cd_node = doc_node->GetFirstChildNamed_NoPrefix(L"CommonData");
  if (!cd_node) {
    cd_node = m_doc->CreateNode<CFX_XMLElement>(L"ofd:CommonData");
    doc_node->AppendFirstChild(cd_node);
  }
  auto res_node = m_doc->CreateNode<CFX_XMLElement>(L"ofd:PublicRes");
  cd_node->AppendLastChild(res_node);
  auto text = m_doc->CreateNode<CFX_XMLText>(wsFileName);
  res_node->AppendFirstChild(text);
  return true;
}

void Cxml_Document::appendAnnot(uint32_t pageID, WideStringView annot_url) {
  if (!m_annots) {
    m_annots = std::make_unique<CFX_XMLDocument>();
    auto annots_node = m_annots->CreateNode<CFX_XMLElement>(L"ofd:Annotations");
    m_annots->GetRoot()->AppendFirstChild(annots_node);
    annots_node->SetAttribute(L"xmlns:ofd", L"http://www.ofdspec.org");
    //update to document.xml
    auto doc_node = ToXMLElement(m_doc->GetRoot()->GetFirstChild());
    auto doc_annots_node = m_doc->CreateNode<CFX_XMLElement>(L"ofd:Annotations");
    doc_node->AppendLastChild(doc_annots_node);
    auto annots_txt = m_doc->CreateNode<CFX_XMLText>(L"Annotations.xml");
    doc_annots_node->AppendLastChild(annots_txt);
  }
  if (!m_annots)
    return;
  auto root = m_annots->GetRoot();
  auto annots_node = root->GetFirstChildNamed(L"ofd:Annotations");
  if (!annots_node) {
    annots_node = m_annots->CreateNode<CFX_XMLElement>(L"ofd:Annotations");
    m_annots->GetRoot()->AppendFirstChild(annots_node);
    annots_node->SetAttribute(L"xmlns:ofd", L"http://www.ofdspec.org");
  }
  if (!annots_node)
    return;
  auto page_node = m_annots->CreateNode<CFX_XMLElement>(L"ofd:Page");
  annots_node->AppendLastChild(page_node);
  page_node->SetAttribute(L"PageID", WideString::Format(L"%d", pageID));
  auto fileloc_node = m_annots->CreateNode<CFX_XMLElement>(L"ofd:FileLoc");
  page_node->AppendLastChild(fileloc_node);
  auto fileloc_data = m_annots->CreateNode<CFX_XMLText>(WideString(annot_url));
  fileloc_node->AppendLastChild(fileloc_data);
}
}//end of namespace
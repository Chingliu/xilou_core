#include "ofd/cofd_common.h"
#include "ofd/cofd_package.h"
#include "ofd/cofd_ofdxml.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"
#include "core/fpdfapi/render/cpdf_docrenderdata.h"
#include "public/fpdf_edit.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "third_party/spdlog/include/spdlog/spdlog.h"
using namespace ofd;

COFD_Package::COFD_Package(const char* zip_path) : zip_(zip_path) {
  //fake_pdf_doc_handle_ =
  //    std::make_unique<CPDF_Document>(std::make_unique<CPDF_DocRenderData>(),
  //                                    std::make_unique<CPDF_DocPageData>());
  //为可能的ofd转pdf服务
  auto pdf_handle = FPDF_CreateNewDocument();
  fake_pdf_doc_handle_.reset(CPDFDocumentFromFPDFDocument(pdf_handle));
}

COFD_Package::COFD_Package(const char* stream, size_t size) : zip_(stream, size) {
  auto pdf_handle = FPDF_CreateNewDocument();
  fake_pdf_doc_handle_.reset(CPDFDocumentFromFPDFDocument(pdf_handle));
}
COFD_Package::~COFD_Package() {

}

bool COFD_Package::ParseOFD() {
  if (!zip_.Open('r'))
    return false;
  CFX_XMLDocument* ofd_xml = zip_.ReadAndCacheXml("ofd.xml");
  if (ofd_xml) {
    ofd_xml_.reset( new COFD_ParseOfdXml(ofd_xml));
    if (ofd_xml_->DocNum() <= 0)
      return false;
    return true;
  }
  return false;
}

COFD_Document* COFD_Package::OpenDocument(unsigned int index) {
  if (!ofd_xml_ || ofd_xml_->DocNum() <= 0)
    return nullptr;
  COFD_DocInfo* info = ofd_xml_->GetDocInfo(index);
  if (!info)
    return nullptr;
  CFX_XMLDocument* document_xml = zip_.ReadAndCacheXml(info->doc_root.ToUTF8().AsStringView());
  if (!document_xml) {
    spdlog::error("[OpenDocument] read document xml failed.\n");
    return nullptr;
  }
  int len = (int)info->doc_root.GetLength();
  /*/// debug log
  WideString strDbgBuf = WideString::Format(L"[OpenDocument] open ofd document: %s(len=%d)\n", info->doc_root.c_str(), len);
  OutputDebugString(strDbgBuf.c_str());
  //*/
  if (len > 4 && 0 == info->doc_root.Last(4).CompareNoCase(L".xml")) {
    len -= 5;
    while (len >= 0) {
      if (info->doc_root[len] == L'\\' || info->doc_root[len] == L'/')
        break;
      len--;
    }
    if (len >= 0)
      return new COFD_Document(this, info,document_xml,
          info->doc_root.Substr(0, len + 1).AsStringView());
  }
  //OutputDebugString(L"[OpenDocument] ofd base url is \"\"\n");
  return new COFD_Document(this, info, document_xml, L"");
}

void COFD_Package::CloseDocument(COFD_Document* doc) {
  if (doc) {
    delete doc;
  }
}

CFX_XMLDocument* COFD_Package::ReadAndCacheXml(ByteStringView entry_name) {
  return zip_.ReadAndCacheXml(entry_name);
}

int COFD_Package::ReadBinary(ByteStringView entry_name,
                             std::vector<uint8_t>* buf) {
  return zip_.ReadBinary(entry_name, buf);
}

CPDF_Document* COFD_Package::GetFakePDFDocument() {
  return fake_pdf_doc_handle_.get();
}
#ifndef XILOU_IMPL_XILOU_XML_DOCUMENT_H
#define XILOU_IMPL_XILOU_XML_DOCUMENT_H
#include <vector>
#include <memory>
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxcrt/fx_string.h"
#include"core/fxcrt/fx_coordinates.h"
namespace ofd {
class COFD_ZipEntry;
}
class CFX_XMLDocument;
class CPDF_Font;

namespace xilou {
class Cxml_package;
class Cxml_Res;
class Cxml_Document {
 public:
  explicit Cxml_Document(CFX_XMLDocument* ofdxml, Cxml_package*pkg);

 public:
  Cxml_Document(const Cxml_Document&) = delete;
  Cxml_Document(Cxml_Document&&) = delete;
  Cxml_Document& operator=(const Cxml_Document&) = delete;
  Cxml_Document& operator=(Cxml_Document&&) = delete;
  ~Cxml_Document();

 public:
  void create(int index = 0);
  WideStringView getDocBaseUrl() { return m_docBaseUrl.AsStringView(); }
  bool save(ofd::COFD_ZipEntry *pzip);
  void appendPage(WideStringView pageurl, uint32_t pageid);
  uint32_t getUniqueID();
  uint32_t getFontID(const WideString& wsFontName);
  uint32_t getFontID(const CPDF_Font* font);
  bool addPublicResNode(const WideString& wsFileName);
  uint32_t appendImageRes(WideStringView image_name,
                          std::vector<uint8_t>* img_data);
  void updatePageArea(const CFX_RectF& pagearea) { m_pagearea = pagearea;
  }
  void appendAnnot(uint32_t pageID, WideStringView annot_url);
 private:
  void createDocBody(int index);
  void createCommonData();
  bool saveRes(ofd::COFD_ZipEntry* pzip);
  bool saveAnnots(ofd::COFD_ZipEntry* pzip);
  Cxml_Res* createPublicRes();
 private:
  UnownedPtr<CFX_XMLDocument> m_ofdxml;
  UnownedPtr<Cxml_package> m_pkg;
  //doc_0/, 带路径/
  WideString m_docBaseUrl;
  WideString m_docroot;
  std::unique_ptr<CFX_XMLDocument> m_doc;
  std::vector<std::unique_ptr<Cxml_Res>> m_res;
  CFX_RectF m_pagearea;
  std::unique_ptr<CFX_XMLDocument> m_annots;
};

}// end of namespace xilou
#endif
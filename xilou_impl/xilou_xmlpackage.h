#ifndef XILOU_IMPL_XILOU_XMLPACKAGE_H
#define XILOU_IMPL_XILOU_XMLPACKAGE_H
#include <vector>
#include <memory>

#include "core/fxcrt/widestring.h"
#include"core/fxcrt/fx_coordinates.h"
class CFX_XMLDocument;
namespace ofd {
class COFD_ZipEntry;
}
class CPDF_Font;
namespace xilou {
class Cxml_Document;
class Cxml_page;
class Cxml_package {
 public:
  explicit Cxml_package();

 public:
  Cxml_package(const Cxml_package&) = delete;
  Cxml_package(Cxml_package&&) = delete;
  Cxml_package& operator=(const Cxml_package&) = delete;
  Cxml_package& operator=(Cxml_package&&) = delete;
  ~Cxml_package();

public:
  void appendPage(std::unique_ptr<Cxml_page> page);
  uint32_t getUniqueID();
  uint32_t getFontID(const WideString& wsFontName);
  uint32_t getFontID(const CPDF_Font* font);
  bool save();
  void setpath(const char* utf8_path);
  uint32_t appendImageRes(WideStringView image_name, std::vector<uint8_t>*img_data);
  ofd::COFD_ZipEntry* getZipEntry() { return m_zip.get(); }
  void updatePageArea(const CFX_RectF& pagearea);
 private:
  bool save_page(Cxml_page* page, int page_index);
  void save_page_annot(Cxml_page* page, int page_index, uint32_t pageID);
  bool create_ofdxml();
  bool save_ofdxml();
 private:
  uint32_t m_id;
  std::vector<std::unique_ptr<Cxml_page> > m_pages;
  WideString m_ofd_path;
  std::unique_ptr<ofd::COFD_ZipEntry> m_zip;
  std::unique_ptr<CFX_XMLDocument> m_ofdxml;
  std::vector<std::unique_ptr<Cxml_Document> > m_docs;
  UnownedPtr<Cxml_Document> m_curdoc;
};

}// end of namespace xilou
#endif
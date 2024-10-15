#ifndef XILOU_IMPL_XILOU_XML_RES_H
#define XILOU_IMPL_XILOU_XML_RES_H
#include <vector>
#include <map>
#include <memory>
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxcrt/fx_string.h"
#include "core/fpdfapi/font/cpdf_font.h"

// 先用结构体，后期再优化
typedef struct FontInfo_S {
  uint32_t fontID = 0;
  fxcrt::WideString fileName = L"";
  char* fontData = NULL;
  size_t len = 0;

  ~FontInfo_S() {
    if (fontData)
      delete[] fontData;
  }
} FontInfo;

namespace ofd {
class COFD_ZipEntry;
}
class CFX_XMLDocument;
namespace xilou {
class Cxml_Document;
class Cxml_Res {
 public:
  explicit Cxml_Res(Cxml_Document* doc);

 public:
  Cxml_Res(const Cxml_Res&) = delete;
  Cxml_Res(Cxml_Res&&) = delete;
  Cxml_Res& operator=(const Cxml_Res&) = delete;
  Cxml_Res& operator=(Cxml_Res&&) = delete;
  ~Cxml_Res();

 public:
  void create();
  WideStringView getResBaseUrl() { return m_resBaseUrl.AsStringView(); }
  bool setName(WideString& name);
  WideString getName() const;
  bool save(ofd::COFD_ZipEntry *pzip);
  uint32_t getFontID(const WideString& fontName);
  uint32_t getFontID(const CPDF_Font* font);

  //存入图像资源返回此图像id
  uint32_t appendImageRes(WideStringView imgName);
 private:
  CFX_XMLElement* createFonts();
  uint32_t createFontNode(CFX_XMLElement* fontsNode, const CPDF_Font* font);
  uint32_t createFontNode(CFX_XMLElement* fontsNode,
                          const WideString& fontName,
                          const WideString& familyName);
  CFX_XMLElement* createColorSpaces();

 private:
  std::unique_ptr<CFX_XMLDocument> m_resxml;
  UnownedPtr<Cxml_Document> m_doc;
  //doc_0/, 带路径/
  WideString m_resBaseUrl;
  WideString m_name;
  std::map<const CPDF_Font*, FontInfo *> m_fontRes;
};

}// end of namespace xilou
#endif
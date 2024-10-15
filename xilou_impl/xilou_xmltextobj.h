#ifndef XILOU_IMPL_XILOU_XMLTEXTOBJECT_H
#define XILOU_IMPL_XILOU_XMLTEXTOBJECT_H
#include <vector>
#include <memory>
#include "core/fxcrt/unowned_ptr.h"
class CPDF_TextObject;
class CFX_XMLElement;
namespace xilou {
class Cxml_page;
class Cxml_package;
class Cxml_textobj :public Cxml_pageobj{
 public:
  explicit Cxml_textobj(CPDF_TextObject* pdf_textobj, Cxml_page* ofd_page);

 public:
  Cxml_textobj(const Cxml_textobj&) = delete;
  Cxml_textobj(Cxml_textobj&&) = delete;
  Cxml_textobj& operator=(const Cxml_textobj&) = delete;
  Cxml_textobj& operator=(Cxml_textobj&&) = delete;
  ~Cxml_textobj();

 public:
  void convert();

 private:
  void CreateStrokeColorNode(CFX_XMLElement* textObj);
  void CreateFillColorNode(CFX_XMLElement* textObj);
  void CreateCGTransform(CFX_XMLElement* textObj);
  WideString GetDeltaX(float& width, float scale);
  WideString GetWordStringEx(int nWordIndex) const;
  WideString GetWordStrings();

 private:
  UnownedPtr<CPDF_TextObject> m_pdftextobj;
  UnownedPtr<Cxml_page> m_ofdpage;
};

}// end of namespace xilou
#endif
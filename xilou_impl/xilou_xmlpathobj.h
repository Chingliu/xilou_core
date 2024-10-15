#ifndef XILOU_IMPL_XILOU_XMLPATHOBJECT_H
#define XILOU_IMPL_XILOU_XMLPATHOBJECT_H
#include <vector>
#include <memory>
#include "core/fxcrt/unowned_ptr.h"
class CPDF_PathObject;
class CFX_XMLElement;
namespace xilou {
class Cxml_page;
class Cxml_package;
class Cxml_pathobj :public Cxml_pageobj{
 public:
  explicit Cxml_pathobj(CPDF_PathObject* pdf_pathobj, Cxml_page* ofd_page);

 public:
  Cxml_pathobj(const Cxml_pathobj&) = delete;
  Cxml_pathobj(Cxml_pathobj&&) = delete;
  Cxml_pathobj& operator=(const Cxml_pathobj&) = delete;
  Cxml_pathobj& operator=(Cxml_pathobj&&) = delete;
  ~Cxml_pathobj();

 public:
  void convert();

 private:
  void setStrokeColor(CFX_XMLElement *pathobj);
  void setFillColor(CFX_XMLElement* pathobj);
  void setDashPattern(CFX_XMLElement* pathobj);
 private:
  UnownedPtr<CPDF_PathObject> m_pdfpathobj;
 UnownedPtr<Cxml_page> m_ofdpage;
};

}// end of namespace xilou
#endif
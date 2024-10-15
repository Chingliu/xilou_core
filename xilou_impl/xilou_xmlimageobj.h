#ifndef XILOU_IMPL_XILOU_XMLIMAGEOBJECT_H
#define XILOU_IMPL_XILOU_XMLIMAGEOBJECT_H
#include <vector>
#include <memory>
#include "core/fxcrt/unowned_ptr.h"
class CPDF_ImageObject;
namespace xilou {
class Cxml_page;
class Cxml_package;
class Cxml_imageobj :public Cxml_pageobj{
 public:
  explicit Cxml_imageobj(CPDF_ImageObject* pdf_imageobj, Cxml_page* ofd_page);

 public:
  Cxml_imageobj(const Cxml_imageobj&) = delete;
  Cxml_imageobj(Cxml_imageobj&&) = delete;
  Cxml_imageobj& operator=(const Cxml_imageobj&) = delete;
  Cxml_imageobj& operator=(Cxml_imageobj&&) = delete;
  ~Cxml_imageobj();

 public:
  void convert();
 private:
  UnownedPtr<CPDF_ImageObject> m_pdfimageobj;
  UnownedPtr<Cxml_page> m_ofdpage;
};

}// end of namespace xilou
#endif
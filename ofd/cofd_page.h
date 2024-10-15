#ifndef COFD_DOCUMENT_PAGE_H_HEADER
#define COFD_DOCUMENT_PAGE_H_HEADER
#include <map>
#include <memory>
#include <vector>
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/unowned_ptr.h"
#include "ofd/cofd_common.h"
#include "ofd/cofd_pageobjectholder.h"
#include "ofd/cofd_page_annot.h"
class CPDF_Path;
namespace ofd {
class COFD_Document;
class COFD_PageAnnot;
class COFD_Page : public COFD_PageAnnot,public COFD_PageObjectHolder {
 public:
  explicit COFD_Page(COFD_Document* doc, CFX_XMLElement* xml, int page_id);

public:
  COFD_Page(const COFD_Page&) = delete;
  COFD_Page(COFD_Page&&) = delete;
  COFD_Page& operator=(const COFD_Page&) = delete;
  COFD_Page& operator=(COFD_Page&&) = delete;
  virtual ~COFD_Page();

public:
  void test_obj();

public:
  CFX_RectF GetPageArea() const { return bbox_; }
  void SetPageArea(CFX_RectF bbox) {
    bbox_ = bbox;
    page_ctm.e += bbox_.Left();
    page_ctm.f += bbox_.Top();
  }
  void SetPageBBOX(CFX_RectF bbox) { bbox_ = bbox; }
  void SetPageCTM(CFX_Matrix* ctm) { page_ctm = *ctm; }
  void SetLayerDrawParam(int id) { layer_drawparam = id; }
  void Parse();
  void ParseBody(CFX_XMLElement* body_layer);
  void ParseXXground();
  void ParseXXTemplate(CFX_XMLElement* xml);
  void ParseArea(CFX_XMLElement* area);
  CFX_Matrix GetDisplayMatrix(const FX_RECT& rect, int iRotate) const;
  COFD_Page* GetBackGroundTemplateLayer() { return background_template_.get(); }
  COFD_Page* GetBodyTemplateLayer() { return body_template_.get(); }
  COFD_Page* GetForeGroundTemplateLayer() { return foreground_template_.get(); }
  COFD_Page* GetBackGroundLayer() { return background_.get(); }
  COFD_Page* GetForeGroundLayer() { return foreground_.get(); }
  //依托这两个接口，可以构建一个派生自COFD_Page的COFD_PageAnnot类，来实现批注的信息获取及批注编辑？
  size_t GetAnnotCount() { return page_annots_.size(); }
  COFD_PageAnnot* GetPageAnnotObj(size_t index) {
    if (index > page_annots_.size())
      return nullptr;
    return page_annots_[index].get();
  }
  COFD_Document* GetDocument() { return doc_; }
  size_t GetPageSignatureCount();
  std::vector<COFD_PageObjectHolder*> GetPageSignature();

 private:
  void InitElem(CFX_XMLElement* elem, CFX_RectF* ebbox, CFX_Matrix* ectm);
  CPDF_Path GenPath(CFX_XMLElement* elem);
  void ApplyClips(CFX_XMLElement* elem,
                  CPDF_PageObject* pPageObj,
                  CFX_RectF ebbox,
                  CFX_Matrix ectm);

 private:
  void ParseElement(CFX_XMLElement* elem);
  void ParsePathObject(CFX_XMLElement* elem);
  void ParseTextObject(CFX_XMLElement* elem);
  void ParseImageObject(CFX_XMLElement* elem);
  void ParseCompositeObject(CFX_XMLElement* elem);
  void ParseShadingObject(CFX_XMLElement* elem,
                          const CFX_RectF& boundary,
                          const CPDF_Path& clippath);
  void ParsePageAnnot(unsigned int idx);
  void ParsePageAnnots();

 private:
  UnownedPtr<COFD_Document> const doc_;
  UnownedPtr<CFX_XMLElement> const xml_;
  std::unique_ptr<COFD_Page> background_template_;
  std::unique_ptr<COFD_Page> body_template_;
  std::unique_ptr<COFD_Page> foreground_template_;
  std::unique_ptr<COFD_Page> foreground_;
  std::unique_ptr<COFD_Page> background_;
  std::map<int, CFX_XMLElement*> annot_nodes_;
  std::vector<std::unique_ptr<COFD_PageAnnot> > page_annots_;
  int page_id_;
  int layer_drawparam;
  CFX_RectF bbox_;
  CFX_Matrix page_ctm;
};
}  // namespace ofd
#endif  // !COFD_DOCUMENT_PAGE_H_HEADER


#include "ofd/cofd_common.h"
#include "ofd/cofd_page.h"
#include "ofd/cofd_document.h"
#include <vector>
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fpdfapi/page/cpdf_colorspace.h"

#include "core/fpdfapi/page/cpdf_pageobject.h"

#include "core/fpdfapi/page/cpdf_path.h"
#include "core/fpdfapi/page/cpdf_textstate.h"
#include "third_party/abseil-cpp/absl/strings/str_split.h"

#include "core/fxge/cfx_fontmapper.h"
#include "core/fpdfapi/font/cpdf_fontglobals.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/progressive_decoder.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "xfa/fxfa/cxfa_ffwidget.h"
#include "core/fxcrt/cfx_readonlymemorystream.h"
using namespace ofd;
using absl::ByAnyChar;
using absl::SkipWhitespace;
namespace {

}//end of namespace

void COFD_Page::ParseCompositeObject(CFX_XMLElement* elem) {
  CFX_RectF ebbox = bbox_;
  CFX_Matrix ectm = CFX_Matrix();
  InitElem(elem, &ebbox, &ectm);
  auto pCompositeGraphicUnit = doc_->FindCompositeObject(
      StringToFloat(elem->GetAttribute(L"ResourceID").AsStringView()));
  if (!pCompositeGraphicUnit)
    return;

  auto w = StringToFloat(pCompositeGraphicUnit->GetAttribute(L"Width").AsStringView());
  auto h = StringToFloat(pCompositeGraphicUnit->GetAttribute(L"Height").AsStringView());
  ebbox.width = ofd_mm_to_one_72_point(w);
  ebbox.height = ofd_mm_to_one_72_point(h);
  CFX_RectF check_box = ectm.TransformRect(ebbox);
  if (check_box.IsEmpty())
    return;
  auto pContent = pCompositeGraphicUnit->GetFirstChildNamed_NoPrefix(L"Content");
  if (!pContent) {
    return;
  }

  float alpha = 1.0;
  if (elem->HasAttribute(L"Alpha")) {
    alpha = StringToFloat(elem->GetAttribute(L"Alpha").AsStringView()) / 255;
  }
  COFD_Page container(doc_, pContent, -1);
  container.SetPageArea(ebbox);
  container.SetPageCTM(&ectm);
  container.ParseBody(pContent);
  auto count = container.GetPageObjectCount();
  for (size_t i = 0; i < count; ++i) {
    auto obj = std::unique_ptr<CPDF_PageObject>(container.PopPageObjectByIndex(i));
    obj->m_GeneralState.SetFillAlpha(alpha);
    obj->m_GeneralState.SetStrokeAlpha(alpha);
    AppendPageObject(std::move(obj));
  }
}


#include "ofd/cofd_common.h"
#include "ofd/cofd_page.h"
#include "ofd/cofd_document.h"
#include "ofd/cofd_signature.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_pathobject.h"
#include "core/fpdfapi/page/cpdf_shadingobject.h"
#include "core/fpdfapi/page/cpdf_shadingpattern.h"
#include "core/fpdfapi/page/cpdf_textobject.h"
#include "third_party/abseil-cpp/absl/strings/str_split.h"
#include <vector>
#include "ofd/cofd_page_annot.h"
using namespace ofd;

COFD_Page::COFD_Page(COFD_Document* doc, CFX_XMLElement* xml, int page_id)
    : COFD_PageObjectHolder(doc),
      doc_(doc),
      xml_(xml), page_id_(page_id) {
  auto area = xml_->GetFirstChildNamed_NoPrefix(L"Area");
  if (area) {
    bbox_ = doc_->ParseArea(area);
    auto rotate = area->GetFirstChildNamed_NoPrefix(L"Rotate");
    if (rotate) {
      //auto angle_degree = StringToFloat(rotate->GetTextData().AsStringView());
      auto angle_degree = rotate->GetTextData().GetInteger();
      if (angle_degree != 0) {

        page_ctm = CFX_Matrix();
        page_ctm.Rotate(angle_degree *( FXSYS_PI / 180));
        auto real_box = page_ctm.TransformRect(bbox_);
        auto real_bbox = real_box.ToFloatRect();
        auto temp = CFX_Matrix();
        temp.Translate(-real_bbox.left, -real_bbox.bottom);
        page_ctm.Concat(temp);
        bbox_ = page_ctm.TransformRect(bbox_);

        return;
      }
    }
  } else {
    bbox_ = doc_->GetDocumentArea();
  }
  page_ctm.e = bbox_.Left();
  page_ctm.f = bbox_.Top();
}

COFD_Page::~COFD_Page() {
}

void COFD_Page::test_obj() {
  auto pPathObj = std::make_unique<CPDF_PathObject>();
  pPathObj->set_stroke(true);
  pPathObj->set_filltype(CFX_FillRenderOptions::FillType::kWinding);
  pPathObj->path().AppendPoint(CFX_PointF(100, 200),
                               CFX_Path::Point::Type::kMove);
  pPathObj->path().AppendPoint(CFX_PointF(300, 400),
                               CFX_Path::Point::Type::kLine);
  pPathObj->path().AppendPointAndClose(CFX_PointF(600, 400),
                                       CFX_Path::Point::Type::kLine);

  const std::vector<float> rgb = {0.5f, 0.7f, 0.35f};
  RetainPtr<CPDF_ColorSpace> pCS =
      CPDF_ColorSpace::GetStockCS(CPDF_ColorSpace::Family::kDeviceRGB);
  pPathObj->m_ColorState.SetFillColor(pCS, rgb);

  const std::vector<float> rgb2 = {1, 0.9f, 0};
  pPathObj->m_ColorState.SetStrokeColor(pCS, rgb2);
  pPathObj->m_GeneralState.SetFillAlpha(0.5f);
  pPathObj->m_GeneralState.SetStrokeAlpha(0.8f);
  AppendPageObject(std::move(pPathObj));
}
void COFD_Page::InitElem(CFX_XMLElement* elem,
                         CFX_RectF* ebbox,
                         CFX_Matrix* ectm) {
  if (elem->HasAttribute(L"CTM")) {
    WideString ectm_str = elem->GetAttribute(L"CTM");
    using absl::ByAnyChar;
    using absl::SkipWhitespace;
    std::vector<std::string> v = absl::StrSplit(ectm_str.ToDefANSI().c_str(), ByAnyChar(" \t\n\r"),
                       SkipWhitespace());
    if (v.size() == 6) {
      ectm->a = StringToFloat(v[0].c_str());
      ectm->b = StringToFloat(v[1].c_str());
      ectm->c = StringToFloat(v[2].c_str());
      ectm->d = StringToFloat(v[3].c_str());
      ectm->e = ofd_mm_to_one_72_point(StringToFloat(v[4].c_str()));
      ectm->f = ofd_mm_to_one_72_point(StringToFloat(v[5].c_str()));
    }
  }
  if (elem->HasAttribute(L"Boundary")) {
    WideString boundary_str = elem->GetAttribute(L"Boundary");
    using absl::ByAnyChar;
    using absl::SkipWhitespace;
    std::vector<std::string> v =
        absl::StrSplit(boundary_str.ToDefANSI().c_str(), ByAnyChar(" \t\n\r"),
                       SkipWhitespace());
    if (v.size() == 4) {
      CFX_RectF parent_box(*ebbox);
      ebbox->left = ofd_mm_to_one_72_point(StringToFloat(v[0].c_str()));
      ebbox->top = ofd_mm_to_one_72_point(StringToFloat(v[1].c_str()));
      ebbox->width = ofd_mm_to_one_72_point(StringToFloat(v[2].c_str()));
      ebbox->height = ofd_mm_to_one_72_point(StringToFloat(v[3].c_str()));
      ectm->e += ebbox->Left();
      ectm->f += ebbox->Top();
      *ebbox = page_ctm.TransformRect(*ebbox);
      if (!parent_box.IsEmpty()) {
        ebbox->Intersect(parent_box);
      }
    }
  }
  ectm->Concat(page_ctm);
}

void COFD_Page::ParseElement(CFX_XMLElement*elem) {
  if (!elem)
    return;
  if (elem->GetLocalTagName() == L"PathObject")
    ParsePathObject(elem);
  else if (elem->GetLocalTagName() == L"TextObject")
    ParseTextObject(elem);
  else if (elem->GetLocalTagName() == L"ImageObject")
    ParseImageObject(elem);
  else if (elem->GetLocalTagName() == L"CompositeObject") {
    ParseCompositeObject(elem);
  }
}

void COFD_Page::ParseBody(CFX_XMLElement* body_layer) {
  if (!body_layer)
    return;
  for (auto* child = body_layer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (!elem)
      continue;
    if (elem->GetLocalTagName() == L"PageBlock")
      ParseBody(elem);
    ParseElement(elem);
  }
}

void COFD_Page::ParseArea(CFX_XMLElement* area) {
  if (!area)
    return;
  for (auto* child = area->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (!elem)
      continue;
    if (elem->GetLocalTagName() == L"Path")
      ParsePathObject(elem);
    else if (elem->GetLocalTagName() == L"Text")
      ParseTextObject(elem);
  }
}

void COFD_Page::ParseXXground() {
  if (!xml_)
    return;
  if (xml_->GetLocalTagName() != L"Layer")
    return;
  for (auto* child = xml_->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (!elem)
      continue;
    if (elem->GetLocalTagName() == L"PageBlock")
      ParseBody(elem);
    ParseElement(elem);
  }
}
void COFD_Page::ParseXXTemplate(CFX_XMLElement* xml) {
  if (!xml)
    return;
  if (xml->GetLocalTagName() != L"Page")
    return;
  {
    auto area = xml->GetFirstChildNamed_NoPrefix(L"Area");
    if (area) {
      bbox_ = doc_->ParseArea(area);
    } else {
      bbox_ = doc_->GetDocumentArea();
    }
    page_ctm.e = bbox_.Left();
    page_ctm.f = bbox_.Top();
  }

  auto content = xml->GetFirstChildNamed_NoPrefix(L"Content");
  if (!content)
    return;
  auto layer = content->GetFirstChildNamed_NoPrefix(L"Layer");
  while (layer) {
    //不搞太复杂了，模板层的不再区分Type了
    layer_drawparam = 0;
    if (layer->HasAttribute(L"DrawParam")) {
      layer_drawparam =
          StringToFloat(layer->GetAttribute(L"DrawParam").AsStringView());
    }

    ParseBody(layer);
    layer = layer->GetNextSiblingNamed_NoPrefix(L"Layer");
  }
}
void COFD_Page::Parse() {
  auto content = xml_->GetFirstChildNamed_NoPrefix(L"Content");
  if (content) {
    auto layer = content->GetFirstChildNamed_NoPrefix(L"Layer");
    while (layer) {
      layer_drawparam = 0;
      if (layer->HasAttribute(L"DrawParam")) {
        layer_drawparam =
            StringToFloat(layer->GetAttribute(L"DrawParam").AsStringView());
      }
      if (layer->HasAttribute(L"Type")) {
        WideString layer_type = layer->GetAttribute(L"Type");
        if (layer_type == L"Body") {
          ParseBody(layer);
        } else if (layer_type == L"Background") {
          if (!background_) {
            background_ = std::make_unique<COFD_Page>(doc_, layer, -1);
            //背景/前景层是虚拟页
            background_->SetPageArea(bbox_);
          }
          background_->SetLayerDrawParam(layer_drawparam);
          background_->ParseXXground();
          background_->SetParseState(
              COFD_PageObjectHolder::ParseState::kParsed);
        } else if (layer_type == L"Foreground") {
          if (!foreground_) {
            foreground_ = std::make_unique<COFD_Page>(doc_, layer, -1);
            foreground_->SetPageArea(bbox_);
          }
          foreground_->SetLayerDrawParam(layer_drawparam);
          foreground_->ParseXXground();
          foreground_->SetParseState(
              COFD_PageObjectHolder::ParseState::kParsed);
        } else {
          ParseBody(layer);
        }
      } else {
        // default is body
        ParseBody(layer);
      }
      layer = layer->GetNextSiblingNamed_NoPrefix(L"Layer");
    }
  }
  for (auto* child = xml_->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (!elem)
      continue;
    if (elem->GetLocalTagName() == L"Template" && elem->HasAttribute(L"TemplateID")) {
      WideString template_id = elem->GetAttribute(L"TemplateID");
      WideString template_id_url = doc_->TemplateUrl(template_id.GetInteger());
      CFX_XMLDocument* template_xml =
          doc_->ReadAndCacheXml(template_id_url.ToUTF8().AsStringView());
      if (!template_xml)
        continue;
      CFX_XMLElement* root = template_xml->GetRoot();
      if (!root)
        continue;
      CFX_XMLElement* page_node = root->GetFirstChildNamed_NoPrefix(L"Page");
      if (!page_node)
        continue;
      if (elem->HasAttribute(L"ZOrder")) {
        WideString zorder = elem->GetAttribute(L"ZOrder");
        if (zorder == L"Foreground") {
          if (!foreground_template_) {
            foreground_template_ = std::make_unique<COFD_Page>(doc_, page_node, template_id.GetInteger());
          }
          foreground_template_->ParseXXTemplate(page_node);
          foreground_template_->SetParseState(
              COFD_PageObjectHolder::ParseState::kParsed);
        } else if (zorder == L"Background") {
          if (!background_template_) {
            background_template_ = std::make_unique<COFD_Page>(doc_, page_node, template_id.GetInteger());
          }
          background_template_->ParseXXTemplate(page_node);
          background_template_->SetParseState(
              COFD_PageObjectHolder::ParseState::kParsed);
        } else {
          if (!body_template_) {
            body_template_ = std::make_unique<COFD_Page>(doc_, page_node, template_id.GetInteger());
          }
          body_template_->ParseXXTemplate(page_node);
          body_template_->SetParseState(
              COFD_PageObjectHolder::ParseState::kParsed);
        }
      } else {
        //default is body
        if (!body_template_) {
          body_template_ = std::make_unique<COFD_Page>(doc_, page_node, template_id.GetInteger());
        }
        body_template_->ParseXXTemplate(page_node);
        body_template_->SetParseState(
            COFD_PageObjectHolder::ParseState::kParsed);
      }
    }
  }
  ParsePageAnnots();
  SetParseState(COFD_PageObjectHolder::ParseState::kParsed);
}
namespace {
WideString findBiggestBoundary(CFX_XMLElement* elem) {
  WideString boundary;
  for (auto* child = elem->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* child_elem = ToXMLElement(child);
    if (!child_elem)
      continue;
    if (child_elem->GetLocalTagName() == L"PageBlock")
      boundary = findBiggestBoundary(child_elem);
    else {
      boundary = child_elem->GetAttribute(L"Boundary");
    }
    //TODO; 计算最大的，boundary
  }
  return boundary;
}
}  // namespace
void COFD_Page::ParsePageAnnots() {
  auto count = doc_->CountPageAnnot(page_id_);
  if (count > 0) {
    for (unsigned int i = 0; i < count; ++i) {
      ParsePageAnnot(i);
    }
  }
}
void COFD_Page::ParsePageAnnot(unsigned int idx) {
  auto annot_url = doc_->QueryPagesAnnot(page_id_, idx);
  if (!annot_url.IsEmpty()) {
    auto annot_xml = doc_->ReadAndCacheXml(annot_url.ToUTF8().AsStringView());
    if (annot_xml) {
      CFX_XMLElement* annot_root = annot_xml->GetRoot();
      if (!annot_root)
        return;
      CFX_XMLElement* annot_node =
          annot_root->GetFirstChildNamed_NoPrefix(L"PageAnnot");
      if (!annot_node)
        return;
      for (auto* child = annot_node->GetFirstChild(); child;
           child = child->GetNextSibling()) {
        CFX_XMLElement* elem = ToXMLElement(child);
        if (!elem)
          continue;
        if (elem->GetLocalTagName() != L"Annot")
          continue;
        auto annot_id = elem->GetAttribute(L"ID").GetInteger();
        auto annot_type = elem->GetAttribute(L"Type");
        annot_nodes_.insert(std::make_pair(annot_id, elem));
        auto apparence = elem->GetFirstChildNamed_NoPrefix(L"Appearance");
        if (apparence) {
          auto annot_obj = std::make_unique<COFD_Page>(doc_, apparence, annot_id);
          CFX_RectF ebbox;
          
          bool annot_has_outerbox = true;
          auto boundary = apparence->GetAttribute(L"Boundary");
          if (boundary.GetStringLength() <= 0) {
            boundary = elem->GetAttribute(L"Boundary");
            if (boundary.GetStringLength() <= 0) {
              annot_has_outerbox = false;
              //查找最大的boundary
              boundary = findBiggestBoundary(apparence);
            }
          }
          {
            using absl::ByAnyChar;
            using absl::SkipWhitespace;
            std::vector<std::string> v =
                absl::StrSplit(boundary.ToDefANSI().c_str(),
                               ByAnyChar(" \t\n\r"), SkipWhitespace());
            if (v.size() == 4) {
              ebbox.left = ofd_mm_to_one_72_point(StringToFloat(v[0].c_str()));
              ebbox.top = ofd_mm_to_one_72_point(StringToFloat(v[1].c_str()));
              ebbox.width =
                  ofd_mm_to_one_72_point(StringToFloat(v[2].c_str()));
              ebbox.height =
                  ofd_mm_to_one_72_point(StringToFloat(v[3].c_str()));
            }
          }
          ebbox = page_ctm.TransformRect(ebbox);
          annot_obj->SetPageBBOX(ebbox);//这里是取一个最大的框来做批注的外框。
          annot_obj->SetPageCTM(&page_ctm);
          if (annot_has_outerbox) {
            auto annot_page_ctm = page_ctm;
            annot_page_ctm.e += ebbox.Left();
            annot_page_ctm.f += ebbox.Top();
            annot_obj->SetPageCTM(&annot_page_ctm);//有外框的时候，调整e,f来匹配内部的相对坐标
          }
          annot_obj->setAnnotType(annot_type.AsStringView());
          annot_obj->ParseBody(apparence);
          
          annot_obj->SetParseState(COFD_PageObjectHolder::ParseState::kParsed);
          page_annots_.push_back(std::move(annot_obj));
        }
      }
    }
  }
}

//TODO 要重新检视这个函数的作用，当时是从PDF处抄过来的，
//实际上目前只用了iRotate = 0的情况，=234时可能是错误的。
CFX_Matrix COFD_Page::GetDisplayMatrix(const FX_RECT& rect, int iRotate) const {
  if (bbox_.width == 0 || bbox_.height == 0)
    return CFX_Matrix();

  float x0 = 0;
  float y0 = 0;
  float x1 = 0;
  float y1 = 0;
  float x2 = 0;
  float y2 = 0;
  iRotate %= 4;
  // This code implicitly inverts the y-axis to account for page coordinates
  // pointing up and bitmap coordinates pointing down. (x0, y0) is the base
  // point, (x1, y1) is that point translated on y and (x2, y2) is the point
  // translated on x. On iRotate = 0, y0 is rect.bottom and the translation
  // to get y1 is performed as negative. This results in the desired
  // transformation.
  switch (iRotate) {
    case 0:
      x0 = rect.left;
      y0 = rect.bottom;
      x1 = rect.left;
      y1 = rect.top;
      x2 = rect.right;
      y2 = rect.bottom;
      break;
    case 1:
      x0 = rect.left;
      y0 = rect.top;
      x1 = rect.right;
      y1 = rect.top;
      x2 = rect.left;
      y2 = rect.bottom;
      break;
    case 2:
      x0 = rect.right;
      y0 = rect.top;
      x1 = rect.right;
      y1 = rect.bottom;
      x2 = rect.left;
      y2 = rect.top;
      break;
    case 3:
      x0 = rect.right;
      y0 = rect.bottom;
      x1 = rect.left;
      y1 = rect.bottom;
      x2 = rect.right;
      y2 = rect.top;
      break;
  }
  CFX_Matrix matrix((x2 - x0) / bbox_.width, (y2 - y0) / bbox_.width,
                    (x1 - x0) / bbox_.height, (y0 - y1) / bbox_.height, x1, y1);
  return matrix;  // page_ctm * matrix;
}
void COFD_Page::ApplyClips(CFX_XMLElement* elem, CPDF_PageObject* pPageObj,
  CFX_RectF ebbox,
  CFX_Matrix ectm) {
  auto clips = elem->GetFirstChildNamed_NoPrefix(L"Clips");
  if (!clips)
    return;
  
  //多clip是交集，多area是并集
  
  CFX_FloatRect multi_clip_boundary = ebbox.ToFloatRect();
  int clip_count = 0;
  for (auto* child = clips->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* clip = ToXMLElement(child);
    if (!clip)
      continue;
    if (clip->GetLocalTagName() != L"Clip")
      continue;
    clip_count++;
    CPDF_Path merged_clipath;
    std::vector<std::unique_ptr<CPDF_TextObject>> vtext_obj;
    CFX_FloatRect multi_area_boundary = ebbox.ToFloatRect();

    for (auto* child2 = clip->GetFirstChild(); child2;
         child2 = child2->GetNextSibling()) {
      CFX_XMLElement* area = ToXMLElement(child2);
      if (!area)
        continue;
      if (area->GetLocalTagName() != L"Area")
        continue;
      //TODO Area应该还有CTM没考虑
      auto clip_obj = std::make_unique<COFD_Page>(doc_, area, -1);
      clip_obj->SetPageArea(ebbox);
      clip_obj->SetPageCTM(&ectm);
      if (area->HasAttribute(L"DrawParam")) {
        clip_obj->SetLayerDrawParam(
            StringToFloat(area->GetAttribute(L"DrawParam").AsStringView()));
      }
      clip_obj->ParseArea(area);
      auto count = clip_obj->GetPageObjectCount();
      for (size_t i = 0; i < count; ++i) {
        auto obj =
         std::unique_ptr<CPDF_PageObject>(clip_obj->PopPageObjectByIndex(i));
        auto clip_rect = obj->m_ClipPath.GetClipBox();
        multi_area_boundary.Intersect(clip_rect);

        if (obj->IsPath()) {
#if 0
          AppendPageObject(std::move(obj));//for testing
#else
          auto path_obj = obj->AsPath();
          auto fx_path = path_obj->path();//TODO:把绘制参数也丢掉了。
          fx_path.Transform(path_obj->matrix());//ContinueSingleObject里的ProcessClipPath没有应用pathobj的matrix
          merged_clipath.Append(*fx_path.GetObject(), nullptr);
#endif
        } else if (obj->IsText()) {
#if 0
          //for testing 
          AppendPageObject(std::move(obj));
#else
          auto text_obj =
              std::unique_ptr<CPDF_TextObject>(obj.release()->AsText());
          vtext_obj.push_back(std::move(text_obj));
#endif
        }
      }//end of for (size_t i = 0; i < count; ++i)

    }//end  of for area
    //处理多area并集的问题
    if (merged_clipath.HasRef())
      multi_clip_boundary.Intersect(merged_clipath.GetBoundingBox());
    multi_clip_boundary.Intersect(multi_area_boundary);
#if 1
    if (vtext_obj.size() > 0) {
      pPageObj->m_ClipPath.AppendTexts(&vtext_obj);
    }
    if (merged_clipath.HasRef() && merged_clipath.GetPoints().size() > 0) {
      auto mc_bbox = merged_clipath.GetBoundingBox();
      auto object_bbox = pPageObj->GetRect();
      mc_bbox.Intersect(object_bbox);
      if (!mc_bbox.IsEmpty()) 
      {
        pPageObj->m_ClipPath.AppendPathWithAutoMerge(
            merged_clipath, CFX_FillRenderOptions::FillType::kWinding);
      }
    }
#endif
  }//end of for clip
  //处理多clip交集问题
  if (!multi_clip_boundary.IsEmpty() && clip_count > 1) {
    CPDF_Path clip_boundary_path;
    clip_boundary_path.AppendFloatRect(multi_clip_boundary);
    // auto test_obj = std::make_unique<CPDF_PathObject>();
    // test_obj->path() = clip_boundary_path;
    // AppendPageObject(std::move(test_obj));  // for testing
#if 1
    pPageObj->m_ClipPath.AppendPathWithAutoMerge(
        clip_boundary_path, CFX_FillRenderOptions::FillType::kWinding);
#endif
  }
}

size_t COFD_Page::GetPageSignatureCount() {
  size_t total_page_sig = 0;
  auto sig_count = doc_->GetSignatureCount();
  for (size_t i = 0; i < sig_count; ++i) {
    auto sig = doc_->GetSignature(i);
    auto ofd_sig = reinterpret_cast<COfdSignature*>(sig);
    auto page_sig_count = ofd_sig->GetPageSignatureCount(page_id_);
    total_page_sig += page_sig_count;
  }
  return total_page_sig;
}
std::vector<COFD_PageObjectHolder*> COFD_Page::GetPageSignature() {
  std::vector<COFD_PageObjectHolder*> total_page_sig;
  auto sig_count = doc_->GetSignatureCount();
  for (size_t i = 0; i < sig_count; ++i) {
    auto sig = doc_->GetSignature(i);
    auto ofd_sig = reinterpret_cast<COfdSignature*>(sig);
    auto page_siges = ofd_sig->GetPageSignature(page_id_);
    total_page_sig.insert(total_page_sig.end(), page_siges.begin(),
                          page_siges.end());
  }
  return total_page_sig;
}
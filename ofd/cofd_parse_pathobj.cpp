
#include "ofd/cofd_common.h"
#include "ofd/cofd_page.h"
#include "ofd/cofd_document.h"

#include "core/fxcrt/retain_ptr.h"
#include "core/fpdfapi/page/cpdf_colorspace.h"

#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_pathobject.h"
#include "core/fpdfapi/page/cpdf_path.h"
#include "third_party/abseil-cpp/absl/strings/str_split.h"
#include <vector>
using namespace ofd;

namespace {
  void SetDefaultParam(CPDF_PathObject* pathobj) {
    pathobj->m_GraphState.SetLineWidth(ofd_mm_to_one_72_point(0.353f));
    pathobj->m_GraphState.SetMiterLimit(3.528f);
    pathobj->m_GraphState.SetLineCap(CFX_GraphStateData::LineCap::kButt);
    pathobj->m_GraphState.SetLineJoin(CFX_GraphStateData::LineJoin::kMiter);
    //图形默认stroke为true
    std::vector<float> value;
    //先按RGB来设个默认值
    value.push_back(0);
    value.push_back(0);
    value.push_back(0);
    value.push_back(0);
    pathobj->m_ColorState.SetStrokeColor(
        CPDF_ColorSpace::GetStockCS(CPDF_ColorSpace::Family::kDeviceRGB), value);
  }
}

void COFD_Page::ParsePathObject(CFX_XMLElement* elem) {
  if (elem->HasAttribute(L"Visible"))
    if (0 == elem->GetAttribute(L"Visible").Compare(L"false"))
      return;
  CFX_RectF ebbox = bbox_;
  CFX_Matrix ectm = CFX_Matrix();
  InitElem(elem, &ebbox, &ectm);
  CPDF_Path path = GenPath(elem);
  auto pPathObj = std::make_unique<CPDF_PathObject>();
  pPathObj->path() = path;
  pPathObj->SetPathMatrix(ectm);
  CPDF_Path clippath;
  clippath.AppendFloatRect(ebbox.ToFloatRect());
  pPathObj->m_ClipPath.AppendPathWithAutoMerge(
      clippath, CFX_FillRenderOptions::FillType::kWinding);
  //
  CFX_FloatRect clip_rect = pPathObj->GetRect();
  //
  SetDefaultParam(pPathObj.get());
  //
  if (layer_drawparam > 0) {
    auto xml_node = doc_->FindDrawParam(layer_drawparam);
    if (xml_node)
      doc_->ParseDrawParam(xml_node, pPathObj.get());
  }
  //
  if (elem->HasAttribute(L"DrawParam")) {
    auto relative_id =
        StringToFloat(elem->GetAttribute(L"DrawParam").AsStringView());
    auto xml_node = doc_->FindDrawParam(relative_id);
    if (xml_node)
      doc_->ParseDrawParam(xml_node, pPathObj.get());
    else
      doc_->ParseDrawParam(elem, pPathObj.get());
  } else {
    doc_->ParseDrawParam(elem, pPathObj.get());
  }

  if (elem->HasAttribute(L"LineWidth")) {
    float lineWidth = StringToFloat(elem->GetAttribute(L"LineWidth").AsStringView());
    float lineWidthPx = ofd_mm_to_one_72_point(lineWidth);
    pPathObj->m_GraphState.SetLineWidth(lineWidthPx);
    clip_rect.left += lineWidthPx;
    clip_rect.top -= lineWidthPx;
    clip_rect.right -= lineWidthPx;
    clip_rect.bottom += lineWidthPx;
  }
  if (elem->HasAttribute(L"MiterLimit")) {
    float miterLimit = StringToFloat(elem->GetAttribute(L"MiterLimit").AsStringView());
    pPathObj->m_GraphState.SetMiterLimit(miterLimit);
  }
  if (elem->HasAttribute(L"DashPattern")) {
    float dashOffset = 0.f;
    if (elem->HasAttribute(L"DashOffset")) {
      dashOffset = StringToFloat(elem->GetAttribute(L"DashOffset").AsStringView());
      dashOffset = ofd_mm_to_one_72_point(dashOffset);
    }
    WideString dashPattern_str = elem->GetAttribute(L"DashPattern");
    std::vector<std::string> v =
        absl::StrSplit(dashPattern_str.ToDefANSI().c_str(), absl::ByAnyChar(" \t\n\r"),
                       absl::SkipWhitespace());
    std::vector<float> dashArray;
    for (int i = 0; i < (int)v.size(); ++i) {
      dashArray.push_back(ofd_mm_to_one_72_point(StringToFloat(v[i].c_str())));
    }
    pPathObj->m_GraphState.SetLineDash(dashArray, dashOffset, 1.f);
  }

  if (elem->HasAttribute(L"Stroke") && elem->GetAttribute(L"Stroke") == L"false") {
    pPathObj->set_stroke(false);
  } else {
    pPathObj->set_stroke(true);
  }
  
  if (elem->HasAttribute(L"Fill") && elem->GetAttribute(L"Fill") == L"true") {
    //检查填充色是否为底纹或渐变
    CFX_XMLElement* shade_elem = nullptr;
    auto fillcolor = elem->GetFirstChildNamed_NoPrefix(L"FillColor");
    auto child = fillcolor ? fillcolor->GetFirstChild() : nullptr;
    for (; child; child = child->GetNextSibling()) {
      if (child->GetType() == CFX_XMLNode::Type::kElement) {
        shade_elem = ToXMLElement(child);
        if (shade_elem) {
          path.Transform(ectm);
          ParseShadingObject(shade_elem, ebbox, path);
        }
        break;
      }
    }
    //
    pPathObj->set_winding_filltype();
    if (elem->HasAttribute(L"Rule")) {
      if (elem->GetAttribute(L"Rule") == L"Even-Odd")
        pPathObj->set_alternate_filltype();
    }
  } else {
    pPathObj->set_no_filltype();
  }
  auto annot_type = getAnnotEnumType();
  if (annot_type == CPDF_Annot::Subtype::HIGHLIGHT) {
    //目前是将批注放在了正文的下层，//TODO批注与正文渲染的先后顺序？
    //TODO BlendMode怎么用起来   ProcessTransparency?
    pPathObj->m_GeneralState.SetBlendMode("Multiply");
    //pPathObj->m_GeneralState.SetFillAlpha(0.5);
  }
  ApplyClips(elem, pPathObj.get(), ebbox, ectm);
  AppendPageObject(std::move(pPathObj));
}
static void append2Path(CPDF_Path &path, std::vector<CFX_PointF>& points,
                        CFX_Path::Point::Type point_type) {
  if (points.size() == 0)
    return;
  if (point_type == CFX_Path::Point::Type::kBezier) {
    auto path_points = path.GetPoints();
    if (path_points.size() > 0) {
      if (points.size() == 2) {
        path.AppendPoint(path_points[path_points.size() - 1].m_Point,
                         point_type);
      } else if (points.size() == 1) {
        path.AppendPoint(path_points[path_points.size() - 1].m_Point,
                         point_type);
        path.AppendPoint(points[0], point_type);
      } 
    }
  }
  for(auto i : points) { 
    path.AppendPoint(i, point_type);
  }
  points.clear();
}
CPDF_Path COFD_Page::GenPath(CFX_XMLElement* elem) {
  auto AbbreviatedData =
      elem->GetFirstChildNamed_NoPrefix(L"AbbreviatedData");
  if (!AbbreviatedData)
    return CPDF_Path();
  using absl::ByAnyChar;
  using absl::SkipWhitespace;
  auto path_desc = AbbreviatedData->GetTextData();
  // M14.783 0.6L 14.572 0.6L 3.564 3.14L 0.6 9.068L0.6 9.914L
  //处理粘连的情况
  //path_desc = L"M14.783 0.6L14.572 0.6L 3.564 3.14L 0.6 9.068L0.6 9.914L";
  std::vector<std::string> orig_v = absl::StrSplit(
      path_desc.ToDefANSI().c_str(), ByAnyChar(" \t\n\r"), SkipWhitespace());

  std::vector<std::string> v;
  for (auto& it : orig_v) {
    bool has_alpha = false;
    size_t flag = 0;
    int c = it[0];
    for (size_t i = 0; i < it.length(); ++i) {
      if (isalpha(it[i])  != isalpha(c)) {
        c = it[i];
        has_alpha = true;
        auto tmpstr = it.substr(flag, i - flag);
        if (!tmpstr.empty())
          v.push_back(tmpstr);
        flag = i;
      }
    }
    if (has_alpha) {
      v.push_back(it.substr(flag, it.length() - flag));
    } else {
      v.push_back(it);
    }
  }
  size_t len = v.size();
  CPDF_Path path;
  std::vector<CFX_PointF> points;
  CFX_Path::Point::Type point_type;
  for (size_t i = 0; i < len; i++) {
    switch (v[i][0]) {
      case 'M':
      case 'S':
        append2Path(path, points, point_type);
        point_type = CFX_Path::Point::Type::kMove;
        break;
      case 'L':
        append2Path(path, points, point_type);
        point_type = CFX_Path::Point::Type::kLine;
        break;
      case 'B': {
        append2Path(path, points, point_type);
        point_type = CFX_Path::Point::Type::kBezier;
      }
        break;
      case 'Q': {
        if (points.size() > 0) {
          auto current_point = points[points.size() - 1];
          append2Path(path, points, point_type);
          point_type = CFX_Path::Point::Type::kBezier;
          points.push_back(current_point);//将当前点作为控制点推入
        }
      }
        break;
      case 'A':
        append2Path(path, points, point_type);
        break;
      case 'C':
        append2Path(path, points, point_type);
        path.ClosePath();
        break;
      default:
        if (isalpha(v[i][0])) {
          append2Path(path, points, point_type);
        }
        if (i + 1 < len) {
          points.push_back(CFX_PointF(ofd_mm_to_one_72_point(StringToFloat(v[i].c_str())), ofd_mm_to_one_72_point(StringToFloat(v[i +1].c_str()))));
          i +=1;
        }
        break;
    }
  }
  append2Path(path, points, point_type);
  return path;
}
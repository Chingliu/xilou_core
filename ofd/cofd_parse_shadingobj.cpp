
#include "ofd/cofd_common.h"
#include "ofd/cofd_page.h"
#include "ofd/cofd_document.h"
#include "ofd/cofd_util.h"

#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfapi/parser/cpdf_boolean.h"
#include "core/fpdfapi/page/cpdf_function.h"
#include "core/fpdfapi/page/cpdf_shadingobject.h"
#include "core/fpdfapi/page/cpdf_shadingpattern.h"

using namespace ofd;

void COFD_Page::ParseShadingObject(CFX_XMLElement* elem_shade,
                                   const CFX_RectF& boundary,
                                   const CPDF_Path& clippath) {
  struct Color_rgba {
    float r;
    float g;
    float b;
    float a;
  };
  WideString shade_name = elem_shade ? elem_shade->GetName() : L"";
  if (!shade_name.IsEmpty()) {
    CFX_FloatRect obj_rect = boundary.ToFloatRect();
    if (shade_name == L"ofd:Pattern") {
      // 底纹
    } else if (shade_name == L"ofd:AxialShd") {
      // 轴向渐变
      // 先解析ofd:Segment，两个以上渐变色才有效
      int iCountSegment = 0;
      std::vector<Color_rgba> vec_colors;
      auto child = elem_shade->GetFirstChild();
      for (; child; child = child->GetNextSibling()) {
        CFX_XMLElement* segment = ToXMLElement(child);
        if (!segment)
          continue;
        if (segment->GetLocalTagName() == L"Segment") {
          ++iCountSegment;
          CFX_XMLElement* color =
              ToXMLElement(segment->GetFirstChildNamed_NoPrefix(L"Color"));
          auto color_value = doc_->ParseColor(color, NULL, false);
          Color_rgba c;
          c.r = color_value[0];
          c.g = color_value[1];
          c.b = color_value[2];
          c.a = color_value[3];
          vec_colors.push_back(c);
        }
      }
      if (iCountSegment > 1) {
        auto pDict = pdfium::MakeRetain<CPDF_Dictionary>();
        pDict->SetNewFor<CPDF_Name>("ColorSpace", "DeviceRGB");
        pDict->SetNewFor<CPDF_Number>("ShadingType", (int)kAxialShading);
        CPDF_Array* pArrayDomain = pDict->SetNewFor<CPDF_Array>("Domain");
        pArrayDomain->AppendNew<CPDF_Number>(0);  // t_min
        pArrayDomain->AppendNew<CPDF_Number>(1);  // t_max
        CPDF_Array* pArrayExtend = pDict->SetNewFor<CPDF_Array>("Extend");
        pArrayExtend->AppendNew<CPDF_Boolean>(true);  // start_extend
        pArrayExtend->AppendNew<CPDF_Boolean>(true);  // end_extend
        CPDF_Array* pArrayBBox = pDict->SetNewFor<CPDF_Array>("BBox");
        pArrayBBox->AppendNew<CPDF_Number>(boundary.Left());
        pArrayBBox->AppendNew<CPDF_Number>(boundary.Top());
        pArrayBBox->AppendNew<CPDF_Number>(boundary.right());
        pArrayBBox->AppendNew<CPDF_Number>(boundary.bottom());
        // 渐变参考线坐标
        CFX_PointF start_point, end_point;
        if (elem_shade->HasAttribute(L"StartPoint")) {
          WideString temp_str = elem_shade->GetAttribute(L"StartPoint");
          start_point = ofd_util::ParseSTPoint(temp_str);
        }
        if (elem_shade->HasAttribute(L"EndPoint")) {
          WideString temp_str = elem_shade->GetAttribute(L"EndPoint");
          end_point = ofd_util::ParseSTPoint(temp_str);
        }
        start_point.x += boundary.Left();
        start_point.y += boundary.Top();
        end_point.x += boundary.Left();
        end_point.y += boundary.Top();
        if (start_point.x > obj_rect.left)
          obj_rect.left = start_point.x;
        if (end_point.x < obj_rect.right)
          obj_rect.right = end_point.x;
        CPDF_Array* pArrayCoords = pDict->SetNewFor<CPDF_Array>("Coords");
        pArrayCoords->AppendNew<CPDF_Number>(start_point.x);
        pArrayCoords->AppendNew<CPDF_Number>(start_point.y);
        pArrayCoords->AppendNew<CPDF_Number>(end_point.x);
        pArrayCoords->AppendNew<CPDF_Number>(end_point.y);
        // 裁剪区外框（非必须）
        // CPDF_Array* pArrayBBox = pDict->SetNewFor<CPDF_Array>("BBox");

        {
          // 创建Function
          CPDF_Dictionary* pDictFunction =
              pDict->SetNewFor<CPDF_Dictionary>("Function");
          pDictFunction->SetNewFor<CPDF_Number>(
              "FunctionType", (int)CPDF_Function::Type::kType3Stitching);
          CPDF_Array* pArrayFuncDomain =
              pDictFunction->SetNewFor<CPDF_Array>("Domain");
          pArrayFuncDomain->AppendNew<CPDF_Number>(0);
          pArrayFuncDomain->AppendNew<CPDF_Number>(1);
          float fAverage = 1.f / iCountSegment;
          CPDF_Array* pArrayBounds =
              pDictFunction->SetNewFor<CPDF_Array>("Bounds");
          CPDF_Array* pArrayEncode =
              pDictFunction->SetNewFor<CPDF_Array>("Encode");
          CPDF_Array* pArrayFunctions =
              pDictFunction->SetNewFor<CPDF_Array>("Functions");
          for (int i = 0; i < iCountSegment - 1; ++i) {
            if (i > 0)
              pArrayBounds->AppendNew<CPDF_Number>(fAverage * i);
            pArrayEncode->AppendNew<CPDF_Number>(0);
            pArrayEncode->AppendNew<CPDF_Number>(1);
            // create kid item in dict "Functions"
            CPDF_Dictionary* pKidItem =
                pArrayFunctions->AppendNew<CPDF_Dictionary>();
            pKidItem->SetNewFor<CPDF_Number>(
                "FunctionType",
                (int)CPDF_Function::Type::kType2ExponentialInterpolation);
            pKidItem->SetNewFor<CPDF_Number>("N", 1);
            CPDF_Array* pArrayKidDomain =
                pKidItem->SetNewFor<CPDF_Array>("Domain");
            pArrayKidDomain->AppendNew<CPDF_Number>(0);
            pArrayKidDomain->AppendNew<CPDF_Number>(1);
            // C0
            CPDF_Array* pArrayC0 = pKidItem->SetNewFor<CPDF_Array>("C0");
            pArrayC0->AppendNew<CPDF_Number>(vec_colors[i].r);
            pArrayC0->AppendNew<CPDF_Number>(vec_colors[i].g);
            pArrayC0->AppendNew<CPDF_Number>(vec_colors[i].b);
            // C1
            int j = (i + 1) % iCountSegment;
            CPDF_Array* pArrayC1 = pKidItem->SetNewFor<CPDF_Array>("C1");
            pArrayC1->AppendNew<CPDF_Number>(vec_colors[j].r);
            pArrayC1->AppendNew<CPDF_Number>(vec_colors[j].g);
            pArrayC1->AppendNew<CPDF_Number>(vec_colors[j].b);
          }
        }

        /*int id = 0;
        if (elem_shade->HasAttribute(L"ID")) {
          WideString temp_str = elem_shade->GetAttribute(L"ID");
          id = StringToFloat(temp_str.c_str());
        }*/
        CFX_Matrix matrix;
        auto pShading = pdfium::MakeRetain<CPDF_ShadingPattern>(
            doc_->GetFakePDFDocument(), pDict.Get(), true, matrix);
        if (pShading->Load()) {
          auto pObj = std::make_unique<CPDF_ShadingObject>(
              CPDF_PageObject::kNoContentStream, pShading.Get(), matrix);
          pObj->SetRect(obj_rect);
          if (clippath.HasRef()) {
            pObj->m_ClipPath.AppendPathWithAutoMerge(
                clippath, CFX_FillRenderOptions::FillType::kWinding);
            CPDF_Path clip_path;
            clip_path.AppendFloatRect(obj_rect);
            pObj->m_ClipPath.AppendPathWithAutoMerge(
                clip_path, CFX_FillRenderOptions::FillType::kWinding);
          }
          AppendPageObject(std::move(pObj));
        }
      }
    } else if (shade_name == L"ofd:RadialShd") {
      // 径向渐变
    } else if (shade_name == L"ofd:GouraudShd") {
      // 高洛德渐变
    } else if (shade_name == L"ofd:LaGouraudShd") {
      // 网格高洛德渐变
    }
  }
}

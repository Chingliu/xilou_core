#include "xilou_impl/xilou_xmlpackage.h"
#include "xilou_impl/xilou_xmlpage.h"
#include "xilou_impl/xilou_xmlpathobj.h"
#include "ofd/cofd_common.h"
#include "core/fxcrt/xml/cfx_xmltext.h"
#include "core/fxcrt/cfx_widetextbuf.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_pathobject.h"
#include "core/fpdfapi/page/cpdf_path.h"
#include "core/fpdfapi/page/cpdf_colorstate.h"
#include "core/fpdfapi/page/cpdf_colorspace.h"
#include <iostream>
namespace xilou {
Cxml_pathobj::Cxml_pathobj(CPDF_PathObject* pdf_pathobj, Cxml_page* ofd_page): 
  Cxml_pageobj(ofd_page),
  m_pdfpathobj(pdf_pathobj), 
  m_ofdpage(ofd_page) {

}
Cxml_pathobj::~Cxml_pathobj() {

}
void Cxml_pathobj::setStrokeColor(CFX_XMLElement* pathobj) {
  if (!m_pdfpathobj->m_ColorState.HasStrokeColor())
    return;
  auto color = m_pdfpathobj->m_ColorState.GetStrokeColor();
  //color->IsColorSpaceRGB();
  //TODO pattern
  int R, G, B;
  if (color && color->GetRGB(&R, &G, &B)) {
    auto strokcolor = m_root->CreateNode<CFX_XMLElement>(L"ofd:StrokeColor");
    pathobj->AppendFirstChild(strokcolor);
    
    strokcolor->SetAttribute(L"Value", WideString::Format(L" %d %d %d", R,G,B));
  }
}
void Cxml_pathobj::setFillColor(CFX_XMLElement* pathobj) {
  if (!m_pdfpathobj->m_ColorState.HasFillColor())
    return;
  auto color = m_pdfpathobj->m_ColorState.GetFillColor();
  // color->IsColorSpaceRGB();
  // TODO pattern
  int R, G, B;
  if (color && color->GetRGB(&R, &G, &B)) {
    auto fillcolor = m_root->CreateNode<CFX_XMLElement>(L"ofd:FillColor");
    pathobj->AppendFirstChild(fillcolor);

    fillcolor->SetAttribute(L"Value",
                             WideString::Format(L" %d %d %d", R, G, B));
  }
}

void Cxml_pathobj::setDashPattern(CFX_XMLElement* pathobj) {
  auto dash_arr = m_pdfpathobj->m_GraphState.GetLineDashArray();
  if (dash_arr.size() == 0)
    return;
  auto dash_offset = m_pdfpathobj->m_GraphState.GetLineDashPhase();
  CFX_WideTextBuf wbuf;
  for (auto dashvalue : dash_arr) {
    wbuf << ofd_one_72_point_to_mm(dashvalue);
    wbuf << L" ";
  }
  pathobj->SetAttribute(L"DashPattern", wbuf.MakeString());
  if (dash_offset > 0.01f) {
    pathobj->SetAttribute(L"DashOffset",WideString::Format(L"%f", ofd_one_72_point_to_mm(dash_offset)));
  }
}
void Cxml_pathobj::convert() {
  //
  auto pathobj = m_root->CreateNode<CFX_XMLElement>(
      L"ofd:PathObject");
  m_objectroot = pathobj;
  auto abbre = m_root->CreateNode<CFX_XMLElement>(
      L"ofd:AbbreviatedData");
  auto abbre_data = m_root->CreateNode<CFX_XMLText>(L"");
  pathobj->InsertChildNode(abbre, 0);
  abbre->InsertChildNode(abbre_data, 0);
  auto pdf_ctm = m_ofdpage->getPDFDisplayCTM();
  auto pathRect = m_pdfpathobj->GetRect();  // TODO 用GetRect还是用GetBBox();
  auto formRect = getFormRect();
  auto formCTM = getFormMatrix();
  pathRect = formCTM.TransformRect(pathRect);
  
  (void)formRect;
  if (!m_ofdpage->is_annot()) {
    //批注时已经引入了dispaly ctm 变换了pdf坐标系到ofd坐标系 //CFX_Matrix mtUser2Device = ofd_page->getPDFDisplayCTM();
    pathRect = pdf_ctm.TransformRect(pathRect);
  }
  auto l = pathRect.Left();
  auto t = pathRect.Bottom();
  auto w = pathRect.Width();
  auto h = pathRect.Height();
#if 1
  if (m_ofdpage->is_annot()) {
    pathRect = m_pdfpathobj->GetRect();
    auto annotRect = pdf_ctm.TransformRect( m_ofdpage->getAnnotRect());
    auto annotMatrix = m_ofdpage->getAnnotCTM();
    pathRect = annotMatrix.TransformRect(pathRect);
    if (insideForm()) {
      annotMatrix = getFormMatrix();
      pdf_ctm = annotMatrix;
      pathRect = annotMatrix.TransformRect(m_pdfpathobj->GetRect());
    }
      
    l = pathRect.Left();
    t = pathRect.Bottom();
    l = l - annotRect.Left();
    t = t - annotRect.Bottom();
  }
#endif
  //TODO CTM怎么计算, CTM还要算吗，bbox,和点都使用ctm去转换了的。
#if 0
  auto ctm_str =
      WideString::Format(L" %f %f %f %f %f %f", pdf_path_ctm.a, pdf_path_ctm.b, pdf_path_ctm.c,
      pdf_path_ctm.d, ofd_one_72_point_to_mm(pdf_path_ctm.e),
      ofd_one_72_point_to_mm(pdf_path_ctm.f));
  pathobj->SetAttribute(L"CTM", ctm_str);
#endif
  //bbox = pdf_ctm.TransformRect(bbox);
  WideString wsBoundary =
      WideString::Format(L"%.3f %.3f %.3f %.3f", 
                          ofd_one_72_point_to_mm(l),
                          ofd_one_72_point_to_mm(t),
                         ofd_one_72_point_to_mm(w),
                         ofd_one_72_point_to_mm(h));
  pathobj->SetAttribute(L"Boundary", wsBoundary);
  if (m_pdfpathobj->stroke()) {
    pathobj->SetAttribute(L"Stroke", L"true");
    setStrokeColor(pathobj);
  } else {
    pathobj->SetAttribute(L"Stroke", L"false");
  }
  switch (m_pdfpathobj->filltype()) {
    case CFX_FillRenderOptions::FillType::kNoFill:
      pathobj->SetAttribute(L"Fill", L"false");
      break;
    case CFX_FillRenderOptions::FillType::kWinding:
      pathobj->SetAttribute(L"Fill", L"true");
      setFillColor(pathobj);
      break;
    case CFX_FillRenderOptions::FillType::kEvenOdd:
      pathobj->SetAttribute(L"Fill", L"true");
      pathobj->SetAttribute(L"Rule", L"Even-Odd");
      setFillColor(pathobj);
      break;
    default:
      pathobj->SetAttribute(L"Fill", L"false");
      break;
  }
  auto linewidth = m_pdfpathobj->m_GraphState.GetLineWidth();
  pathobj->SetAttribute(L"LineWidth", WideString::Format(L"%f", ofd_one_72_point_to_mm(linewidth)));

  auto miterlimit = m_pdfpathobj->m_GraphState.GetMiterLimit();
  pathobj->SetAttribute(
      L"MiterLimit",
      WideString::Format(L"%f", ofd_one_72_point_to_mm(miterlimit)));

  switch (m_pdfpathobj->m_GraphState.GetLineCap()) {
    case CFX_GraphStateData::LineCap::kButt:
      pathobj->SetAttribute(L"Cap", L"Butt");
      break;
    case CFX_GraphStateData::LineCap::kRound:
      pathobj->SetAttribute(L"Cap", L"Round");
      break;
    case CFX_GraphStateData::LineCap::kSquare:
      pathobj->SetAttribute(L"Cap", L"Square");
      break;
    default: 
    pathobj->SetAttribute(L"Cap", L"Butt");
      break;
  }
  switch (m_pdfpathobj->m_GraphState.GetLineJoin()) {
    case CFX_GraphStateData::LineJoin::kMiter:
      pathobj->SetAttribute(L"Join", L"Miter");
      break;
    case CFX_GraphStateData::LineJoin::kRound:
      pathobj->SetAttribute(L"Join", L"Round");
      break;
    case CFX_GraphStateData::LineJoin::kBevel:
      pathobj->SetAttribute(L"Join", L"Bevel");
      break;
    default:
      pathobj->SetAttribute(L"Join", L"Miter");
      break;
  }
  setDashPattern(pathobj);
  auto path = m_pdfpathobj->path();
  auto points = path.GetPoints();
  CFX_WideTextBuf wbuf;
  int pointcounter = 0;
  //for (auto p : points) {
  for (size_t i = 0; i < points.size(); i++){
    auto p = points.at(i);
    switch (p.m_Type) {
      case CFX_Path::Point::Type::kMove:
        wbuf << L"M ";
        break;
      case CFX_Path::Point::Type::kLine:
        if (p.m_CloseFigure) {
          wbuf << L"C ";//OFD的C->close不要指明坐标点
        } else {
          wbuf << L"L ";
        }        break;
      case CFX_Path::Point::Type::kBezier:
        if (i > 0 && points[i - 1].m_Type == CFX_Path::Point::Type::kBezier && pointcounter <3) {
          //OFD B有三个点
          wbuf << L" ";
          pointcounter++;
        } else {
          wbuf << L"B ";
          pointcounter = 1;
        }
        
        break;
    }
    if (!p.m_CloseFigure) {
      auto path_ctm = m_pdfpathobj->matrix();
      if (m_ofdpage->is_annot()) {
        //TODO 需要全盘考虑，重构
        path_ctm = CFX_Matrix();
      }
      auto point = path_ctm.Transform(p.m_Point);
      point = pdf_ctm.Transform(point);
      wbuf << ofd_one_72_point_to_mm(point.x - pathRect.Left());
      wbuf << L" ";
      wbuf << ofd_one_72_point_to_mm(point.y - pathRect.Bottom());
      wbuf << L" ";
    }
  }
  abbre_data->SetText(wbuf.MakeString());
  //auto stream = pdfium::MakeRetain<StringWriteStream>();
  //pathobj->Save(stream);
  //TODO 还是要个log系统
  //auto test = stream->ToString();
  //std::cout << test;
}
}
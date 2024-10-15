
#include <algorithm>
#include <vector>
#include "core/fpdfapi/page/cpdf_colorspace.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"
#include "ofd/cofd_common.h"
#include "ofd/cofd_document.h"
#include "ofd/cofd_page.h"
#include "ofd/cofd_textobj.h"

#include "core/fpdfapi/page/cpdf_pageobject.h"

#include "core/fpdfapi/page/cpdf_path.h"
#include "core/fpdfapi/page/cpdf_textstate.h"
#include "third_party/abseil-cpp/absl/strings/str_split.h"

#include "core/fpdfapi/font/cpdf_fontglobals.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fxge/cfx_fontmapper.h"

using namespace ofd;
using absl::ByAnyChar;
using absl::SkipWhitespace;

namespace {
void SetDefaultParam(CPDF_TextObject* textobj) {
  textobj->m_GraphState.SetLineWidth(ofd_mm_to_one_72_point(0.353f));
  textobj->m_GraphState.SetMiterLimit(3.528f);
  textobj->m_GraphState.SetLineCap(CFX_GraphStateData::LineCap::kButt);
  textobj->m_GraphState.SetLineJoin(CFX_GraphStateData::LineJoin::kMiter);
  //图形默认stroke为true
  std::vector<float> value;
  //先按RGB来设个默认值
  value.push_back(0);
  value.push_back(0);
  value.push_back(0);
  value.push_back(0);
  textobj->m_ColorState.SetStrokeColor(
      CPDF_ColorSpace::GetStockCS(CPDF_ColorSpace::Family::kDeviceRGB), value);
  textobj->m_TextState.SetFontSize(0);
  textobj->SetTextRenderMode(TextRenderingMode::MODE_FILL);
}  // SetDefaultParam

class CTextCodeData {
 public:
  CTextCodeData(CFX_XMLElement* TextCode, CFX_PointF DefaultPos)
      : m_TextCode(TextCode), m_loc(DefaultPos) {}
  void parseXY() {
    if (m_TextCode->HasAttribute(L"X")) {
      m_loc.x = ofd_mm_to_one_72_point(
          StringToFloat(m_TextCode->GetAttribute(L"X").AsStringView()));
    }
    if (m_TextCode->HasAttribute(L"Y")) {
      m_loc.y = ofd_mm_to_one_72_point(
          StringToFloat(m_TextCode->GetAttribute(L"Y").AsStringView()));
    }
  }
  void updateLastXY(float* x, float* y) {
    *x = m_loc.x;
    *y = m_loc.y;
  }
  void expand_g(std::vector<std::string>& delta, const char* key) {
    std::vector<std::string>::iterator it;
    it = std::find(delta.begin(), delta.end(), key);
    while (it != delta.end()) {
      auto count_str = *(it + 1);
      auto repeat_delta = *(it + 2);
      int count = atoi(count_str.c_str());
      switch (count) {
        case 0:
        case 1:
          delta.erase(it, it + 1);
          break;
        case 2:
          it->assign(repeat_delta);
          delta.erase(it + 1);
          break;
        default:
          it->assign(repeat_delta);
          (++it)->assign(repeat_delta);
          count -= 3;
          delta.insert(it, count, repeat_delta);
          break;
      }
      it = std::find(delta.begin(), delta.end(), "g");
    }
  }
  void parseDeltaXY() {
    if (m_TextCode->HasAttribute(L"DeltaX")) {
      WideString DeltaXStr = m_TextCode->GetAttribute(L"DeltaX");
      m_DeltaX = absl::StrSplit(DeltaXStr.ToDefANSI().c_str(),
                                ByAnyChar(" \t\n\r"), SkipWhitespace());
      expand_g(m_DeltaX, "g");
      expand_g(m_DeltaX, "G");
    }
    if (m_TextCode->HasAttribute(L"DeltaY")) {
      WideString DeltaYStr = m_TextCode->GetAttribute(L"DeltaY");
      m_DeltaY = absl::StrSplit(DeltaYStr.ToDefANSI().c_str(),
                                ByAnyChar(" \t\n\r"), SkipWhitespace());
      expand_g(m_DeltaY, "g");
      expand_g(m_DeltaX, "G");
    }
  }
  CFX_PointF calcCodePosAtIndex(const size_t i) {
    size_t szx = m_DeltaX.size();
    size_t szy = m_DeltaY.size();
    CFX_PointF codeLoc(m_loc);
    codeLoc.x +=
        (i == 0 || szx == 0) ? 0
        : ofd_mm_to_one_72_point(StringToFloat(m_DeltaX[std::min(szx,i) - 1].c_str()));
    codeLoc.y +=
        (i == 0 || szy == 0) ? 0
        : ofd_mm_to_one_72_point(StringToFloat(m_DeltaY[std::min(szy,i) - 1].c_str()));
    m_loc = codeLoc;
    return codeLoc;
  }
  void appendChar(wchar_t wc,
                  const size_t i,
                  CFX_Matrix codeCTM,
                  COFD_TextObject* pTextObj,
                  COFD_Page* pPage,
                  float hScale = 1.f) {
    CFX_PointF codeLoc = calcCodePosAtIndex(i);
    codeLoc = codeCTM.Transform(codeLoc);

    ByteString byteText;
    uint32_t charCode = pTextObj->GetFont()->CharCodeFromUnicode(wc);
    auto oneCharTextObj = pTextObj->Clone();
    if (0 == charCode) {
      // no glyphIndex using simsun
      oneCharTextObj->m_TextState.SetFont(pPage->GetDocument()->FindFont(-1));
      charCode = oneCharTextObj->GetFont()->CharCodeFromUnicode(wc);
    }
    oneCharTextObj->GetFont()->AppendChar(&byteText, charCode);

    // 以下暂留，charCode不确定是否为0时就没有glyphIndex，也许以下逻辑更严谨
    /*uint32_t fallbackFontPosition = oneCharTextObj->GetFont()->FallbackFontFromCharcode(charCode);
    //int glyphIndex = oneCharTextObj->GetFont()->GlyphFromCharCode(charCode);
    int glyphIndex = oneCharTextObj->GetFont()->FallbackGlyphFromCharcode(
        fallbackFontPosition, charCode);
    if (glyphIndex  < 0) {
      // no glyphIndex using simsun
      oneCharTextObj->m_TextState.SetFont(pPage->GetDocument()->FindFont(-1));
      charCode = oneCharTextObj->GetFont()->CharCodeFromUnicode(wc);
    }
    oneCharTextObj->GetFont()->AppendChar(&byteText, charCode);*/

    CFX_Matrix scale_ctm({hScale, 0, 0, 1, 0, 0});
    codeCTM.Concat(scale_ctm);
    oneCharTextObj->SetTextMatrix(codeCTM);

    CFX_PointF origin(codeLoc.x, codeLoc.y);
    oneCharTextObj->SetText(byteText);
    oneCharTextObj->SetPosition(origin);

    pPage->AppendPageObject(std::move(oneCharTextObj));
  }
  void appendGlyph(uint32_t glyph,
                   wchar_t wc,
                   const size_t i,
                   CFX_Matrix codeCTM,
                   COFD_TextObject* pTextObj,
                   COFD_Page* pPage) {
    CFX_PointF codeLoc = calcCodePosAtIndex(i);

    codeLoc = codeCTM.Transform(codeLoc);
    ByteString byteText;
    pTextObj->GetFont()->AppendChar(
        &byteText, pTextObj->GetFont()->CharCodeFromUnicode(wc));

    auto oneCharTextObj = pTextObj->Clone();
    CFX_PointF origin(codeLoc.x, codeLoc.y);
    oneCharTextObj->SetText(byteText);
    oneCharTextObj->SetPosition(origin);
    oneCharTextObj->m_GlyphIndex = glyph;
    oneCharTextObj->m_Unicode = static_cast<uint32_t>(wc);
    pPage->AppendPageObject(std::move(oneCharTextObj));
  }

 private:
  CFX_XMLElement* m_TextCode;
  CFX_PointF m_loc;
  std::vector<std::string> m_DeltaX;
  std::vector<std::string> m_DeltaY;
};  // end of CTextCodeData

class CCGTransformData {
 public:
  int m_CodePosition;
  int m_CodeCount;
  int m_GlyphCount;
  std::vector<std::string> m_Glyphs;
};
}  // end of namespace

void COFD_Page::ParseTextObject(CFX_XMLElement* elem) {
  if (elem->HasAttribute(L"Visible"))
    if (0 == elem->GetAttribute(L"Visible").Compare(L"false"))
      return;
  CFX_RectF ebbox = bbox_;
  CFX_Matrix ectm = CFX_Matrix();
  InitElem(elem, &ebbox, &ectm);
  auto pTextObj = std::make_unique<COFD_TextObject>();
  //
  CPDF_Path clippath;
  clippath.AppendFloatRect(ebbox.ToFloatRect());
  pTextObj->m_ClipPath.AppendPathWithAutoMerge(
      clippath, CFX_FillRenderOptions::FillType::kWinding);
  //
  SetDefaultParam(pTextObj.get());
  //
  doc_->ParseDrawParam(elem, pTextObj.get());
  //
  if (layer_drawparam > 0) {
    auto xml_node = doc_->FindDrawParam(layer_drawparam);
    if (xml_node)
      doc_->ParseDrawParam(xml_node, pTextObj.get());
  }
  //
  if (elem->HasAttribute(L"DrawParam")) {
    auto relative_id =
        StringToFloat(elem->GetAttribute(L"DrawParam").AsStringView());
    auto xml_node = doc_->FindDrawParam(relative_id);
    if (xml_node)
      doc_->ParseDrawParam(xml_node, pTextObj.get());
  }

  if (elem->HasAttribute(L"Font")) {
    pTextObj->m_TextState.SetFont(doc_->FindFont(
        StringToFloat(elem->GetAttribute(L"Font").AsStringView())));
  } else {
    //using simsun
    pTextObj->m_TextState.SetFont(doc_->FindFont(-1));
  }

  //pTextObj->m_TextState.SetFontSize(11);
  if (elem->HasAttribute(L"Size")) {
    pTextObj->m_TextState.SetFontSize(ofd_mm_to_one_72_point(
        StringToFloat(elem->GetAttribute(L"Size").AsStringView())));
  }
  // 文本对象，Fill默认true，Stroke默认false
  int renderMode = 2;  // 10 -->Fill
  //pTextObj->SetTextRenderMode(TextRenderingMode::MODE_FILL);
  if (elem->HasAttribute(L"Stroke")) {
    if (elem->GetAttribute(L"Stroke") == L"true")
      renderMode |= 1;
  }
  if (elem->HasAttribute(L"Fill")) {
    if (elem->GetAttribute(L"Fill") == L"false")
      renderMode &= 1;
  }
  switch (renderMode) {
    case 1:
      pTextObj->SetTextRenderMode(TextRenderingMode::MODE_STROKE);
      break;
    case 2:
      pTextObj->SetTextRenderMode(TextRenderingMode::MODE_FILL);
      break;
    case 3:
      pTextObj->SetTextRenderMode(TextRenderingMode::MODE_FILL_STROKE);
      break;
    default:
      pTextObj->SetTextRenderMode(TextRenderingMode::MODE_FILL);
      // Fill=false，Strok=false，则都为透明色
      pTextObj->m_GeneralState.SetFillAlpha(0.f);
      break;
  }

  // 解析CharDirection
  if (elem->HasAttribute(L"CharDirection")) {
    int charDirection =
        StringToFloat(elem->GetAttribute(L"CharDirection").AsStringView());
    CFX_Matrix rotate;
    rotate.Rotate(FXSYS_PI * charDirection / 180);
    rotate.Concat(ectm);
    pTextObj->SetTextMatrix(rotate);
  } else
    pTextObj->SetTextMatrix(ectm);

  // core\fxge\cfx_renderdevice.cpp:1084没弄明白为什么做这个变换
  // char2device.Scale(font_size, -font_size);
  //CFX_Matrix txt_matrix(ectm);
  //txt_matrix.d = -txt_matrix.d;  //不能这样搞，会丢字错位等副作用
  //pTextObj->SetTextMatrix(ectm); //在ofd_textrender中使用m_AdjustMatrix来调整文字倒置的问题

  ApplyClips(elem, pTextObj.get(), ebbox, ectm);

  // 解析HScale
  float hScale = 1.f;
  if (elem->HasAttribute(L"HScale")) {
    hScale = StringToFloat(elem->GetAttribute(L"HScale").AsStringView());
  }

  std::map<int, std::unique_ptr<CCGTransformData> > glyph_map;
  for (auto* child = elem->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* cg = ToXMLElement(child);
    if (!cg)
      continue;
    if (cg->GetLocalTagName() != L"CGTransform")
      continue;
    auto glyphNode = cg->GetFirstChildNamed_NoPrefix(L"Glyphs");
    if (!glyphNode)
      continue;
    int data;
    auto cgData = std::make_unique<CCGTransformData>();
    data = StringToFloat(cg->GetAttribute(L"CodePosition").AsStringView());
    cgData->m_CodePosition = data;
    data = StringToFloat(cg->GetAttribute(L"CodeCount").AsStringView());
    cgData->m_CodeCount = data > 0 ? data : 1;
    data = StringToFloat(cg->GetAttribute(L"GlyphCount").AsStringView());
    cgData->m_GlyphCount = data > 0 ? data : 1;
    auto glyphs = glyphNode->GetTextData();
    cgData->m_Glyphs = absl::StrSplit(glyphs.ToDefANSI().c_str(),
                                      ByAnyChar(" \t\n\r"), SkipWhitespace());
    glyph_map.insert(std::make_pair(cgData->m_CodePosition, std::move(cgData)));
  }
  //Textcode
  float x = 0;
  float y = 0;
  for (auto* child = elem->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* textcode = ToXMLElement(child);
    if (!textcode)
      continue;
    //CPDF_TextObject似乎不能单独控制每个字的xy坐标，没找到办法前先每个字占一个TextObject
    if (textcode->GetLocalTagName() != L"TextCode")
      continue;

    CTextCodeData textCodeData(textcode, CFX_PointF(x, y));
    textCodeData.parseXY();
    textCodeData.updateLastXY(&x, &y);
    textCodeData.parseDeltaXY();
    WideString content_orig = textcode->GetTextData();
    WideString content;
    content.Reserve(content_orig.GetLength());
    auto content_iter = content_orig.begin();
    for (; content_iter != content_orig.end(); ++content_iter) {
      if (*content_iter > L'\x1F') {
        content.InsertAtBack(*content_iter);
      }
    }
    size_t contentLen = content.GetStringLength();
    if (glyph_map.size() > 0) {
      //标记CGTrans,
      pTextObj->m_hasCGTrans = true;
    }
    //针对OFD内嵌字体走了，OFD_textobj定制的字形绘制，那就要收集
    //全文内嵌字体的字形及unicode在转换时，生成ToUnicode
    for (size_t i = 0; i < contentLen;) {
      auto iter = glyph_map.find(i);
      if (iter != glyph_map.end()) {
        for (size_t j = 0; j < iter->second->m_Glyphs.size(); ++j) {
          textCodeData.appendGlyph(
              StringToFloat(iter->second->m_Glyphs[j].c_str()),
              i + j < contentLen ? content[i + j] : L'?', i + j, ectm,
              pTextObj.get(), this);
        }
        i += iter->second->m_CodeCount;
      } else {
        textCodeData.appendChar(content[i], i, ectm, pTextObj.get(), this, hScale);
        i++;
      }
    }
  }
}

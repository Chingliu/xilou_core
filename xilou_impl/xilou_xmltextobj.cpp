#include "ofd/cofd_common.h"
#include "xilou_impl/xilou_xmlpackage.h"
#include "xilou_impl/xilou_xmlpage.h"
#include "xilou_impl/xilou_xmltextobj.h"
#include "core/fxcrt/xml/cfx_xmltext.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_textobject.h"
#include "core/fpdfapi/font/cpdf_font.h"

using namespace fxcrt;
namespace xilou {
Cxml_textobj::Cxml_textobj(CPDF_TextObject* pdf_textobj, Cxml_page* ofd_page): 
  Cxml_pageobj(ofd_page),
  m_pdftextobj(pdf_textobj), 
  m_ofdpage(ofd_page) {

}

Cxml_textobj::~Cxml_textobj() {

}

void Cxml_textobj::convert() {
  auto textobj = m_root->CreateNode<CFX_XMLElement>(L"ofd:TextObject");
  m_objectroot = textobj;
  CPDF_TextState state = m_pdftextobj->m_TextState;

  // 创建ID
  uint32_t uID = m_ofdpage->getUniqueID();
  WideString wsID = WideString::Format(L"%u", uID);
  textobj->SetAttribute(L"ID", wsID);

  // 解析ctm
  WideString wsCtm;
  float scale = 1.f;
  CFX_Matrix formCtm;
  // 先不考虑from及页CTM，后期优化 TODO
  // if (insideForm())
    //formCtm = getFormMatrix();
  CFX_Matrix annotCtm;
  if (m_ofdpage->is_annot()) {
    annotCtm = m_ofdpage->getAnnotCTM();
    WideString wsBlendMode = WideString::FromDefANSI(
        m_pdftextobj->m_GeneralState.GetBlendMode().AsStringView());
    if (wsBlendMode.GetLength())
      textobj->SetAttribute(L"BlendMode", wsBlendMode);
    else {
      // 批注默认设置为Multiply
      textobj->SetAttribute(L"BlendMode", L"Multiply");
    }
  }
  annotCtm.Concat(formCtm);
  CFX_Matrix textCtm = m_pdftextobj->GetTextMatrix();
  //transform_ofd_ctm(textCtm, annotCtm, ofdCtm, scale);
  CFX_Matrix ofdCtm = transformOfdCtm(textCtm, annotCtm, scale);
  // 后期考虑此操作是否放到transformOfdCtm中 TODO
  if (ofdCtm.d < 0.f)
    ofdCtm.d = -ofdCtm.d;
  wsCtm = WideString::Format(L"%.3f %.3f %.3f %.3f %.3f %.3f", ofdCtm.a,
                             ofdCtm.b, ofdCtm.c, ofdCtm.d, ofdCtm.e, ofdCtm.f);
  textobj->SetAttribute(L"CTM", wsCtm);

  // 先假设只有水平方向 TODO
  float wordsWidth = 0.f;
  WideString wsDeltaX = GetDeltaX(wordsWidth, scale);

  // 解析Boundary，暂未考虑垂直方向 TODO
  CFX_RectF pageRect = m_ofdpage->getPDFBox();
  CFX_PointF pos = m_pdftextobj->GetPos();
  CFX_FloatRect textRect = m_pdftextobj->GetRect();
  float x, y;
  if (pos.x < textRect.Left())
    x = ofd_one_72_point_to_mm(pos.x);
  else
    x = ofd_one_72_point_to_mm(textRect.Left());
  if (m_ofdpage->is_annot())
    y = ofd_one_72_point_to_mm(m_ofdpage->getAnnotRect().Height() - textRect.Top());
  else
    y = ofd_one_72_point_to_mm(pageRect.Height() - textRect.Top());
  float rectWidth = ofd_one_72_point_to_mm(textRect.Width());
  float w;
  if (wordsWidth > rectWidth)
    w = wordsWidth;
  else
    w = rectWidth;
  float h = ofd_one_72_point_to_mm(textRect.Height());
  WideString wsBoundary = WideString::Format(L"%.3f %.3f %.3f %.3f", x, y, w, h);
  textobj->SetAttribute(L"Boundary", wsBoundary);

  // 解析字体
  //FromDefANSI是否正确
  // WideString fontName = WideString::FromDefANSI(state.GetFont()->GetBaseFontName().AsStringView());
  //uint32_t uFontID = m_ofdpage->getFontID(fontName);
  uint32_t uFontID = m_ofdpage->getFontID(m_pdftextobj->GetFont().Get());
  WideString wsFontID = WideString::Format(L"%u", uFontID);
  textobj->SetAttribute(L"Font", wsFontID);

  // 解析字体大小
  float fontsize = ofd_one_72_point_to_mm(state.GetFontSize() * scale);
  WideString wsSize = WideString::Format(L"%.3f", fontsize);
  textobj->SetAttribute(L"Size", wsSize);

  // 解析stroke、fill
  auto renderMode = m_pdftextobj->GetTextRenderMode();
  switch (renderMode) {
    case TextRenderingMode::MODE_STROKE:
      // Fill的默认值为true，此处需设置
      textobj->SetAttribute(L"Fill", L"false");
      textobj->SetAttribute(L"Stroke", L"true");
      CreateStrokeColorNode(textobj);
      break;
    case TextRenderingMode::MODE_FILL:
      // Stroke的默认值为false，Fill的默认值为true，此处无需设置
      //textobj->SetAttribute(L"Fill", L"true");
      CreateFillColorNode(textobj);
      break;
    case TextRenderingMode::MODE_FILL_STROKE:
      textobj->SetAttribute(L"Stroke", L"true");
      CreateStrokeColorNode(textobj);
      textobj->SetAttribute(L"Fill", L"true");
      CreateFillColorNode(textobj);
      break;
    case TextRenderingMode::MODE_INVISIBLE:
      textobj->SetAttribute(L"Alpha", L"0");
      break;
    default:
      // Fill=false，Stroke=false，则都为透明色
      //textobj->SetAttribute(L"Alpha", L"0");
      break;
  }

  // 解析Weight
  int nStemV = state.GetFont()->GetFontWeight();
  int nWeight = nStemV < 700 ? nStemV / 5 : (nStemV - 140) / 4;
  if (0 != nWeight && 400 != nWeight) {
    WideString wsWeight = WideString::Format(L"%d", nWeight);
    textobj->SetAttribute(L"Weight", wsWeight);
  }

  // 解析Italic
  int nItalicAngle = state.GetFont()->GetItalicAngle();
  if (0 != nItalicAngle) {
    textobj->SetAttribute(L"Italic", L"true");
  }

  // 解析CGTransform
  CreateCGTransform(textobj);

  // 解析TextCode
  auto textCode = m_root->CreateNode<CFX_XMLElement>(L"ofd:TextCode");
  CFX_PointF posXY;
  posXY.x = pos.x - textRect.Left();
  if (posXY.x < 0.f)
    posXY.x = 0.f;
  posXY.y = textRect.Top() - pos.y;
  CFX_Matrix inverseCtm = ofdCtm.GetInverse();
  posXY = inverseCtm.Transform(posXY);
  WideString wsX = WideString::Format(L"%.3f", ofd_one_72_point_to_mm(posXY.x));
  textCode->SetAttribute(L"X", wsX);
  WideString wsY = WideString::Format(L"%.3f", ofd_one_72_point_to_mm(posXY.y));
  textCode->SetAttribute(L"Y", wsY);

  // 先假设只有水平方向
  if (!wsDeltaX.IsEmpty())
    textCode->SetAttribute(L"DeltaX", wsDeltaX);
  WideString wsText = GetWordStrings();
  size_t len = wsText.GetStringLength();
  if (len) {
    auto text = m_root->CreateNode<CFX_XMLText>(wsText);
    textCode->InsertChildNode(text, 0);
  }
  textobj->InsertChildNode(textCode, 0);
}

void Cxml_textobj::CreateStrokeColorNode(CFX_XMLElement* textObj) {
  if (!textObj)
    return;
  auto strokeColor = m_root->CreateNode<CFX_XMLElement>(L"ofd:StrokeColor");
  int r, g, b;
  m_pdftextobj->m_ColorState.GetStrokeColor()->GetRGB(&r, &g, &b);
  WideString value = WideString::Format(L"%d %d %d", r, g, b);
  strokeColor->SetAttribute(L"Value", value);
  textObj->InsertChildNode(strokeColor, 0);
}

void Cxml_textobj::CreateFillColorNode(CFX_XMLElement* textObj) {
  if (!textObj)
    return;

  if (m_pdftextobj->m_ColorState.HasRef() &&
      !m_pdftextobj->m_ColorState.GetFillColor()->IsNull()) {
    auto fillColor = m_root->CreateNode<CFX_XMLElement>(L"ofd:FillColor");
    int r, g, b;
    m_pdftextobj->m_ColorState.GetFillColor()->GetRGB(&r, &g, &b);
    WideString value = WideString::Format(L"%d %d %d", r, g, b);
    fillColor->SetAttribute(L"Value", value);
    textObj->InsertChildNode(fillColor, 0);
  }
}

void Cxml_textobj::CreateCGTransform(CFX_XMLElement* textObj) {
  if (!textObj)
    return;
  auto charCodes = m_pdftextobj->GetCharCodes();
  if (0 == charCodes.size())
    return;
  size_t cnt = 0;
  WideString glyphsText(L"");
  auto font = m_pdftextobj->GetFont();
  bool vertGlyph = false;
  for (auto it : charCodes){
    if (CPDF_Font::kInvalidCharCode == it)
        continue;
    int glyph = font->GlyphFromCharCode(it, &vertGlyph);
    if (glyphsText.IsEmpty())
      glyphsText = WideString::Format(L"%d", glyph);
    else
      glyphsText += WideString::Format(L" %d", glyph);
    cnt++;
  }
  if (glyphsText.IsEmpty())
    return;
  auto transform = m_root->CreateNode<CFX_XMLElement>(L"ofd:CGTransform");
  // 后期需结合PDF的字型变换做优化 TODO
  transform->SetAttribute(L"CodePosition", L"0");
  transform->SetAttribute(L"CodeCount", WideString::Format(L"%u", cnt));
  transform->SetAttribute(L"GlyphCount", WideString::Format(L"%u", cnt));
  auto glyphsNode = m_root->CreateNode<CFX_XMLElement>(L"ofd:Glyphs");
  auto text = m_root->CreateNode<CFX_XMLText>(glyphsText);
  transform->InsertChildNode(glyphsNode, 0);
  glyphsNode->InsertChildNode(text, 0);
  textObj->InsertChildNode(transform, 0);
}

WideString Cxml_textobj::GetDeltaX(float& width, float scale) {
  CPDF_TextObject df;
  WideString values(L"");
  width = 0.f;
  int cnt = m_pdftextobj->CountChars();
  if (1 == cnt) {
    uint32_t code = m_pdftextobj->GetCharCode(0);
    if (CPDF_Font::kInvalidCharCode != code)
      width = ofd_one_72_point_to_mm(m_pdftextobj->GetCharWidth(code));
    return values;
  }
  auto pos = m_pdftextobj->GetCharPositions();
  auto codes = m_pdftextobj->GetCharCodes();
  std::vector<float> realPos;
  // 过滤无效的字符位置
  for (int i = 0; i < (int)pos.size(); ++i) {
    if (CPDF_Font::kInvalidCharCode == codes[i + 1])
      continue;
    realPos.push_back(pos[i]);
  }
  float pre = 0.f;
  for (auto it : realPos) {
    WideString tmp(L"");
    float w = ofd_one_72_point_to_mm(it - pre) * scale;
    if (values.IsEmpty()) {
      tmp = WideString::Format(L"%.3f", w);
      values = tmp;
    } else {
      tmp = WideString::Format(L" %.3f", w);
      values += tmp;
    }
    pre = it;
    width += w;
  }
  uint32_t charCode = codes[codes.size() - 1];
  if (CPDF_Font::kInvalidCharCode == charCode)
    charCode = codes[codes.size() - 2];
  if (CPDF_Font::kInvalidCharCode != charCode)
    width += ofd_one_72_point_to_mm(m_pdftextobj->GetCharWidth(charCode) * scale);

  // 暂留
  /*int i = 0;
  int cnt = m_pdftextobj->CountChars();
  auto codes = m_pdftextobj->GetCharCodes();
  for (auto it : codes) {
    if (CPDF_Font::kInvalidCharCode == it)
      continue;
    WideString tmp(L"");
    float w = ofd_one_72_point_to_mm(m_pdftextobj->GetCharWidth(it));
    if (values.IsEmpty()) {
      tmp = WideString::Format(L"%.3f", w);
      values = tmp;
    } else {
      tmp = WideString::Format(L" %.3f", w);
      values += tmp;
    }
    ++i;
    if (i == cnt - 1)
        break;
  }*/
  return values;
}

#define ISLATINWORDEX(u) (u != 0x20 && u <= 0x28FF)
WideString Cxml_textobj::GetWordStringEx(int nWordIndex) const {
  // 此方法不排除空格
  RetainPtr<CPDF_Font> pFont = m_pdftextobj->GetFont();
  WideString swRet;
  int nWords = 0;
  bool bInLatinWord = false;
  for (size_t i = 0, sz = m_pdftextobj->CountChars(); i < sz; ++i) {
    uint32_t charcode = m_pdftextobj->GetCharCode(i);

    WideString swUnicode = pFont->UnicodeFromCharCode(charcode);
    uint16_t unicode = 0;
    if (swUnicode.GetLength() > 0)
      unicode = swUnicode[0];

    bool bIsLatin = ISLATINWORDEX(unicode);
    if (!bIsLatin || !bInLatinWord) {
      bInLatinWord = bIsLatin;
      nWords++;
    }
    if (nWords - 1 == nWordIndex)
      swRet += unicode;
  }
  return swRet;
}

WideString Cxml_textobj::GetWordStrings() {
  WideString value(L"");
  WideString defaultChar = L"¤";
  int cnt = m_pdftextobj->CountWords();
  int i = 0;
  for (; i < cnt; i++)
  {
    WideString wsTemp = m_pdftextobj->GetWordString(i);
    size_t len = wsTemp.GetStringLength();
    if (len)
      value += wsTemp;
    else
      value += defaultChar;
  }
  int charCnt = m_pdftextobj->CountChars();
  if (0 == cnt && 1 == charCnt) {
    // 处理特殊情况，比如某些字体的空格，CountWords()会等于0
    //uint32_t code = m_pdftextobj->GetCharCode(0);
    WideString wsTemp = GetWordStringEx(0);
    if (wsTemp.GetStringLength())
      value += wsTemp;
    else 
      value += defaultChar;
  }
  if (charCnt != cnt && 1 == cnt && 1 != charCnt && value == defaultChar) {
    // 补占位符
    for (int j = 0; j < charCnt - i; j++) {
      value += defaultChar;
    }
  }
  return value;
}

}
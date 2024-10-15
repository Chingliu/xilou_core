
#include "ofd/cofd_common.h"
#include "ofd/cofd_package.h"
#include "ofd/constants/page_object.h"
#include "ofd/cofd_ofdxml.h"
#include "ofd/cofd_signature.h"
#include "ofd/cofd_page.h"
#include "xilou_impl/xilou_signature_iface.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"
#include "core/fpdfapi/render/cpdf_docrenderdata.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_colorspace.h"
#include "third_party/abseil-cpp/absl/strings/str_split.h"
#include "third_party/abseil-cpp/absl/strings/string_view.h"
#include "core/fxge/cfx_fontmapper.h"
#include "core/fxge/fx_font.h"
#include "core/fpdfapi/font/cpdf_fontglobals.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfapi/parser/cpdf_boolean.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fxcrt/fx_codepage.h"
#include "public/fpdf_edit.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include <vector>
#include "cofd_document.h"
#include "third_party/spdlog/include/spdlog/spdlog.h"
using namespace ofd;

namespace {
#define FPDF_FONT_TYPE1 1
#define FPDF_FONT_TRUETYPE 2
}

const int COFD_Document::kDefaultFontID = -1001;
//CFX_FloatRect
//UpdateDimensions
COFD_Document::COFD_Document(COFD_Package* pkg,
                             COFD_DocInfo* docinfo,
                             CFX_XMLDocument* xml,
                             WideStringView base_url)
: pkg_(pkg)
, docinfo_(docinfo)
, xml_(xml)
, base_url_(base_url)
, max_id_(15856)
, bbox_(0, 0, 612, 792) {
  
  CFX_XMLElement* root = xml_->GetRoot();
  if (!root) {
    spdlog::error("[COFD_Document()] xml root is null.\n");
    return;
  }
  CFX_XMLElement* document_node = root->GetFirstChildNamed_NoPrefix(L"Document");
  if (!document_node) {
    spdlog::error("[COFD_Document()] xml document_node is null.\n");
    return;
  }
  ParseCommonData(document_node);
  CFX_XMLElement* pages = document_node->GetFirstChildNamed_NoPrefix(L"Pages");
  if (!pages) {
    spdlog::error("[COFD_Document()] xml pages is null.\n");
    return;
  }
  CFX_XMLElement* page = pages->GetFirstChildNamed_NoPrefix(L"Page");
  while (page) {
    if (page->HasAttribute(L"ID") && page->HasAttribute(L"BaseLoc")) {
      WideString base_loc = page->GetAttribute(L"BaseLoc");
      if (!base_loc.IsEmpty()) {
        int pageid = page->GetAttribute(L"ID").GetInteger();
        pageno2pageid_.push_back(pageid);
        if (base_loc[0] == L'/') {
          pageid2pageurl_.insert(std::make_pair(pageid, base_loc));
        } else {
          pageid2pageurl_.insert(std::make_pair(pageid, base_url_ + base_loc));
        }
      }
    }
    page = page->GetNextSiblingNamed_NoPrefix(L"Page");
  }
  ParseAnnotations(document_node);
  ParseSignatures();
}

COFD_Document::~COFD_Document() {

}

WideString COFD_Document::PageUrl(unsigned int index) {
  if (index > pageno2pageid_.size()) {
    return L"";
  }
  auto url = pageid2pageurl_.find(pageno2pageid_[index]);
  return url == pageid2pageurl_.end() ? L"" : url->second;
}

size_t COFD_Document::CountPageAnnot(unsigned int pageid) {
  return pageid2annoturl_.count(pageid);
}
WideString COFD_Document::QueryPagesAnnot(unsigned int pageid,
                                          unsigned int idx) {
  auto count = pageid2annoturl_.count(pageid);
  if (count == 0)
    return L"";
  auto url = pageid2annoturl_.find(pageid);
  for (unsigned int i = 0; i < idx; ++i) {
    ++url;
  }
  return url->second;
}

CFX_RectF COFD_Document::ParseArea(CFX_XMLElement* area_node) {
  if (!area_node)
    return bbox_;
  auto physical_box = area_node->GetFirstChildNamed_NoPrefix(L"PhysicalBox");
  if (physical_box) {
    auto physical_box_text = physical_box->GetTextData();
    float x;
    float y;
    float width;
    float height;
    int parsed = swscanf(physical_box_text.c_str(), L"%f %f %f %f", &x, &y, &width,
            &height);
    if (parsed == 4) {
      bbox_.left = ofd_mm_to_one_72_point(x);
      bbox_.top = ofd_mm_to_one_72_point(y);
      bbox_.width = ofd_mm_to_one_72_point(width);
      bbox_.height = ofd_mm_to_one_72_point(height);
    }
  }
  return bbox_;
}

void COFD_Document::ParseCommonData(CFX_XMLElement* root) {
  CFX_XMLElement* common = root->GetFirstChildNamed_NoPrefix(L"CommonData");
  if (!common)
    return;
  auto max_id_node = common->GetFirstChildNamed_NoPrefix(L"MaxUnitID");
  if (max_id_node) {
    auto data = max_id_node->GetTextData().GetInteger();
    if (data > 0)
      max_id_ = data;
  }
  bbox_ = ParseArea(common->GetFirstChildNamed_NoPrefix(L"PageArea"));

  for (auto* child = common->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (!elem )
      continue;
    if (elem->GetLocalTagName() == L"PublicRes" ||
        elem->GetLocalTagName() == L"DocumentRes") {
      auto res_url = elem->GetTextData();
      if (res_url[0] == L'/') {
        ParseRes(res_url);
      } else {
        ParseRes(base_url_ + res_url);
      }
    }// end of PublicRes/DocumentRes
    if (elem->GetLocalTagName() == L"TemplatePage") {
      auto template_id = elem->GetAttribute(L"ID").GetInteger();
      if (template_id > 0) {
        auto template_url = elem->GetAttribute(L"BaseLoc");
        if (template_url[0] == L'/') {
          id2templates_.insert(std::make_pair(template_id, template_url));
        } else {
          id2templates_.insert(
              std::make_pair(template_id, base_url_ + template_url));
        }
      }
    }  // end of TemplatePage
  }
}

void COFD_Document::DropPage(COFD_Page* page) {
  if (page) {
    delete page;
  }
}

COFD_Page* COFD_Document::LoadPage(unsigned int index) {
  if (index > pageno2pageid_.size()) {
    return nullptr;
  }
  auto url = pageid2pageurl_.find(pageno2pageid_[index]);
  if (url == pageid2pageurl_.end() ) {
    return nullptr;
  }
  if (url->second.GetLength() <= 0) {
    return nullptr;
  }
  auto page_xml = pkg_->ReadAndCacheXml(url->second.ToUTF8().AsStringView());
  if (!page_xml) {
    return nullptr;
  }
  CFX_XMLElement* root = page_xml->GetRoot();
  if (!root)
    return nullptr;
  CFX_XMLElement* page_node = root->GetFirstChildNamed_NoPrefix(L"Page");
  if (!page_node)
    return nullptr;
  return new COFD_Page(this, page_node, pageno2pageid_[index]);
}

CFX_XMLDocument* COFD_Document::ReadAndCacheXml(ByteStringView entry_name) {
  return pkg_->ReadAndCacheXml(entry_name);
}

int COFD_Document::ReadBinary(ByteStringView entry_name,
                              std::vector<uint8_t>* buf) {
  return pkg_->ReadBinary(entry_name, buf);
}

WideString COFD_Document::TemplateUrl(unsigned int id) {
  auto iter = id2templates_.find(id);
  if (iter != id2templates_.end())
    return iter->second;
  return L"";
}

CFX_XMLElement* COFD_Document::FindDrawParam(int id) {
  auto it = drawparams_.find(id);
  if (it != drawparams_.end()) {
    return it->second;
  }
  return nullptr;
}

CFX_XMLElement* COFD_Document::FindCompositeObject(int id) {
  auto it = compositeobj_.find(id);
  if (it != compositeobj_.end()) {
    return it->second;
  }
  return nullptr;
}

std::vector<float> COFD_Document::ParseColor(CFX_XMLElement* elem,
                                             CPDF_PageObject* page_object,
                                             bool is_fill) {
  //TODO 解析Index
  RetainPtr<CPDF_ColorSpace> cs = CPDF_ColorSpace::GetStockCS(CPDF_ColorSpace::Family::kDeviceRGB);
  if (elem->HasAttribute(L"ColorSpace")) {
    cs = FindColorSpace(
        StringToFloat(elem->GetAttribute(L"ColorSpace").AsStringView()));
  }
  uint32_t value_size = 0;
  std::vector<float> value;
  //先按RGB来设个默认值
  value.push_back(0);
  value.push_back(0);
  value.push_back(0);
  value.push_back(1);
  if (elem->HasAttribute(L"Value")) {
    auto color_str = elem->GetAttribute(L"Value");
    using absl::ByAnyChar;
    using absl::SkipWhitespace;
    std::vector<std::string> v = absl::StrSplit(
        color_str.ToDefANSI().c_str(), ByAnyChar(" \t\n\r"), SkipWhitespace());
    value_size = v.size();
    if (value_size >= cs->CountComponents()) {
      switch (value_size) {
        case 4:
          value[3] = StringToFloat(v[3].c_str()) / 255.0f;
          FALLTHROUGH;
        case 3:
          value[2] = StringToFloat(v[2].c_str()) / 255.0f;
          value[1] = StringToFloat(v[1].c_str()) / 255.0f;
          FALLTHROUGH;
        case 1:
          value[0] = StringToFloat(v[0].c_str()) / 255.0f;
          break;
        default:
          break;
      }
    }
  }
  if (page_object) {
    if (value_size != cs->CountComponents()) {
      WideString parent_name;
      CFX_XMLElement* parent = ToXMLElement(elem->GetParent());
      if (parent) {
        parent_name = parent->GetName();
      }
      if (is_fill && parent_name == L"ofd:PathObject") {
        page_object->m_GeneralState.SetFillAlpha(0);
      } else if (!is_fill && parent_name == L"ofd:TextObject") {
        page_object->m_GeneralState.SetStrokeAlpha(0);
      }
    }
    if (is_fill) {
      page_object->m_ColorState.SetFillColor(cs, value);
    } else {
      page_object->m_ColorState.SetStrokeColor(cs, value);
    }
  }
  return value;
}

RetainPtr<CPDF_ColorSpace> COFD_Document::FindColorSpace(int id) {
  auto it = colorspaces_.find(id);
  if (it != colorspaces_.end()) {
    return it->second;
  }
  return CPDF_ColorSpace::GetStockCS(CPDF_ColorSpace::Family::kDeviceRGB);
}

void COFD_Document::ParseDrawParam(CFX_XMLElement* elem,
                                   CPDF_PageObject* page_object) {
  if (!elem)
    return;
  if (elem->HasAttribute(L"Relative")) {
    auto relative_id = StringToFloat(elem->GetAttribute(L"Relative").AsStringView());
    auto xml_node = FindDrawParam(relative_id);
    if (xml_node)
      ParseDrawParam(xml_node, page_object);
  }
  if (elem->HasAttribute(L"LineWidth")) {
    auto line_width =
        ofd_mm_to_one_72_point(StringToFloat(elem->GetAttribute(L"LineWidth").AsStringView()));
    page_object->m_GraphState.SetLineWidth(line_width);
  }
  if (elem->HasAttribute(L"Cap")) {
    auto cap = elem->GetAttribute(L"Cap");
    if (cap == L"Butt") {
      page_object->m_GraphState.SetLineCap(CFX_GraphStateData::LineCap::kButt);
    } else if (cap == L"Round") {
      page_object->m_GraphState.SetLineCap(CFX_GraphStateData::LineCap::kRound);
    } else if (cap == L"Square") {
      page_object->m_GraphState.SetLineCap(CFX_GraphStateData::LineCap::kSquare);
    } else {
      page_object->m_GraphState.SetLineCap(CFX_GraphStateData::LineCap::kButt);
    }
  }

  if (elem->HasAttribute(L"Join")) {
    auto join = elem->GetAttribute(L"Join");
    if (join == L"Miter") {
      page_object->m_GraphState.SetLineJoin(
          CFX_GraphStateData::LineJoin::kMiter);
    } else if (join == L"Round") {
      page_object->m_GraphState.SetLineJoin(
          CFX_GraphStateData::LineJoin::kRound);
    } else if (join == L"Bevel") {
      page_object->m_GraphState.SetLineJoin(
          CFX_GraphStateData::LineJoin::kBevel);
    } else {
      page_object->m_GraphState.SetLineJoin(
          CFX_GraphStateData::LineJoin::kMiter);
    }
  }
  if (elem->HasAttribute(L"MiterLimit")) {
    //有可能是错的，但使mm转px后的值太大导致与数科效果不一致
    page_object->m_GraphState.SetMiterLimit(
        StringToFloat(elem->GetAttribute(L"MiterLimit").AsStringView()));
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
    page_object->m_GraphState.SetLineDash(dashArray, dashOffset, 1.f);
  }
  auto fillcolor = elem->GetFirstChildNamed_NoPrefix(L"FillColor");
  auto strokecolor = elem->GetFirstChildNamed_NoPrefix(L"StrokeColor");
  if (fillcolor) {
    if (fillcolor->HasAttribute(L"Alpha")) {
      page_object->m_GeneralState.SetFillAlpha(StringToFloat(fillcolor->GetAttribute(L"Alpha").AsStringView())/255.0f);
    } else {
      page_object->m_GeneralState.SetFillAlpha(1.0f);
    }
    ParseColor(fillcolor, page_object, true);
  } else {
    if (0 == elem->GetLocalTagName().Compare(L"PathObject"))
      // 图形对象FillColor默认为透明色
      page_object->m_GeneralState.SetFillAlpha(0.f);
  }
  if (strokecolor) {
    if (strokecolor->HasAttribute(L"Alpha")) {
      page_object->m_GeneralState.SetStrokeAlpha(
          StringToFloat(strokecolor->GetAttribute(L"Alpha").AsStringView()) /
          255.0f);
    } else {
      page_object->m_GeneralState.SetStrokeAlpha(1.0f);
    }
    ParseColor(strokecolor, page_object, false);
  } else {
    if (0 == elem->GetLocalTagName().Compare(L"TextObject"))
      // 文本对象StrokeColor默认为透明色
      page_object->m_GeneralState.SetStrokeAlpha(0.f);
  }
}

void COFD_Document::HandleResDrawParam(CFX_XMLElement* elem) {
  for (auto* child = elem->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* dp = ToXMLElement(child);
    if (!dp)
      continue;
    if (dp->GetLocalTagName() == L"DrawParam" && dp->HasAttribute(L"ID")) {
      drawparams_.insert(std::make_pair(
          StringToFloat(dp->GetAttribute(L"ID").AsStringView()), dp));
    }
  }
}

void ofd::COFD_Document::HandleResColorSpaces(CFX_XMLElement * elem) {
  for (auto* child = elem->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* dp = ToXMLElement(child);
    if (!dp)
      continue;
    if (dp->GetLocalTagName() == L"ColorSpace" && dp->HasAttribute(L"ID")) {
      RetainPtr<CPDF_ColorSpace> cs = nullptr;
      if (dp->HasAttribute(L"Type")) {
        WideString type = dp->GetAttribute(L"Type");
        ByteString byteName = type.ToLatin1();
        cs = CPDF_ColorSpace::GetStockCSForName(byteName);
      }
      if (!cs) {
        ByteString byteDefName("RGB");
        cs = CPDF_ColorSpace::GetStockCSForName(byteDefName);
      }
      colorspaces_.insert(std::make_pair(
          StringToFloat(dp->GetAttribute(L"ID").AsStringView()), cs));
    }
  }
}

void COFD_Document::HandleResCompositeObj(CFX_XMLElement* elem) {
  for (auto* child = elem->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* dp = ToXMLElement(child);
    if (!dp)
      continue;
    if (dp->GetLocalTagName() == L"CompositeGraphicUnit" && dp->HasAttribute(L"ID")) {
      compositeobj_.insert(std::make_pair(
          StringToFloat(dp->GetAttribute(L"ID").AsStringView()), dp));
    }
  }
}

void COFD_Document::HandleFonts(CFX_XMLElement* elem, WideStringView base_loc) {
  for (auto* child = elem->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* dp = ToXMLElement(child);
    if (!dp)
      continue;
    if (dp->GetLocalTagName() == L"Font" && dp->HasAttribute(L"ID")) {
      WideString simsun(L"宋体");
      ByteString fontName(simsun.ToDefANSI());  //("simsun");
      if (dp->HasAttribute(L"FontName")) {
        //中文的不行
        fontName = dp->GetAttribute(L"FontName").ToDefANSI();
      }
      WideString fontLoc;
      CFX_XMLElement* fontFileNode = dp->GetFirstChildNamed_NoPrefix(L"FontFile");
      if (fontFileNode) {
        auto fontFile = fontFileNode->GetTextData();
        if (fontFile.GetLength() > 0 && fontFile[0] == L'/') {
          fontLoc = fontFile;
        } else {
          fontLoc = base_loc + fontFile;
        }
      }
      pdfium::span<uint8_t> fontSpan;
      std::vector<uint8_t> buf;
      auto pfxFont = std::make_unique<CFX_Font>();
      RetainPtr<CPDF_Font> font;
      if (fontLoc.GetStringLength() > 0) {
        int len = ReadBinary(fontLoc.ToUTF8().AsStringView(), &buf);
        if (len > 0) {
          fontSpan = pdfium::make_span(buf);
          auto fpdfFont = FPDFText_LoadFont(
              FPDFDocumentFromCPDFDocument(GetFakePDFDocument()),
              fontSpan.data(), fontSpan.size(), FPDF_FONT_TRUETYPE, true);
          font.Unleak(CPDFFontFromFPDFFont(fpdfFont));
        } 
      } else {
        pfxFont->LoadSubst(fontName, true, 0, FXFONT_FW_NORMAL, 0,
                          FX_CodePage::kDefANSI, 0);
        fontSpan = pfxFont->GetFontSpan();
        auto fpdfFont = FPDFText_LoadFont(
            FPDFDocumentFromCPDFDocument(GetFakePDFDocument()), fontSpan.data(),
            fontSpan.size(), FPDF_FONT_TRUETYPE, true);
        font.Unleak(CPDFFontFromFPDFFont(fpdfFont));
      }
      if (font) {
        fonts_.insert(std::make_pair(
            StringToFloat(dp->GetAttribute(L"ID").AsStringView()), font));
      }
    }//end of if (dp->GetLocalTagName() == L"Font" && dp->HasAttribute(L"ID"))
  }
}

void COFD_Document::ParseRes(WideString res_url) {
  auto res_xml = ReadAndCacheXml(res_url.ToUTF8().AsStringView());
  if (!res_xml)
    return;
  auto root = res_xml->GetRoot();
  if (!root)
    return;
  CFX_XMLElement* res_node = root->GetFirstChildNamed_NoPrefix(L"Res");
  if (!res_node)
    return;
  for (auto* child = res_node->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (!elem)
      continue;
    WideString tagName = elem->GetLocalTagName();
    if (tagName == L"DrawParams" ) {
      HandleResDrawParam(elem);
    } else if (tagName == L"ColorSpaces") {
      HandleResColorSpaces(elem);
    } else if (tagName == L"Fonts") {
      WideString base_loc;
      if (res_node->HasAttribute(L"BaseLoc")) {
        base_loc = res_node->GetAttribute(L"BaseLoc");
      }
      if (base_loc.GetLength() > 0 && base_loc[0] != L'/') {
        HandleFonts(elem, (base_url_ + base_loc + L"/").AsStringView());
      } else if (base_loc.GetLength() > 0) {
        HandleFonts(elem, (base_loc + L"/").AsStringView());
      } else {
        HandleFonts(elem, (base_url_ ).AsStringView());
      }
    } else if (tagName == L"MultiMedias") {
      WideString base_loc;
      if (res_node->HasAttribute(L"BaseLoc")) {
        base_loc = res_node->GetAttribute(L"BaseLoc");
      }
      if (base_loc.GetLength() > 0 && base_loc[0] != L'/') {
        HandleMultiMedias(elem, (base_url_ + base_loc + L"/").AsStringView());
      } else if (base_loc.GetLength() > 0) {
        HandleMultiMedias(elem, (base_loc + L"/").AsStringView());
      } else {
        HandleMultiMedias(elem, (base_url_ ).AsStringView());
      }
    } else if (tagName == L"CompositeGraphicUnits") {
      HandleResCompositeObj(elem);
    }
  }
}

CPDF_Document* COFD_Document::GetFakePDFDocument() {
  return pkg_->GetFakePDFDocument();
}

RetainPtr<CPDF_Font> COFD_Document::FindFont(int id) {
  auto it = fonts_.find(id);
  if (it != fonts_.end()) {
    return it->second;
  }
  it = fonts_.find(kDefaultFontID);
  if (it != fonts_.end()) {
    return it->second;
  } else {
    pdfium::span<uint8_t> fontSpan;
    std::vector<uint8_t> buf;
    auto pfxFont = std::make_unique<CFX_Font>();
    RetainPtr<CPDF_Font> font;
    {
      pfxFont->LoadSubst("SimSun", true, 0, FXFONT_FW_NORMAL, 0,
                         FX_CodePage::kDefANSI, 0);
      fontSpan = pfxFont->GetFontSpan();
      auto fpdfFont = FPDFText_LoadFont(
          FPDFDocumentFromCPDFDocument(GetFakePDFDocument()), fontSpan.data(),
          fontSpan.size(), FPDF_FONT_TRUETYPE, true);
      font.Unleak(CPDFFontFromFPDFFont(fpdfFont));
    }
    if (font) {
      fonts_.insert(std::make_pair(kDefaultFontID, font));
    }
  }
  it = fonts_.find(kDefaultFontID);
  if (it != fonts_.end()) {
    return it->second;
  }
  return nullptr;
}

void COFD_Document::HandleMultiMedias(CFX_XMLElement* elem,
                                      WideStringView base_loc) {
  for (auto* child = elem->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* dp = ToXMLElement(child);
    if (!dp)
      continue;
    if (dp->GetLocalTagName() == L"MultiMedia" && dp->HasAttribute(L"ID")) {
      auto multiMedia = std::make_unique<CMultiMedia>();
      CFX_XMLElement* mediaFileNode =
          dp->GetFirstChildNamed_NoPrefix(L"MediaFile");
      if (mediaFileNode) {
        auto mediaFile = mediaFileNode->GetTextData();
        if (mediaFile.GetLength() > 0 && mediaFile[0] == L'/') {
          multiMedia->m_fullpath = mediaFile;
        } else {
          multiMedia->m_fullpath = base_loc + mediaFile;
        }
      }
      if (multiMedia->m_fullpath.GetStringLength() > 0) {
        multiMedia->m_id = StringToFloat(dp->GetAttribute(L"ID").AsStringView());
        multiMedia->m_type= dp->GetAttribute(L"Type");
        multiMedia->m_format = dp->GetAttribute(L"Format");
        multi_medias_.insert(std::make_pair(multiMedia->m_id, std::move(multiMedia)));
      }
    }  //if (dp->GetLocalTagName() == L"MultiMedia" && dp->HasAttribute(L"ID"))
  }
}

COFD_Document::CMultiMedia* COFD_Document::FindMedia(int id) {
  auto it = multi_medias_.find(id);
  if (it != multi_medias_.end()) {
    return it->second.get();
  }
  return nullptr;
}

void COFD_Document::ParseAnnotations(CFX_XMLElement* root) {
  CFX_XMLElement* annot = root->GetFirstChildNamed_NoPrefix(L"Annotations");
  if (!annot)
    return;
  auto annot_url = annot->GetTextData();
  if (annot_url.IsEmpty())
    return;

  {
    if (annot_url[0] != L'/') {
      annot_url = base_url_ + annot_url;
    }
    auto annotations_xml = ReadAndCacheXml(annot_url.ToUTF8().AsStringView());
    if (!annotations_xml)
      return;
    CFX_XMLElement* annot_root = annotations_xml->GetRoot();
    if (!annot_root)
      return;
    CFX_XMLElement* annot_node = annot_root->GetFirstChildNamed_NoPrefix(L"Annotations");
    if (!annot_node)
      return;
    auto last_delimeter = annot_url.ReverseFind(L'/');
    auto annots_path = base_url_;
    if (last_delimeter.has_value()) {
      annots_path = annot_url.Substr(0, last_delimeter.value());
      annots_path += L"/";
    }
    for (auto* child = annot_node->GetFirstChild(); child;
         child = child->GetNextSibling()) {
      CFX_XMLElement* elem = ToXMLElement(child);
      if (!elem)
        continue;
      if (elem->GetLocalTagName() != L"Page")
        continue;
      auto annots_pageid = elem->GetAttribute(L"PageID").GetInteger();
      //TODO最好判断annots_pageid是否有效
      auto file_loc = elem->GetFirstChildNamed_NoPrefix(L"FileLoc");
      while (file_loc) {
        auto annot_file_loc = file_loc->GetTextData();
        if (annot_file_loc.IsEmpty())
          continue;
        if (annot_file_loc[0] != L'/') {
          annot_file_loc = annots_path + annot_file_loc;
        }
        pageid2annoturl_.insert(std::make_pair(annots_pageid, annot_file_loc));
        file_loc = file_loc->GetNextSiblingNamed_NoPrefix(L"FileLoc");
      }
    }
  }
}

void COFD_Document::ParseSignatures() {
  if (docinfo_->signature_root.IsEmpty())
    return;
  CFX_XMLDocument* signatures_xml =
      ReadAndCacheXml(docinfo_->signature_root.ToUTF8().AsStringView());
  if (!signatures_xml)
    return;
  CFX_XMLElement* root = signatures_xml->GetRoot();
  if (!root)
    return;
  CFX_XMLElement* signatures_node = root->GetFirstChildNamed_NoPrefix(L"Signatures");
  if (!signatures_node)
    return;
  for (auto* child = signatures_node->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (!elem)
      continue;
    if (elem->GetLocalTagName() == L"MaxSignId") {
      max_signature_id_ = StringToFloat(elem->GetTextData().AsStringView());
      continue;
    }
    if (elem->GetLocalTagName() == L"Signature") {
      int id = ++max_signature_id_;
      if (elem->HasAttribute(L"ID"))
        id = StringToFloat(elem->GetAttribute(L"ID").AsStringView());
      WideString sig_type(L"Seal");
      if (elem->HasAttribute(L"Type"))
        sig_type = elem->GetAttribute(L"Type");
      WideString sig_url;
      if (elem->HasAttribute(L"BaseLoc")) {
        sig_url = elem->GetAttribute(L"BaseLoc");
        if (sig_url.IsEmpty())
          continue;
        if (sig_url[0] != L'/') {
          //sig_url = base_url_ + sig_url;
          auto delimiter = docinfo_->signature_root.ReverseFind(L'/');
          if (delimiter.has_value()) {
            sig_url =
                docinfo_->signature_root.Substr(0,delimiter.value() + 1) + sig_url;
          } else {
            sig_url = base_url_ + sig_url;
          }
        }
      }
      std::unique_ptr<COfdSignature> sig_obj = std::make_unique<COfdSignature>(this, id, sig_type, sig_url);
      signatures_.push_back(std::move(sig_obj));
      continue;
    }
  }
}

xilou::SignatureIface* COFD_Document::GetSignature(size_t index) {
  if (signatures_.empty() || index > signatures_.size())
    return nullptr;
  return signatures_[index].get();
}

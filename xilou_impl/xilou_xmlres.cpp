#include "ofd/cofd_common.h"
#include "xilou_impl/xilou_xmldocument.h"
#include "ofd/cofd_zipentry.h"
#include "xilou_impl/xilou_xmlres.h"
#include "xilou_impl/xilou_xmlpage.h"
#include "core/fxcrt/xml/cfx_xmltext.h"
#include "core/fxge/cfx_font.h"


using namespace ofd;
using namespace fxcrt;
namespace xilou {
Cxml_Res::Cxml_Res(Cxml_Document* doc)
    : m_doc(doc), m_name(L"PublicRes.xml") {
  m_fontRes.clear();
}

Cxml_Res::~Cxml_Res() {
  std::map<const CPDF_Font*, FontInfo*>::iterator it = m_fontRes.begin();
  while (it != m_fontRes.end()) {
    delete it->second;
    it->second = NULL;
    it = m_fontRes.erase(it);
  }
  m_fontRes.clear();
}

void Cxml_Res::create() {
  m_resxml = std::make_unique<CFX_XMLDocument>();
  auto res_node = m_resxml->CreateNode<CFX_XMLElement>(L"ofd:Res");
  m_resxml->GetRoot()->AppendFirstChild(res_node);
  res_node->SetAttribute(L"xmlns:ofd", L"http://www.ofdspec.org");
  res_node->SetAttribute(L"BaseLoc", L"Res");
  m_resBaseUrl = WideString(m_doc->getDocBaseUrl()) + L"Res/";
}

CFX_XMLElement* Cxml_Res::createFonts() {
  auto fonts = m_resxml->CreateNode<CFX_XMLElement>(L"ofd:Fonts");
  m_resxml->GetRoot()->GetFirstChild()->AppendLastChild(fonts);
  return ToXMLElement(fonts);
}

uint32_t Cxml_Res::createFontNode(CFX_XMLElement* fontsNode, const CPDF_Font* font) {
  if (!fontsNode || !font)
    return 0;
  const CFX_Font* cfxFont = font->GetFont();
  if (!cfxFont)
    return 0;
  uint32_t uID = m_doc->getUniqueID();
  auto font_node = m_resxml->CreateNode<CFX_XMLElement>(L"ofd:Font");
  WideString wsID = WideString::Format(L"%u", uID);
  font_node->SetAttribute(L"ID", wsID);
  WideString fontName =
      WideString::FromDefANSI(font->GetBaseFontName().AsStringView());
  font_node->SetAttribute(L"FontName", fontName);
  WideString familyName =
      WideString::FromDefANSI(cfxFont->GetFamilyName().AsStringView());
  font_node->SetAttribute(L"FamilyName", familyName);
  if (cfxFont->IsBold())
    font_node->SetAttribute(L"Bold", L"true");
  if (cfxFont->IsFixedWidth())
    font_node->SetAttribute(L"FixedWidth", L"true");
  WideString wsFontFileType(L"otf");
  if (cfxFont->IsTTFont())
    wsFontFileType = L"ttf";
  FontInfo* info = new FontInfo();
  info->fontID = uID;
  if (cfxFont->IsEmbedded()) {
    // 内嵌字体
    auto fontFile = m_resxml->CreateNode<CFX_XMLElement>(L"ofd:FontFile");
    WideString fileName = WideString::Format(L"font_%u.", uID) + wsFontFileType;
    auto text = m_resxml->CreateNode<CFX_XMLText>(fileName);
    info->fileName = fileName;
    fontFile->InsertChildNode(text, 0);
    font_node->InsertChildNode(fontFile, 0);
    uint8_t* fontData = cfxFont->GetFontSpan().data();
    if (fontData) {
      size_t size = cfxFont->GetFontSpan().size();
      info->fontData = new char[size];
      memcpy(info->fontData, fontData, size);
      info->len = size;
    }
  }
  fontsNode->AppendLastChild(font_node);
  m_fontRes.insert(std::make_pair(font, info));
  return uID;
}


uint32_t Cxml_Res::createFontNode(CFX_XMLElement* fontsNode,
                                  const WideString& fontName,
                                  const WideString& familyName) {
  if (!fontsNode || fontName.IsEmpty())
    return 0;
  uint32_t uID = m_doc->getUniqueID();
  auto font = m_resxml->CreateNode<CFX_XMLElement>(L"ofd:Font");
  WideString wsID = WideString::Format(L"%u", uID);
  font->SetAttribute(L"ID", wsID);
  font->SetAttribute(L"FontName", fontName);
  if (familyName.IsEmpty())
    font->SetAttribute(L"FamilyName", fontName);
  else
    font->SetAttribute(L"FamilyName", familyName);
  fontsNode->AppendLastChild(font);
  return uID;
}

CFX_XMLElement* Cxml_Res::createColorSpaces() {
  auto colorSpaces = m_resxml->CreateNode<CFX_XMLElement>(L"ofd:ColorSpaces");
  m_resxml->GetRoot()->GetFirstChild()->AppendFirstChild(colorSpaces);
  return ToXMLElement(colorSpaces);
}

bool Cxml_Res::setName(WideString& name) {
  if (name.IsEmpty())
    return false;
  m_name = name;
  return true;
}

WideString Cxml_Res::getName() const {
  return m_name;
}

bool Cxml_Res::save(ofd::COFD_ZipEntry* pzip) {
  std::map<const CPDF_Font*, FontInfo*>::iterator it = m_fontRes.begin();
  for (; it != m_fontRes.end(); ++it) {
    FontInfo* info = it->second;
    if (info && info->fontData) {
      // 内嵌字体
      WideString fontPath = m_resBaseUrl + info->fileName;
      auto stream = pdfium::MakeRetain<BinaryWriteStream>();
      stream->WriteBlock(info->fontData, info->len);
      pzip->WriteBinary(fontPath.ToUTF8().AsStringView(), stream->getBufAddr());
    }
  }
  if ((size_t)-1 == m_name.Find(L".xml"))
    m_doc->addPublicResNode(m_name + L".xml");
  else
    m_doc->addPublicResNode(m_name);
  if (m_resxml && m_resxml->GetRoot() && m_resxml->GetRoot()->GetFirstChild()) {
    auto stream = pdfium::MakeRetain<BinaryWriteStream>();
    stream->WriteString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    m_resxml->GetRoot()->GetFirstChild()->Save(stream);
    WideString filePath = m_doc->getDocBaseUrl() + m_name;
    if ((size_t)-1 == filePath.Find(L".xml"))
      filePath += L".xml";
    if (0 != pzip->WriteBinary(filePath.ToUTF8().AsStringView(),
                                stream->getBufAddr())) {
      return false;
    }
    return true;
  }
  return false;
}

uint32_t Cxml_Res::getFontID(const WideString& fontName) {
  uint32_t fontID = 0;
  CFX_XMLElement* res_node = ToXMLElement(m_resxml->GetRoot()->GetFirstChild());
  if (!res_node)
    return fontID;
  auto fonts = res_node->GetFirstChildNamed(L"ofd:Fonts");
  if (!fonts)
    fonts = createFonts();
  for (auto* child = fonts->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (elem && elem->GetAttribute(L"FontName") == fontName) {
      fontID = elem->GetAttribute(L"ID").GetInteger();
      break;
    }
  }
  if (!fontID) {
    fontID = createFontNode(fonts, fontName, fontName);
  }
  return fontID;
}


uint32_t Cxml_Res::getFontID(const CPDF_Font* font) {
  if (!font)
    return 0;
  std::map<const CPDF_Font*, FontInfo*>::iterator it = m_fontRes.find(font);
  if (it != m_fontRes.end())
    return it->second->fontID;
  uint32_t fontID = 0;
  CFX_XMLElement* res_node = ToXMLElement(m_resxml->GetRoot()->GetFirstChild());
  if (!res_node)
    return fontID;
  WideString fontName = WideString::FromDefANSI(font->GetBaseFontName().AsStringView());
  auto fonts = res_node->GetFirstChildNamed(L"ofd:Fonts");
  if (!fonts)
    fonts = createFonts();
  else { // 二次匹配，防重
    const CFX_Font* cfxFont = font->GetFont();
    WideString familyName =
        WideString::FromDefANSI(cfxFont->GetFamilyName().AsStringView());
    for (auto* child = fonts->GetFirstChild(); child;
         child = child->GetNextSibling()) {
      CFX_XMLElement* elem = ToXMLElement(child);
      if (elem && elem->GetAttribute(L"FontName") == fontName &&
          elem->GetAttribute(L"FamilyName") == familyName) {
        fontID = elem->GetAttribute(L"ID").GetInteger();
        return fontID;
      }
    }
  }
  fontID = createFontNode(fonts, font);
  return fontID;
}

uint32_t Cxml_Res::appendImageRes(WideStringView imgName) {
  CFX_XMLElement* res_node = ToXMLElement(m_resxml->GetRoot()->GetFirstChild());
  DCHECK(res_node);
  if (!res_node)
    return 0;
  auto medias = res_node->GetFirstChildNamed(L"ofd:MultiMedias");
  if (!medias) {
    medias = m_resxml->CreateNode<CFX_XMLElement>(L"ofd:MultiMedias");
    m_resxml->GetRoot()->GetFirstChild()->AppendLastChild(medias);
  }
  DCHECK(medias);
  auto media = m_resxml->CreateNode<CFX_XMLElement>(L"ofd:MultiMedia");
  medias->AppendLastChild(media);
  auto res_id = m_doc->getUniqueID();
  media->SetAttribute(L"ID", WideString::Format(L"%d", res_id));
  media->SetAttribute(L"Type", L"Image");
  auto mediaFile = m_resxml->CreateNode<CFX_XMLElement>(L"ofd:MediaFile");
  media->AppendLastChild(mediaFile);
  auto fileUrl = m_resxml->CreateNode<CFX_XMLText>(WideString(imgName));
  mediaFile->AppendLastChild(fileUrl);
  return res_id;
}
}//end of namespace
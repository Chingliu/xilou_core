#ifndef COFD_DOCUMENT_H_HEADER
#define COFD_DOCUMENT_H_HEADER
#include <vector>
#include <map>
#include <unordered_map>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/unowned_ptr.h"
#include "ofd/cofd_common.h"
#include "core/fxcrt/retain_ptr.h"

class CPDF_PageObject;
class CPDF_ColorSpace;
class CPDF_Document;
class CPDF_Font;
namespace xilou {
class SignatureIface;
}

namespace ofd {

class COFD_Package;
class COFD_Page;
class COFD_DocInfo;
class COfdSignature;

using FontUnicodeMap = std::map<RetainPtr<CPDF_Font>, std::multimap<uint32_t, uint32_t> >;
class COFD_Document {
 public:
  explicit COFD_Document(COFD_Package* pkg,
                         COFD_DocInfo* docinfo,
                         CFX_XMLDocument* xml,
                         WideStringView base_url);

 public:
  COFD_Document(const COFD_Document&) = delete;
  COFD_Document(COFD_Document&&) = delete;
  COFD_Document& operator=(const COFD_Document&) = delete;
  COFD_Document& operator=(COFD_Document&&) = delete;
  ~COFD_Document();

 public:
   class CMultiMedia {
   public:
     int m_id;
     WideString m_type;
     WideString m_format;
     WideString m_fullpath;
  };
 public:
  int PageCount() { return pageno2pageid_.size(); }
  //index = page number start with 0;
  WideString PageUrl(unsigned int index);
  WideString QueryPagesAnnot(unsigned int pageid, unsigned int idx);
  size_t CountPageAnnot(unsigned int pageid);
  //TODO, 现在是返回了new的指针，应该使用智能指针的方式管理
  COFD_Page* LoadPage(unsigned int index);
  void DropPage(COFD_Page* page);
  CFX_RectF ParseArea(CFX_XMLElement* area_node);
  CFX_RectF GetDocumentArea() const { return bbox_; }
  int GetDocMaxId() const { return max_id_; }
  WideString TemplateUrl(unsigned int id);
  WideString GetDocBaseUrl() { return base_url_; }
 public:
  std::vector<float> ParseColor(CFX_XMLElement* elem,
                                CPDF_PageObject* page_object,
                                bool is_fill);
  void ParseDrawParam(CFX_XMLElement* elem, CPDF_PageObject* page_object);
  CFX_XMLElement* FindDrawParam(int id);
  CFX_XMLElement* FindCompositeObject(int id);
  RetainPtr<CPDF_ColorSpace> FindColorSpace(int id);
  RetainPtr<CPDF_Font> FindFont(int id);
  CMultiMedia* FindMedia(int id);
 public:
  UnownedPtr<COFD_Package> const pkg_;
  CFX_XMLDocument* ReadAndCacheXml(ByteStringView entry_name);
  int ReadBinary(ByteStringView entry_name, std::vector<uint8_t>* buf);
  CPDF_Document* GetFakePDFDocument();
  FontUnicodeMap& GetFontUnicodeMap() { return under2unicode; }
  size_t GetSignatureCount() { return signatures_.size();}
  xilou::SignatureIface* GetSignature(size_t index);
 private:
  void ParseCommonData(CFX_XMLElement* root);
  void ParseRes(WideString res_url);
  void HandleResDrawParam(CFX_XMLElement* elem);
  void HandleResColorSpaces(CFX_XMLElement* elem);
  void HandleResCompositeObj(CFX_XMLElement* elem);
  void HandleFonts(CFX_XMLElement* elem, WideStringView base_loc);
  void HandleMultiMedias(CFX_XMLElement* elem, WideStringView base_loc);
  void ParseAnnotations(CFX_XMLElement* root);
  void ParseSignatures();
 private:
  static const int kDefaultFontID;
 private:
  UnownedPtr<COFD_DocInfo> const docinfo_;
  UnownedPtr<CFX_XMLDocument> const xml_;
  WideString base_url_;
  std::vector<int> pageno2pageid_;
  std::vector<std::unique_ptr<COfdSignature> > signatures_;
  std::map<int, WideString> pageid2pageurl_;
  std::multimap<int, WideString> pageid2annoturl_;
  std::map<int, CFX_XMLElement*> drawparams_;
  std::map<int, CFX_XMLElement*> compositeobj_;
  std::map<int, RetainPtr<CPDF_ColorSpace> > colorspaces_;
  std::map<int, RetainPtr<CPDF_Font> > fonts_;
  std::map<RetainPtr<CPDF_Font>, std::multimap<uint32_t, uint32_t> > under2unicode;
  std::map<int, std::unique_ptr<CMultiMedia> > multi_medias_;
  std::map<int, WideString> id2templates_;
  int max_id_;
  CFX_RectF bbox_;
  int max_signature_id_;
};

}
#endif  // !COFD_PACKAGE_H_HEADER

#ifndef XILOU_IMPL_XILOU_XMLPAGE_H
#define XILOU_IMPL_XILOU_XMLPAGE_H
#include <vector>
#include <memory>
#include <sstream>
#include <string>
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_string.h"
#include "core/fpdfdoc/cpdf_annot.h"
#include "public/fpdfview.h"
class CFX_XMLDocument;
class CFX_XMLElement;
class CPDF_Font;
namespace xilou {
class Cxml_package;
class Cxml_page;

  class Cxml_pageobj {
  UnownedPtr<Cxml_page> m_page;
    CFX_Matrix m_formMatrix;
  CFX_FloatRect m_formRect;
    bool m_hasParentForm;
 protected:
  UnownedPtr<CFX_XMLDocument> m_root;
  UnownedPtr<CFX_XMLElement> m_objectroot;
   public:
  virtual void convert() = 0;
 public:
  Cxml_pageobj(Cxml_page* page);
  virtual ~Cxml_pageobj();
  CFX_XMLElement* getObjectRoot();
  CFX_Matrix getFormMatrix() { return m_formMatrix; }
  void setFormMatrix(const CFX_Matrix& matrix) {
    m_formMatrix = matrix;
    m_hasParentForm = true;
  }
  CFX_FloatRect getFormRect() { return m_formRect; }
  void setFromRect(const CFX_FloatRect& rect) { m_formRect = rect; }
  bool insideForm() { return m_hasParentForm; }
  // 后期理清form的CTM后，可在方法里结合form的CTM计算，页的CTM也可考虑是否计算进去 TODO
  CFX_Matrix transformOfdCtm(CFX_Matrix& ctm, CFX_Matrix& annotCtm, float& scale);
  };

class Cxml_pageAnnot {
   public:
    explicit Cxml_pageAnnot();
    virtual ~Cxml_pageAnnot();

   public:
    bool is_annot() { return m_bAnnot;}
    void setAnnotCTM(const CFX_Matrix& ctm) { m_annot_ctm = ctm; }
    CFX_Matrix getAnnotCTM() { return m_annot_ctm; }
    void setAnnotSubType(CPDF_Annot::Subtype subtype) {
      m_subtype = subtype;
      m_bAnnot = true;
    }
    void setAnnotRect(const CFX_FloatRect& rect) { m_rect = rect;}
    CFX_FloatRect getAnnotRect() { return m_rect; }
    void setAnnotBBox(const CFX_FloatRect& bbox) { m_bbox = bbox; }
    CFX_FloatRect getAnnotBBox() { return m_bbox; }
   public:
    virtual void  annot2stream(
        const RetainPtr<IFX_RetainableWriteStream>& pXMLStream) = 0;
   private:
    bool m_bAnnot;
    CFX_Matrix m_annot_ctm;
    CPDF_Annot::Subtype m_subtype;
    CFX_FloatRect m_rect;
    CFX_FloatRect m_bbox;
  };
class Cxml_page :public Cxml_pageAnnot{
 public:
  explicit Cxml_page(Cxml_package* package);

 public:
  Cxml_page(const Cxml_page&) = delete;
  Cxml_page(Cxml_page&&) = delete;
  Cxml_page& operator=(const Cxml_page&) = delete;
  Cxml_page& operator=(Cxml_page&&) = delete;
  ~Cxml_page();

 public:
  void annot2stream(const RetainPtr<IFX_RetainableWriteStream>& pXMLStream);
 public:
  void appendObj(std::unique_ptr<Cxml_pageobj> xml_pageobj);
  //m_pagenode 移至genPageXml中生成
  CFX_XMLElement* getOfdPageNode() { return m_pagenode; }
  CFX_XMLDocument* getPageXmlRoot() { return m_pagexml.get(); }
  void genPageXml();
  uint32_t getUniqueID();
  void setPDFBox(const CFX_RectF& rect) { m_pdfbox = rect; }
  CFX_RectF getPDFBox() { return m_pdfbox; }
  void setPDFDisplayCTM(const CFX_Matrix& ctm) { m_pdf_display_ctm = ctm; }
  CFX_Matrix getPDFDisplayCTM() { return m_pdf_display_ctm; }
  uint32_t getFontID(const WideString& wsFontName);
  uint32_t getFontID(const CPDF_Font* font);
  uint32_t appendImageRes(WideStringView image_name,
                          std::vector<uint8_t>* img_data);
  void appendPageAnnot(std::unique_ptr<Cxml_pageAnnot> pageAnnot);
  size_t annotCount() { return m_pageAnnots.size(); }
  Cxml_pageAnnot* getAnnot(size_t index) { 
    if (index < m_pageAnnots.size())
      return m_pageAnnots.at(index).get();
    else
      return nullptr;
  }

 public:
  void setPdfPage(FPDF_PAGE pdfpage) { m_fpdfpage = pdfpage; }
  FPDF_PAGE getPdfPage() { return m_fpdfpage; }
  void setPdfDoc(FPDF_DOCUMENT pdfdoc) { m_fpdfdoc = pdfdoc; }
  FPDF_DOCUMENT getPdfDoc() { return m_fpdfdoc; }
 private:
  std::vector<std::unique_ptr<Cxml_pageobj> > m_objects;
  UnownedPtr<Cxml_package> m_package;
  std::unique_ptr<CFX_XMLDocument> m_pagexml;
  UnownedPtr<CFX_XMLElement> m_pagenode;
  CFX_RectF m_pdfbox;
  CFX_Matrix m_pdf_display_ctm;
  std::vector < std::unique_ptr<Cxml_pageAnnot> >m_pageAnnots;
  FPDF_PAGE m_fpdfpage;//生命周期由外部管理，使用是要判空
  FPDF_DOCUMENT m_fpdfdoc;//生命周期由外部管理，使用是要判空
};


class StringWriteStream final : public IFX_RetainableWriteStream {
 public:
  StringWriteStream();
  ~StringWriteStream() override;

  // IFX_WriteStream:
  bool WriteBlock(const void* pData, size_t size) override;

  std::string ToString() const { return stream_.str(); }

 private:
  std::ostringstream stream_;
};

class BinaryWriteStream final : public IFX_RetainableWriteStream {
 public:
  BinaryWriteStream();
  ~BinaryWriteStream() override;

  // IFX_WriteStream:
  bool WriteBlock(const void* pData, size_t size) override;
  std::vector<uint8_t>* getBufAddr() { return &m_buf; }
 private:
  std::vector<uint8_t> m_buf;
};

}// end of namespace xilou
#endif
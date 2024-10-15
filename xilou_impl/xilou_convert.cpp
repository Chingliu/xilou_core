#include <stdio.h>
#include "xilou_public/xilou_convert.h"
#include "xilou_impl/xilou_impl.h"
#include "xilou_public/xilou_errcode.h"
#include <memory>
#include <string>
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxcrt/unowned_ptr.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"
#include "public/fpdf_save.h"
#include "public/cpp/fpdf_scopers.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/page/cpdf_path.h"
#include "core/fpdfapi/page/cpdf_pathobject.h"
#include "core/fpdfapi/page/cpdf_textobject.h"
#include "xfa/fxfa/cxfa_ffwidget.h"
#include "fpdfsdk/cpdfsdk_formfillenvironment.h"
#include "xilou_ofdmerge.h"
#define JSON_NOEXCEPTION
#include "third_party/nlohmann/json.hpp"
#include "third_party/harfbuzz/hbsubset.h"
using json = nlohmann::json;
using namespace xilou;

struct XILOUPackageDeleter {
  inline void operator()(XILOU_PACKAGE pkg) { xilou_ClosePackage(pkg); }
};

struct XILOUDocumentDeleter {
  inline void operator()(XILOU_DOCUMENT doc) { xilou_DropDocument(doc); }
};

using ScopedXILOUPackage =
    std::unique_ptr<std::remove_pointer<XILOU_PACKAGE>::type,
                    XILOUPackageDeleter>;

using ScopedXILOUDocument =
    std::unique_ptr<std::remove_pointer<XILOU_DOCUMENT>::type,
                    XILOUDocumentDeleter>;


namespace {

  class CWritePDF : public FPDF_FILEWRITE {
  RetainPtr<IFX_SeekableStream> file_stream;
 public:
    CWritePDF(const char* file_path) {
    FPDF_FILEWRITE::version = 1;
    FPDF_FILEWRITE::WriteBlock = WriteBlockCallback;
    WideString utf8_path = FX_UTF8Decode(file_path);
#if !defined(OS_WIN)
    file_stream =
        IFX_SeekableStream::CreateFromFilename(utf8_path.c_str(),
                                                         FX_FILEMODE_Truncate);
#else
    ByteString utf16_byte = utf8_path.ToUTF16LE();
    WideString utf16_path = WideString::FromUTF16LE(
        reinterpret_cast<const unsigned short*>(utf16_byte.raw_span().data()),
        utf16_byte.GetLength());
    file_stream = IFX_SeekableStream::CreateFromFilename(utf16_path.c_str(),
                                                         FX_FILEMODE_Truncate);
#endif
    }
    ~CWritePDF() { 
      
    }
    static int WriteBlockCallback(FPDF_FILEWRITE* pFileWrite,
      const void* data,
      unsigned long size) {
      CWritePDF* pThis = static_cast<CWritePDF*>(pFileWrite);

      if (pThis->file_stream)
        return pThis->file_stream->WriteBlock(static_cast<const char*>(data), size);

      return 0;
  }
  };

  bool ofd2pdf(XILOU_DOCUMENT src_doc,
    XILOU_UTF8STRING dest_fpath,
    XILOU_UTF8STRING page_range) {

    CXilou_ChildDocument* doc = reinterpret_cast<CXilou_ChildDocument*>(src_doc);
    if (!doc)
      return false;

    ofd::COFD_Document* ofd_doc = doc->ofd;
    FPDF_DOCUMENT pdf_doc= FPDFDocumentFromCPDFDocument(ofd_doc->GetFakePDFDocument());
    int page_count = ofd_doc->PageCount();
    //此处不能新建，因为下面使用了insertobject，只是转移了pageobj
    // //如果要使用FPDF_CreateNewDocument，意味着所有资源类对像要在此新建文档中加载
    //auto newPdf = FPDF_CreateNewDocument();
    for (int i = page_count -1; i >= 0; --i) {
      std::unique_ptr<ofd::COFD_Page> page(ofd_doc->LoadPage(i));
      page->Parse();
      CFX_RectF page_area = page->GetPageArea();
      auto width = static_cast<int>(page_area.Width());
      auto height = static_cast<int>(page_area.Height());
      auto newPage = FPDFPage_New(pdf_doc, 0, width, height);

      std::vector<ofd::COFD_Page*> layers;
      if (page->GetBackGroundTemplateLayer()) {
        layers.push_back(page->GetBackGroundTemplateLayer());
      }
      if (page->GetBackGroundLayer()) {
        layers.push_back(page->GetBackGroundLayer());
      }
      if (page->GetBodyTemplateLayer()) {
        layers.push_back(page->GetBodyTemplateLayer());
      }
      layers.push_back(page.get());
      if (page->GetForeGroundTemplateLayer()) {
        layers.push_back(page->GetForeGroundTemplateLayer());
      }
      if (page->GetForeGroundLayer()) {
        layers.push_back(page->GetForeGroundLayer());
      }
      auto it = layers.begin();
      for (; it != layers.end(); ++it) {
        auto page_objs = (*it)->GetPageObjectCount();
        // TODO字体内嵌要裁剪大小
        for (size_t objs = 0; objs < page_objs; objs++) {
          CPDF_PageObject* pageobj = (*it)->PopPageObjectByIndex(objs);
#if 1          
          if (pageobj->GetType() == CPDF_PageObject::Type::TEXT) {
            CPDF_TextObject* txtobj = pageobj->AsText();
            CFX_Matrix tm = txtobj->GetTextMatrix();
            CFX_PointF txtpoint(tm.e, tm.f);
            CPDF_Page* pdf_page = CPDFPageFromFPDFPage(newPage);
            const FX_RECT rect = page_area.GetOuterRect();
            int irotate = pdf_page->GetPageRotation();
            auto dp = pdf_page->DeviceToPage(rect, irotate, txtpoint);
            if (!dp.has_value())
              continue;
            tm.e = dp->x;
            tm.f = dp->y;
            txtobj->m_ClipPath.SetNull();
            txtobj->SetTextMatrix(tm);
            
          } 
          else if (pageobj->GetType() == CPDF_PageObject::Type::PATH) {
            CPDF_PathObject* pathobj = pageobj->AsPath();
            auto ori_path = pathobj->path();
            auto path_ctm = pathobj->matrix();
            CFX_PointF path_point(path_ctm.e, path_ctm.f);
            CPDF_Page* pdf_page = CPDFPageFromFPDFPage(newPage);
            const FX_RECT rect = page_area.GetOuterRect();
            int irotate = pdf_page->GetPageRotation();
            auto dp = pdf_page->DeviceToPage(rect, irotate, path_point);
            if (!dp.has_value())
              continue;
            path_ctm.d = -path_ctm.d;
            path_ctm.e = dp->x;
            path_ctm.f = dp->y;
            pathobj->m_ClipPath.SetNull();
            pathobj->SetPathMatrix(path_ctm);
          } else if (pageobj->GetType() == CPDF_PageObject::Type::IMAGE) {
            CPDF_ImageObject* imgobj = pageobj->AsImage();
            auto path_ctm = imgobj->matrix();
            CFX_PointF path_point(path_ctm.e, path_ctm.f);
            CPDF_Page* pdf_page = CPDFPageFromFPDFPage(newPage);
            const FX_RECT rect = page_area.GetOuterRect();
            int irotate = pdf_page->GetPageRotation();
            auto dp = pdf_page->DeviceToPage(rect, irotate, path_point);
            if (!dp.has_value())
              continue;
            path_ctm.d = -path_ctm.d;
            path_ctm.e = dp->x;
            path_ctm.f = dp->y;

            imgobj->SetImageMatrix(path_ctm);
          }
#endif
          FPDFPage_InsertObject(newPage,
                                FPDFPageObjectFromCPDFPageObject(pageobj));
        }
      }      
      FPDFPage_GenerateContent(newPage);
      FPDF_ClosePage(newPage);
    }
    std::unique_ptr<CWritePDF> pWritor =
        std::make_unique<CWritePDF>(dest_fpath);
    FPDF_SaveAsCopy(pdf_doc, pWritor.get(), 0);
    //不能释放，是由ofd_package来管理的
    //FPDF_CloseDocument(newPdf);
    return true;
  }

/*
  // Helper function to load font data from file using unique_ptr
  std::unique_ptr<uint8_t[]> LoadFontData(const char* filename,
                                          unsigned long* size) {
    FILE* file = fopen(filename, "rb");
    if (!file)
      return nullptr;

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::unique_ptr<uint8_t[]> buffer(new uint8_t[*size]);
    fread(buffer.get(), 1, *size, file);
    fclose(file);

    return buffer;
  }
//*/
  }//end of namespace

namespace xilou {
bool pdf2ofd(XILOU_DOCUMENT src_doc,
             XILOU_UTF8STRING dest_fpath,
             XILOU_UTF8STRING page_range);

bool xtotxt(XILOU_DOCUMENT src_doc,
            XILOU_UTF8STRING dest_fpath,
            XILOU_UTF8STRING page_range);

}// end of xilou

  XILOU_EXPORT bool XILOU_CALLCONV xilou_Convert(XILOU_UTF8STRING src_fpath,
  XILOU_UTF8STRING dest_fpath,
  XILOU_UTF8STRING page_range) {
  if (!src_fpath || !dest_fpath)
    return false;

  ScopedXILOUPackage src_pkg(xilou_OpenPackage(src_fpath, nullptr));
  if (!src_pkg)
    return false;
  CXilou_Document* doc = reinterpret_cast<CXilou_Document*>(src_pkg.get());
  auto output_fpath = FX_UTF8Decode(dest_fpath);
  auto extname = output_fpath.AsStringView().Last(4);
  if (extname == L".txt") {
    ScopedXILOUDocument src_doc(xilou_LoadDocument(src_pkg.get(), 0));
    return xtotxt(src_doc.get(), dest_fpath, page_range);
  }
  if (doc->m_filetype == E_FILETYPE_OFD)
  { 
    ScopedXILOUDocument src_doc(xilou_LoadDocument(src_pkg.get(), 0));
    return ofd2pdf(src_doc.get(), dest_fpath, page_range);
  } else {
    ScopedXILOUDocument src_doc(xilou_LoadDocument(src_pkg.get(), 0));
    return pdf2ofd(src_doc.get(), dest_fpath, page_range);
  }
}

XILOU_EXPORT int XILOU_CALLCONV
xilou_AppendImagePage(XILOU_PACKAGE* pkg, XILOU_UTF8STRING img_path) {
  if (!pkg)
    return XILOU_E_INVALID_HANDLE_ERROR;
  if (!img_path)
    return XILOU_E_INVALIDPARAMETER;
  WideString utf8_path = FX_UTF8Decode(img_path);
#if !defined(OS_WIN)
  RetainPtr<IFX_SeekableStream> file_stream =
      IFX_SeekableStream::CreateFromFilename(utf8_path.c_str(),
                                             FX_FILEMODE_ReadOnly);
#else
  ByteString utf16_byte = utf8_path.ToUTF16LE();
  WideString utf16_path = WideString::FromUTF16LE(
      reinterpret_cast<const unsigned short*>(utf16_byte.raw_span().data()),
      utf16_byte.GetLength());
  RetainPtr<IFX_SeekableStream> file_stream =
      IFX_SeekableStream::CreateFromFilename(utf16_path.c_str(),
                                             FX_FILEMODE_ReadOnly);
#endif

  int32_t iImageXDpi;
  int32_t iImageYDpi;
  auto pBitmap = XFA_LoadImageFromBuffer(file_stream, FXCODEC_IMAGE_UNKNOWN,
                                         iImageXDpi, iImageYDpi);
  if (!pBitmap)
    return XILOU_E_LOADIMAGE;
  double page_w = pBitmap->GetWidth() * 72.f / iImageXDpi;
  double page_h = pBitmap->GetHeight() * 72.f / iImageYDpi;
  if (!*pkg) {
    std::unique_ptr<CXilou_Document> doc =
        std::make_unique<CXilou_Document>();
    if (!doc)
      return XILOU_E_INVALID_HANDLE_ERROR;
    doc->m_filetype = E_FILETYPE_PDF;
    doc->f.pdf = FPDF_CreateNewDocument();
    *pkg = reinterpret_cast<XILOU_PACKAGE>(doc.release());
  }
  CXilou_Document* doc = reinterpret_cast<CXilou_Document*>(*pkg);
  if (!doc)
    return XILOU_E_INVALID_HANDLE_ERROR;
  if (doc->m_filetype != E_FILETYPE_PDF)
    return XILOU_E_FILEHANDLETYPE;
  if (!doc->f.pdf)
    return XILOU_E_INVALID_HANDLE_ERROR;

  auto imgobj = FPDFPageObj_NewImageObj(doc->f.pdf);
  if (!imgobj)
    return XILOU_E_NEWPAGEOBJECT;
  auto bret = FPDFImageObj_SetBitmap(nullptr, 0, imgobj,
                                     FPDFBitmapFromCFXDIBitmap(pBitmap.Leak()));
  if (!bret)
    return XILOU_E_SETBITMAP;
  auto pages = FPDF_GetPageCount(doc->f.pdf);
  FPDF_PAGE pdf_page = FPDFPage_New(doc->f.pdf, pages + 1, page_w, page_h);
  if (!pdf_page)
    return XILOU_E_NEWPAGE;
  FPDFPageObj_Transform(imgobj, 595, 0, 0, 842, 0, 0);//TODO
  FPDFPage_InsertObject(pdf_page, imgobj);
  FPDFPage_GenerateContent(pdf_page);
  FPDF_ClosePage(pdf_page);
  return XILOU_E_SUC;
}

XILOU_EXPORT int XILOU_CALLCONV xilou_Save2File(XILOU_PACKAGE* pkg,
  XILOU_UTF8STRING dst_path) {
  if (!pkg)
    return XILOU_E_INVALID_HANDLE_ERROR;
  if (!dst_path)
    return XILOU_E_INVALIDPARAMETER;
  CXilou_Document* doc = reinterpret_cast<CXilou_Document*>(*pkg);
  if (!doc)
    return XILOU_E_INVALID_HANDLE_ERROR;
  auto len = strlen(dst_path);
  if ((dst_path[len - 1] == 'f' || dst_path[len - 1] == 'F') &&
      (dst_path[len - 2] == 'd' || dst_path[len - 2] == 'D') &&
      (dst_path[len - 3] == 'p' || dst_path[len - 3] == 'P') &&
      (dst_path[len - 4] == '.') ) {

    if (doc->m_filetype == E_FILETYPE_PDF) {
      std::unique_ptr<CWritePDF> pWritor =
          std::make_unique<CWritePDF>(dst_path);
      if (!doc->f.pdf)
        return XILOU_E_INVALID_HANDLE_ERROR;
      FPDF_SaveAsCopy(doc->f.pdf, pWritor.get(), 0);
    } else {
      //TODO,调用ofd2pdf
      return XILOU_E_NOTIMPLEMENT;
    }
  } else {
    if (doc->m_filetype == E_FILETYPE_PDF) {
      ScopedXILOUDocument src_doc(xilou_LoadDocument(*pkg, 0));
      if (!pdf2ofd(src_doc.get(), dst_path, "*"))
        return XILOU_E_PDF2OFD;
    } else {
      //TODO, 调用另存接口
      return XILOU_E_NOTIMPLEMENT;
    }
  }
  return XILOU_E_SUC;
}

namespace {
  class CFontSubset {
  void* face;
    void* blob;
 public:
    CFontSubset(const CFontSubset&) = delete;
  CFontSubset(CFontSubset&&) = delete;
    CFontSubset& operator=(const CFontSubset&) = delete;
  CFontSubset& operator=(CFontSubset&&) = delete;
    CFontSubset(XILOU_UTF8STRING txt_json) { 
      face = nullptr;
      blob = nullptr;
      face = fnhbsubset("C:\\Windows\\Fonts\\msyh.ttc", txt_json);
      if (face) {
        blob = fnhb_getblob_from_face(face);
      }
    }
    const char* get_font_data(unsigned int* outsize) { 
      if (blob) {
        return fnhb_getdata_from_blob(blob, outsize);
      }
      return nullptr;
    }
    ~CFontSubset() { 
      //释放有顺序要求
      if (blob) {
        fnhb_release_blob(blob);
      }
      if (face) {
        fnhb_release_face(face);
      }
  }
  };
}
XILOU_EXPORT int XILOU_CALLCONV xilou_AddTextOnPage(XILOU_PACKAGE pkg,
                                                    int page_idx,
                                                    XILOU_UTF8STRING txt_json) {
  if (!pkg)
    return XILOU_E_INVALID_HANDLE_ERROR;
  CXilou_Document* doc = reinterpret_cast<CXilou_Document*>(pkg);
  if (!doc)
    return XILOU_E_INVALID_HANDLE_ERROR;
  if (doc->m_filetype != E_FILETYPE_PDF)
    return XILOU_E_NOTIMPLEMENT;
  if (!doc->f.pdf)
    return XILOU_E_INVALID_HANDLE_ERROR;
  auto pages = FPDF_GetPageCount(doc->f.pdf);
  if (page_idx < 0 || page_idx > pages)
    return XILOU_E_INVALIDPAGEINDEX;
  if (!txt_json)
    return XILOU_E_INVALIDPARAMETER;
  auto json_data = json::parse(txt_json, nullptr,false);
  if (json_data.is_discarded()) {
    return XILOU_E_INVALIDTEXTJSON;
  }
  if (!json_data.contains("ocrResult"))
    return XILOU_E_INVALIDTEXTJSON;
  unsigned int size = 0;
  CFontSubset subset(txt_json);
  auto font_data = subset.get_font_data(&size);
  //auto font_data = LoadFontData("C:\\Windows\\Fonts\\msyh.ttc", &size);
  //if (!font_data)
  //  return XILOU_E_READFONTFILE;
  ScopedFPDFFont font(FPDFText_LoadFont(doc->f.pdf, (const uint8_t*)font_data, size,
                                        FPDF_FONT_TRUETYPE, true));
  if (!font)
    return XILOU_E_LOADFONTDATA;
  ScopedFPDFPage page(FPDF_LoadPage(doc->f.pdf, page_idx));
  if (!page)
    return XILOU_E_LOADPAGE;
  for (const auto& txt : json_data["ocrResult"]) {
    if (txt.contains("text") && txt.contains("location")) {
      // 对于双层版式文档，直接使用96DPI一般情况下没有问题，但最好在添加图片页的时候，记录下对应的图片DPI，用以替换以下逻辑中的96，TODO
      auto location = txt["location"];
      if (location.contains("left") && location.contains("bottom") &&
          location["left"].is_number() && location["bottom"].is_number()) {
        float font_size = (float(location["bottom"]) - float(location["top"])) * 72.f / 96.f;
        auto txtobj = FPDFPageObj_CreateTextObj(
            doc->f.pdf, font.get(), font_size);
        if (!txtobj)
          continue;
        WideString utf8_txt = FX_UTF8Decode(ByteStringView(txt["text"]));
        ByteString utf16_byte = utf8_txt.ToUTF16LE();
        FPDFText_SetText(txtobj, AsFPDFWideString(&utf16_byte));
        auto x = double(location["left"]) * 72.f / 96.f;
        CPDF_Page* pPage = CPDFPageFromFPDFPage(page.get());
        auto y = pPage->GetPageHeight() - double(location["bottom"]) * 72.f / 96.f;
        // RGBA最好支持设置，此处仅为双层pdf、ofd设置完全透明
        //FPDFPageObj_SetFillColor(txtobj, 0, 0, 0, 0);
        FPDFTextObj_SetTextRenderMode(txtobj, FPDF_TEXTRENDERMODE_INVISIBLE);
        FPDFPageObj_Transform(txtobj, 1, 0, 0, 1, x, y);
        FPDFPage_InsertObject(page.get(), txtobj);
      }      
    }
  }
  FPDFPage_GenerateContent(page.get());
  return XILOU_E_SUC;
}

/*
* 涉及id的属性
* ID=
* Relative=
* DrawParam=
* TemplateID=
* ResourceID=
* PageID=
* RefId=
* PageRef=  
涉及id的对象//<od:ObjectRef PageRef="1">16</od:ObjectRef> PageRef是页id, 16是对象id
*
* 
*/
XILOU_EXPORT int XILOU_CALLCONV
xilou_OfdMerge(XILOU_UTF8STRING merge2ofd_path,
               XILOU_UTF8STRING been_merged_ofd_path)
{
  COfd_Merge merge(merge2ofd_path, been_merged_ofd_path);
  return merge.do_merge();
}
#include "xilou_public/xilou_view.h"
#include "xilou_impl/xilou_impl.h"
#include "public/fpdfview.h"
#include "core/fxcrt/cfx_readonlymemorystream.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxcrt/unowned_ptr.h"
#include "public/cpp/fpdf_scopers.h"
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "fpdfsdk/cpdfsdk_helpers.h"

#include "ofd/render/cofd_pagerendercontext.h"
#include "ofd/render/cofd_renderpage.h"

using namespace xilou;
namespace{

int GetBlock(void* param,
             unsigned long position,
             unsigned char* pBuf,
             unsigned long size) {
  IFX_SeekableStream* stream = reinterpret_cast<IFX_SeekableStream*>(param);
  return stream->ReadBlockAtOffset(pBuf, position, size);
}
bool IsPDF(const char* phead, int len) {
  int i = 0;
  char tag[] = "%PDF-";
  while (i < len) {
    if (0 == memcmp(&phead[i], tag, strlen(tag))) {
      return true;
    }
    i++;
  }
  return false;
}


void OFD_RenderPageBitmap(XILOU_BITMAP bitmap, ofd::COFD_Page* page, int start_x, int start_y, int size_x, int size_y, int rotate, int flags) {
    auto pOwnedContext = std::make_unique<ofd::COFD_PageRenderContext>();
    ofd::COFD_PageRenderContext* pContext = pOwnedContext.get();

    auto pOwnedDevice = std::make_unique<CFX_DefaultRenderDevice>();
    CFX_DefaultRenderDevice* pDevice = pOwnedDevice.get();
    pContext->m_pDevice = std::move(pOwnedDevice);
    RetainPtr<CFX_DIBitmap> pBitmap(CFXDIBitmapFromFPDFBitmap(bitmap));
    auto attached = pDevice->Attach(pBitmap, !!(flags & FPDF_REVERSE_BYTE_ORDER), nullptr,
                    false);
    if (!attached)
      return;
    const FX_RECT rect(start_x, start_y, start_x + size_x, start_y + size_y);
    COFD_RenderPageWithContext(pContext, page,
                               page->GetDisplayMatrix(rect, rotate), start_x,
                               start_y, size_x,
                               size_y, rotate, flags,
                               /*color_scheme=*/nullptr,
                               /*need_to_restore=*/true,
                               /*pause=*/nullptr);
  }
  }  // namespace


XILOU_EXPORT void XILOU_CALLCONV xilou_InitLibrary() {
  FPDF_InitLibrary();
}

XILOU_EXPORT void XILOU_CALLCONV xilou_DestroyLibrary() {
  FPDF_DestroyLibrary();
}

XILOU_EXPORT XILOU_PACKAGE XILOU_CALLCONV
xilou_OpenPackage(XILOU_UTF8STRING file_path, XILOU_BYTESTRING password) {
  WideString utf8_path = FX_UTF8Decode(file_path);
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
  FPDF_FILEACCESS file_access;
  file_access.m_FileLen = file_stream->GetSize();
  file_access.m_GetBlock = GetBlock;
  file_access.m_Param = file_stream.Get();
  char pdf_header[1025] = {0};
  bool bread = file_stream->ReadBlockAtOffset(pdf_header, 0, 1024);
  if (!bread)
    return nullptr;
  std::unique_ptr<CXilou_Document> doc = std::make_unique<CXilou_Document>();
  if (!doc)
    return nullptr;
  doc->m_file_stream = file_stream;
  //TODO 文件头可能有0导致strstr查找失败
  //if (strstr(pdf_header, "%PDF-")){
  if (IsPDF(pdf_header, sizeof(pdf_header))){
    doc->m_filetype = E_FILETYPE_PDF;
    doc->f.pdf = FPDF_LoadCustomDocument(&file_access, password);
  } else {
    doc->m_filetype = E_FILETYPE_OFD;
    doc->f.ofd = new ofd::COFD_Package(file_path);
    if (doc->f.ofd) {
      doc->f.ofd->ParseOFD();
    }
  }
  if (doc->f.pdf || doc->f.ofd)
    return reinterpret_cast<XILOU_PACKAGE>(doc.release());
  return nullptr;
}

XILOU_EXPORT XILOU_DOCUMENT XILOU_CALLCONV
xilou_LoadDocument(XILOU_PACKAGE package, int index) {
  CXilou_Document* doc = reinterpret_cast<CXilou_Document*>(package);
  if (!doc)
    return nullptr;
  std::unique_ptr<CXilou_ChildDocument> childDoc =
      std::make_unique<CXilou_ChildDocument>();
  if (!childDoc)
    return nullptr;
  if (doc->m_filetype == E_FILETYPE_PDF) {
    childDoc->pdf = doc->f.pdf;
  } else {
    if (doc->f.ofd) {
      ofd::COFD_Document* ofd_doc = doc->f.ofd->OpenDocument(index);
      if (ofd_doc)
        childDoc->ofd = ofd_doc;
    }
  }

  if (childDoc->ofd || childDoc->pdf)
    return reinterpret_cast<XILOU_DOCUMENT>(childDoc.release());
  return nullptr;
}

XILOU_EXPORT void XILOU_CALLCONV xilou_ClosePackage(XILOU_PACKAGE package) {
  std::unique_ptr<CXilou_Document>(reinterpret_cast<CXilou_Document*>(package));
}
XILOU_EXPORT void XILOU_CALLCONV xilou_DropDocument(XILOU_DOCUMENT document) {
  std::unique_ptr<CXilou_ChildDocument>(
      reinterpret_cast<CXilou_ChildDocument*>(document));
}


XILOU_EXPORT int XILOU_CALLCONV xilou_GetPageCount(XILOU_DOCUMENT document) {
  CXilou_ChildDocument* doc = reinterpret_cast<CXilou_ChildDocument*>(document);
  if (!doc)
    return 0;
  if (doc->pdf) {
    return FPDF_GetPageCount(doc->pdf);
  }
  if (doc->ofd) {
    return doc->ofd->PageCount();
  }
  return 0;
}
XILOU_EXPORT XILOU_PAGE XILOU_CALLCONV xilou_LoadPage(XILOU_DOCUMENT document,
  int page_index) {
  CXilou_ChildDocument* doc = reinterpret_cast<CXilou_ChildDocument*>(document);
  if (!doc)
    return nullptr;
  std::unique_ptr<CXilou_Page> page = std::make_unique<CXilou_Page>();
  if (!page)
    return nullptr;
  if (doc->pdf) {
    page->pdf_page = FPDF_LoadPage(doc->pdf, page_index);
  }
  if (doc->ofd) {
    page->ofd_page = doc->ofd->LoadPage(page_index);
    if (page->ofd_page)
      page->ofd_page->Parse();
  }
  if (page->pdf_page || page->ofd_page) {
    return reinterpret_cast<XILOU_PAGE>(page.release());
  }
  return nullptr;
}
XILOU_EXPORT void XILOU_CALLCONV xilou_DropPage(XILOU_PAGE page) {
  std::unique_ptr<CXilou_Page>(reinterpret_cast<CXilou_Page*>(page));
}

XILOU_EXPORT double XILOU_CALLCONV xilou_GetPageWidth(XILOU_PAGE page) {
  CXilou_Page* dual_page = reinterpret_cast<CXilou_Page*>(page);
  if (!dual_page)
    return 0;
  if (dual_page->pdf_page) {
    return FPDF_GetPageWidth(dual_page->pdf_page);
  }
  if (dual_page->ofd_page) {
    CFX_RectF page_area = dual_page->ofd_page->GetPageArea();
    return page_area.Width();
  }
  return 0;
}

XILOU_EXPORT double XILOU_CALLCONV xilou_GetPageHeight(XILOU_PAGE page) {
  CXilou_Page* dual_page = reinterpret_cast<CXilou_Page*>(page);
  if (!dual_page)
    return 0;
  if (dual_page->pdf_page) {
    return FPDF_GetPageHeight(dual_page->pdf_page);
  }
  if (dual_page->ofd_page) {
    CFX_RectF page_area = dual_page->ofd_page->GetPageArea();
    return page_area.Height();
  }
  return 0;
}


XILOU_EXPORT void XILOU_CALLCONV xilou_RenderPageBitmap(XILOU_BITMAP bitmap,
  XILOU_PAGE page,
  int start_x,
  int start_y,
  int size_x,
  int size_y,
  int rotate,
  int flags) {
  if (!bitmap)
    return;
  CXilou_Page* dual_page = reinterpret_cast<CXilou_Page*>(page);
  if (!dual_page)
    return ;
  if (dual_page->pdf_page) {
    return FPDF_RenderPageBitmap(bitmap, dual_page->pdf_page, start_x, start_y,
                                 size_x, size_y, rotate, flags);
  }
  if (dual_page->ofd_page) {
    return OFD_RenderPageBitmap(bitmap, dual_page->ofd_page, start_x, start_y,
                                size_x, size_y, rotate, flags);
  }
  return;
}
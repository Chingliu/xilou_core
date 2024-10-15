#include "xilou_public/xilou_convert.h"
#include "xilou_impl/xilou_impl.h"
#include <memory>
#include <string>
#include <vector>
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "public/fpdf_text.h"
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/fx_string.h"
#include "core/fpdftext/cpdf_textpage.h"
using namespace xilou;

namespace {

class CWriteTxt {
 public:
  RetainPtr<IFX_SeekableStream> file_stream;

 public:
  CWriteTxt(const char* file_path) {
    WideString utf8_path = FX_UTF8Decode(file_path);
#if !defined(OS_WIN)
    file_stream = IFX_SeekableStream::CreateFromFilename(utf8_path.c_str(),
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
  ~CWriteTxt() {}
};
}  // namespace

namespace xilou {

bool pdf2txt(FPDF_DOCUMENT src_doc,
            XILOU_UTF8STRING dest_fpath,
            XILOU_UTF8STRING page_range) {
  std::unique_ptr<CWriteTxt> pWritor = std::make_unique<CWriteTxt>(dest_fpath);
  auto pages = FPDF_GetPageCount(src_doc);
  for (int i = 0; i < pages; i++) {
    WideString text;
    auto fpage = FPDF_LoadPage(src_doc, i);
    if (fpage) {
      auto txt_page = FPDFText_LoadPage(fpage);
      CPDF_TextPage* textpage = CPDFTextPageFromFPDFTextPage(txt_page);
      int char_count = textpage->CountChars();
      text = textpage->GetPageText(0, char_count);
      FPDFText_ClosePage(txt_page);
    }
    FPDF_ClosePage(fpage);
    auto utf_txt = text.ToUTF8();
    auto txt_span = utf_txt.span();
    pWritor->file_stream->WriteBlock(txt_span.data(), txt_span.size_bytes());
    pWritor->file_stream->Flush();
  }
  return true;
}
bool xtotxt(XILOU_DOCUMENT src_doc,
             XILOU_UTF8STRING dest_fpath,
             XILOU_UTF8STRING page_range) {
  CXilou_ChildDocument* doc = reinterpret_cast<CXilou_ChildDocument*>(src_doc);
  if (!doc)
    return false;
  if (doc->pdf) {
    return pdf2txt(doc->pdf, dest_fpath, page_range);
  } else if (doc->ofd) {
  } else {
    return false;
  }
  return true;
}

}  // namespace xilou
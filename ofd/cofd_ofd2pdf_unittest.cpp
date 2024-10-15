#include <string>

#include "ofd/cofd_zipentry.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ofd/cofd_ofdxml.h"
#include "ofd/cofd_document.h"
#include "ofd/cofd_package.h"
#include "ofd/cofd_page.h"
#include "public/cpp/fpdf_scopers.h"
#include "public/fpdfview.h"
#include "ofd/render/cofd_pagerendercontext.h"
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "ofd/render/cofd_renderpage.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"
#include "samples/pdfium_test_write_helper.h"

#include "core/fpdfapi/page/cpdf_pageobject.h"

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_annot.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"
#include "testing/embedder_test.h"
#include "testing/fake_file_access.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/utils/file_util.h"

using namespace ofd;

class COFD_ConvertTest : public EmbedderTest {};

TEST_F(COFD_ConvertTest, ofd2pdf) {
  getchar();
  FPDF_LIBRARY_CONFIG config;
  config.version = 3;
  config.m_pUserFontPaths = nullptr;
  config.m_pIsolate = nullptr;
  config.m_v8EmbedderSlot = 0;
  config.m_pPlatform = nullptr;

  //CPDF_PageModule::Create();

  char fpath[1024];
  snprintf(fpath, sizeof fpath, "%s", __FILE__);
  char* p = fpath + strlen(fpath);
  while (*p != '\\' && *p != '/') {
    p--;
  }
  p++;
  *p = 0;
  std::string pdf_path(fpath);
  strcat(
      fpath,
      "/../testing/ofd_resources/ofd2pdf.ofd");  //依赖于编译环境，在哪编译的要在哪运行才能保证路径正确
  COFD_Package package(fpath);
  EXPECT_EQ(true, package.ParseOFD());

  COFD_Document* doc2 = package.OpenDocument(0);
  EXPECT_NE(nullptr, doc2);

  do {
    std::unique_ptr<COFD_Page> page(doc2->LoadPage(0));
    EXPECT_NE(nullptr, page);
    page->Parse();
    CFX_RectF page_area = page->GetPageArea();
    double scale = 1.33;
    auto width = static_cast<int>(page_area.Width() * scale);
    auto height = static_cast<int>(page_area.Height() * scale);
    auto newPdf = FPDF_CreateNewDocument();
    auto newPage = FPDFPage_New(newPdf, 0, width, height);

    auto page_objs = page->GetPageObjectCount();
    for (size_t i = 0; i < page_objs; i++) {
      FPDFPage_InsertObject(
          newPage,
          FPDFPageObjectFromCPDFPageObject(page->GetPageObjectByIndex(i)));
    }
    auto genResult = FPDFPage_GenerateContent(newPage);
    EXPECT_TRUE(genResult);
    pdf_path.append("/../out/debug/ofd2pdf.pdf");
#ifndef NDEBUG
    OpenPDFFileForWrite(pdf_path);

    FPDF_SaveAsCopy(newPdf, this, 0);
    ClosePDFFileForWrite();
#endif
  } while (0);  //确保page在关闭文档前被释放
  package.CloseDocument(doc2);

  //CPDF_PageModule::Destroy();
}
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
#include "samples/pdfium_test_write_helper.cc"
#include "core/fpdfapi/page/cpdf_pageobject.h"
using namespace ofd;
TEST(OfdPackage, parse) {
  getchar();
  char fpath[1024];
  snprintf(fpath, sizeof fpath, "%s", __FILE__);
  char *p = fpath + strlen(fpath);
  while (*p != '\\' && *p != '/') {
    p--;
  }
  p++;
  *p = 0;
  strcat(fpath,
         "/../testing/ofd_resources/ofdxml_case.ofd");  //依赖于编译环境，在哪编译的要在哪运行才能保证路径正确
  COFD_Package package(fpath);
  EXPECT_EQ(true, package.ParseOFD());
  COFD_Document* doc0 = package.OpenDocument();
  EXPECT_NE(nullptr, doc0);
  EXPECT_EQ(1, doc0->PageCount());
  package.CloseDocument(doc0);
  COFD_Document* doc1 = package.OpenDocument(1);
  EXPECT_NE(nullptr, doc1);
  EXPECT_EQ(1, doc1->PageCount());
  package.CloseDocument(doc1);
  COFD_Document* doc2 = package.OpenDocument(2);
  EXPECT_NE(nullptr, doc2);
  EXPECT_EQ(1, doc2->PageCount());
  WideString page0_url = doc2->PageUrl(0);
  EXPECT_EQ(L"/Doc_2/Page_0/Content.xml", page0_url);
  auto xml = package.ReadAndCacheXml(page0_url.ToUTF8().AsStringView());
  EXPECT_NE(nullptr, xml);
  COFD_Page* page = doc2->LoadPage(0);
  EXPECT_NE(nullptr, page);
  CFX_RectF page_area = page->GetPageArea();
  EXPECT_FLOAT_EQ(0, page_area.Left());
  EXPECT_FLOAT_EQ(0, page_area.Top());
  EXPECT_FLOAT_EQ(595.275574f, page_area.Width());
  EXPECT_FLOAT_EQ(841.889771f, page_area.Height());
  //由谁来管理page
  delete page;
  package.CloseDocument(doc2);
}

TEST(OfdPackage, render) {
  getchar();
  FPDF_LIBRARY_CONFIG config;
  config.version = 3;
  config.m_pUserFontPaths = nullptr;
  config.m_pIsolate = nullptr;
  config.m_v8EmbedderSlot = 0;
  config.m_pPlatform = nullptr;

  CPDF_PageModule::Create();

  char fpath[1024];
  snprintf(fpath, sizeof fpath, "%s", __FILE__);
  char* p = fpath + strlen(fpath);
  while (*p != '\\' && *p != '/') {
    p--;
  }
  p++;
  *p = 0;
  std::string png_path(fpath);
  strcat(
      fpath,
      "/../testing/ofd_resources/ofdxml_case.ofd");  //依赖于编译环境，在哪编译的要在哪运行才能保证路径正确
  COFD_Package package(fpath);
  EXPECT_EQ(true, package.ParseOFD());
 
  COFD_Document* doc2 = package.OpenDocument(2);
  EXPECT_NE(nullptr, doc2);
  EXPECT_EQ(1, doc2->PageCount());
  do{
    std::unique_ptr<COFD_Page> page(doc2->LoadPage(0));
    EXPECT_NE(nullptr, page);
    page->Parse();
    page->test_obj();
    CFX_RectF page_area = page->GetPageArea();
    double scale = 1.0;
    auto width = static_cast<int>(page_area.Width() * scale);
    auto height = static_cast<int>(page_area.Height() * scale);
    int alpha = 0;
    ScopedFPDFBitmap bitmap(FPDFBitmap_Create(width, height, alpha));
    FPDF_DWORD fill_color = alpha ? 0x00000000 : 0xFFFFFFFF;
    FPDFBitmap_FillRect(bitmap.get(), 0, 0, width, height, fill_color);
    if (!bitmap)
      break;

    auto pOwnedContext = std::make_unique<COFD_PageRenderContext>();
    COFD_PageRenderContext* pContext = pOwnedContext.get();
    //pPage->SetRenderContext(std::move(pOwnedContext));

    auto pOwnedDevice = std::make_unique<CFX_DefaultRenderDevice>();
    CFX_DefaultRenderDevice* pDevice = pOwnedDevice.get();
    pContext->m_pDevice = std::move(pOwnedDevice);
    int flags = 0;
    RetainPtr<CFX_DIBitmap> pBitmap(CFXDIBitmapFromFPDFBitmap(bitmap.get()));
    pDevice->Attach(pBitmap, !!(flags & FPDF_REVERSE_BYTE_ORDER), nullptr,
                    false);
    COFD_RenderPageWithContext(pContext, page.get(), CFX_Matrix(), 0, 0, width, height,
                               0, flags,
                                  /*color_scheme=*/nullptr,
                                  /*need_to_restore=*/true,
                                  /*pause=*/nullptr);

    int stride = FPDFBitmap_GetStride(bitmap.get());
    void* buffer = FPDFBitmap_GetBuffer(bitmap.get());
    png_path.append("/../out/debug/test_page_");
    WritePng(png_path.c_str(), 0, buffer, stride, width, height);
  } while (0);  //确保page在关闭文档前被释放
  package.CloseDocument(doc2);

  CPDF_PageModule::Destroy();
}

TEST(OfdPackage, parse_path) {
  getchar();
  FPDF_LIBRARY_CONFIG config;
  config.version = 3;
  config.m_pUserFontPaths = nullptr;
  config.m_pIsolate = nullptr;
  config.m_v8EmbedderSlot = 0;
  config.m_pPlatform = nullptr;

  CPDF_PageModule::Create();

  char fpath[1024];
  snprintf(fpath, sizeof fpath, "%s", __FILE__);
  char* p = fpath + strlen(fpath);
  while (*p != '\\' && *p != '/') {
    p--;
  }
  p++;
  *p = 0;
  std::string png_path(fpath);
  strcat(
      fpath,
      "/../testing/ofd_resources/path.ofd");  //依赖于编译环境，在哪编译的要在哪运行才能保证路径正确
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
    EXPECT_GE(static_cast<int>(page->GetPageObjectCount()), 1);

    int alpha = 0;
    ScopedFPDFBitmap bitmap(FPDFBitmap_Create(width, height, alpha));
    FPDF_DWORD fill_color = alpha ? 0x00000000 : 0xFFFFFFFF;
    FPDFBitmap_FillRect(bitmap.get(), 0, 0, width, height, fill_color);
    if (!bitmap)
      break;

    auto pOwnedContext = std::make_unique<COFD_PageRenderContext>();
    COFD_PageRenderContext* pContext = pOwnedContext.get();
    // pPage->SetRenderContext(std::move(pOwnedContext));

    auto pOwnedDevice = std::make_unique<CFX_DefaultRenderDevice>();
    CFX_DefaultRenderDevice* pDevice = pOwnedDevice.get();
    pContext->m_pDevice = std::move(pOwnedDevice);
    int flags = 0;
    RetainPtr<CFX_DIBitmap> pBitmap(CFXDIBitmapFromFPDFBitmap(bitmap.get()));
    pDevice->Attach(pBitmap, !!(flags & FPDF_REVERSE_BYTE_ORDER), nullptr,
                    false);
    CFX_Matrix screen_ctm;
    screen_ctm.Scale(scale, scale);
    COFD_RenderPageWithContext(pContext, page.get(), screen_ctm, 0, 0, width,
                               height, 0, flags,
                               /*color_scheme=*/nullptr,
                               /*need_to_restore=*/true,
                               /*pause=*/nullptr);

    int stride = FPDFBitmap_GetStride(bitmap.get());
    void* buffer = FPDFBitmap_GetBuffer(bitmap.get());
    png_path.append("/../out/debug/test_parse_path");
    WritePng(png_path.c_str(), 0, buffer, stride, width, height);
  } while (0);  //确保page在关闭文档前被释放
  package.CloseDocument(doc2);

  CPDF_PageModule::Destroy();
}
#include "ofd/cofd_zipentry.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ofd/cofd_ofdxml.h"
#include "ofd/cofd_document.h"

using namespace ofd;
TEST(OfdDocument, parse) {
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
  ofd::COFD_ZipEntry zip(fpath);
  EXPECT_EQ(true, zip.Open('r'));
  auto xml = zip.ReadAndCacheXml("ofd.xml");
  EXPECT_NE(nullptr, xml);
  COFD_ParseOfdXml ofd_xml(xml);
  EXPECT_EQ(3, static_cast<int>( ofd_xml.DocNum()));
  COFD_DocInfo* doc_info = ofd_xml.GetDocInfo(0);
  EXPECT_EQ(L"Doc_0/Document.xml", doc_info->doc_root);
  xml = zip.ReadAndCacheXml(doc_info->doc_root.ToUTF8().AsStringView());
  EXPECT_NE(nullptr, xml);
  COFD_Document doc(nullptr, doc_info, xml, doc_info->doc_root.AsStringView());
  EXPECT_EQ(1, doc.PageCount());
  EXPECT_EQ(12100, doc.GetDocMaxId());
  CFX_RectF doc_area = doc.GetDocumentArea();
  EXPECT_FLOAT_EQ(0, doc_area.Left());
  EXPECT_FLOAT_EQ(0, doc_area.Top());
  EXPECT_FLOAT_EQ(595.275574f, doc_area.Width());
  EXPECT_FLOAT_EQ(841.889771f, doc_area.Height());

}

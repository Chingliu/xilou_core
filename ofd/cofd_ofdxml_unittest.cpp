#include "ofd/cofd_zipentry.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ofd/cofd_ofdxml.h"
using namespace ofd;
TEST(OfdXml, parse) {
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
  EXPECT_EQ(L"0aabd2be93f0424ba17e5f921a9d5bc3", doc_info->doc_id);

  doc_info = ofd_xml.GetDocInfo(2);
  EXPECT_EQ(L"Doc_2/Document.xml", doc_info->doc_root);
  EXPECT_EQ(L"0aabd2be93f0424ba17e5f921a9d5bc5", doc_info->doc_id);
}

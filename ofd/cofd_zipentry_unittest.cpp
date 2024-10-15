#include "ofd/cofd_zipentry.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(ZipEntry, open) {
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
         "/../testing/ofd_resources/open_case.ofd");  //依赖于编译环境，在哪编译的要在哪运行才能保证路径正确
  ofd::COFD_ZipEntry zip(fpath);
  EXPECT_EQ(true, zip.Open('r'));
  auto xml = zip.ReadAndCacheXml("ofd.xml");
  EXPECT_NE(nullptr, xml);
  auto cached_xml = zip.ReadAndCacheXml("ofd.xml");
  EXPECT_NE(nullptr, cached_xml);
  EXPECT_EQ(xml, cached_xml);
}

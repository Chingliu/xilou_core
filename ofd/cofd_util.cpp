#include "ofd/cofd_util.h"
#include "public/fpdfview.h"
#include "third_party/abseil-cpp/absl/strings/str_split.h"
#include<vector>
namespace ofd_util {

#ifdef _WIN32

std::string DbgWriteBitmapAsBmp(const char* name,
                                int num,
                                const RetainPtr<CFX_DIBitmap>& pBitmap) {
  void* buffer;
  int stride;
  int width;
  int height;
  buffer = pBitmap->GetBuffer();
  stride = pBitmap->GetPitch();
  width = pBitmap->GetWidth();
  height = pBitmap->GetHeight();
  int out_len = stride * height;
  if (out_len > INT_MAX / 3)
    return "";
  char fpath[1024];
  snprintf(fpath, sizeof fpath, "%s", __FILE__);
  char* p = fpath + strlen(fpath);
  while (*p != '\\' && *p != '/') {
    p--;
  }
  p++;
  *p = 0;

  char filename[256];
  snprintf(filename, sizeof(filename), "%s/../out/%s.%d.bmp", fpath, name, num);
  FILE* fp = fopen(filename, "wb");
  if (!fp)
    return "";

  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(bmi) - sizeof(RGBQUAD);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;  // top-down image
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = pBitmap->GetBPP();
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;

  BITMAPFILEHEADER file_header = {};
  file_header.bfType = 0x4d42;
  file_header.bfSize = sizeof(file_header) + bmi.bmiHeader.biSize + out_len;
  file_header.bfOffBits = file_header.bfSize - out_len;

  if (fwrite(&file_header, sizeof(file_header), 1, fp) != 1 ||
      fwrite(&bmi, bmi.bmiHeader.biSize, 1, fp) != 1 ||
      fwrite(buffer, out_len, 1, fp) != 1) {
    fprintf(stderr, "Failed to write to %s\n", filename);
  }
  fclose(fp);
  return std::string(filename);
}

void ConvertBitmapAsBmp(const RetainPtr<CFX_DIBitmap>& pBitmap,
  std::vector<uint8_t>& bmp) {
  void* buffer;
  int stride;
  int width;
  int height;
  buffer = pBitmap->GetBuffer();
  stride = pBitmap->GetPitch();
  width = pBitmap->GetWidth();
  height = pBitmap->GetHeight();
  int out_len = stride * height;
  if (out_len > INT_MAX / 3)
    return ;

  BITMAPINFO bmi = {};
  bmi.bmiHeader.biSize = sizeof(bmi) - sizeof(RGBQUAD);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;  // top-down image
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = pBitmap->GetBPP();
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;

  BITMAPFILEHEADER file_header = {};
  file_header.bfType = 0x4d42;
  file_header.bfSize = sizeof(file_header) + bmi.bmiHeader.biSize + out_len;
  file_header.bfOffBits = file_header.bfSize - out_len;

  bmp.resize(sizeof(file_header) + bmi.bmiHeader.biSize + out_len);
  memcpy(&bmp[0], &file_header, sizeof(file_header));
  memcpy(&bmp[sizeof(file_header)], &bmi, bmi.bmiHeader.biSize);
  memcpy(&bmp[sizeof(file_header) + bmi.bmiHeader.biSize], buffer, out_len);
  return;
}
#else

std::string DbgWriteBitmapAsBmp(const char* name,
  int num,
  const RetainPtr<CFX_DIBitmap>& pBitmap) {
  return "no implement";
}


#endif

CFX_RectF ParseSTBOX(const WideString& STBOX_string) {
  CFX_RectF boundary(0,0,0,0);
  using absl::ByAnyChar;
  using absl::SkipWhitespace;
  std::vector<std::string> v = absl::StrSplit(
      STBOX_string.ToDefANSI().c_str(), ByAnyChar(" \t\n\r"), SkipWhitespace());
  if (v.size() == 4) {
    boundary.left = ofd_mm_to_one_72_point(StringToFloat(v[0].c_str()));
    boundary.top = ofd_mm_to_one_72_point(StringToFloat(v[1].c_str()));
    boundary.width = ofd_mm_to_one_72_point(StringToFloat(v[2].c_str()));
    boundary.height = ofd_mm_to_one_72_point(StringToFloat(v[3].c_str()));
  }
  return boundary;
}

CFX_PointF ParseSTPoint(const WideString& STPoint_string) {
  CFX_PointF point(0, 0);
  using absl::ByAnyChar;
  using absl::SkipWhitespace;
  std::vector<std::string> v = absl::StrSplit(
      STPoint_string.ToDefANSI().c_str(), ByAnyChar(" \t\n\r"), SkipWhitespace());
  if (v.size() == 2) {
    point.x = ofd_mm_to_one_72_point(StringToFloat(v[0].c_str()));
    point.y = ofd_mm_to_one_72_point(StringToFloat(v[1].c_str()));
  }
  return point;
}

void SaveBinaryFile(const char* fpath, const unsigned char* data, int datalen) {
  std::string fullpath("d:\\dumpimage\\");
  fullpath.append(fpath);
  FILE* pfile = fopen(fullpath.c_str(), "wb");
  if (pfile) {
    fwrite(data, sizeof(char), datalen, pfile);
    fflush(pfile);
    fclose(pfile);
  }
}

}  // namespace ofd_util
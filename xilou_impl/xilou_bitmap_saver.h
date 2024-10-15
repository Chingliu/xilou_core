// Copyright 2018 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XILOU_IMPL_XILOU_BITMAP_SAVER_H_
#define XILOU_IMPL_XILOU_BITMAP_SAVER_H_

#include <string>

#include "public/fpdfview.h"
class vector;
class CFX_DIBitmap;
namespace xilou {
class BitmapSaver {
 public:
  static void WriteBitmapToPng(FPDF_BITMAP bitmap, const std::string& filename);
  static void WriteBitmapToPng(CFX_DIBitmap* bitmap,
                               const std::string& filename);
  static std::vector<uint8_t> ConvertBitmapToPng(
      CFX_DIBitmap* dibbitmap);
};
}  // namespace xilou

#endif  // TESTING_UTILS_BITMAP_SAVER_H_

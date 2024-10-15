// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef XILOU_IMPL_XILOU_IMAGE_PNG_H_
#define XILOU_IMPL_XILOU_IMAGE_PNG_H_

#include <stdlib.h>  // for size_t.

#include <vector>

#include "third_party/base/span.h"

namespace xilou {

// Decode a PNG into an RGBA pixel array, or BGRA pixel array if
// |reverse_byte_order| is set to true.
std::vector<uint8_t> DecodePNG(pdfium::span<const uint8_t> input,
                               bool reverse_byte_order,
                               int* width,
                               int* height);

// Encode a BGR pixel array into a PNG.
std::vector<uint8_t> EncodeBGRPNG(pdfium::span<const uint8_t> input,
                                  int width,
                                  int height,
                                  int row_byte_width);

// Encode an RGBA pixel array into a PNG.
std::vector<uint8_t> EncodeRGBAPNG(pdfium::span<const uint8_t> input,
                                   int width,
                                   int height,
                                   int row_byte_width);

// Encode an BGRA pixel array into a PNG.
std::vector<uint8_t> EncodeBGRAPNG(pdfium::span<const uint8_t> input,
                                   int width,
                                   int height,
                                   int row_byte_width,
                                   bool discard_transparency);

// Encode a grayscale pixel array into a PNG.
std::vector<uint8_t> EncodeGrayPNG(pdfium::span<const uint8_t> input,
                                   int width,
                                   int height,
                                   int row_byte_width);

}  // namespace xilou

#endif  // XILOU_IMPL_XILOU_IMAGE_PNG_H_

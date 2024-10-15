// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "ofd/render/cofd_textrenderer.h"

#include <algorithm>
#include <vector>
#include "core/fpdfapi/render/charposlist.h"
#include "core/fpdfapi/font/cpdf_font.h"
#include "ofd/render/charposlist.h"
#include "core/fpdfapi/render/cpdf_renderoptions.h"
#include "core/fxge/cfx_graphstatedata.h"
#include "core/fxge/cfx_path.h"
#include "core/fxge/cfx_renderdevice.h"
#include "core/fxge/cfx_textrenderoptions.h"
#include "core/fxge/fx_font.h"
#include "core/fxge/text_char_pos.h"

namespace {

CFX_Font* GetFont(CPDF_Font* pFont, int32_t position) {
  return position == -1 ? pFont->GetFont() : pFont->GetFontFallback(position);
}

CFX_TextRenderOptions GetTextRenderOptionsHelper(
    const CPDF_Font* pFont,
    const CPDF_RenderOptions& options) {
  CFX_TextRenderOptions text_options;

  if (pFont->IsCIDFont())
    text_options.font_is_cid = true;

  if (options.GetOptions().bNoTextSmooth)
    text_options.aliasing_type = CFX_TextRenderOptions::kAliasing;
  else if (options.GetOptions().bClearType)
    text_options.aliasing_type = CFX_TextRenderOptions::kLcd;

  if (options.GetOptions().bNoNativeText)
    text_options.native_text = false;

  return text_options;
}

}  // namespace
namespace ofd {
// static
bool COFD_TextRenderer::DrawTextPath(
    CFX_RenderDevice* pDevice,
    pdfium::span<const uint32_t> char_codes,
    pdfium::span<const float> char_pos,
    CPDF_Font* pFont,
    float font_size,
    const CFX_Matrix& mtText2User,
    const CFX_Matrix* pUser2Device,
    const CFX_GraphStateData* pGraphState,
    FX_ARGB fill_argb,
    FX_ARGB stroke_argb,
    CFX_Path* pClippingPath,
    const CFX_FillRenderOptions& fill_options) {
  std::vector<TextCharPos> pos =
      ofd::OfdGetCharPosList(char_codes, char_pos, pFont, font_size);
  if (pos.empty())
    return true;
  for (size_t i = 0; i < pos.size(); ++i) {
    if (pos[i].m_bGlyphAdjust) {
      pos[i].m_AdjustMatrix[3] = -pos[i].m_AdjustMatrix[3];
    } else {
      pos[i].m_bGlyphAdjust = true;
      pos[i].m_AdjustMatrix[0] = 1;
      pos[i].m_AdjustMatrix[1] = 0;
      pos[i].m_AdjustMatrix[2] = 0;
      pos[i].m_AdjustMatrix[3] = -1;
    }
  }
  bool bDraw = true;
  int32_t fontPosition = pos[0].m_FallbackFontPosition;
  size_t startIndex = 0;
  for (size_t i = 0; i < pos.size(); ++i) {
    int32_t curFontPosition = pos[i].m_FallbackFontPosition;
    if (fontPosition == curFontPosition)
      continue;

    CFX_Font* font = GetFont(pFont, fontPosition);
    if (!pDevice->DrawTextPath(i - startIndex, &pos[startIndex], font,
                               font_size, mtText2User, pUser2Device,
                               pGraphState, fill_argb, stroke_argb,
                               pClippingPath, fill_options)) {
      bDraw = false;
    }
    fontPosition = curFontPosition;
    startIndex = i;
  }
  CFX_Font* font = GetFont(pFont, fontPosition);
  if (!pDevice->DrawTextPath(pos.size() - startIndex, &pos[startIndex], font,
                             font_size, mtText2User, pUser2Device, pGraphState,
                             fill_argb, stroke_argb, pClippingPath,
                             fill_options)) {
    bDraw = false;
  }
  return bDraw;
}

// static
bool COFD_TextRenderer::DrawTextPath(
    CFX_RenderDevice* pDevice,
    uint32_t glyph_index,
    uint32_t unicode,
    CPDF_Font* pFont,
    float font_size,
    const CFX_Matrix& mtText2User,
    const CFX_Matrix* pUser2Device,
    const CFX_GraphStateData* pGraphState,
    FX_ARGB fill_argb,
    FX_ARGB stroke_argb,
    CFX_Path* pClippingPath,
    const CFX_FillRenderOptions& fill_options) {
  TextCharPos onechar;
  onechar.m_GlyphIndex = glyph_index;
  onechar.m_Unicode = unicode;

  //  // core\fxge\cfx_renderdevice.cpp:1084没弄明白为什么做这个变换
  // char2device.Scale(font_size, -font_size);
  onechar.m_bGlyphAdjust = true;
  onechar.m_AdjustMatrix[0] = 1;
  onechar.m_AdjustMatrix[1] = 0;
  onechar.m_AdjustMatrix[2] = 0;
  onechar.m_AdjustMatrix[3] = -1;

  bool bDraw = true;

  CFX_Font* font = pFont->GetFont();
  if (!pDevice->DrawTextPath(1, &onechar, font,
                             font_size, mtText2User, pUser2Device, pGraphState,
                             fill_argb, stroke_argb, pClippingPath,
                             fill_options)) {
    bDraw = false;
  }
  return bDraw;
}
// static
bool COFD_TextRenderer::DrawNormalText(CFX_RenderDevice* pDevice,
                                       uint32_t glyph_index,
                                       uint32_t unicode,
                                       CPDF_Font* pFont,
                                       float font_size,
                                       const CFX_Matrix& mtText2Device,
                                       FX_ARGB fill_argb,
                                       const CPDF_RenderOptions& options) {
  TextCharPos onechar;
  onechar.m_GlyphIndex = glyph_index;
  onechar.m_Unicode = unicode;
  //  // core\fxge\cfx_renderdevice.cpp:1084没弄明白为什么做这个变换
  // char2device.Scale(font_size, -font_size);
  onechar.m_bGlyphAdjust = true;
  onechar.m_AdjustMatrix[0] = 1;
  onechar.m_AdjustMatrix[1] = 0;
  onechar.m_AdjustMatrix[2] = 0;
  onechar.m_AdjustMatrix[3] = -1;

  CFX_TextRenderOptions text_options =
      GetTextRenderOptionsHelper(pFont, options);
  bool bDraw = true;

  CFX_Font* font = pFont->GetFont();
  if (!pDevice->DrawNormalText(1, &onechar, font,
                               font_size, mtText2Device, fill_argb,
                               text_options)) {
    bDraw = false;
  }
  return bDraw;
}

// static
bool COFD_TextRenderer::DrawNormalText(CFX_RenderDevice* pDevice,
                                       pdfium::span<const uint32_t> char_codes,
                                       pdfium::span<const float> char_pos,
                                       CPDF_Font* pFont,
                                       float font_size,
                                       const CFX_Matrix& mtText2Device,
                                       FX_ARGB fill_argb,
                                       const CPDF_RenderOptions& options) {
  std::vector<TextCharPos> pos =
      GetCharPosList(char_codes, char_pos, pFont, font_size);
  if (pos.empty())
    return true;
  for (size_t i = 0; i < pos.size(); ++i) {
    if (pos[i].m_bGlyphAdjust) {
      pos[i].m_AdjustMatrix[3] = -pos[i].m_AdjustMatrix[3];
    } else {
      pos[i].m_bGlyphAdjust = true;
      pos[i].m_AdjustMatrix[0] = 1;
      pos[i].m_AdjustMatrix[1] = 0;
      pos[i].m_AdjustMatrix[2] = 0;
      pos[i].m_AdjustMatrix[3] = -1;
    }
    
  }
  CFX_TextRenderOptions text_options =
      GetTextRenderOptionsHelper(pFont, options);
  bool bDraw = true;
  int32_t fontPosition = pos[0].m_FallbackFontPosition;
  size_t startIndex = 0;
  for (size_t i = 0; i < pos.size(); ++i) {
    int32_t curFontPosition = pos[i].m_FallbackFontPosition;
    if (fontPosition == curFontPosition)
      continue;

    CFX_Font* font = GetFont(pFont, fontPosition);
    if (!pDevice->DrawNormalText(i - startIndex, &pos[startIndex], font,
                                 font_size, mtText2Device, fill_argb,
                                 text_options)) {
      bDraw = false;
    }
    fontPosition = curFontPosition;
    startIndex = i;
  }
  CFX_Font* font = GetFont(pFont, fontPosition);
  if (!pDevice->DrawNormalText(pos.size() - startIndex, &pos[startIndex], font,
                               font_size, mtText2Device, fill_argb,
                               text_options)) {
    bDraw = false;
  }
  return bDraw;
}

}  // namespace ofd
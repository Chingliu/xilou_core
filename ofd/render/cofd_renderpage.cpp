// Copyright 2020 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "ofd/render/cofd_renderpage.h"

#include <memory>
#include <utility>
#include "ofd/cofd_page.h"
#include "ofd/render/cofd_rendercontext.h"

#include "ofd/render/cofd_progressiverenderer.h"
#include "ofd/render/cofd_pagerendercontext.h"
#include "core/fpdfapi/render/cpdf_renderoptions.h"
#include "core/fxge/cfx_renderdevice.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "fpdfsdk/cpdfsdk_pauseadapter.h"
#include "ofd/cofd_page_annot.h"
#include "ofd/cofd_page.h"
using namespace ofd;
namespace {

void RenderPageImpl(COFD_PageRenderContext* pContext,
                    COFD_Page* pPage,
                    const CFX_Matrix& matrix,
                    const FX_RECT& clipping_rect,
                    int flags,
                    const FPDF_COLORSCHEME* color_scheme,
                    bool need_to_restore,
                    CPDFSDK_PauseAdapter* pause) {
  if (!pContext->m_pDevice->GetDeviceDriver())
    return;
  if (!pContext->m_pOptions)
    pContext->m_pOptions = std::make_unique<CPDF_RenderOptions>();

  auto& options = pContext->m_pOptions->GetOptions();
  options.bClearType = !!(flags & FPDF_LCD_TEXT);
  options.bNoNativeText = !!(flags & FPDF_NO_NATIVETEXT);
  options.bLimitedImageCache = !!(flags & FPDF_RENDER_LIMITEDIMAGECACHE);
  options.bForceHalftone = !!(flags & FPDF_RENDER_FORCEHALFTONE);
  options.bNoTextSmooth = !!(flags & FPDF_RENDER_NO_SMOOTHTEXT);
  options.bNoImageSmooth = !!(flags & FPDF_RENDER_NO_SMOOTHIMAGE);
  options.bNoPathSmooth = !!(flags & FPDF_RENDER_NO_SMOOTHPATH);

  // Grayscale output
  if (flags & FPDF_GRAYSCALE)
    pContext->m_pOptions->SetColorMode(CPDF_RenderOptions::kGray);

  if (color_scheme) {
    pContext->m_pOptions->SetColorMode(CPDF_RenderOptions::kForcedColor);
    SetColorFromScheme(color_scheme, pContext->m_pOptions.get());
    options.bConvertFillToStroke = !!(flags & FPDF_CONVERT_FILL_TO_STROKE);
  }

  pContext->m_pDevice->SaveState();
  pContext->m_pDevice->SetBaseClip(clipping_rect);
  pContext->m_pDevice->SetClip_Rect(clipping_rect);
  pContext->m_pContext = std::make_unique<COFD_RenderContext>();

  if (pPage->GetBackGroundTemplateLayer()) {
    pContext->m_pContext->AppendLayer(pPage->GetBackGroundTemplateLayer(),
                                      matrix);
  }
  if (pPage->GetBackGroundLayer()) {
    pContext->m_pContext->AppendLayer(pPage->GetBackGroundLayer(),
                                      matrix);
  }
  if (flags & FPDF_ANNOT) {
    auto annots = pPage->GetAnnotCount();
    for (size_t i = 0; i < annots; ++i) {
      auto annot_obj = static_cast<COFD_Page*>(pPage->GetPageAnnotObj(i));
      if (annot_obj) {
        pContext->m_pContext->AppendLayer((annot_obj), matrix);
      }
    }
  }

  if (pPage->GetBodyTemplateLayer()) {
    pContext->m_pContext->AppendLayer(pPage->GetBodyTemplateLayer(), matrix);
  }
  pContext->m_pContext->AppendLayer(pPage, matrix);
  if (pPage->GetForeGroundTemplateLayer()) {
    pContext->m_pContext->AppendLayer(pPage->GetForeGroundTemplateLayer(),
                                      matrix);
  }
  if (pPage->GetForeGroundLayer()) {
    pContext->m_pContext->AppendLayer(pPage->GetForeGroundLayer(),
                                      matrix);
  }

  // pPage->GetPageSignatureCount();
  auto page_sig = pPage->GetPageSignature();
  for (auto it : page_sig) {
    pContext->m_pContext->AppendLayer(it, matrix);
  }

  pContext->m_pRenderer = std::make_unique<COFD_ProgressiveRenderer>(
      pContext->m_pContext.get(), pContext->m_pDevice.get(),
      pContext->m_pOptions.get());
  pContext->m_pRenderer->Start(pause);

  if (need_to_restore)
    pContext->m_pDevice->RestoreState(false);
}

}  // namespace

void COFD_RenderPage(COFD_PageRenderContext* pContext,
                        COFD_Page* pPage,
                        const CFX_Matrix& matrix,
                        const FX_RECT& clipping_rect,
                        int flags,
                        const FPDF_COLORSCHEME* color_scheme) {
  RenderPageImpl(pContext, pPage, matrix, clipping_rect, flags, color_scheme,
                 /*need_to_restore=*/true, /*pause=*/nullptr);
}

void COFD_RenderPageWithContext(COFD_PageRenderContext* pContext,
                                   COFD_Page* pPage,
                                   const CFX_Matrix& matrix,
                                   int start_x,
                                   int start_y,
                                   int size_x,
                                   int size_y,
                                   int rotate,
                                   int flags,
                                   const FPDF_COLORSCHEME* color_scheme,
                                   bool need_to_restore,
                                   CPDFSDK_PauseAdapter* pause) {
  const FX_RECT rect(start_x, start_y, start_x + size_x, start_y + size_y);
  RenderPageImpl(pContext, pPage, matrix, rect,
                 flags, color_scheme, need_to_restore, pause);
}

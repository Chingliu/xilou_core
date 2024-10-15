// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "ofd/render/cofd_scaledrenderbuffer.h"

#include "core/fpdfapi/render/cpdf_devicebuffer.h"
#include "core/fpdfapi/render/cpdf_rendercontext.h"
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "ofd/render/cofd_rendercontext.h"
using namespace ofd;
namespace {

constexpr size_t kImageSizeLimitBytes = 30 * 1024 * 1024;

}  // namespace

COFD_ScaledRenderBuffer::COFD_ScaledRenderBuffer() = default;

COFD_ScaledRenderBuffer::~COFD_ScaledRenderBuffer() = default;

bool COFD_ScaledRenderBuffer::Initialize(COFD_RenderContext* pContext,
                                         CFX_RenderDevice* pDevice,
                                         const FX_RECT& rect,
                                         const CPDF_PageObject* pObj,
                                         const CPDF_RenderOptions* pOptions,
                                         int max_dpi) {
  m_pDevice = pDevice;
  if (m_pDevice->GetDeviceCaps(FXDC_RENDER_CAPS) & FXRC_GET_BITS)
    return true;

  m_Rect = rect;
  m_Matrix = CPDF_DeviceBuffer::CalculateMatrix(pDevice, rect, max_dpi,
                                                /*scale=*/true);
  m_pBitmapDevice = std::make_unique<CFX_DefaultRenderDevice>();
  bool bIsAlpha =
      !!(m_pDevice->GetDeviceCaps(FXDC_RENDER_CAPS) & FXRC_ALPHA_OUTPUT);
  FXDIB_Format dibFormat = bIsAlpha ? FXDIB_Format::kArgb : FXDIB_Format::kRgb;
  while (1) {
    FX_RECT bitmap_rect =
        m_Matrix.TransformRect(CFX_FloatRect(rect)).GetOuterRect();
    int32_t width = bitmap_rect.Width();
    int32_t height = bitmap_rect.Height();
    // Set to 0 to make CalculatePitchAndSize() calculate it.
    constexpr uint32_t kNoPitch = 0;
    absl::optional<CFX_DIBitmap::PitchAndSize> pitch_size =
        CFX_DIBitmap::CalculatePitchAndSize(width, height, dibFormat, kNoPitch);
    if (!pitch_size.has_value())
      return false;

    if (pitch_size.value().size <= kImageSizeLimitBytes &&
        m_pBitmapDevice->Create(width, height, dibFormat, nullptr)) {
      break;
    }
    m_Matrix.Scale(0.5f, 0.5f);
  }
  pContext->GetBackground(m_pBitmapDevice->GetBitmap(), pObj, pOptions,
                          m_Matrix);
  return true;
}

CFX_RenderDevice* COFD_ScaledRenderBuffer::GetDevice() const {
  return m_pBitmapDevice ? m_pBitmapDevice.get() : m_pDevice.Get();
}

void COFD_ScaledRenderBuffer::OutputToDevice() {
  if (m_pBitmapDevice) {
    m_pDevice->StretchDIBits(m_pBitmapDevice->GetBitmap(), m_Rect.left,
                             m_Rect.top, m_Rect.Width(), m_Rect.Height());
  }
}

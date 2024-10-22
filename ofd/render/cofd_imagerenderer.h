﻿// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_RENDER_COFD_ImageRenderer_H_
#define CORE_FPDFAPI_RENDER_COFD_ImageRenderer_H_

#include <memory>

#include "ofd/render/cofd_imageloader.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/dib/cfx_imagerenderer.h"
#include "core/fxge/dib/fx_dib.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class CFX_DIBBase;
class CFX_DefaultRenderDevice;
class CFX_ImageTransformer;
class CPDF_ImageObject;
class CPDF_Pattern;
class CPDF_RenderOptions;

namespace ofd {
class COFD_RenderStatus;

class COFD_ImageRenderer {
 public:
  COFD_ImageRenderer();
  ~COFD_ImageRenderer();

  bool Start(COFD_RenderStatus* pStatus,
             CPDF_ImageObject* pImageObject,
             const CFX_Matrix& mtObj2Device,
             bool bStdCS,
             BlendMode blendType);

  bool Start(COFD_RenderStatus* pStatus,
             const RetainPtr<CFX_DIBBase>& pDIBBase,
             FX_ARGB bitmap_argb,
             const CFX_Matrix& mtImage2Device,
             const FXDIB_ResampleOptions& options,
             bool bStdCS);

  bool Continue(PauseIndicatorIface* pPause);
  bool GetResult() const { return m_Result; }

 public:
  static void WriteBmp(const char* pdf_name, int num, CFX_DIBBase* pDIB); 
 private:
  enum class Mode {
    kNone = 0,
    kDefault,
    kBlend,
    kTransform,
  };

  bool StartBitmapAlpha();
  bool StartDIBBase();
  bool StartRenderDIBBase();
  bool StartLoadDIBBase();
  bool ContinueDefault(PauseIndicatorIface* pPause);
  bool ContinueBlend(PauseIndicatorIface* pPause);
  bool ContinueTransform(PauseIndicatorIface* pPause);
  bool DrawMaskedImage();
  bool DrawPatternImage();
  bool NotDrawing() const;
  FX_RECT GetDrawRect() const;
  CFX_Matrix GetDrawMatrix(const FX_RECT& rect) const;
  void CalculateDrawImage(CFX_DefaultRenderDevice* pBitmapDevice1,
                          CFX_DefaultRenderDevice* pBitmapDevice2,
                          const RetainPtr<CFX_DIBBase>& pDIBBase,
                          const CFX_Matrix& mtNewMatrix,
                          const FX_RECT& rect) const;
  const CPDF_RenderOptions& GetRenderOptions() const;
  void HandleFilters();
  absl::optional<FX_RECT> GetUnitRect() const;
  bool GetDimensionsFromUnitRect(const FX_RECT& rect,
                                 int* left,
                                 int* top,
                                 int* width,
                                 int* height) const;

  UnownedPtr<COFD_RenderStatus> m_pRenderStatus;
  UnownedPtr<CPDF_ImageObject> m_pImageObject;
  RetainPtr<CPDF_Pattern> m_pPattern;
  RetainPtr<CFX_DIBBase> m_pDIBBase;
  CFX_Matrix m_mtObj2Device;
  CFX_Matrix m_ImageMatrix;
  COFD_ImageLoader m_Loader;
  std::unique_ptr<CFX_ImageTransformer> m_pTransformer;
  std::unique_ptr<CFX_ImageRenderer> m_DeviceHandle;
  Mode m_Mode = Mode::kNone;
  int m_BitmapAlpha = 0;
  BlendMode m_BlendType = BlendMode::kNormal;
  FX_ARGB m_FillArgb = 0;
  FXDIB_ResampleOptions m_ResampleOptions;
  bool m_bPatternColor = false;
  bool m_bStdCS = false;
  bool m_Result = true;
};

}  // namespace ofd
#endif  // CORE_FPDFAPI_RENDER_COFD_ImageRenderer_H_

// Copyright 2019 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_RENDER_COFD_RENDERSHADING_H_
#define CORE_FPDFAPI_RENDER_COFD_RENDERSHADING_H_

class CFX_Matrix;
class CFX_RenderDevice;
class CPDF_PageObject;
class CPDF_RenderOptions;
class CPDF_ShadingPattern;
struct FX_RECT;

namespace ofd {
class COFD_RenderContext;
class COFD_RenderShading {
 public:
  static void Draw(CFX_RenderDevice* pDevice,
                   COFD_RenderContext* pContext,
                   const CPDF_PageObject* pCurObj,
                   const CPDF_ShadingPattern* pPattern,
                   const CFX_Matrix& mtMatrix,
                   const FX_RECT& clip_rect,
                   int alpha,
                   const CPDF_RenderOptions& options);

  COFD_RenderShading() = delete;
  COFD_RenderShading(const COFD_RenderShading&) = delete;
  COFD_RenderShading& operator=(const COFD_RenderShading&) = delete;
};
}

#endif  // CORE_FPDFAPI_RENDER_CPDF_RENDERSHADING_H_

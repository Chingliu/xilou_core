// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef COFD_RENDER_PAGERENDERCONTEXT_H_
#define COFD_RENDER_PAGERENDERCONTEXT_H_

#include <memory>

#include "core/fpdfapi/page/cpdf_page.h"

class CFX_RenderDevice;

class CPDF_RenderOptions;
namespace ofd {

class COFD_ProgressiveRenderer;
class COFD_RenderContext;


// Everything about rendering is put here: for OOM recovery
class COFD_PageRenderContext final : public CPDF_Page::RenderContextIface {
 public:
  COFD_PageRenderContext();
  ~COFD_PageRenderContext() override;

  // Specific destruction order required.
  std::unique_ptr<CPDF_RenderOptions> m_pOptions;
  std::unique_ptr<CFX_RenderDevice> m_pDevice;
  std::unique_ptr<COFD_RenderContext> m_pContext;
  std::unique_ptr<COFD_ProgressiveRenderer> m_pRenderer;
};
}  // namespace ofd
#endif  // CORE_FPDFAPI_RENDER_CPDF_PAGERENDERCONTEXT_H_

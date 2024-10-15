// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef COFD_RENDER_RENDERCONTEXT_H_
#define COFD_RENDER_RENDERCONTEXT_H_

#include <vector>

#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"

class CPDF_Dictionary;
class CPDF_Document;
class CPDF_PageObject;

class CPDF_PageRenderCache;
class CPDF_RenderOptions;
class CFX_DIBitmap;
class CFX_Matrix;
class CFX_RenderDevice;

namespace ofd {
class COFD_PageObjectHolder;


class COFD_RenderContext {
 public:
  class Layer {
   public:
    Layer(COFD_PageObjectHolder* pHolder, const CFX_Matrix& matrix);
    Layer(const Layer& that);
    ~Layer();

    COFD_PageObjectHolder* GetObjectHolder() { return m_pObjectHolder.Get(); }
    const COFD_PageObjectHolder* GetObjectHolder() const {
      return m_pObjectHolder.Get();
    }
    const CFX_Matrix& GetMatrix() const { return m_Matrix; }

   private:
    UnownedPtr<COFD_PageObjectHolder> const m_pObjectHolder;
    const CFX_Matrix m_Matrix;
  };

  COFD_RenderContext();
  ~COFD_RenderContext();

  void AppendLayer(COFD_PageObjectHolder* pObjectHolder,
                   const CFX_Matrix& mtObject2Device);

  void Render(CFX_RenderDevice* pDevice,
              const CPDF_RenderOptions* pOptions,
              const CFX_Matrix* pLastMatrix);

  void Render(CFX_RenderDevice* pDevice,
              const CPDF_PageObject* pStopObj,
              const CPDF_RenderOptions* pOptions,
              const CFX_Matrix* pLastMatrix);

  void GetBackground(const RetainPtr<CFX_DIBitmap>& pBuffer,
                     const CPDF_PageObject* pObj,
                     const CPDF_RenderOptions* pOptions,
                     const CFX_Matrix& mtFinal);

  size_t CountLayers() const { return m_Layers.size(); }
  Layer* GetLayer(uint32_t index) { return &m_Layers[index]; }

 protected:
  std::vector<Layer> m_Layers;
};
}  // namespace ofd
#endif  // CORE_FPDFAPI_RENDER_CPDF_RENDERCONTEXT_H_

﻿// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef COFD_RENDER_RENDERSTATUS_H_
#define COFD_RENDER_RENDERSTATUS_H_

#include <memory>
#include <vector>

#include "core/fpdfapi/page/cpdf_clippath.h"
#include "core/fpdfapi/page/cpdf_colorspace.h"
#include "core/fpdfapi/page/cpdf_graphicstates.h"
#include "core/fpdfapi/page/cpdf_transparency.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/render/cpdf_renderoptions.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/dib/fx_dib.h"

class CFX_DIBitmap;
class CFX_Path;
class CFX_RenderDevice;
class CPDF_Color;
class CPDF_Font;
class CPDF_FormObject;
class CPDF_ImageCacheEntry;
class CPDF_ImageObject;

class CPDF_Object;
class CPDF_PageObject;
class CPDF_PageObjectHolder;
class CPDF_PathObject;

class CPDF_ShadingObject;
class CPDF_ShadingPattern;
class CPDF_TilingPattern;
class CPDF_TransferFunc;
class CPDF_Type3Char;
class CPDF_Type3Font;
class PauseIndicatorIface;

namespace ofd {
class COFD_RenderContext;
class COFD_PageObjectHolder;
class COFD_ImageRenderer;
class COFD_RenderStatus {
 public:
  COFD_RenderStatus(COFD_RenderContext* pContext, CFX_RenderDevice* pDevice);
  ~COFD_RenderStatus();

  // Called prior to Initialize().
  void SetOptions(const CPDF_RenderOptions& options) { m_Options = options; }
  void SetDeviceMatrix(const CFX_Matrix& matrix) { m_DeviceMatrix = matrix; }
  void SetStopObject(const CPDF_PageObject* pStopObj) { m_pStopObj = pStopObj; }

  void SetType3Char(CPDF_Type3Char* pType3Char) { m_pType3Char = pType3Char; }
  void SetFillColor(FX_ARGB color) { m_T3FillColor = color; }
  void SetDropObjects(bool bDropObjects) { m_bDropObjects = bDropObjects; }
  void SetLoadMask(bool bLoadMask) { m_bLoadMask = bLoadMask; }
  void SetStdCS(bool bStdCS) { m_bStdCS = bStdCS; }
  void SetGroupFamily(CPDF_ColorSpace::Family family) {
    m_GroupFamily = family;
  }
  void SetTransparency(const CPDF_Transparency& transparency) {
    m_Transparency = transparency;
  }

  void Initialize(const COFD_RenderStatus* pParentStatus,
                  const CPDF_GraphicStates* pInitialStates);

  void RenderObjectList(const COFD_PageObjectHolder* pObjectHolder,
                        const CFX_Matrix& mtObj2Device);
  void RenderSingleObject(CPDF_PageObject* pObj,
                          const CFX_Matrix& mtObj2Device);
  bool ContinueSingleObject(CPDF_PageObject* pObj,
                            const CFX_Matrix& mtObj2Device,
                            PauseIndicatorIface* pPause);
  void ProcessClipPath(const CPDF_ClipPath& ClipPath,
                       const CFX_Matrix& mtObj2Device);

  CPDF_ColorSpace::Family GetGroupFamily() const { return m_GroupFamily; }
  bool GetLoadMask() const { return m_bLoadMask; }
  bool GetDropObjects() const { return m_bDropObjects; }
  bool IsPrint() const { return m_bPrint; }
  bool IsStopped() const { return m_bStopped; }
  COFD_RenderContext* GetContext() const { return m_pContext.Get(); }

  CFX_RenderDevice* GetRenderDevice() const { return m_pDevice; }
  const CPDF_RenderOptions& GetRenderOptions() const { return m_Options; }

#if defined(_SKIA_SUPPORT_)
  void DebugVerifyDeviceIsPreMultiplied() const;
#endif

  RetainPtr<CPDF_TransferFunc> GetTransferFunc(
      const CPDF_Object* pObject) const;

  FX_ARGB GetFillArgb(CPDF_PageObject* pObj) const;
  FX_ARGB GetFillArgbForType3(CPDF_PageObject* pObj) const;

  void DrawTilingPattern(CPDF_TilingPattern* pPattern,
                         CPDF_PageObject* pPageObj,
                         const CFX_Matrix& mtObj2Device,
                         bool stroke);
  void DrawShadingPattern(CPDF_ShadingPattern* pPattern,
                          const CPDF_PageObject* pPageObj,
                          const CFX_Matrix& mtObj2Device,
                          bool stroke);
  void CompositeDIBitmap(const RetainPtr<CFX_DIBitmap>& pDIBitmap,
                         int left,
                         int top,
                         FX_ARGB mask_argb,
                         int bitmap_alpha,
                         BlendMode blend_mode,
                         const CPDF_Transparency& transparency);

  static std::unique_ptr<CPDF_GraphicStates> CloneObjStates(
      const CPDF_GraphicStates* pSrcStates,
      bool stroke);

 private:
  bool ProcessTransparency(CPDF_PageObject* PageObj,
                           const CFX_Matrix& mtObj2Device);
  void ProcessObjectNoClip(CPDF_PageObject* pObj,
                           const CFX_Matrix& mtObj2Device);
  void DrawObjWithBackground(CPDF_PageObject* pObj,
                             const CFX_Matrix& mtObj2Device);
  bool DrawObjWithBlend(CPDF_PageObject* pObj, const CFX_Matrix& mtObj2Device);
  bool ProcessPath(CPDF_PathObject* path_obj, const CFX_Matrix& mtObj2Device);
  void ProcessPathPattern(CPDF_PathObject* path_obj,
                          const CFX_Matrix& mtObj2Device,
                          CFX_FillRenderOptions::FillType* fill_type,
                          bool* stroke);
  void DrawPathWithPattern(CPDF_PathObject* path_obj,
                           const CFX_Matrix& mtObj2Device,
                           const CPDF_Color* pColor,
                           bool stroke);
  bool ClipPattern(const CPDF_PageObject* page_obj,
                   const CFX_Matrix& mtObj2Device,
                   bool stroke);
  bool SelectClipPath(const CPDF_PathObject* path_obj,
                      const CFX_Matrix& mtObj2Device,
                      bool stroke);
  bool ProcessImage(CPDF_ImageObject* pImageObj,
                    const CFX_Matrix& mtObj2Device);
  void ProcessShading(const CPDF_ShadingObject* pShadingObj,
                      const CFX_Matrix& mtObj2Device);

  bool ProcessText(CPDF_TextObject* textobj,
                   const CFX_Matrix& mtObj2Device,
                   CFX_Path* clipping_path);
  void DrawTextPathWithPattern(const CPDF_TextObject* textobj,
                               const CFX_Matrix& mtObj2Device,
                               CPDF_Font* pFont,
                               float font_size,
                               const CFX_Matrix& mtTextMatrix,
                               bool fill,
                               bool stroke);

  FX_RECT GetClippedBBox(const FX_RECT& rect) const;
  RetainPtr<CFX_DIBitmap> GetBackdrop(const CPDF_PageObject* pObj,
                                      const FX_RECT& rect,
                                      bool bBackAlphaRequired);

  // Optionally write the colorspace family value into |pCSFamily|.
  FX_ARGB GetBackColor(const CPDF_Dictionary* pSMaskDict,
                       const CPDF_Dictionary* pGroupDict,
                       CPDF_ColorSpace::Family* pCSFamily);
  FX_ARGB GetStrokeArgb(CPDF_PageObject* pObj) const;
  FX_RECT GetObjectClippedRect(const CPDF_PageObject* pObj,
                               const CFX_Matrix& mtObj2Device) const;

  CPDF_RenderOptions m_Options;

  std::vector<UnownedPtr<const CPDF_Type3Font>> m_Type3FontCache;
  UnownedPtr<COFD_RenderContext> const m_pContext;
  UnownedPtr<CFX_RenderDevice> const m_pDevice;
  CFX_Matrix m_DeviceMatrix;
  CPDF_ClipPath m_LastClipPath;
  UnownedPtr<const CPDF_PageObject> m_pCurObj;
  UnownedPtr<const CPDF_PageObject> m_pStopObj;
  CPDF_GraphicStates m_InitialStates;
  std::unique_ptr<COFD_ImageRenderer> m_pImageRenderer;
  UnownedPtr<const CPDF_Type3Char> m_pType3Char;
  CPDF_Transparency m_Transparency;
  bool m_bStopped = false;
  bool m_bPrint = false;
  bool m_bDropObjects = false;
  bool m_bStdCS = false;
  bool m_bLoadMask = false;
  CPDF_ColorSpace::Family m_GroupFamily = CPDF_ColorSpace::Family::kUnknown;
  FX_ARGB m_T3FillColor = 0;
  BlendMode m_curBlend = BlendMode::kNormal;
};
}  // namespace ofd
#endif  // CORE_FPDFAPI_RENDER_CPDF_RENDERSTATUS_H_

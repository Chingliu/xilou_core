﻿// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "ofd/render/cofd_imageloader.h"

#include "core/fpdfapi/page/cpdf_dib.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fpdfapi/page/cpdf_transferfunc.h"
#include "core/fpdfapi/render/cpdf_imagecacheentry.h"
#include "core/fpdfapi/render/cpdf_pagerendercache.h"


#include "core/fxge/dib/cfx_dibitmap.h"
#include "third_party/base/check.h"

#include "ofd/render/cofd_renderstatus.h"
#include "ofd/render/cofd_rendercontext.h"
using namespace ofd;
COFD_ImageLoader::COFD_ImageLoader() = default;

COFD_ImageLoader::~COFD_ImageLoader() = default;

bool COFD_ImageLoader::Start(const CPDF_ImageObject* pImage,
                             const COFD_RenderStatus* pRenderStatus,
                             bool bStdCS) {
  //晚点恢复
  //m_pCache = pRenderStatus->GetContext()->GetPageCache();
  m_pCache = nullptr;
  m_pImageObject = pImage;
  bool ret;
  //if (m_pCache) {
  //  ret = m_pCache->StartGetCachedBitmap(m_pImageObject->GetImage(),
  //                                       pRenderStatus, bStdCS);
  //} else 
  {
    ret = m_pImageObject->GetImage()->StartLoadDIBBase(
        nullptr, nullptr,
        bStdCS, pRenderStatus->GetGroupFamily(), pRenderStatus->GetLoadMask());
  }
  if (!ret)
    HandleFailure();
  return ret;
}

bool COFD_ImageLoader::Continue(PauseIndicatorIface* pPause,
                                COFD_RenderStatus* pRenderStatus) {
  //bool ret = m_pCache ? m_pCache->Continue(pPause, pRenderStatus)
  //                    : m_pImageObject->GetImage()->Continue(pPause);
  bool ret = m_pImageObject->GetImage()->Continue(pPause);
  if (!ret)
    HandleFailure();
  return ret;
}

RetainPtr<CFX_DIBBase> COFD_ImageLoader::TranslateImage(
    const RetainPtr<CPDF_TransferFunc>& pTransferFunc) {
  DCHECK(pTransferFunc);
  DCHECK(!pTransferFunc->GetIdentity());

  m_pBitmap = pTransferFunc->TranslateImage(m_pBitmap);
  if (m_bCached && m_pMask)
    m_pMask = m_pMask->Clone(nullptr);
  m_bCached = false;
  return m_pBitmap;
}

void COFD_ImageLoader::HandleFailure() {
  if (m_pCache) {
    CPDF_ImageCacheEntry* entry = m_pCache->GetCurImageCacheEntry();
    m_bCached = true;
    m_pBitmap = entry->DetachBitmap();
    m_pMask = entry->DetachMask();
    m_MatteColor = entry->GetMatteColor();
    return;
  }
  RetainPtr<CPDF_Image> pImage = m_pImageObject->GetImage();
  m_bCached = false;
  m_pBitmap = pImage->DetachBitmap();
  m_pMask = pImage->DetachMask();
  m_MatteColor = pImage->GetMatteColor();
}

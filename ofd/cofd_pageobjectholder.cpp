// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "ofd/cofd_pageobjectholder.h"
#include "ofd/cofd_document.h"

#include <algorithm>
#include <utility>

#include "constants/transparency.h"
#include "core/fpdfapi/page/cpdf_allstates.h"
#include "core/fpdfapi/page/cpdf_contentparser.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"

#include "core/fxcrt/fx_extension.h"
#include "core/fxcrt/stl_util.h"
#include "third_party/base/check.h"
#include "third_party/base/check_op.h"
using namespace ofd;


COFD_PageObjectHolder::COFD_PageObjectHolder(COFD_Document* pDoc)
    : m_pDocument(pDoc) {

}

COFD_PageObjectHolder::~COFD_PageObjectHolder() = default;

bool COFD_PageObjectHolder::IsPage() const {
  return false;
}


void COFD_PageObjectHolder::AddImageMaskBoundingBox(const CFX_FloatRect& box) {
  m_MaskBoundingBoxes.push_back(box);
}


CPDF_PageObject* COFD_PageObjectHolder::GetPageObjectByIndex(
    size_t index) const {
  return fxcrt::IndexInBounds(m_PageObjectList, index)
             ? m_PageObjectList[index].get()
             : nullptr;
}

CPDF_PageObject* COFD_PageObjectHolder::PopPageObjectByIndex(
    size_t index)  {
  return fxcrt::IndexInBounds(m_PageObjectList, index)
             ? m_PageObjectList[index].release()
             : nullptr;
}

void COFD_PageObjectHolder::AppendPageObject(
    std::unique_ptr<CPDF_PageObject> pPageObj) {
  m_PageObjectList.push_back(std::move(pPageObj));
}

bool COFD_PageObjectHolder::RemovePageObject(CPDF_PageObject* pPageObj) {
  fxcrt::FakeUniquePtr<CPDF_PageObject> p(pPageObj);

  auto it =
      std::find(std::begin(m_PageObjectList), std::end(m_PageObjectList), p);
  if (it == std::end(m_PageObjectList))
    return false;

  it->release();
  m_PageObjectList.erase(it);

  return true;
}

bool COFD_PageObjectHolder::ErasePageObjectAtIndex(size_t index) {
  if (index >= m_PageObjectList.size())
    return false;

  m_PageObjectList.erase(m_PageObjectList.begin() + index);
  return true;
}
void COFD_PageObjectHolder::StartParse() {}
void COFD_PageObjectHolder::ContinueParse(PauseIndicatorIface* pPause) {}

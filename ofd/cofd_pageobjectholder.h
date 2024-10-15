// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef OFD_PAGE_COFD_PAGEOBJECTHOLDER_H_
#define OFD_PAGE_COFD_PAGEOBJECTHOLDER_H_

#include <stddef.h>
#include <stdint.h>

#include <deque>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "core/fpdfapi/page/cpdf_transparency.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/unowned_ptr.h"
#include "core/fxge/dib/fx_dib.h"
#include "third_party/abseil-cpp/absl/types/optional.h"


class CPDF_PageObject;
class PauseIndicatorIface;

namespace ofd {

class COFD_Document;
class COFD_PageObjectHolder {
public:
  enum class ParseState : uint8_t { kNotParsed, kParsing, kParsed };

  using iterator = std::deque<std::unique_ptr<CPDF_PageObject>>::iterator;
  using const_iterator =
      std::deque<std::unique_ptr<CPDF_PageObject>>::const_iterator;

  COFD_PageObjectHolder(COFD_Document* pDoc);
  virtual ~COFD_PageObjectHolder();

  virtual bool IsPage() const;

  //等xml分段解析时再实现
  void StartParse();
  void ContinueParse(PauseIndicatorIface* pPause);
  void SetParseState(ParseState status) { m_ParseState = status; }
  ParseState GetParseState() const { return m_ParseState; }

  COFD_Document* GetDocument() const { return m_pDocument.Get(); }

  size_t GetPageObjectCount() const { return m_PageObjectList.size(); }
  CPDF_PageObject* GetPageObjectByIndex(size_t index) const;
  //转移pageobject所有权
  CPDF_PageObject* PopPageObjectByIndex(size_t index);
  void AppendPageObject(std::unique_ptr<CPDF_PageObject> pPageObj);
  bool RemovePageObject(CPDF_PageObject* pPageObj);
  bool ErasePageObjectAtIndex(size_t index);


  iterator begin() { return m_PageObjectList.begin(); }
  const_iterator begin() const { return m_PageObjectList.begin(); }

  iterator end() { return m_PageObjectList.end(); }
  const_iterator end() const { return m_PageObjectList.end(); }

  const CFX_Matrix& GetLastCTM() const { return m_LastCTM; }
  const CFX_FloatRect& GetBBox() const { return m_BBox; }

  const CPDF_Transparency& GetTransparency() const { return m_Transparency; }
  bool BackgroundAlphaNeeded() const { return m_bBackgroundAlphaNeeded; }
  void SetBackgroundAlphaNeeded(bool needed) {
    m_bBackgroundAlphaNeeded = needed;
  }

  bool HasImageMask() const { return !m_MaskBoundingBoxes.empty(); }
  const std::vector<CFX_FloatRect>& GetMaskBoundingBoxes() const {
    return m_MaskBoundingBoxes;
  }
  void AddImageMaskBoundingBox(const CFX_FloatRect& box);

protected:

  CFX_FloatRect m_BBox;
  CPDF_Transparency m_Transparency;

private:
  bool m_bBackgroundAlphaNeeded = false;
  ParseState m_ParseState = ParseState::kNotParsed;

  UnownedPtr<COFD_Document> m_pDocument;
  std::vector<CFX_FloatRect> m_MaskBoundingBoxes;

  std::deque<std::unique_ptr<CPDF_PageObject>> m_PageObjectList;
  CFX_Matrix m_LastCTM;

};

}  // namespace ofd
#endif  // OFD_PAGE_COFD_PAGEOBJECTHOLDER_H_

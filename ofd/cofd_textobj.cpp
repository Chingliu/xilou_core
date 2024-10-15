
#include "ofd/cofd_textobj.h"

namespace ofd {

COFD_TextObject::COFD_TextObject()
    : CPDF_TextObject(kNoContentStream),
      m_GlyphIndex(0),
      m_Unicode(0),m_hasCGTrans(false)
       {}

COFD_TextObject::~COFD_TextObject() {

}

//模仿PDF_TextObject的clone，但不用复制m_charcode之类
//因为稍后会SetText，SetPosition
std::unique_ptr<COFD_TextObject> COFD_TextObject::Clone() const {
  auto obj = std::make_unique<COFD_TextObject>();
  obj->CopyData(this);
  obj->m_GlyphIndex = this->m_GlyphIndex;
  obj->m_Unicode = this->m_Unicode;
  obj->m_hasCGTrans = this->m_hasCGTrans;
  return obj;
}
}

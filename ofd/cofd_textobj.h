
#ifndef OFD_RENDER_TEXTOBJ_H_
#define OFD_RENDER_TEXTOBJ_H_

#include <stdint.h>

#include <vector>


#include "core/fpdfapi/page/cpdf_textobject.h"

namespace ofd {
//
class COFD_TextObject final: public CPDF_TextObject {
 public:
  explicit COFD_TextObject();
  ~COFD_TextObject() override;
  std::unique_ptr<COFD_TextObject> Clone() const;
 public:
//因为还没解决在一个PDF_TextObject排版多个字符，因此目前一个OFD_TextObject就是一个字符 
  uint32_t m_GlyphIndex;
  uint32_t m_Unicode;
  bool m_hasCGTrans;
};

}

#endif  // CORE_FPDFAPI_RENDER_CHARPOSLIST_H_

#include "ofd/cofd_common.h"
#include "ofd/cofd_page.h"
#include "ofd/cofd_document.h"
#include "ofd/cofd_page_annot.h"

using namespace ofd;

namespace ofd {

  COFD_PageAnnot::COFD_PageAnnot() : m_annot_type(L"NOTANNOT") {
  }

  COFD_PageAnnot::~COFD_PageAnnot() {
  }

  void COFD_PageAnnot::setAnnotType(WideStringView annot_type) {
    m_annot_type = annot_type;
    m_enum_type = CPDF_Annot::StringToAnnotSubtype(m_annot_type.ToDefANSI());
  }
  WideStringView COFD_PageAnnot::getAnnotType() {
    return m_annot_type.AsStringView();
  }
  CPDF_Annot::Subtype COFD_PageAnnot::getAnnotEnumType() {
    return m_enum_type;
  }
  }  // namespace ofd
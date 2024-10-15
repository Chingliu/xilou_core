#ifndef COFD_DOCUMENT_PAGE_ANNOT_H_HEADER
#define COFD_DOCUMENT_PAGE_ANNOT_H_HEADER
#include "core/fpdfdoc/cpdf_annot.h"
namespace ofd {

class COFD_PageAnnot  {
 public:
  explicit COFD_PageAnnot();

 public:
  COFD_PageAnnot(const COFD_PageAnnot&) = delete;
  COFD_PageAnnot(COFD_PageAnnot&&) = delete;
  COFD_PageAnnot& operator=(const COFD_PageAnnot&) = delete;
  COFD_PageAnnot& operator=(COFD_PageAnnot&&) = delete;
  virtual ~COFD_PageAnnot();

  public:
  void setAnnotType(WideStringView annot_type);
   WideStringView getAnnotType();
  CPDF_Annot::Subtype getAnnotEnumType();
 private:
  WideString m_annot_type;
  CPDF_Annot::Subtype m_enum_type;
};

}  // namespace ofd

#endif  // !COFD_DOCUMENT_PAGE_ANNOT_H_HEADER
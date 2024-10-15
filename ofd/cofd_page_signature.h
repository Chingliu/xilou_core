#ifndef OFD_COFD_PAGE_SIGNATURE_HEADER_H
#define OFD_COFD_PAGE_SIGNATURE_HEADER_H

#include <map>
#include <memory>
#include <vector>
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxcrt/unowned_ptr.h"
#include "ofd/cofd_common.h"
#include "ofd/cofd_pageobjectholder.h"

class CPDF_Path;

namespace ofd {
class COFD_Package;
class COFD_Document;
class COFD_Page;
class COfdSignature;
class COFD_SEAL {
 public:
  COFD_Package* pkg_;
  COFD_Document* doc_;
  COFD_Page* page_;
  COFD_SEAL();
  ~COFD_SEAL();
};
class COFD_PageSignature : public COFD_PageObjectHolder {
 public:
  explicit COFD_PageSignature(COFD_Document* doc, WideString &sig_url);

 public:
  COFD_PageSignature(const COFD_PageSignature&) = delete;
  COFD_PageSignature(COFD_PageSignature&&) = delete;
  COFD_PageSignature& operator=(const COFD_PageSignature&) = delete;
  COFD_PageSignature& operator=(COFD_PageSignature&&) = delete;
  ~COFD_PageSignature();

 public:
  void SetPageId(int page_id);
  void SetBoundary(const CFX_RectF& boundary);
  void SetClip(const CFX_RectF& clip);
  void GenAppearance();

 private:
  void GetSealImageFromESL(std::vector<uint8_t>& img_data);
  void GetSealImageFromOFD(std::vector<uint8_t>& zip_data);
 private:
  WideString sig_url_;
  UnownedPtr<COFD_Document> doc_;
  UnownedPtr<COfdSignature> parent_;
  int page_id_;
  CFX_RectF bbox_;
  CFX_RectF clip_;
  COFD_SEAL ofd_seal_;
};
}  // namespace ofd
#endif

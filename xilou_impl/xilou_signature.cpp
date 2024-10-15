#include "xilou_public/xilou_view.h"
#include "xilou_impl/xilou_impl.h"
#include "xilou_public/xilou_signature.h"
#include "xilou_public/xilou_errcode.h"
#include "xilou_impl/xilou_signature_iface.h"
#include "public/fpdfview.h"
#include "core/fxcrt/cfx_readonlymemorystream.h"
#include "core/fxcrt/fx_safe_types.h"
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxcrt/unowned_ptr.h"
#include "public/cpp/fpdf_scopers.h"
#include "core/fxge/cfx_defaultrenderdevice.h"
#include "fpdfsdk/cpdfsdk_helpers.h"

#include "ofd/render/cofd_pagerendercontext.h"
#include "ofd/render/cofd_renderpage.h"

using namespace xilou;

XILOU_EXPORT size_t XILOU_CALLCONV xilou_sig_errmsg(XILOU_DOCUMENT document,
                                                    int index,
                                                    unsigned char** errmsg) {
  CXilou_ChildDocument* doc = reinterpret_cast<CXilou_ChildDocument*>(document);
  if (!doc)
    return 0;
  if (doc->pdf) {
    return 0;
  }
  if (doc->ofd) {
    auto count = doc->ofd->GetSignatureCount();
    for (size_t i = 0; i < count; i++) {
      if (i == (size_t)index) {
        auto sig = doc->ofd->GetSignature(index);
        unsigned char* pmsg = nullptr;
        auto len = sig->errmsg(&pmsg);
        if (errmsg) {
          *errmsg = pmsg;
        }
        return len;
      }
    }
  }
  return 0;
}
XILOU_EXPORT unsigned long XILOU_CALLCONV xilou_verify(XILOU_DOCUMENT document,
  int index) {
  CXilou_ChildDocument* doc = reinterpret_cast<CXilou_ChildDocument*>(document);
  if (!doc)
    return XILOU_E_INVALID_HANDLE_ERROR;
  if (doc->pdf) {
    return XILOU_E_FEATURE_NOT_IMPLEMENT_ERROR;
  }
  if (doc->ofd) {
    auto count = doc->ofd->GetSignatureCount();
    for (size_t i = 0; i < count; i++) {
      if (i == (size_t)index) {
        auto sig = doc->ofd->GetSignature(index);
        return sig->verify();
      }
    }
  }
  return XILOU_E_OUT_OF_RANGE_ERROR;
}

XILOU_EXPORT long XILOU_CALLCONV
xilou_docsign_count(XILOU_DOCUMENT document) {
  CXilou_ChildDocument* doc = reinterpret_cast<CXilou_ChildDocument*>(document);
  if (!doc)
    return -XILOU_E_INVALID_HANDLE_ERROR;
  if (doc->pdf) {
    return -XILOU_E_FEATURE_NOT_IMPLEMENT_ERROR;
  }
  if (doc->ofd) {
    return doc->ofd->GetSignatureCount();
  }
  return -XILOU_E_FEATURE_NOT_IMPLEMENT_ERROR;
}

#ifndef OFD_COFD_SIGNATURE_HEADER_H
#define OFD_COFD_SIGNATURE_HEADER_H
#include "ofd/cofd_common.h"
#include "ofd/cofd_page_signature.h"
#include "xilou_impl/xilou_signature_iface.h"
#include <map>
#include <vector>
class CFX_XMLElement;
namespace ofd {
class COFD_Document;
class COFD_PageSignature;
class COfdSignature : public xilou::SignatureIface {

 public:
  explicit COfdSignature(COFD_Document* doc,
                         int id,
                         WideString type, WideString url);

 public:
  COfdSignature(const COfdSignature&) = delete;
  COfdSignature(COfdSignature&&) = delete;
  COfdSignature& operator=(const COfdSignature&) = delete;
  COfdSignature& operator=(COfdSignature&&) = delete;
  ~COfdSignature();

 public:
  int GetSigId() { return id_; }
  WideString GetSigType() { return sig_type_; }
  WideString GetSigUrl() { return sig_url_; }
  size_t GetPageSignatureCount();
  size_t GetPageSignatureCount(int pageid);
  std::vector<COFD_PageSignature*> GetPageSignature(int pageid);

 public:
  int verify();
  size_t errmsg(unsigned char** errmsg);

 private:
  int VerifyReferences();
  int VerifyReference(CFX_XMLElement* ref, WideString check_method);
  int VerifySignedValue();
 private:
  int id_;
  WideString sig_type_;
  WideString sig_url_;
  unsigned long oes_errcode_;
  std::vector<unsigned char> oes_err_msg_;
 private:
  UnownedPtr<COFD_Document> doc_;
  std::multimap<int, std::unique_ptr<COFD_PageSignature> > page_signatures_;
  bool collected_page_signature;
};
}

#endif
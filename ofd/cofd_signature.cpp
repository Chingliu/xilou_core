#include "xilou_public/xilou_errcode.h"
#include "ofd/cofd_common.h"
#include "ofd/cofd_util.h"
#include "ofd/cofd_signature.h"
#include "ofd/cofd_document.h"
#include "third_party/abseil-cpp/absl/strings/str_split.h"
#include "ofd/oes/oes_loader.h"
#include "third_party/abseil-cpp/absl/strings/escaping.h"
using namespace ofd;

COfdSignature::COfdSignature(COFD_Document* doc,
  int id,
  WideString type,
                             WideString url)
    : id_(id), sig_type_(type), sig_url_(url), doc_(doc) {
  collected_page_signature = false;
  oes_errcode_ = 0;
}

COfdSignature ::~COfdSignature() {

}

size_t COfdSignature ::GetPageSignatureCount() {
  if (collected_page_signature)
    return page_signatures_.size();
  auto sig_xml = doc_->ReadAndCacheXml(sig_url_.ToUTF8().AsStringView());
  CFX_XMLElement* root = sig_xml->GetRoot();
  if (!root)
    return 0;
  CFX_XMLElement* signature_node =
      root->GetFirstChildNamed_NoPrefix(L"Signature");
  if (!signature_node)
    return 0;
  auto sig_info = signature_node->GetFirstChildNamed_NoPrefix(L"SignedInfo");
  if (!sig_info)
    return 0;
  for (auto* child = sig_info->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (!elem)
      continue;
    if (elem->GetLocalTagName() == L"StampAnnot") {
      int page_id;
      if (elem->HasAttribute(L"PageRef"))
        page_id = StringToFloat(elem->GetAttribute(L"PageRef").AsStringView());
      else
        continue;
      CFX_RectF boundary;
      if (elem->HasAttribute(L"Boundary")) {
        WideString boundary_str = elem->GetAttribute(L"Boundary");
        boundary = ofd_util::ParseSTBOX(boundary_str);
      } else {
        continue;
      }
      CFX_RectF clip;
      if (elem->HasAttribute(L"Clip")) {
        WideString clip_str = elem->GetAttribute(L"Clip");
        clip = ofd_util::ParseSTBOX(clip_str);
      }
      auto page_signature = std::make_unique<COFD_PageSignature>(doc_, sig_url_);
      page_signature->SetPageId(page_id);
      page_signature->SetBoundary(boundary);
      page_signature->SetClip(clip);
      page_signature->GenAppearance();
      page_signatures_.insert(
          std::make_pair(page_id, std::move(page_signature)));
      //page_signatures_.push_back(std::move(page_signature));
      continue;
    }
  }
  collected_page_signature = true;
  return page_signatures_.size();
}

size_t COfdSignature::GetPageSignatureCount(int pageid) {
  if (!collected_page_signature)
    GetPageSignatureCount();
  return page_signatures_.count(pageid);
}

std::vector<COFD_PageSignature*> COfdSignature::GetPageSignature(int pageid) {
  if (!collected_page_signature)
    GetPageSignatureCount();

  std::vector<COFD_PageSignature*> page_siges;
  auto pr = page_signatures_.equal_range(pageid);
  if (pr.first != page_signatures_.end()) {
    for (auto it = pr.first; it != pr.second; ++it) {
      it->second->SetParseState(COFD_PageObjectHolder::ParseState::kParsed);
      page_siges.push_back(it->second.get());
    }
  }
  return page_siges;
}

int COfdSignature::verify() {
  auto ret = VerifyReferences();
  if (ret != XILOU_E_SUC)
    return ret;
  return VerifySignedValue();
}

int COfdSignature::VerifyReferences() {
  auto sig_xml = doc_->ReadAndCacheXml(sig_url_.ToUTF8().AsStringView());
  CFX_XMLElement* root = sig_xml->GetRoot();
  if (!root)
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
  CFX_XMLElement* signature_node =
      root->GetFirstChildNamed_NoPrefix(L"Signature");
  if (!signature_node)
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
  auto sign_info = signature_node->GetFirstChildNamed_NoPrefix(L"SignedInfo");
  if (!sign_info)
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
  auto references = sign_info->GetFirstChildNamed_NoPrefix(L"References");
  if (!references)
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
  auto check_method = references->GetAttribute(L"CheckMethod");
  if (check_method.GetStringLength() <= 1) {
    check_method = L"MD5";
  }
  for (auto* child = references->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    CFX_XMLElement* elem = ToXMLElement(child);
    if (!elem)
      continue;
    if (elem->GetLocalTagName() == L"Reference") {
      auto entry_result = VerifyReference(elem, check_method);
      if (XILOU_E_SUC != entry_result) {
        return entry_result;
      }
      continue;
    }
  }
  return XILOU_E_SUC;
}

int COfdSignature::VerifyReference(CFX_XMLElement* ref,
                                   WideString check_method) {
  auto file_url = ref->GetAttribute(L"FileRef");
  auto org_hash_node = ref->GetFirstChildNamed_NoPrefix(L"CheckValue");
  auto org_hash = org_hash_node->GetTextData();
  std::vector<uint8_t> data;
  doc_->ReadBinary(file_url.ToUTF8().AsStringView(), &data);
  if (data.size() <= 0 || org_hash.GetStringLength() <= 0) {
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
  }
  auto oes_err = OES::COESLoader::Instance()->ErrorCode();
  if (oes_err  != 0) {
    // TODO 需要有错误及日志系统
    return XILOU_E_OFD_OES_LOADER;
  }
  auto str_check_method = check_method.ToUTF8();
  unsigned char digest_value[128] = {0};
  int digest_len = 0;
  if (OES::COESLoader::Instance()->m_si.pOESV4_Digest_Init &&
      OES::COESLoader::Instance()->m_si.pOESV4_Digest_Update &&
      OES::COESLoader::Instance()->m_si.pOESV4_Digest_Final) {
    OES_HANDLE ihash = nullptr;
    OES::COESLoader::Instance()->m_si.pOESV4_Digest_Init(
        &OES::COESLoader::Instance()->m_si.oesctx, (unsigned char *)str_check_method.c_str(),
        str_check_method.GetLength(), &ihash);
    OES::COESLoader::Instance()->m_si.pOESV4_Digest_Update(
        &OES::COESLoader::Instance()->m_si.oesctx, ihash, &data[0],
        data.size());
    OES::COESLoader::Instance()->m_si.pOESV4_Digest_Final(
        &OES::COESLoader::Instance()->m_si.oesctx, ihash, digest_value, &digest_len);
    auto based64_digest =
        absl::Base64Escape(absl::string_view((const char *)digest_value, digest_len));
    if (0 == based64_digest.compare(org_hash.ToUTF8().c_str())) {
      return XILOU_E_SUC;
    } else {
      WideString errmsg(L"file hash has been changed:");
      errmsg += file_url;
      auto utf_msg = errmsg.ToUTF8();
      oes_err_msg_.resize(utf_msg.GetStringLength() + 10);
      memcpy(&oes_err_msg_[0], utf_msg.c_str(), utf_msg.GetStringLength());
      return XILOU_E_OFD_ENTRY_HASH;
    }
  }
  return XILOU_E_OFD_OES_LOADER;
}
int COfdSignature::VerifySignedValue() {
  auto sig_xml = doc_->ReadAndCacheXml(sig_url_.ToUTF8().AsStringView());
  CFX_XMLElement* root = sig_xml->GetRoot();
  if (!root)
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
  CFX_XMLElement* signature_node =
      root->GetFirstChildNamed_NoPrefix(L"Signature");
  if (!signature_node)
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
#if 1
  auto sign_info = signature_node->GetFirstChildNamed_NoPrefix(L"SignedInfo");
  if (!sign_info)
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
  auto references = sign_info->GetFirstChildNamed_NoPrefix(L"References");
  if (!references)
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
  auto check_method = references->GetAttribute(L"CheckMethod");
  if (check_method.GetStringLength() <= 1) {
    check_method = L"MD5";
  }
  std::string sign_method("1.2.156.10197.1.501");
  auto sign_method_node = sign_info->GetFirstChildNamed_NoPrefix(L"SignatureMethod");
  if (sign_method_node) {
    auto sign_method_txt = sign_method_node->GetTextData();
    if (sign_method_txt.GetStringLength() > 0) {
      sign_method.assign(sign_method_txt.ToUTF8().c_str());
    }
  }
  std::string sign_datetime("");
  auto sign_date_node = sign_info->GetFirstChildNamed_NoPrefix(L"SignatureDateTime");
  if (sign_date_node) {
    auto sign_date_txt = sign_date_node->GetTextData();
    if (sign_date_txt.GetStringLength() > 0) {
      sign_datetime.assign(sign_date_txt.ToUTF8().c_str());
    }
  }
#endif
  auto signed_value_node = signature_node->GetFirstChildNamed_NoPrefix(L"SignedValue");
  if (!signed_value_node)
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
  auto signed_url = signed_value_node->GetTextData();
  if (signed_url.IsEmpty())
    return XILOU_E_OFD_INVALID_SIGNATURE_XML;
  if (signed_url[0] != L'/') {
    auto delimiter = sig_url_.ReverseFind(L'/');
    if (delimiter.has_value()) {
      signed_url = sig_url_.Substr(0, delimiter.value() + 1) + signed_url;
    } else {
      signed_url = doc_->GetDocBaseUrl() + signed_url;
    }
  }
#if 0
  WideString seal_url;
  auto seal_node = sign_info->GetFirstChildNamed_NoPrefix(L"Seal");
  if (seal_node) {
    auto seal_loc = seal_node->GetFirstChildNamed_NoPrefix(L"BaseLoc");
    if (seal_loc) {
      seal_url = seal_loc->GetTextData();
      if (!seal_url.IsEmpty()) {
        if (seal_url[0] != L'/') {
          auto delimiter = sig_url_.ReverseFind(L'/');
          if (delimiter.has_value()) {
            seal_url = sig_url_.Substr(0, delimiter.value() + 1) + seal_url;
          } else {
            seal_url = doc_->GetDocBaseUrl() + seal_url;
          }
        }
      }
    }
  }
  std::vector<uint8_t> esl_data;
  doc_->ReadBinary(seal_url.ToUTF8().AsStringView(), &esl_data);
  if (esl_data.size() == 0) {
    //从签名值里解析出来
  }
#endif

  auto oes_err = OES::COESLoader::Instance()->ErrorCode();
  if (oes_err != 0) {
    // TODO 需要有错误及日志系统
    return XILOU_E_OFD_OES_LOADER;
  }
  OES::oes_interface psi;
  memset(&psi, 0, sizeof(psi));
  OES::COESLoader::Instance()->CopyOESFnPointer(&psi);
  if (!(psi.pOESV4_Digest_Init && psi.pOESV4_Digest_Update &&
      psi.pOESV4_Digest_Final)) {
    return XILOU_E_OFD_OES_LOADER;
  }
  auto str_check_method = check_method.ToUTF8();
  unsigned char digest_value[128] = {0};
  int digest_len = 0;
  std::vector<uint8_t> data;
  doc_->ReadBinary(sig_url_.ToUTF8().AsStringView(), &data);
  OES_HANDLE ihash = nullptr;
  psi.pOESV4_Digest_Init(&psi.oesctx,
      (unsigned char*)str_check_method.c_str(), str_check_method.GetLength(),
      &ihash);
  psi.pOESV4_Digest_Update(&psi.oesctx, ihash, &data[0], data.size());
  psi.pOESV4_Digest_Final(&psi.oesctx, ihash, digest_value,
      &digest_len);

  std::vector<uint8_t> signed_value_data;
  doc_->ReadBinary(signed_url.ToUTF8().AsStringView(), &signed_value_data);
  oes_errcode_ = psi.pOESV4_Verify(&psi.oesctx, digest_value, digest_len,
                    &signed_value_data[0], signed_value_data.size(), 0);

  if (oes_errcode_ == 0) {
    return XILOU_E_SUC;
  }
  int msglen = 0;
  psi.pOESV4_GetErrMessage(&psi.oesctx, oes_errcode_, nullptr, &msglen);
  if (msglen != 0) {
    oes_err_msg_.resize(msglen + 10);
    psi.pOESV4_GetErrMessage(&psi.oesctx, oes_errcode_, &oes_err_msg_[0],
                             &msglen);
  }
  return XILOU_E_OFD_SIGNED_VERIFY_ERROR;
}

size_t COfdSignature::errmsg(unsigned char** errmsg) {
  if (errmsg) {
    *errmsg = &oes_err_msg_[0];
  }
  return oes_err_msg_.size();
}
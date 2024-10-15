#include "ofd/cofd_common.h"
#include "ofd/cofd_page_signature.h"
#include "ofd/cofd_package.h"
#include "ofd/cofd_document.h"
#include "ofd/cofd_page.h"
#include "ofd/oes/oes_loader.h"
#include "ofd/cofd_util.h"



#include "core/fpdfapi/font/cpdf_fontglobals.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/progressive_decoder.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "xfa/fxfa/cxfa_ffwidget.h"
#include "core/fxcrt/cfx_readonlymemorystream.h"

#include "ofd/FIIO_Mem.h"
using namespace ofd;


COFD_SEAL::COFD_SEAL() : pkg_(nullptr), doc_(nullptr), page_(nullptr) {}

COFD_SEAL::~COFD_SEAL() {
  //按顺序析构
  if (page_ && doc_) {
    doc_->DropPage(page_);
  }
  if (doc_ && pkg_) {
    pkg_->CloseDocument(doc_);
  }
  //TODO 会崩溃，要调查
  //if (pkg_)
  //  delete pkg_;
}

COFD_PageSignature::COFD_PageSignature(COFD_Document* doc, WideString& sig_url)
    : COFD_PageObjectHolder(doc), sig_url_(sig_url), doc_(doc) {}

COFD_PageSignature::~COFD_PageSignature() {}

void COFD_PageSignature::SetBoundary(const CFX_RectF& boundary) {
  bbox_ = boundary;
}
void COFD_PageSignature::SetClip(const CFX_RectF& clip) {
  clip_ = clip;
}
void COFD_PageSignature::SetPageId(int page_id) {
  page_id_ = page_id;
}

void COFD_PageSignature::GetSealImageFromESL(std::vector<uint8_t>& img_data) {
  auto sig_xml = doc_->ReadAndCacheXml(sig_url_.ToUTF8().AsStringView());
  CFX_XMLElement* root = sig_xml->GetRoot();
  if (!root)
    return;
  CFX_XMLElement* signature_node =
      root->GetFirstChildNamed_NoPrefix(L"Signature");
  if (!signature_node)
    return;
  auto sign_info = signature_node->GetFirstChildNamed_NoPrefix(L"SignedInfo");
  if (!sign_info)
    return;
  auto seal_node = sign_info->GetFirstChildNamed_NoPrefix(L"Seal");
  if (!seal_node)
    return;
  auto seal_loc = seal_node->GetFirstChildNamed_NoPrefix(L"BaseLoc");
  if (!seal_loc)
    return;
  auto seal_url = seal_loc->GetTextData();
  if (seal_url.IsEmpty())
    return;
  if (seal_url[0] != L'/') {
    auto delimiter = sig_url_.ReverseFind(L'/');
    if (delimiter.has_value()) {
      seal_url = sig_url_.Substr(0, delimiter.value() + 1) + seal_url;
    } else {
      seal_url = doc_->GetDocBaseUrl() + seal_url;
    }
  }
  std::vector<uint8_t> seal_data;
  doc_->ReadBinary(seal_url.ToDefANSI().AsStringView(), &seal_data);
  if (seal_data.size() < 10)
    return;
  if (!OES::COESLoader::Instance()->m_si.pOESV4_GetSealImage) {
    // TODO 需要有错误及日志系统
    return;
  }
  seal_data.push_back(0);
  seal_data.push_back(0);
  seal_data.push_back(0);
  seal_data.push_back(0);
  int img_data_len = 0;
  int img_type_len = 0;
  int w;
  int h;
  OES::COESLoader::Instance()->m_si.pOESV4_GetSealImage(
      OES::COESLoader::Instance()->m_si.oesctx, &seal_data[0], seal_data.size(),
      nullptr, 0, nullptr, &img_data_len, nullptr, &img_type_len, &w, &h);
  if (img_data_len <= 0)
    return;
  img_data.resize(img_data_len);
  OES::COESLoader::Instance()->m_si.pOESV4_GetSealImage(
      OES::COESLoader::Instance()->m_si.oesctx, &seal_data[0], seal_data.size(),
      nullptr, 0, &img_data[0], &img_data_len, nullptr, nullptr, nullptr,
      nullptr);
}
void COFD_PageSignature::GenAppearance() {
  auto sig_xml = doc_->ReadAndCacheXml(sig_url_.ToUTF8().AsStringView());
  CFX_XMLElement* root = sig_xml->GetRoot();
  if (!root)
    return;
  CFX_XMLElement* signature_node =
      root->GetFirstChildNamed_NoPrefix(L"Signature");
  if (!signature_node)
    return;
  auto signed_node =
      signature_node->GetFirstChildNamed_NoPrefix(L"SignedValue");
  if (!signed_node)
    return;
  auto signed_url = signed_node->GetTextData();
  if (signed_url.IsEmpty())
    return;
  if (signed_url[0] != L'/') {
    auto delimiter = sig_url_.ReverseFind(L'/');
    if (delimiter.has_value()) {
      signed_url = sig_url_.Substr(0, delimiter.value() + 1) + signed_url;
    } else {
      signed_url = doc_->GetDocBaseUrl() + signed_url;
    }
  }
  std::vector<uint8_t> signed_data;
  doc_->ReadBinary(signed_url.ToDefANSI().AsStringView(), &signed_data);
  if (signed_data.size() < 10)
    return;
  if (OES::COESLoader::Instance()->ErrorCode() != 0) {
    // TODO 需要有错误及日志系统
    return;
  }
  int img_data_len = 0;
  int img_type_len = 0;
  std::vector<uint8_t> img_data;
  OES::COESLoader::Instance()->m_si.pOESV4_GetSignImage(
      OES::COESLoader::Instance()->m_si.oesctx, &signed_data[0],
      signed_data.size(), nullptr, 0, nullptr, &img_data_len, nullptr,
      &img_type_len, nullptr, nullptr);
  if (img_data_len > 0) {
    std::vector<uint8_t> img_type;
    img_type.resize(img_type_len + 100);

    img_data.resize(img_data_len);
    OES::COESLoader::Instance()->m_si.pOESV4_GetSignImage(
        OES::COESLoader::Instance()->m_si.oesctx, &signed_data[0],
        signed_data.size(), nullptr, 0, &img_data[0], &img_data_len,
        &img_type[0], &img_type_len, nullptr, nullptr);
  } else {
    GetSealImageFromESL(img_data);
  }
  if (img_data.size() < 10)
    return;
  if (img_data[0] == 0x50 && img_data[1] == 0x4b && img_data[2] == 0x03 &&
      img_data[3] == 0x04) {
    return GetSealImageFromOFD(img_data);
  }
  //它还有可能是个ofd，
  // ofd_util::SaveBinaryFile("img_inside_sign", &img_data[0], img_data.size());
  fiio_mem_handle fmh;
  fmh.data = &img_data[0];
  fmh.filelen = fmh.datalen = img_data.size();
  auto image_type = FreeImage_GetImageTypeFromMem(&fmh);
  if (image_type == FIF_UNKNOWN)
    return;
  if (image_type != FIF_PNG && image_type != FIF_JPEG) {
    //暂时使用freeimage 的能力来解码其它格式图像
    // TODO修复codec中的Resample的bug
    FIBITMAP* bitmap_org;
    fmh.data = &img_data[0];
    fmh.filelen = fmh.datalen = img_data.size();
    bitmap_org = FreeImage_LoadFromMem(image_type, &fmh, 0);
    if (bitmap_org) {
      fiio_mem_handle hconvert;
      hconvert.data = nullptr;
      hconvert.datalen = 0;
      auto bret = FreeImage_SaveToMem(FIF_PNG, bitmap_org, &hconvert, 0);
      if (bret && hconvert.data && hconvert.filelen > 0) {
        img_data.clear();
        img_data.resize(hconvert.filelen);
        memcpy(&img_data[0], hconvert.data, hconvert.filelen);
        free(hconvert.data);
      }
      FreeImage_Unload(bitmap_org);
    }
  }
  std::unique_ptr<CPDF_ImageObject> pImageObj =
      std::make_unique<CPDF_ImageObject>();
  pdfium::span<const uint8_t> bsData(img_data);
  RetainPtr<IFX_SeekableReadStream> pImageFileRead;
  pImageFileRead = pdfium::MakeRetain<CFX_ReadOnlyMemoryStream>(bsData);
  // if (image_type != FIF_JPEG) //直接SetJpegImageInline图没有flip
  {
    // XFA_LoadImageFromBuffer
    int32_t iImageXDpi;
    int32_t iImageYDpi;
    auto pBitmap = XFA_LoadImageFromBuffer(
        pImageFileRead, FXCODEC_IMAGE_UNKNOWN, iImageXDpi, iImageYDpi);
    if (!pBitmap)
      return;
    // static int dbgnum = 0;
    // ofd_util::DbgWriteBitmapAsBmp("test", dbgnum++, pBitmap);
    auto pdf_image = pdfium::MakeRetain<CPDF_Image>(doc_->GetFakePDFDocument());
    RetainPtr<CFX_DIBitmap> pFlipped = pBitmap->FlipImage(false, true);
    // ofd_util::DbgWriteBitmapAsBmp("test", dbgnum++, pFlipped);
    // pFlipped->ConvertColorScale(0, 0xffffff);
    pdf_image->SetImage(pFlipped);
    pImageObj->SetImage(pdf_image);
  }
  CFX_Matrix ectm = CFX_Matrix();
  ectm.a = bbox_.Width();
  ectm.d = bbox_.Height();
  ectm.e += bbox_.Left();
  ectm.f += bbox_.Top();
  pImageObj->SetImageMatrix(ectm);

  // TODO 没起作用
  pImageObj->m_GeneralState.SetBlendMode("Multiply");
  pImageObj->m_GeneralState.SetBlendType(BlendMode::kMultiply);
  AppendPageObject(std::move(pImageObj));
}

void COFD_PageSignature::GetSealImageFromOFD(std::vector<uint8_t>& zip_data) {
  ofd_seal_.pkg_ = new COFD_Package((const char *) & zip_data[0], zip_data.size());
  if (!ofd_seal_.pkg_)
    return;
  ofd_seal_.pkg_->ParseOFD();
  ofd_seal_.doc_ = ofd_seal_.pkg_->OpenDocument();
  if (!ofd_seal_.doc_)
    return;
  ofd_seal_.page_ = ofd_seal_.doc_->LoadPage(0);
  if (!ofd_seal_.page_)
    return;
  //auto page_box = ofd_seal_.page_->GetBBox();
  ofd_seal_.page_->SetPageArea(bbox_);//TODO 没考虑page可能的pagearea,不是从0，0开始的情况
  ofd_seal_.page_->Parse();
  auto page_objs = ofd_seal_.page_->GetPageObjectCount();
  for (size_t objs = 0; objs < page_objs; objs++) {
    CPDF_PageObject* pageobj = ofd_seal_.page_->PopPageObjectByIndex(objs);
    std::unique_ptr<CPDF_PageObject> pPageObjHolder(pageobj);
    AppendPageObject(std::move(pPageObjHolder));
  }

}


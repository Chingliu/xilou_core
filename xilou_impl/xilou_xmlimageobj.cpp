#include "xilou_impl/xilou_xmlpackage.h"
#include "xilou_impl/xilou_xmlpage.h"
#include "xilou_impl/xilou_xmlimageobj.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "ofd/cofd_common.h"
#include "core/fxcrt/xml/cfx_xmltext.h"
#include "xilou_impl/xilou_bitmap_saver.h"
#include "core/fxge/dib/cfx_dibbase.h"
#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcrt/fx_safe_types.h"
#include <stdio.h>
#include "ofd/cofd_util.h"
#include "public/cpp/fpdf_scopers.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
namespace xilou {
Cxml_imageobj::Cxml_imageobj(CPDF_ImageObject* pdf_imageobj, Cxml_page* ofd_page): 
  Cxml_pageobj(ofd_page),
  m_pdfimageobj(pdf_imageobj), 
  m_ofdpage(ofd_page) {

}

Cxml_imageobj::~Cxml_imageobj() {

}

void Cxml_imageobj::convert() {
  auto imgobj = m_root->CreateNode<CFX_XMLElement>(L"ofd:ImageObject");
  m_objectroot = imgobj;
  std::vector<uint8_t> png_data;
  if (m_ofdpage->getPdfPage() && m_ofdpage->getPdfDoc()) {
    ScopedFPDFBitmap bitmap(FPDFImageObj_GetRenderedBitmap(
        m_ofdpage->getPdfDoc(), m_ofdpage->getPdfPage(),
        FPDFPageObjectFromCPDFPageObject(m_pdfimageobj.Get())));
    if (bitmap) {
      png_data = xilou::BitmapSaver::ConvertBitmapToPng(
          CFXDIBitmapFromFPDFBitmap(bitmap.get()));
    }

  }
  if (png_data.empty()) {
    auto dib = m_pdfimageobj->GetIndependentBitmap();

    auto format = dib->GetFormat();
    switch (format) {
      case FXDIB_Format::k1bppMask:
      case FXDIB_Format::k1bppRgb:
        dib->ConvertFormat(FXDIB_Format::k8bppRgb);
        break;
    }
    png_data = xilou::BitmapSaver::ConvertBitmapToPng(dib.Get());
  }
  
  if (png_data.empty()) {
    auto dib = m_pdfimageobj->GetIndependentBitmap();
    dib->ConvertFormat(FXDIB_Format::kArgb);
    ofd_util::ConvertBitmapAsBmp(dib, png_data);
#if 0
    //TODO
    auto stream = m_pdfimageobj->GetImage()->GetStream();
    auto dict = m_pdfimageobj->GetImage()->GetDict();
    auto width = dict->GetIntegerFor("Width");
    auto height = dict->GetIntegerFor("Height");
    absl::optional<DecoderArray> decoder_array = GetDecoderArray(dict);
    auto acc = pdfium::MakeRetain<CPDF_StreamAcc>(stream);
    const ByteString& filter = decoder_array.value().back().first;
    if (filter == "JBIG2Decode") {
      const absl::optional<uint32_t> maybe_size =
          fxcodec::CalculatePitch8(1, 1, width);
      if (!maybe_size.has_value())
        return ;

      FX_SAFE_UINT32 src_size = maybe_size.value();
      src_size *= height;
      if (!src_size.IsValid())
        return ;
      acc->LoadAllDataImageAcc(src_size.ValueOrDie());
      if (acc->GetSize() == 0 || !acc->GetData())
        return ;
      //拼出jbig2文件
      png_data.resize(acc->GetSize()+ 9);
      const uint8_t jbig2_id_string[9] = {0x97, 0x4a, 0x42, 0x32, 0x0d,
                                          0x0a, 0x1a, 0x0a, 0x0};
      memcpy(&png_data[0], jbig2_id_string, 9);
      memcpy(&png_data[9], acc->GetData(), acc->GetSize());
    }
#endif
  }
  if (png_data.empty()) {
    return;
  }
  auto id = m_ofdpage->getUniqueID();
  auto img_name = WideString::Format(L"Image_%d.png", id);
  auto img_id = m_ofdpage->appendImageRes(img_name.AsStringView(), &png_data);

  imgobj->SetAttribute(L"ID", WideString::Format(L"%d", id));
  imgobj->SetAttribute(L"ResourceID", WideString::Format(L"%d", img_id));

  auto pdf_ctm = m_ofdpage->getPDFDisplayCTM();
  auto imgRect = m_pdfimageobj->GetRect();
  auto formRect = getFormRect();
  auto formCTM = getFormMatrix();
  imgRect = formCTM.TransformRect(imgRect);
  (void)formRect;
  (void)pdf_ctm;
  if (!insideForm())
    imgRect = pdf_ctm.TransformRect(imgRect);
  WideString wsBoundary = WideString::Format(
      L"%.3f %.3f %.3f %.3f", ofd_one_72_point_to_mm(imgRect.Left()),
      ofd_one_72_point_to_mm(imgRect.Bottom()),
      ofd_one_72_point_to_mm(imgRect.Width()),
      ofd_one_72_point_to_mm(imgRect.Height()));
  imgobj->SetAttribute(L"Boundary", wsBoundary);
  auto img_ctm = m_pdfimageobj->matrix();
  img_ctm = img_ctm * formCTM;
  img_ctm.e = 0;
  img_ctm.f = 0;
  auto ctm_str =
      WideString::Format(L" %f %f %f %f %f %f", ofd_one_72_point_to_mm(img_ctm.a), img_ctm.b,
      img_ctm.c, ofd_one_72_point_to_mm(img_ctm.d < 0 ? -img_ctm.d : img_ctm.d),
      img_ctm.e, img_ctm.f);
  imgobj->SetAttribute(L"CTM", ctm_str);
}
}

#include "ofd/cofd_common.h"
#include "ofd/cofd_page.h"
#include "ofd/cofd_document.h"
#include "ofd/cofd_util.h"
#include "ofd/FIIO_Mem.h"

#include <vector>
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/cfx_readonlymemorystream.h"
#include "core/fpdfapi/page/cpdf_colorspace.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_path.h"
#include "core/fpdfapi/page/cpdf_textstate.h"
#include "third_party/abseil-cpp/absl/strings/str_split.h"

#include "core/fxge/cfx_fontmapper.h"
#include "core/fpdfapi/font/cpdf_fontglobals.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fpdfapi/page/cpdf_pathobject.h"
#include "core/fxcodec/fx_codec.h"
#include "core/fxcodec/progressive_decoder.h"

#include "core/fxge/dib/cfx_dibitmap.h"
#include "core/fxcodec/jbig2/JBig2_Context.h"
#include "core/fxcodec/jbig2/JBig2_DocumentContext.h"
#include "core/fxcodec/jbig2/jbig2_decoder.h"

#include "xfa/fxfa/cxfa_ffwidget.h"

#define FLT_EPSILON 1.192092896e-07F 

using namespace ofd;
using absl::ByAnyChar;
using absl::SkipWhitespace;
namespace {}  // end of namespace

int GetInteger(const uint8_t* data) {
  return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
}

uint32_t ParseJbig2SegmentHeader(const uint8_t* buf, size_t buf_size) {
  uint32_t offset = 0;
  if (buf_size < 11)
    return offset;
  /* 7.2.2 */
  uint32_t number = GetInteger(buf);
  /* 7.2.3 */
  uint8_t flags = buf[4];
  /* 7.2.4 referred-to segments */
  uint32_t rts_count;
  uint8_t rtscarf = buf[5];
  if ((rtscarf & 0xe0) == 0xe0) {
    uint32_t rtscarf_long = GetInteger(buf + 5);
    rts_count = rtscarf_long & 0x1fffffff;
    offset = 5 + 4 + (rts_count + 1) / 8;
  } else {
    rts_count = (rtscarf >> 5);
    offset = 5 + 1;
  }
  /* 7.2.5 */
  uint32_t rts_size = number <= 256 ? 1 : number <= 65536 ? 2 : 4;
  /* 7.2.6 */
  uint32_t pa_size = flags & 0x40 ? 4 : 1;
  uint32_t rts_offset = rts_count * rts_size;
  if (offset + rts_offset + pa_size + 4 > buf_size) {
    OutputDebugStringA("ParseJbig2SegmentHeader() called with insufficient data.\n");
    return 0;
  }
  offset += rts_offset;
  offset += (flags & 0x40) ? 4 : 1;
  /* 7.2.7 */
  //int data_length = GetInteger(buf + offset);
  offset += 4;
  return offset;
}

int ParseJbig2FileHeader(const uint8_t* jbig2_data,
                         size_t jbig2_data_size,
                         int& jbig2_width,
                         int& jbig2_height) {
  int header_size = 0;
  const uint8_t jbig2_id_string[8] = {0x97, 0x4a, 0x42, 0x32, 0x0d, 0x0a, 0x1a, 0x0a};
  if (memcmp(jbig2_data, jbig2_id_string, 8)) {
    OutputDebugStringA("Not a JBIG2 file header.\n");
    return 0;
  }
  uint8_t file_header_flags = jbig2_data[8];
  if (file_header_flags & 0xFC) {
    OutputDebugStringA(
        "reserved bits(2-7) of file header flags are not zero.\n");
    return 0;
  }
  if (!(file_header_flags & 2)) { /* number of pages is known */
    if (jbig2_data_size < 13)
      return 0;
    int page_count = GetInteger(jbig2_data + 9);
    if (page_count == 1)
      OutputDebugStringA(
		  "file header indicates a single page document.\n");
    else
      OutputDebugStringA(
		  "file header indicates multiple page document.\n");
    header_size += 13;
  } else { /* number of pages not known */
    header_size += 9;
  }
  int seg_offset = ParseJbig2SegmentHeader(jbig2_data + header_size,
                          jbig2_data_size - header_size);
  if (seg_offset > 0) {
    int data_offset = header_size + seg_offset;
    jbig2_width = GetInteger(jbig2_data + data_offset);
    jbig2_height = GetInteger(jbig2_data + data_offset + 4);
    return header_size;
  }
  return 0;
}

void GenerateBorderPath(const CFX_RectF& ebbox,
                        const float h_radius,
                        const float v_radius,
                        const float line_width,
                        CPDF_Path& destpath) {
  CFX_RectF innerbox;
  if (line_width < FLT_EPSILON) {
    innerbox = ebbox;
  } else {
    innerbox.left = ebbox.left + line_width / 2;
    innerbox.top = ebbox.top + line_width / 2;
    innerbox.width = ebbox.width - line_width;
    innerbox.height = ebbox.height - line_width;
  }
  if (h_radius < FLT_EPSILON && v_radius < FLT_EPSILON) {
    destpath.AppendFloatRect(innerbox.ToFloatRect());
  } else {
    float kappa = 0.5522848f;
    float half_width = innerbox.Width() / 2;
    float half_height = innerbox.Height() / 2;
    float radius_x = fmin(h_radius, half_width);
    float radius_y = fmin(v_radius, half_height);
    float ctrl_ox = radius_x * kappa;
    float ctrl_oy = radius_y * kappa;

    CFX_PointF lts(innerbox.Left(), innerbox.Top() + radius_y);
    CFX_PointF lte(innerbox.Left() + radius_x, innerbox.Top());
    CFX_PointF olts(lts.x, lts.y - ctrl_oy);
    CFX_PointF olte(lte.x - ctrl_ox, lte.y);

    CFX_PointF rts(innerbox.right() - radius_x, innerbox.Top());
    CFX_PointF rte(innerbox.right(), innerbox.Top() + radius_y);
    CFX_PointF orts(rts.x + ctrl_ox, rts.y);
    CFX_PointF orte(rte.x, rte.y - ctrl_oy);

    CFX_PointF rbs(innerbox.right(), innerbox.bottom() - radius_y);
    CFX_PointF rbe(innerbox.right() - radius_x, innerbox.bottom());
    CFX_PointF orbs(rbs.x, rbs.y + ctrl_oy);
    CFX_PointF orbe(rbe.x + ctrl_ox, rbe.y);

    CFX_PointF lbs(innerbox.Left() + radius_x, innerbox.bottom());
    CFX_PointF lbe(innerbox.Left(), innerbox.bottom() - radius_y);
    CFX_PointF olbs(lbs.x - ctrl_ox, lbs.y);
    CFX_PointF olbe(lbe.x, lbe.y + ctrl_oy);

    destpath.AppendPoint(lte, CFX_Path::Point::Type::kMove);
    destpath.AppendPoint(rts, CFX_Path::Point::Type::kLine);
    destpath.AppendPoint(orts, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(orte, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(rte, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(rbs, CFX_Path::Point::Type::kLine);
    destpath.AppendPoint(orbs, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(orbe, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(rbe, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(lbs, CFX_Path::Point::Type::kLine);
    destpath.AppendPoint(olbs, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(olbe, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(lbe, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(lts, CFX_Path::Point::Type::kLine);
    destpath.AppendPoint(olts, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(olte, CFX_Path::Point::Type::kBezier);
    destpath.AppendPoint(lte, CFX_Path::Point::Type::kBezier);
    destpath.ClosePath();
  }
}

void COFD_Page::ParseImageObject(CFX_XMLElement* elem) {
  if (elem->HasAttribute(L"Visible"))
    if (0 == elem->GetAttribute(L"Visible").Compare(L"false"))
      return;
  CFX_RectF ebbox = bbox_;
  CFX_Matrix ectm = CFX_Matrix();
  InitElem(elem, &ebbox, &ectm);
  auto pMedia = doc_->FindMedia(
      StringToFloat(elem->GetAttribute(L"ResourceID").AsStringView()));
  if (!pMedia || pMedia->m_fullpath.GetStringLength() <= 0)
    return;
  std::vector<uint8_t> img_data;
  // doc_->ReadBinary(pMedia->m_fullpath.ToUTF8().AsStringView(), &img_data);
  doc_->ReadBinary(pMedia->m_fullpath.ToDefANSI().AsStringView(), &img_data);
  if (img_data.size() < 10)
    return;
  std::unique_ptr<CPDF_ImageObject> pImageObj;
  pdfium::span<const uint8_t> bsData(img_data);
  fiio_mem_handle fmh;
  fmh.data = &img_data[0];
  fmh.filelen = fmh.datalen = img_data.size();
  auto image_type = FreeImage_GetImageTypeFromMem(&fmh);
  if (image_type == FIF_UNKNOWN) {
    const uint8_t* jdata = bsData.data();
    size_t jsize = bsData.size();
    int jwidth = 0, jheight = 0;
    int header_size = ParseJbig2FileHeader(jdata, jsize, jwidth, jheight);
    if (header_size > 0) {
      auto bitmap = pdfium::MakeRetain<CFX_DIBitmap>();
      if (!bitmap->Create(jwidth, jheight, FXDIB_Format::k1bppRgb))
        return;
      Jbig2Context jbig2_context;
      JBig2_DocumentContext doc_context;
      FXCODEC_STATUS status = Jbig2Decoder::StartDecode(
          &jbig2_context, &doc_context, jwidth, jheight,
          {jdata + header_size, jsize - header_size}, 1, 
          {}, 0, bitmap->GetBuffer(), bitmap->GetPitch(), nullptr);
      while (status == FXCODEC_STATUS::kDecodeToBeContinued)
        status = Jbig2Decoder::ContinueDecode(&jbig2_context, nullptr);
      auto pdf_image =
          pdfium::MakeRetain<CPDF_Image>(doc_->GetFakePDFDocument());
      RetainPtr<CFX_DIBitmap> pFlipped = bitmap->FlipImage(false, true);
      pdf_image->SetImage(pFlipped);
      pImageObj = std::make_unique<CPDF_ImageObject>();
      pImageObj->SetImage(pdf_image);
    } else {
      return;
    }
  } else {
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
          bsData = img_data;
          free(hconvert.data);
        }
        FreeImage_Unload(bitmap_org);
      }
    }
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
      pdf_image->SetImage(pFlipped);
      pImageObj = std::make_unique<CPDF_ImageObject>();
      pImageObj->SetImage(pdf_image);
    }
    // else {
    //  auto pdf_image =
    //  pdfium::MakeRetain<CPDF_Image>(doc_->GetFakePDFDocument());
    //  pdf_image->SetJpegImageInline(pImageFileRead);
    //  pImageObj->SetImage(pdf_image);
    //}
    ///*
  }
  if (pImageObj) {
    // parse image border
    float line_width = 0.f, h_radius = 0.f, v_radius = 0.f;
    CFX_XMLElement* pElemBorder = elem->GetFirstChildNamed_NoPrefix(L"Border");
    if (pElemBorder) {
      if (pElemBorder->HasAttribute(L"HorizonalCornerRadius")) {
        h_radius = StringToFloat(
            pElemBorder->GetAttribute(L"HorizonalCornerRadius").AsStringView());
        h_radius = ofd_mm_to_one_72_point(h_radius);
      }
      if (pElemBorder->HasAttribute(L"VerticalCornerRadius")) {
        v_radius = StringToFloat(
            pElemBorder->GetAttribute(L"VerticalCornerRadius").AsStringView());
        v_radius = ofd_mm_to_one_72_point(v_radius);
      }
      if (pElemBorder->HasAttribute(L"LineWidth")) {
        line_width = StringToFloat(
            pElemBorder->GetAttribute(L"LineWidth").AsStringView());
        line_width = ofd_mm_to_one_72_point(line_width);
      } else {
        line_width = 1.f;
      }
    }
    // generate clip path of image
    CPDF_Path clippath;
    GenerateBorderPath(ebbox, h_radius, v_radius, 0.f, clippath);
    pImageObj->m_ClipPath.AppendPathWithAutoMerge(
        clippath, CFX_FillRenderOptions::FillType::kWinding);
    //*/
    CFX_Matrix clip_ctm = ectm;
    auto raw_obj = pImageObj.get();
    ectm.a = ofd_mm_to_one_72_point(ectm.a);
    ectm.d = ofd_mm_to_one_72_point(ectm.d);  //和文字一样有渲染倒置问题
    pImageObj->SetImageMatrix(ectm);
    //
    if (elem->HasAttribute(L"Alpha")) {
      float alpha =
          StringToFloat(elem->GetAttribute(L"Alpha").AsStringView()) / 255;
      pImageObj->m_GeneralState.SetFillAlpha(alpha);
    }
    AppendPageObject(std::move(pImageObj));

    if (pElemBorder && line_width > 0.f) {
      // new path object
      auto pPathObj = std::make_unique<CPDF_PathObject>();
      auto vecPoints = clippath.GetPoints();
      for (auto it : vecPoints) {
        pPathObj->path().AppendPoint(it.m_Point, it.m_Type);
      }
      pPathObj->path().ClosePath();
      // parse draw param 
      if (elem->HasAttribute(L"DrawParam")) {
        auto relative_id =
            StringToFloat(elem->GetAttribute(L"DrawParam").AsStringView());
        auto xml_node = doc_->FindDrawParam(relative_id);
        if (xml_node)
          doc_->ParseDrawParam(xml_node, pPathObj.get());
      }
      doc_->ParseDrawParam(pElemBorder, pPathObj.get());
      // parse border color
      auto bordercolor =
          pElemBorder->GetFirstChildNamed_NoPrefix(L"BorderColor");
      if (bordercolor) {
        float alpha = 1.0f;
        if (bordercolor->HasAttribute(L"Alpha")) {
          alpha =
              StringToFloat(bordercolor->GetAttribute(L"Alpha").AsStringView());
          alpha /= 255.0f;
        } 
        pPathObj->m_GeneralState.SetStrokeAlpha(alpha);
        doc_->ParseColor(bordercolor, pPathObj.get(), false);
      }
      //
      pPathObj->set_stroke(true);
      pPathObj->set_no_filltype();
      pPathObj->m_ClipPath.AppendPathWithAutoMerge(
          clippath, CFX_FillRenderOptions::FillType::kWinding);
      AppendPageObject(std::move(pPathObj));
    }

    ApplyClips(elem, raw_obj, ebbox, clip_ctm);
  }
}

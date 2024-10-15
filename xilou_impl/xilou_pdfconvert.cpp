#include "xilou_public/xilou_convert.h"
#include "xilou_impl/xilou_impl.h"
#include <memory>
#include <string>
#include "core/fxcrt/fx_stream.h"
#include "core/fxcrt/fx_system.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/stl_util.h"
#include "core/fxcrt/unowned_ptr.h"
#include "public/fpdf_edit.h"
#include "public/fpdfview.h"
#include "public/fpdf_save.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "core/fpdfapi/page/cpdf_pageobject.h"
#include "core/fpdfapi/page/cpdf_image.h"
#include "core/fpdfapi/page/cpdf_imageobject.h"
#include "core/fpdfapi/page/cpdf_page.h"
#include "core/fpdfapi/page/cpdf_path.h"
#include "core/fpdfapi/page/cpdf_pathobject.h"
#include "core/fpdfapi/page/cpdf_textobject.h"
#include "core/fpdfapi/page/cpdf_formobject.h"
#include "core/fpdfapi/page/cpdf_form.h"
#include "xilou_impl/xilou_xmlpackage.h"
#include "xilou_impl/xilou_xmlpage.h"
#include "xilou_impl/xilou_xmlpathobj.h"
#include "xilou_impl/xilou_xmltextobj.h"
#include "xilou_impl/xilou_xmlimageobj.h"
#include "core/fpdfdoc/cpdf_annotlist.h"
#include "core/fpdfdoc/cpdf_annot.h"
#include "constants/annotation_common.h"
#include "constants/annotation_flags.h"
#include "core/fpdfapi/page/cpdf_form.h"
using namespace xilou;
namespace xilou {
static bool path_object_convert(CPDF_PathObject* pdf_pathobj,
                                Cxml_page* ofd_page,
                                CPDF_FormObject* parentForm,
                                const CFX_Matrix& ctm) {
  auto xml_pathobj = std::make_unique<Cxml_pathobj>(pdf_pathobj, ofd_page);
  if (parentForm) {
    xml_pathobj->setFromRect(parentForm->GetRect());
    //TODO 这个ctm或许可以不受form这个语义限制
    //目前对象转换时使用insideForm来区分了
    xml_pathobj->setFormMatrix(ctm);
  }
  xml_pathobj->convert();
  ofd_page->appendObj(std::move(xml_pathobj));
  return true;
}

static bool text_object_convert(CPDF_TextObject* pdf_textobj,
                                Cxml_page* ofd_page,
                                CPDF_FormObject* parentForm,
                                const CFX_Matrix& ctm) {
  auto xml_textobj = std::make_unique<Cxml_textobj>(pdf_textobj, ofd_page);
  if (parentForm) {
    xml_textobj->setFromRect(parentForm->GetRect());
    xml_textobj->setFormMatrix(ctm);
  }
  xml_textobj->convert();
  ofd_page->appendObj(std::move(xml_textobj));
  return true;
}

static bool image_object_convert(CPDF_ImageObject* pdf_imageobj,
                                 Cxml_page* ofd_page,
                                 CPDF_FormObject* parentForm,
                                 const CFX_Matrix& ctm) {
  auto xml_imageobj = std::make_unique<Cxml_imageobj>(pdf_imageobj, ofd_page);
  if (parentForm) {
    xml_imageobj->setFromRect(parentForm->GetRect());
    xml_imageobj->setFormMatrix(ctm);
  }
  xml_imageobj->convert();
  ofd_page->appendObj(std::move(xml_imageobj));
  return true;
}

static bool page_object_convert(CPDF_PageObject* pdf_pageobj,
                                Cxml_page* ofd_page,
                                CPDF_FormObject* parentForm,
                                const CFX_Matrix& ctm) {
  bool ret = true;
  switch (pdf_pageobj->GetType()) {
    case CPDF_PageObject::TEXT: {
      ret = text_object_convert(pdf_pageobj->AsText(), ofd_page, parentForm, ctm);
    } break;
    case CPDF_PageObject::PATH: {
      ret = path_object_convert(pdf_pageobj->AsPath(), ofd_page, parentForm, ctm);
    } break;
    case CPDF_PageObject::IMAGE: {
      ret = image_object_convert(pdf_pageobj->AsImage(), ofd_page, parentForm, ctm);
    } break;
    case CPDF_PageObject::SHADING: {
    } break;
    case CPDF_PageObject::FORM: {
      CPDF_FormObject * formObject = pdf_pageobj->AsForm();
      auto form = formObject->form();
      auto it = form->begin();
      for (; it != form->end(); ++it) {
        ret = page_object_convert((*it).get(), ofd_page, formObject, formObject->form_matrix() * ctm);
      }
    } break;
    default:
      break;
  }
  return ret;
}
CPDF_Form* AnnotGetMatrix(const CPDF_Page* pPage,
                          CPDF_Annot* pAnnot,
                          CPDF_Annot::AppearanceMode mode,
                          const CFX_Matrix& mtUser2Device,
                          CFX_Matrix* matrix) {
  CPDF_Form* pForm = pAnnot->GetAPForm(pPage, mode);
  if (!pForm)
    return nullptr;

  CFX_Matrix form_matrix = pForm->GetDict()->GetMatrixFor("Matrix");
  CFX_FloatRect form_bbox =
      form_matrix.TransformRect(pForm->GetDict()->GetRectFor("BBox"));
  matrix->MatchRect(pAnnot->GetRect(), form_bbox);
  matrix->Concat(mtUser2Device);
  return pForm;
}
bool convert_pdf_annot(CPDF_Page* pdf_page, Cxml_page* ofd_page, Cxml_package*pkg) {
  auto annot_list = std::make_unique<CPDF_AnnotList>(pdf_page);
  //TODO:创建pageannot
  auto count = annot_list->Count();
  for (size_t i = 0; i< count; i++)
  {
    CFX_Matrix mtUser2Device = ofd_page->getPDFDisplayCTM();
    CFX_Matrix matrix;
    auto ofd_page_annot = std::make_unique<Cxml_page>(pkg);
    ofd_page_annot->setPDFBox(ofd_page->getPDFBox());
    ofd_page_annot->setPDFDisplayCTM(ofd_page->getPDFDisplayCTM());
    auto pdf_annot = annot_list->GetAt(i);
    ofd_page_annot->setAnnotRect(pdf_annot->GetRect());
    ofd_page_annot->setAnnotSubType(pdf_annot->GetSubtype());
    auto form = AnnotGetMatrix(pdf_page, pdf_annot,
                               CPDF_Annot::AppearanceMode::kNormal,
                               mtUser2Device, &matrix);
    if (!form)
      continue;
    ofd_page_annot->setAnnotCTM(matrix);
    ofd_page_annot->setAnnotBBox(form->GetDict()->GetRectFor("BBox"));
    for (auto it = form->begin(); it != form->end(); ++it) {
      page_object_convert((*it).get(), ofd_page_annot.get(), nullptr, matrix);
    }
    ofd_page->appendPageAnnot(std::move(ofd_page_annot));
  }
  return true;
}
bool pdf2ofd(XILOU_DOCUMENT src_doc,
             XILOU_UTF8STRING dest_fpath,
             XILOU_UTF8STRING page_range) {
  CXilou_ChildDocument* doc = reinterpret_cast<CXilou_ChildDocument*>(src_doc);
  if (!doc)
    return false;
  //CPDF_Document* pdf = CPDFDocumentFromFPDFDocument(doc->pdf);
  auto ofd_packge = std::make_unique<Cxml_package>();
  ofd_packge->setpath(dest_fpath);
  auto pages = FPDF_GetPageCount(doc->pdf);
  for (int i = 0; i < pages; i++) {
    auto fpage = FPDF_LoadPage(doc->pdf, i);
    if (fpage) {
      auto page = CPDFPageFromFPDFPage(fpage);
      const CFX_RectF rect(0, 0, page->GetPageWidth(),
                               page->GetPageHeight());
      //TODO:应该更最大页的尺寸到document.xml中
      ofd_packge->updatePageArea(rect);
      auto page_display_ctm = page->GetDisplayMatrix(rect.GetOuterRect(), 0);
      //page_display_ctm.e = 0;
       auto ofd_page = std::make_unique<Cxml_page>(ofd_packge.get());
       ofd_page->setPDFBox(rect);
       ofd_page->setPDFDisplayCTM(page_display_ctm);
       ofd_page->setPdfPage(fpage);
       ofd_page->setPdfDoc(doc->pdf);
      //创建ofd 页面, 还需要能访问到资源文件页面，更新资源页面
      for (auto it = page->begin(); it != page->end(); ++it) {
         page_object_convert((*it).get(), ofd_page.get(), nullptr,
                             page_display_ctm);
      }
      
      convert_pdf_annot(page, ofd_page.get(), ofd_packge.get());
      ofd_page->setPdfPage(nullptr);
      ofd_page->setPdfDoc(nullptr);
      ofd_packge->appendPage(std::move(ofd_page));
    }
    FPDF_ClosePage(fpage);
  }
  ofd_packge->save();
  return true;
}

}  // namespace xilou


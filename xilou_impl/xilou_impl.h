/**
 * Copyright (C) 2023 chingliu.  All rights reserved.
 *
 * @file		xilou_impl.h
 * @author		余清留(chingliu)
 * @date		2023-5-01
 * @brief		内部类型定义
 */
#ifndef XILOU_IMPL_HEADER_H
#define XILOU_IMPL_HEADER_H
#include "core/fxcrt/fx_stream.h"
#include "public/fpdfview.h"
#include "ofd/cofd_package.h"
#include "ofd/cofd_ofdxml.h"
#include "ofd/cofd_document.h"
#include "ofd/cofd_page.h"
namespace xilou{
enum FileType {
  E_FILETYPE_PDF,
  E_FILETYPE_OFD,
};
class CXilou_Document {
 public:
  explicit CXilou_Document() {
    f.pdf = nullptr;
    f.ofd = nullptr;
  }

 public:
  CXilou_Document(const CXilou_Document&) = delete;
  CXilou_Document(CXilou_Document&&) = delete;
  CXilou_Document& operator=(const CXilou_Document&) = delete;
  CXilou_Document& operator=(CXilou_Document&&) = delete;
  ~CXilou_Document() {
    if (f.pdf)
      FPDF_CloseDocument(f.pdf);
    if (f.ofd) {
      delete f.ofd;
    }
  }

 public:
  FileType m_filetype;
  RetainPtr<IFX_SeekableStream> m_file_stream;
  struct {
    FPDF_DOCUMENT pdf;
    ofd::COFD_Package* ofd;
  } f;
};

class CXilou_ChildDocument {
 public:
  CXilou_ChildDocument() : pdf(nullptr), ofd(nullptr) {}
  FPDF_DOCUMENT pdf;  // pdf未支持多文档，此句柄由Package管理
  ofd::COFD_Document* ofd;
  ~CXilou_ChildDocument() {
    if (ofd) {
      ofd::COFD_Package* pkg = ofd->pkg_;
      pkg->CloseDocument(ofd);
    }
  }
};


class CXilou_Page {
 public:
  CXilou_Page() : pdf_page(nullptr), ofd_page(nullptr) {}
  FPDF_PAGE pdf_page;
  ofd::COFD_Page* ofd_page;
  ~CXilou_Page() {
    if (pdf_page) {
      FPDF_ClosePage(pdf_page);
    }
    if (ofd_page) {
      //delete ofd_page;
      auto ofd_doc = ofd_page->GetDocument();
      ofd_doc->DropPage(ofd_page);
    }
  }
};

}  // namespace

#endif
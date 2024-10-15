#ifndef COFD_PACKAGE_H_HEADER
#define COFD_PACKAGE_H_HEADER

#include "ofd/cofd_zipentry.h"
#include "ofd/cofd_document.h"
class CPDF_Document;
namespace ofd {

class COFD_ParseOfdXml;
class COFD_Package {
 public:
  explicit COFD_Package(const char* zip_path);
  explicit COFD_Package(const char* stream, size_t size);

 public:
  COFD_Package(const COFD_Package&) = delete;
  COFD_Package(COFD_Package&&) = delete;
  COFD_Package& operator=(const COFD_Package&) = delete;
  COFD_Package& operator=(COFD_Package&&) = delete;
  ~COFD_Package();

public:
  bool ParseOFD();
 // TODO, 现在是返回了new的指针，应该使用智能指针的方式管理
  COFD_Document* OpenDocument(unsigned int index = 0);
  void CloseDocument(COFD_Document*doc);
  CFX_XMLDocument* ReadAndCacheXml(ByteStringView entry_name);
  int ReadBinary(ByteStringView entry_name, std::vector<uint8_t>* buf);
 public:
  CPDF_Document* GetFakePDFDocument();

 private:
  COFD_ZipEntry zip_;
  std::unique_ptr<COFD_ParseOfdXml> ofd_xml_;
  std::unique_ptr < CPDF_Document> fake_pdf_doc_handle_;
        };
}
#endif  // !COFD_PACKAGE_H_HEADER

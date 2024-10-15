#include "ofd/cofd_zipentry.h"
#include <vector>
#include "core/fxcrt/xml/cfx_xmlparser.h"
#include "core/fxcrt/xml/cfx_xmldocument.h"
#include "core/fxcrt/xml/cfx_xmlelement.h"
#include "core/fxcrt/cfx_readonlymemorystream.h"
#include "third_party/spdlog/include/spdlog/spdlog.h"
#include <errno.h>
using namespace ofd;


COFD_ZipEntry::COFD_ZipEntry(const char* zip_path)
    : zip_path_(zip_path),
      zip_handler_(nullptr){
  //cached_entries_() = {};
  //zip_entries_name_() = {};

}
COFD_ZipEntry::COFD_ZipEntry(const char* stream, size_t size) {
  zip_content_.resize(size + 10);
  memcpy(&zip_content_[0], stream, size);
}
COFD_ZipEntry::~COFD_ZipEntry() {
  if (zip_content_.size() > 0 && zip_handler_) {
    zip_stream_close(zip_handler_);
    zip_handler_ = nullptr;
  }
  if (zip_handler_) {
    zip_close(zip_handler_);
  }
}
bool COFD_ZipEntry::Open(char mode) {
  int level = 0;
  if (mode == 'w')
    level = 9;
  if (zip_path_.GetStringLength() > 1) {
    zip_handler_ = zip_open(reinterpret_cast<const char*>(zip_path_.raw_str()),
                            level, mode);
  }
  if (zip_content_.size() > 4) {
    zip_handler_ =
        zip_stream_open(&zip_content_[0], zip_content_.size(), level, mode);
  }
  if (!zip_handler_) {
    auto error_no = errno;
    spdlog::error("[COFD_ZipEntry::Open]failed to open {}; errno ={}",
                  zip_path_.c_str(), error_no);
    return false;
  }
  int i, n = zip_entries_total(zip_handler_);
  for (i = 0; i < n; ++i) {
    if(0 == zip_entry_openbyindex(zip_handler_, i))
    {
      const char* name = zip_entry_name(zip_handler_);
      if (name) {
        // int isdir = zip_entry_isdir(zip_handler_);
        // unsigned long long size = zip_entry_size(zip_handler_);
        // unsigned int crc32 = zip_entry_crc32(zip_handler_);
        zip_entries_name_.insert(name);
      }
      zip_entry_close(zip_handler_);
    }
   
  }
  return zip_handler_ != nullptr;
}

CFX_XMLDocument* COFD_ZipEntry::ReadAndCacheXml(
    ByteStringView entry_name) {
  ByteStringView zip_entry_name = entry_name;
  if (entry_name.GetLength() <= 0)
    return nullptr;
  if (entry_name.CharAt(0) == '/')
    zip_entry_name = entry_name.Substr(1, entry_name.GetLength() -1);
  auto cached = cached_entries_.find(ByteString(zip_entry_name));
  if (cached != cached_entries_.end()) {
    return cached->second.get();
  }
  if (zip_entry_open(zip_handler_, zip_entry_name.unterminated_c_str()) < 0)
    return nullptr;
  std::vector<uint8_t> buf;
  size_t len = zip_entry_size(zip_handler_);
  buf.resize(len);
  zip_entry_noallocread(zip_handler_, (void*)buf.data(), len);
  zip_entry_close(zip_handler_);
  CFX_XMLParser parser(
      pdfium::MakeRetain<CFX_ReadOnlyMemoryStream>(buf));
  CFX_XMLDocument* ret = nullptr;
  auto xml = parser.Parse();
  if (xml) {
    ret = xml.get();
    cached_entries_.insert(
        std::make_pair(ByteString(zip_entry_name), std::move(xml)));
  }
  return ret;
}

int COFD_ZipEntry::ReadBinary(ByteStringView entry_name,
                              std::vector<uint8_t>* buf) {
  ByteStringView zip_entry_name = entry_name;
  if (entry_name.GetLength() <= 0)
    return -1;
  if (entry_name.CharAt(0) == '/')
    zip_entry_name = entry_name.Substr(1, entry_name.GetLength() - 1);

  if (zip_entry_open(zip_handler_, zip_entry_name.unterminated_c_str()) < 0)
    return -1;
  size_t len = zip_entry_size(zip_handler_);
  buf->resize(len);
  zip_entry_noallocread(zip_handler_, (void*)buf->data(), len);
  zip_entry_close(zip_handler_);
  return (int)len;
}

int COFD_ZipEntry::WriteBinary(ByteStringView entry_name,
                               std::vector<uint8_t>* buf) {
  ByteStringView zip_entry_name = entry_name;
  if (entry_name.GetLength() <= 0)
    return -1;
  if (entry_name.CharAt(0) == '/')
    zip_entry_name = entry_name.Substr(1, entry_name.GetLength() - 1);

  if (zip_entry_open(zip_handler_, zip_entry_name.unterminated_c_str()) < 0)
    return -1;
  auto wret = zip_entry_write(zip_handler_, (void*)buf->data(), buf->size());
  wret += zip_entry_close(zip_handler_);
  return wret;
}

int COFD_ZipEntry::DeleteEntries(std::vector<WideString> entries) {
  std::vector<const char*> entries_;
  std::vector<ByteString> tmp;
  for (auto& entry : entries) {
    auto entry_name = entry.ToUTF8();
    if (entry_name[0] == '/')
      tmp.push_back(entry_name.Substr(1, entry_name.GetLength() - 1));
    else
      tmp.push_back(entry_name);
  }
  for (auto& entry : tmp) {
    entries_.push_back(entry.c_str());
  }
  return zip_entries_delete(zip_handler_, (char* const*)(&entries_[0]),
                     entries_.size());
  
}
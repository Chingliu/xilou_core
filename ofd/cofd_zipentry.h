#pragma once
#ifndef COFD_ZIPENTRY_H
#define COFD_ZIPENTRY_H
#include<vector>
#include<unordered_set>
#include<unordered_map>
#include<memory>
#include "core/fxcrt/retain_ptr.h"
#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/string_view_template.h"
#include "core/fxcrt/xml/cfx_xmldocument.h"
#include "third_party/zip/src/zip.h"

namespace ofd {
class COFD_ZipEntry {
public:
  explicit COFD_ZipEntry(const char* zip_path);
  explicit COFD_ZipEntry(const char* stream, size_t size);

public:
  COFD_ZipEntry(const COFD_ZipEntry&) = delete;
  COFD_ZipEntry(COFD_ZipEntry&&) = delete;
  COFD_ZipEntry& operator=(const COFD_ZipEntry&) = delete;
  COFD_ZipEntry& operator=(COFD_ZipEntry&&) = delete;
  ~COFD_ZipEntry();

 public:
  bool Opened() { return zip_handler_ != nullptr; }
  bool Open(char mode);
  CFX_XMLDocument* ReadAndCacheXml(ByteStringView entry_name);
  //entry_name是带路径的
  int ReadBinary(ByteStringView entry_name, std::vector<uint8_t>* buf);
  //返回0是正确
  int WriteBinary(ByteStringView entry_name, std::vector<uint8_t>* buf);
  const std::unordered_set<ByteString>& entries() { return zip_entries_name_; }
  int DeleteEntries(std::vector<WideString> entries);//需要open时以'd'打开
 private:
  ByteString zip_path_;
  struct zip_t* zip_handler_;
  std::unordered_map<ByteString, std::unique_ptr<CFX_XMLDocument>> cached_entries_;
  std::unordered_set<ByteString> zip_entries_name_;
  std::vector<char> zip_content_;
};
}  // namespace ofd

#endif  // !COFD_ZIPENTRY_H

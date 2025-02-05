// Copyright 2018 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_FPDFAPI_PARSER_CPDF_OBJECT_STREAM_H_
#define CORE_FPDFAPI_PARSER_CPDF_OBJECT_STREAM_H_

#include <map>
#include <memory>

#include "core/fpdfapi/parser/cpdf_object.h"
#include "core/fxcrt/retain_ptr.h"

class CPDF_IndirectObjectHolder;
class CPDF_Stream;
class IFX_SeekableReadStream;

// Implementation of logic of PDF "Object Streams".
// See ISO 32000-1:2008 spec, section 7.5.7.
class CPDF_ObjectStream {
 public:
  static std::unique_ptr<CPDF_ObjectStream> Create(const CPDF_Stream* stream);

  ~CPDF_ObjectStream();

  bool HasObject(uint32_t obj_number) const;
  RetainPtr<CPDF_Object> ParseObject(CPDF_IndirectObjectHolder* pObjList,
                                     uint32_t obj_number) const;
  const std::map<uint32_t, uint32_t>& objects_offsets() const {
    return objects_offsets_;
  }

 private:
  explicit CPDF_ObjectStream(const CPDF_Stream* stream);

  void Init(const CPDF_Stream* stream);
  RetainPtr<CPDF_Object> ParseObjectAtOffset(
      CPDF_IndirectObjectHolder* pObjList,
      uint32_t object_offset) const;

  RetainPtr<IFX_SeekableReadStream> data_stream_;
  int first_object_offset_ = 0;
  std::map<uint32_t, uint32_t> objects_offsets_;
};

#endif  // CORE_FPDFAPI_PARSER_CPDF_OBJECT_STREAM_H_

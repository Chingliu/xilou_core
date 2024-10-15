/**
 * Copyright (C) 2023 chingliu.  All rights reserved.
 *
 * @file		xilou_signature_iface.h
 * @author		余清留(chingliu)
 * @date		2023-11-03
 * @brief		签章接口定义
 */
#ifndef XILOU_IMPL_SIGNATURE_IFACE_HEADER_H
#define XILOU_IMPL_SIGNATURE_IFACE_HEADER_H

namespace xilou {
  class SignatureIface {
  public:
  virtual ~SignatureIface() = default;
   /////////////////////////////////////////////////////////////////////////////////////
   /// @brief		<tt><b>签章验证</b></tt>
   /// @return XILOU_E_SUC 表示有效，其它为错误码参见xilou_errcode.h
   /// @since
   /// @author		chingliu
   /// @note  
   virtual int verify() = 0;
  virtual size_t errmsg(unsigned char **errmsg) = 0;
  };
}//end of namespace
#endif
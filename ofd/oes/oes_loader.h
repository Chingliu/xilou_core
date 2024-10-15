#pragma once

#ifndef XILOU_OFD_OES_LOADER_HEADER_H
#define XILOU_OFD_OES_LOADER_HEADER_H
#include "ofd/oes/oes_util.h"
namespace OES {
const int OES_E_NO_ENV = 10001;
const int OES_E_LOAD_ERR = 10002;


#ifndef OES_API
#ifdef WIN32
#define OES_API __stdcall
#else
#define OES_API
#endif
#endif
#ifndef OES_HANDLE
#define OES_HANDLE void*
#endif



#ifdef __cplusplus
extern "C" {
#endif
/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:获取库提供者信息</b></tt>
/// @param[out]	name 库名
/// @param[out]	name_len 库名字符串长度
/// @param[out]	company 单位名
/// @param[out]	company_len 单位名字符串长度
/// @param[out]	version 版本
/// @param[out]	version_len 版本字符串长度
/// @param[out]	extend 扩展
/// @param[out]	extend_len 扩展字符串长度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_GetProviderInfo)(unsigned char* name,
                                           int* name_len,
                                           unsigned char* company,
                                           int* company_len,
                                           unsigned char* version,
                                           int* version_len,
                                           unsigned char* extend,
                                           int* extend_len);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:获取签章列表</b></tt>
/// @param[out]	seal_list_data 签章列表数据
/// @param[out]	seal_list_data_len 签章列表数据长度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_GetSealList)(unsigned char* seal_list_data,
                                       int* seal_list_data_len);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:获取签章数据</b></tt>
/// @param[in]	seal_id_or_name 签章ID或者名字
/// @param[in]	seal_id_or_name_len 签章ID或者名字字符串的长度
/// @param[out]	seal_data 签章数据
/// @param[out]	seal_data_len 签章数据长度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_GetSeal)(unsigned char* seal_id_or_name,
                                   int seal_id_or_name_len,
                                   unsigned char* seal_data,
                                   int* seal_data_len);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:获取签章图片</b></tt>
/// @param[in]	seal_data 签章数据
/// @param[in]	seal_data_len 签章数据长度
/// @param[out]	render_flag 单位名
/// @param[out]	seal_image 签章图片数据
/// @param[out]	seal_image_len 签章图片数据长度
/// @param[out]	seal_width 图片宽度
/// @param[out]	seal_height 图片高度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_GetSealImage)(unsigned char* seal_data,
                                        int seal_data_len,
                                        int render_flag,
                                        unsigned char* seal_image,
                                        int* seal_image_len,
                                        int* seal_width,
                                        int* seal_height);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:获取签章图片</b></tt>
/// @param[in]	seal_data 签章数据
/// @param[in]	seal_data_len 签章数据长度
/// @param[out]	render_flag 单位名
/// @param[out]	seal_image 签章图片数据
/// @param[out]	seal_image_len 签章图片数据长度
/// @param[out]	seal_width 图片宽度
/// @param[out]	seal_height 图片高度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_GetSignImage)(unsigned char* seal_data,
                                        int seal_data_len,
                                        int render_flag,
                                        unsigned char* seal_image,
                                        int* seal_image_len,
                                        int* seal_width,
                                        int* seal_height);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:获取盖章当前时间</b></tt>
/// @param[out]	sign_date_time 当前时间
/// @param[out]	sign_date_time_len 时间字符串长度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_GetSignDateTime)(unsigned char* sign_date_time,
                                           int* sign_date_time_len);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:获取签名算法</b></tt>
/// @param[out]	sign_method 签名算法
/// @param[out]	sign_method_len 签名算法长度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_GetSignMethod)(unsigned char* sign_method,
                                         int* sign_method_len);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:获取杂凑算法</b></tt>
/// @param[out]	digest_method 杂凑算法
/// @param[out]	digest_method_len 杂凑算法长度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_GetDigestMethod)(unsigned char* digest_method,
                                           int* digest_method_len);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:数据杂凑接口</b></tt>
/// @param[in]	data 待杂凑的数据
/// @param[in]	data_len 待杂凑的数据长度
/// @param[in]	digest_method 杂凑算法
/// @param[in]	digest_method_len 杂凑算法长度
/// @param[out]	digest_data 杂凑值
/// @param[out]	digest_data_len 杂凑值长度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_Digest)(unsigned char* data,
                                  int data_len,
                                  unsigned char* digest_method,
                                  int digest_method_len,
                                  unsigned char* digest_data,
                                  int* digest_data_len);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:获取电子签章信息</b></tt>
/// @param[in]	seal_data 签章数据
/// @param[in]	seal_data_len 签章数据长度
/// @param[out]	seal_id 签章ID
/// @param[out]	seal_id_len 签章ID长度
/// @param[out]	version 版本
/// @param[out]	version_len 版本长度
/// @param[out]	vender_id 制造商ID
/// @param[out]	vender_id_len 制造商ID长度
/// @param[out]	seal_type 签章类型
/// @param[out]	seal_type_len 签章类型长度
/// @param[out]	seal_name 签章名
/// @param[out]	seal_name_len 签章名长度
/// @param[out]	cert_info 证书信息
/// @param[out]	cert_info_len 证书信息长度
/// @param[out]	valid_start
/// @param[out]	valid_start_len
/// @param[out]	valid_end
/// @param[out]	valid_end_len
/// @param[out]	signed_date 制章日期
/// @param[out]	signed_date_len 制章日期字符串长度
/// @param[out]	signer_name 制章人
/// @param[out]	signer_name_len 制章人字符串长度
/// @param[out]	sign_method 签章算法
/// @param[out]	sign_method_len 签章算法长度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_GetSealInfo)(unsigned char* seal_data,
                                       int seal_data_len,
                                       unsigned char* seal_id,
                                       int* seal_id_len,
                                       unsigned char* version,
                                       int* version_len,
                                       unsigned char* vender_id,
                                       int* vender_id_len,
                                       unsigned char* seal_type,
                                       int* seal_type_len,
                                       unsigned char* seal_name,
                                       int* seal_name_len,
                                       unsigned char* cert_info,
                                       int* cert_info_len,
                                       unsigned char* valid_start,
                                       int* valid_start_len,
                                       unsigned char* valid_end,
                                       int* valid_end_len,
                                       unsigned char* signed_date,
                                       int* signed_date_len,
                                       unsigned char* signer_name,
                                       int* signer_name_len,
                                       unsigned char* sign_method,
                                       int* sign_method_len);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:电子签章盖章接口</b></tt>
/////////////////////////////////////////////////////////////////////////////////////
typedef unsigned long(OES_API* _OES_Sign)(unsigned char* seal_id,
                                          int seal_id_len,
                                          unsigned char* doc_property,
                                          int doc_property_len,
                                          unsigned char* digest_data,
                                          int digest_data_len,
                                          unsigned char* sign_method,
                                          int sign_method_len,
                                          unsigned char* sign_date_time,
                                          int sign_date_time_len,
                                          unsigned char* sign_value,
                                          int* sign_value_len);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:电子签章验章接口</b></tt>
/////////////////////////////////////////////////////////////////////////////////////
typedef unsigned long(OES_API* _OES_Verify)(unsigned char* seal_data,
                                            int seal_data_len,
                                            unsigned char* doc_property,
                                            int doc_property_len,
                                            unsigned char* digest_data,
                                            int digest_data_len,
                                            unsigned char* sign_method,
                                            int sign_method_len,
                                            unsigned char* sign_date_time,
                                            int sign_date_time_len,
                                            unsigned char* sign_value,
                                            int sign_value_len,
                                            int iOnline);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:获取OES操作错误信息</b></tt>
/// @param[in]	code OES错误码，参见\link krc_errorcode_def.h\endlink
/// @param[out]	error_msg 错误信息
/// @param[out]	error_msg_len 错误信息长度
/// @return		OES错误码
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_GetErrMessage)(unsigned long code,
                                         unsigned char* error_msg,
                                         int* error_msg_len);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:电子签章删章接口</b></tt>
/// @note		GB/T 33481之外的接口
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_CancelSeal)(unsigned char* puchSignValue,
                                      int iSignValueLen);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:登入</b></tt>
/// @note		GB/T 33481之外的接口
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_Login)(unsigned char* puchPIN, int iPINLen);

/////////////////////////////////////////////////////////////////////////////////////
/// @brief		<tt><b>OES接口:退出</b></tt>
/// @note		GB/T 33481之外的接口
/////////////////////////////////////////////////////////////////////////////////////
typedef int(OES_API* _OES_Logout)();

/// @} OES库接口


    /**
 * @brief 返回签章组件提供者(OES实现厂商)信息接口 [Required]
 * @param[out]     puchName
 * 出参，签章组件名称（UTF-8编码字符串）应取值为“商标+应用领域”，例如“卫士通-国密1”，当为NULL时，通过piNameLen给出长度
 * @param[out/in]  piNameLen		出参/入参，签章组件名称长度
 * @param[out]     puchCompany
 * 出参，签章组件公司名称（UTF-8编码字符串），当为NULL时，通过piCompanyLen给出长度
 * @param[out/in]  piCompanyLen		出参/入参，签章组件公司名称长度
 * @param[out]     puchVersion
 * 出参，签章组件版本（UTF-8编码字符串），当为NULL时，通过piVersionLen给出长度
 * @param[out/in]  piVersionLen		出参/入参，签章组件版本长度
 * @param[out]     puchExtend
 * 出参，签章组件扩展信息（UTF-8编码字符串），当为NULL时，通过piExtendLen给出长度
 * @param[out/in]  piExtendLen		出参/入参，签章组件扩展信息长度
 * @return 调用成功返回OES_OK，一般不返回错误代码。
 */
typedef int(OES_API* _OESV4_GetProviderInfo)(unsigned char* puchName,
                                             int* piNameLen,
                                             unsigned char* puchCompany,
                                             int* piCompanyLen,
                                             unsigned char* puchVersion,
                                             int* piVersionLen,
                                             unsigned char* puchExtend,
                                             int* piExtendLen);

/**
 * @brief 返回签章组件的接口协议版本号 [Required]
 * @param[out]     puchVersion
 * 出参，OES接口协议版本（UTF-8编码字符串，固定为“4”），当为NULL时，通过piVersionLen给出长度
 * @param[out/in]  piVersionLen  出参/入参，OES接口协议版本长度
 *                               例如如果要返回“4.0”，则Len应取值为3；
 * @return 调用成功返回OES_OK，一般不返回错误代码。
 */
typedef int(OES_API* _OESV4_GetProtocolVersion)(unsigned char* puchVersion,
                                                int* piVersionLen);

/**
 * @brief 开始线程  [Required]
 * @param[out]     piSession  出参，线程句柄
 * @return 调用成功返回OES_OK，一般不返回错误代码。
 */
typedef int(OES_API* _OESV4_OpenSession)(OES_HANDLE* piSession);

/**
 * @brief 结束线程 [Required]
 * @param[in]      iSession  入参，线程句柄
 * @return 调用成功返回OES_OK，一般不返回错误代码。
 */
typedef int(OES_API* _OESV4_CloseSession)(OES_HANDLE iSession);

/**
 * @brief 从签章组件中获取可用的电子印章列表，可用来进行印章名称到标识的转换
 * [Required] 实现时务必使用实际内容按编码计算后的长度，禁止使用预估长度
 * 后台调用时，不建议调用
 * @param[in]      iSession				   入参，线程句柄
 * @param[out]     puchSealList
 * 出参，印章列表（UTF-8编码字符串），当为NULL时，通过piSealListLen给出长度 形如
 * “ID1\0Name1\0ID2\0Name2\0\0”，注意以两个\0结尾。其中最后一个以外的‘\0’可使用‘;’代替
 * @param[out/in]  piSealListLen           出参/入参，印章列表长度
 * @return 调用成功返回OES_OK，一般不返回错误代码。
 */
typedef int(OES_API* _OESV4_GetSealList)(OES_HANDLE iSession,
                                         unsigned char* puchSealList,
                                         int* piSealListLen);

/**
 * @brief
 * 从签章组件中获取可用的签署人身份列表，可用来进行签署人名称（CN项）到密钥对标识的转换
 * [Required] 实现时务必使用实际内容按编码计算后的长度，禁止使用预估长度
 * 后台调用时，不建议调用
 * @param[in]      iSession				   入参，线程句柄
 * @param[out]     puchCertList
 * 出参，签署人列表（UTF-8编码字符串），当为NULL时，通过piCertListLen给出长度
 *                                         形如
 * ID1\0CN_Name1\0ID2\0CN_Name2\0\0，注意以两个\0结尾。其中最后一个以外的‘\0’可使用‘;’代替
 * @param[out/in]  piCertListLen           出参/入参，签署人列表长度
 * @return 调用成功返回OES_OK，一般不返回错误代码。
 */
typedef int(OES_API* _OESV4_GetCertList)(OES_HANDLE iSession,
                                         unsigned char* puchCertList,
                                         int* piCertListLen);

/**
 * @brief 从签章组件中获取指定的印章图像及格式和尺寸信息  [Required]
 * @param[in]      iSession				    入参，线程句柄
 * @param[in]      puchSealId 入参，印章标识或名称（UTF-8编码字符串）
 * @param[in]      iSealIdLen               入参，印章标识或名称长度
 * @param[out]     puchImageData
 * 出参，印章图像数据（二进制值），当为NULL时，通过piImageDataLen给出长度
 * @param[out/in]  piImageDataLen           出参/入参，印章图像数据长度
 * @param[out]     puchImageType
 * 出参，印章图像数据格式（UTF-8编码字符串，取值如"GIF"、"BMP"、"JPG"等）
 * @param[out/in]  piImageTypeLen
 * 出参/入参，印章图像数据格式长度，如想返回"GIF"，对应的长度为3
 * @param[out]	   piSealWidth              出参，印章宽度（单位mm）
 * @param[out]     piSealHeight             出参，印章高度（单位mm）
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef int(OES_API* _OESV4_GetSealImageById)(OES_HANDLE iSession,
                                              unsigned char* puchSealId,
                                              int iSealIdLen,
                                              unsigned char* puchImageData,
                                              int* piImageDataLen,
                                              unsigned char* puchImageType,
                                              int* piImageTypeLen,
                                              int* piSealWidth,
                                              int* piSealHeight);

/**
 * @brief 从签章组件中获取指定标识的电子印章相关信息  [Required]
 * @param[in]      iSession				  入参，线程句柄
 * @param[in]      puchSealId             入参，电子印章标识（UTF-8编码字符串）
 * @param[in]      iSealIdLen             入参，电子印章标识长度
 * @param[out]     puchID
 * 出参，头信息-电子印章数据标识（UTF-8编码字符串，固定为"ES"），当为NULL时，通过piIDLen给出长度
 * @param[out/in]  piIDLen
 * 出参/入参，头信息-电子印章数据标识长度
 * @param[out]     puchVersion
 * 出参，头信息-版本（UTF-8编码字符串），当为NULL时，通过piVersionLen给出长度
 * @param[out/in]  piVersionLen           出参/入参，头信息-版本数据长度
 * @param[out]     puchVenderId
 * 出参，头信息-厂商（UTF-8编码字符串），当为NULL时，通过piVenderIdLen给出长度
 * @param[out/in]  piVenderIdLen          出参/入参，头信息-厂商长度
 * @param[out]     piSealType
 * 出参，印章信息-印章类型（UTF-8编码字符串，如"1","2"），当为NULL时，通过piSealTypeLen给出长度
 * @param[out/in]  piSealTypeLen          出参/入参，印章信息-印章类型长度
 * @param[out]     puchSealName
 * 出参，印章信息-印章名称（UTF-8编码字符串），当为NULL时，通过piSealNameLen给出长度
 * @param[out/in]  piSealNameLen          出参/入参，印章信息-印章名称长度
 * @param[out]     puchCertList
 * 出参，印章信息-签章人证书或证书摘要值列表（二进制值）。当为NULL时，通过piCertInfoLen给出长度
 * @param[out/in]  piCertListLen
 * 出参/入参，印章信息-签章人证书或证书摘要值列表长度
 * @param[out]     puchSignedDate
 * 出参，印章信息-制作日期（UTF-8编码字符串，如“20180101115959Z”），当为NULL时，通过piSignedDateLen给出长度
 * @param[out/in]  piSignedDateLen        出参/入参，印章信息-制作日期长度
 * @param[out]     puchValidStart
 * 出参，印章信息-有效起始时间（UTF-8编码字符串，如“20180101115959Z”），当为NULL时，通过piValidStartLen给出长度
 * @param[out/in]  piValidStartLen        出参/入参，印章信息-有效起始时间长度
 * @param[out]     puchValidEnd
 * 出参，印章信息-有效结束时间（UTF-8编码字符串，如“20180101115959Z”），当为NULL时，通过piValidEndLen给出长度
 * @param[out/in]  piValidEndLen          出参/入参，印章信息-有效结束长度
 * @param[out]     puchSignerName
 * 出参，签名信息-制章人证书，也可是为空数组（二进制值）。当为NULL时，通过piSignerNameLen给出长度
 * @param[out/in]  piSignerNameLen        出参/入参，签名信息-制章人证书长度
 * @param[out]     puchSignMethod
 * 出参，签名信息-制章签名方法（UTF-8编码字符串，算法OID），当为NULL时，通过piSignMethodLen给出长度
 * @param[out/in]  piSignMethodLen        出参/入参，签名信息-制章签名方法长度
 * @param[out]     puchSignature
 * 出参，签名信息-签名值（二进制值），当为NULL时，通过piSignatureLen给出长度
 * @param[out/in]  piSignatureLen         出参/入参，签名信息-签名值长度
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef int(OES_API* _OESV4_GetSealInfoById)(OES_HANDLE iSession,
                                             unsigned char* puchSealId,
                                             int iSealIdLen,
                                             unsigned char* puchVersion,
                                             int* piVersionLen,
                                             unsigned char* puchVenderId,
                                             int* piVenderIdLen,
                                             unsigned char* puchSealType,
                                             int* piSealTypeLen,
                                             unsigned char* puchSealName,
                                             int* piSealNameLen,
                                             unsigned char* puchCertList,
                                             int* piCertListLen,
                                             unsigned char* puchSignedDate,
                                             int* piSignedDateLen,
                                             unsigned char* puchValidStart,
                                             int* piValidStartLen,
                                             unsigned char* puchValidEnd,
                                             int* piValidEndLen,
                                             unsigned char* puchSignerName,
                                             int* piSignerNameLen,
                                             unsigned char* puchSignMethod,
                                             int* piSignMethodLen,
                                             unsigned char* puchSignature,
                                             int* piSignatureLen);

/**
 * @brief 从签章组件中获取指定的公钥证书  [Required]
 * @param[in]      iSession				    入参，线程句柄
 * @param[in]      puchCertId 入参，证书标识或名称（UTF-8编码字符串）
 * @param[in]      iCertIdLen               入参，证书标识或名称长度
 * @param[out]     puchCertData
 * 出参，证书数据（二进制值），当为NULL时，通过piCertDataLen给出长度
 * @param[out/in]  piCertDataLen            出参/入参，证书数据长度
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef int(OES_API* _OESV4_GetCertById)(OES_HANDLE iSession,
                                         unsigned char* puchCertId,
                                         int iCertIdLen,
                                         unsigned char* puchCertData,
                                         int* piCertDataLen);

/**
 * @brief 从电子签章数据中获取印章图像及尺寸  [Required]
 * @param[in]      iSession				      入参，线程句柄
 * @param[in]      puchSignedValue
 * 入参，签章数据（二进制值，符合国密标准，见GM/T 0031 6.2节）
 * @param[in]      iSignedValueLen            入参，签章数据长度
 * @param[in]      puchExtendParam
 * 入参，扩展参数（UTF-8编码字符串），例如可取值为{RenderFlag:1;Extend_N:value_N}，注：RenderFlag取值含义见OES_SEALIMAGE_FLAG_XXX常量定义
 *                                            Extend_N留作扩展，如调用双方确需使用该协议，需向联盟备案
 * @param[in]      iExtendParamLen            入参，扩展参数长度
 * @param[out]     puchPictureData
 * 出参，印章图像数据（二进制值），当为NULL时，通过piSealImageLen给出长度
 * @param[out/in]  piPictureDataLen           出参/入参，印章图像数据长度
 * @param[out]     puchPictureType
 * 出参，印章图像数据格式（UTF-8编码字符串，如"GIF"、"BMP"、"JPG"等）
 * @param[out/in]  piPictureTypeLen
 * 出参/入参，印章图像数据格式长度，返回"GIF"时，对应长度为3
 * @param[out]	   piPictureWidth             出参，印章图像宽度（单位mm）
 * @param[out]     piPictureHeight            出参，印章图像高度（单位mm）
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef int(OES_API* _OESV4_GetSignImage)(OES_HANDLE iSession,
                                          unsigned char* puchSignedValue,
                                          int iSignedValueLen,
                                          unsigned char* puchExtendParam,
                                          int iExtendParamLen,
                                          unsigned char* puchPictureData,
                                          int* piPictureDataLen,
                                          unsigned char* puchPictureType,
                                          int* piPictureTypeLen,
                                          int* piPictureWidth,
                                          int* piPictureHeight);
/**
 * @brief 从电子印章数据中代理解析出来印章图像数据（仅用于兼容历史文件）
 * [Optional]
 * @param[in]      iSession					  入参，线程句柄
 * @param[in]      puchSealData 入参，印章数据（二进制值，符合国密标准，见GM/T
 * 0031 6.1节）
 * @param[in]      iSealDataLen               入参，印章数据长度
 * @param[in]      puchExtendParam
 * 入参，扩展参数（UTF-8编码字符串），例如可取值为{RenderFlag:1;Extend_N:value_N}，注：RenderFlag取值含义见OES_SEALIMAGE_FLAG_XXX常量定义
 * @param[in]      iExtendParamLen            入参，扩展参数长度
 * @param[out]     puchSealPicture
 * 出参，印章图像数据（二进制值），当为NULL时，通过piSealImageLen给出长度
 * @param[out/in]  piSealPictureLen           出参/入参，印章图像数据长度
 * @param[out]     puchPictureType
 * 出参，印章图像数据格式（UTF-8编码字符串，如"GIF"、"BMP"、"JPG"等）
 * @param[out/in]  piPictureTypeLen
 * 出参/入参，印章图像数据格式长度，返回"GIF"时，对应长度为3
 * @param[out]     piPictureWidth             出参，印章图像宽度（单位mm）
 * @param[out]     piPictureHeight            出参，印章图像高度（单位mm）
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef int(OES_API* _OESV4_GetSealImage)(OES_HANDLE iSession,
                                          unsigned char* puchSealData,
                                          int iSealDataLen,
                                          unsigned char* puchExtendParam,
                                          int iExtendParamLen,
                                          unsigned char* puchSealPicture,
                                          int* piSealPictureLen,
                                          unsigned char* puchPictureType,
                                          int* piPictureTypeLen,
                                          int* piPictureWidth,
                                          int* piPictureHeight);

/**
 * @brief 从电子签章数据中获取相关信息  [Required]
 * @param[in]      iSession				    出参/入参，线程句柄
 * @param[in]      puchSignedValue
 * 出参/入参，签章数据（二进制值，符合国密标准，见GM/T0031 6.2节）
 * @param[in]      iSignedValueLen          出参/入参，签章数据长度
 * @param[out]     puchVersion
 * 出参，签章数据版本（UTF-8编码字符串），当为NULL时，通过piVersionLen给出长度
 * @param[out/in]  piVersionLen 出参/入参，签章数据版本长度
 * @param[out]     puchSealId
 * 出参，印章-标识（UTF-8编码字符串）
 * @param[out/in]  iSealIdLen				出参/入参，印章-标识长度
 * @param[out]     puchVersion
 * 出参，印章-印章数据版本（UTF-8编码字符串），当为NULL时，通过piVersionLen给出长度
 * @param[out/in]  piVersionLen             出参/入参，印章-印章数据版本长度
 * @param[out]     puchVenderId
 * 出参，印章-厂商标识（UTF-8编码字符串），当为NULL时，通过piVenderIdLen给出长度
 * @param[out/in]  piVenderIdLen            出参/入参，印章-厂商标识长度
 * @param[out]     puchSealType
 * 出参，印章-类型（UTF-8编码字符串，如"1"、"2"），当为NULL时，通过piSealTypeLen给出长度
 * @param[out/in]  piSealTypeLen            出参/入参，印章-类型长度
 * @param[out]     puchSealName
 * 出参，印章-名称（UTF-8编码字符串），当为NULL时，通过piSealNameLen给出长度
 * @param[out/in]  piSealNameLen            出参/入参，印章-名称长度
 * @param[out]     puchCertInfo
 * 出参，印章-签章人证书或其摘要列表，也可是为空数组（二进制值）。当为NULL时，通过piCertInfoLen给出长度
 * @param[out/in]  piCertInfoLen 出参/入参，印章-签章人证书或其摘要列表长度
 * @param[out]     puchValidStart
 * 出参，印章-有效起始时间（UTF-8编码字符串，如“20180101115959Z”），当为NULL时，通过piValidStartLen给出长度
 * @param[out/in]  piValidStartLen          出参/入参，印章-有效起始时间长度
 * @param[out]     puchValidEnd
 * 出参，印章-有效结束时间（UTF-8编码字符串，如“20180101115959Z”），当为NULL时，通过piValidEndLen给出长度
 * @param[out/in]  piValidEndLen            出参/入参，印章-有效结束长度
 * @param[out]     puchSignedDate
 * 出参，印章-制作日期（UTF-8编码字符串，如“20180101115959Z”），当为NULL时，通过piSignedDateLen给出长度
 * @param[out/in]  piSignedDateLen          出参/入参，印章-制作日期长度
 * @param[out]     puchSignerName
 * 出参，印章-制章人证书，也可是为空数组（二进制值）。当为NULL时，通过piSignerNameLen给出长度
 * @param[out/in]  piSignerNameLen          出参/入参，印章-制章人证书长度
 * @param[out]     puchSealSignMethod
 * 出参，印章-制章签名方法（UTF-8编码字符串,算法OID），当为NULL时，通过piSignMethodLen给出长度
 * @param[out/in]  piSealSignMethodLen      出参/入参，印章-制章签名方法长度
 * @param[out]     puchSealSignature
 * 出参，印章-制章签名值（二进制值），当为NULL时，通过piSignatureLen给出长度
 * @param[out/in]  piSealSignatureLen       出参/入参，印章-制章签名值长度
 * @param[out]     puchTimeInfo
 * 出参，签章时间（UTF-8编码字符串，如“20180101115959Z”），当为NULL时，通过piTimeInfoLen给出长度
 * @param[out/in]  piTimeInfoLen			出参/入参，签章时间长度
 * @param[out]     puchDataHash
 * 出参，原文摘要（二进制值），当为NULL时，通过piDataHashLen给出长度
 * @param[out/in]  piDataHashLen			出参/入参，原文摘要长度
 * @param[out]     puchPropertyInfo
 * 出参，原文属性信息（UTF-8编码字符串），当为NULL时，通过piPropertyInfoLen给出长度
 * @param[out/in]  piPropertyInfoLen		出参/入参，原文属性信息长度
 * @param[out]     puchCert
 * 出参，签章人证书（二进制），当为NULL时，通过piCertLen给出长度
 * @param[out/in]  piCertLen 出参/入参，签章人证书长度
 * @param[out]     puchSignMethod
 * 出参，签名算法（UTF-8编码字符串，算法OID），当为NULL时，通过piSignMethodLen给出长度
 * @param[out/in]  piSignMethodLen			出参/入参，签名算法长度
 * @param[out]     puchSignature
 * 出参，签名值（二进制值），当为NULL时，通过piSignatureLen给出长度
 * @param[out/in]  piSignatureLen           出参/入参，签名值长度
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef int(OES_API* _OESV4_GetSignInfo)(OES_HANDLE iSession,
                                         unsigned char* puchSignedValue,
                                         int iSignedValueLen,
                                         unsigned char* puchSignVersion,
                                         int* piSignVersionLen,
                                         unsigned char* puchSealId,
                                         int* piSealIdLen,
                                         unsigned char* puchSealVersion,
                                         int* piSealVersionLen,
                                         unsigned char* puchVenderId,
                                         int* piVenderIdLen,
                                         unsigned char* puchSealType,
                                         int* piSealTypeLen,
                                         unsigned char* puchSealName,
                                         int* piSealNameLen,
                                         unsigned char* puchCertInfo,
                                         int* piCertInfoLen,
                                         unsigned char* puchValidStart,
                                         int* piValidStartLen,
                                         unsigned char* puchValidEnd,
                                         int* piValidEndLen,
                                         unsigned char* puchSignedDate,
                                         int* piSignedDateLen,
                                         unsigned char* puchSignerName,
                                         int* piSignerNameLen,
                                         unsigned char* puchSealSignMethod,
                                         int* piSealSignMethodLen,
                                         unsigned char* puchSealSignature,
                                         int* piSealSignatureLen,
                                         unsigned char* puchTimeInfo,
                                         int* piTimeInfoLen,
                                         unsigned char* puchDataHash,
                                         int* piDataHashLen,
                                         unsigned char* puchPropertyInfo,
                                         int* piPropertyLen,
                                         unsigned char* puchCert,
                                         int* piCertLen,
                                         unsigned char* puchSignMethod,
                                         int* piSignMethodLen,
                                         unsigned char* puchSignature,
                                         int* piSignatureLen);

/**
 * @brief 从电子签名数据中获取相关信息  [Required]
 * @param[in]      iSession				    入参，线程句柄
 * @param[in]      puchSignedValue
 * 入参，签名数据（二进制值，符合国密标准，见GM/T0010-2012中"8签名数据类型"）
 * @param[in]      iSignedValueLen          入参，签名数据长度
 * @param[out]     puchVersion
 * 出参，签名数据版本，当为NULL时，通过piVersionLen给出长度
 * @param[out/in]  piVersionLen 出参/入参，签名数据版本长度
 * @param[out]     puchTimeInfo
 * 出参，签名时间（UTF-8编码字符串，如“20180101115959Z”），当为NULL时，通过piTimeInfoLen给出长度
 * @param[out/in]  piTimeInfoLen			出参/入参，签名时间长度
 * @param[out]     puchDataHash
 * 出参，原文摘要（二进制），当为NULL时，通过piDataHashLen给出长度
 * @param[out/in]  piDataHashLen			出参/入参，原文摘要长度
 * @param[out]     puchPropertyInfo
 * 出参，原文属性信息（UTF-8编码字符串），当为NULL时，通过piPropertyInfoLen给出长度
 * @param[out/in]  piPropertyInfoLen		出参/入参，原文属性信息长度
 * @param[out]     puchCert
 * 出参，签名人证书（二进制），当为NULL时，通过piCertLen给出长度
 * @param[out/in]  piCertLen 出参/入参，签名人证书长度
 * @param[out]     puchSignMethod
 * 出参，签名算法（UTF-8编码字符串，算法OID），当为NULL时，通过piSignMethodLen给出长度
 * @param[out/in]  piSignMethodLen			出参/入参，签名算法长度
 * @param[out]     puchSignature
 * 出参，签名值（二进制值），当为NULL时，通过piSignatureLen给出长度
 * @param[out/in]  piSignatureLen           出参/入参，签名值长度
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef int(OES_API* _OESV4_GetRawSignInfo)(OES_HANDLE iSession,
                                            unsigned char* puchSignedValue,
                                            int iSignedValueLen,
                                            unsigned char* puchVersion,
                                            int* piVersionLen,
                                            unsigned char* puchTimeInfo,
                                            int* piTimeInfoLen,
                                            unsigned char* puchDataHash,
                                            int* piDataHashLen,
                                            unsigned char* puchPropertyInfo,
                                            int* piPropertyLen,
                                            unsigned char* puchCert,
                                            int* piCertLen,
                                            unsigned char* puchSignMethod,
                                            int* piSignMethodLen,
                                            unsigned char* puchSignature,
                                            int* piSignatureLen);

/**
 * @brief 获取摘要算法标识 [Required]
 * @param[in]      iSession				     入参，线程句柄
 * @param[out]     puchDigestMethod
 * 出参，摘要算法（UTF-8编码字符串，算法OID），当为NULL时，通过piDigestMethodLen给出长度
 * @param[out/in]  piDigestMethodLen         出参/入参，摘要算法长度
 * @return 调用成功返回OES_OK，一般不返回错误码
 */
typedef int(OES_API* _OESV4_GetDigestMethod)(OES_HANDLE iSession,
                                             unsigned char* puchDigestMethod,
                                             int* piDigestMethodLen);

/**
 * @brief 代理计算摘要  [Required]
 * @param[in]      iSession				     入参，线程句柄
 * @param[in]      puchData                  入参，待摘要的数据（二进制值）
 * @param[in]      iDataLen                  入参，待摘要的数据长度
 * @param[in]      puchDigestMethod 入参，摘要算法（UTF-8编码字符串，算法OID）
 * @param[in]      iDigestMethodLen          入参，摘要算法长度
 * @param[out]     puchDigestValue
 * 出参，摘要值（二进制值），当为NULL时，通过piDigestValueLen给出长度
 * @param[out/in]  piDigestValueLen          出参/入参，摘要值长度
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef unsigned long(OES_API* _OESV4_Digest)(OES_HANDLE iSession,
                                              unsigned char* puchData,
                                              int iDataLen,
                                              unsigned char* puchDigestMethod,
                                              int iDigestMethodLen,
                                              unsigned char* puchDigestValue,
                                              int* piDigestValueLen);
/**
 * @brief 代理计算摘要_初始化
 * @param[in]      iSession				     入参，线程句柄
 * @param[in]      puchDigestMethod 入参，摘要算法（UTF-8编码字符串，算法OID）
 * @param[in]      iDigestMethodLen          入参，摘要算法长度
 * @param[out]     piHash                    出参，摘要句柄
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef int(OES_API* _OESV4_Digest_Init)(OES_HANDLE iSession,
                                         unsigned char* puchDigestMethod,
                                         int iDigestMethodLen,
                                         OES_HANDLE* piHash);
/**
 * @brief 代理计算摘要_更新数据
 * @param[in]      iSession				     入参，线程句柄
 * @param[in]      iHash        	         入参，摘要句柄
 * @param[in]      puchData                  入参，待摘要的数据（二进制值）
 * @param[in]      iDataLen                  入参，待摘要的数据长度
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef int(OES_API* _OESV4_Digest_Update)(OES_HANDLE iSession,
                                           OES_HANDLE iHash,
                                           unsigned char* puchData,
                                           int iDataLen);

/**
 * @brief 代理计算摘要_计算摘要值
 * @param[in]      iSession				     入参，线程句柄
 * @param[in]      iHandler        	         入参，摘要句柄
 * @param[out]     puchDigestValue
 * 出参，摘要值（二进制值），当为NULL时，通过piDigestValueLen给出长度
 * @param[out/in]  piDigestValueLen          出参/入参，摘要值长度
 * @return
 * 调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 */
typedef int(OES_API* _OESV4_Digest_Final)(OES_HANDLE iSession,
                                          OES_HANDLE iHash,
                                          unsigned char* puchDigestValue,
                                          int* piDigestValueLen);

/**
 * @brief 代理计算签名1  [Required]
 * 注意：该方法生成的签章数据的版本号固定为4。
 * 注意：部分调用者ofd.Sign曾写作Entry，不加doc.usr.等前缀，请注意兼容
 * @param[in]      iSession				   入参，线程句柄
 * @param[in]      puchSealId              入参，印章标识（UTF-8编码字符串）
 * @param[in]      iSealIdLen              入参，印章标识长度
 * @param[in]      puchDocProperty
 入参，原文属性，一般为文档标识等文档元数据信息（UTF-8编码字符串）
 *
 例如:阅读器A签名时{"doc.DocID":"xxx";"doc.Title":"yyy";"ofd.Sign":"/Doc_0/Signs/Sign_0/Signature.xml"}
                                           或
 阅读器B验证时/Doc_0/Signs/Sign_0/Signature.xml
 * @param[in]      iDocPropertyLen         入参，原文属性长度
 * @param[in]      puchDigestData          入参，摘要数据（二进制值）
 * @param[in]      iDigestDataLen          入参，摘要数据长度
 * @param[out]     puchSignValue
 出参，签名值（二进制值，符合国密标准，见GM/T0031 6.2节，或其升级版），当为NULL时，通过piSignValueLen给出长度
 * @param[out/in]  piSignValueLen          出参/入参，签名值长度
 * @return 应判断返回值与常量OES_OK和OES_RESERVED_MAX的关系
 *         如果大于MAX，可调用OES_GetErrMessage()获取详细信息；
 *
 如果等于NEED_PARAM，可依次调用GetExtendParamNameList,SetExtendParam设置所需的参数后再次调用；
 *         如果是小于MAX的其他值，可按预定义协议进行操作如提供PIN码或取消操作；
 *         如果等于OES_OK，可调用OES_GetErrMessage(OES_SIGNINFO)获取详细信息。
 */
typedef unsigned long(OES_API* _OESV4_Sign)(OES_HANDLE iSession,
                                            unsigned char* puchSealId,
                                            int iSealIdLen,
                                            unsigned char* puchDocProperty,
                                            int iDocPropertyLen,
                                            unsigned char* puchDigestData,
                                            int iDigestDataLen,
                                            unsigned char* puchSignValue,
                                            int* piSignValueLen);

/**
 * @brief 代理计算签名2  [Required]
 * 注意：部分调用者ofd.Sign曾写作Entry，不加doc.usr.等前缀，请注意兼容
 * @param[in]      iSession				 入参，线程句柄
 * @param[in]      puchCertId            入参，证书标识（UTF-8编码字符串）
 * @param[in]      iCertIdLen            入参，证书标识长度
 * @param[in]      puchDocProperty
 *入参，原文属性，一般为文档标识等文档元数据信息（UTF-8编码字符串）
 *                                         例如:阅读器A签名时{"doc.DocID":"xxx";"doc.Title":"yyy";"ofd.Sign":"/Doc_0/Signs/Sign_0/Signature.xml"}
 *										   或
 *阅读器B验证时/Doc_0/Signs/Sign_0/Signature.xml
 * @param[in]      iDocPropertyLen       入参，原文属性长度
 * @param[in]      puchDigestData        入参，摘要数据（二进制值）
 * @param[in]      iDigestDataLen        入参，摘要数据长度
 * @param[out]     puchSignValue 出参，签名值（二进制值，符合国密标准，见GB/T
 *35275-2017签名数据类型），当为NULL时，通过piSignValueLen给出长度
 * @param[out/in]  piSignValueLen        出参/入参，签名值长度
 * @return 应判断返回值与常量OES_OK和OES_RESERVED_MAX的关系
 *         如果大于MAX，可调用OES_GetErrMessage()获取详细信息；
 *         如果等于NEED_PARAM，可依次调用GetExtendParamNameList,SetExtendParam设置所需的参数后再次调用；
 *         如果是小于MAX的其他值，可按预定义协议进行操作如提供PIN码或取消操作；
 *         如果等于OES_OK，可调用OES_GetErrMessage(OES_RAWSIGNINFO)获取详细信息。
 */
typedef unsigned long(OES_API* _OESV4_RawSign)(OES_HANDLE iSession,
                                               unsigned char* puchCertId,
                                               int iCertIdLen,
                                               unsigned char* puchDocProperty,
                                               int iDocPropertyLen,
                                               unsigned char* puchDigestData,
                                               int iDigestDataLen,
                                               unsigned char* puchSignValue,
                                               int* piSignValueLen);

/**
 * @brief 代理验签实现  [Required]
 * @param[in]  iSession				    入参，线程句柄
 * @param[in]  puchDigestData           入参，摘要数据（二进制值）
 * @param[in]  iDigestDataLen           入参，摘要数据长度
 * @param[in]  puchSignValue            入参，签章数据（二进制值）
 * @param[in]  iSignValueLen            入参，签章数据长度
 * @param[in]  iOnline                  入参，是否在线验证，1表示在线，2表示离线
 * @return 应判断返回值与常量OES_OK和OES_RESERVED_MAX的关系
 *         返回大于RESERVED_MAX的错误代码时，可调用GetErrMessage()获取详细信息，并提示或记录日志;
 *         返回OES_UNKOWN时，可调用GetMessage(OES_VERIFYINFO)获取详细信息，并提示或记录日志;
 *         返回OES_OK代码时，可调用GetErrMessage(OES_VERIFYINFO)获取详细信息;
 *         返回OES_NEEDPARAM代码时，可依次调用GetExtendParamNameList,SetExtendParam设置所需的参数后再次调用;
 *         返回小于RESERVED_MAX的预定代码时，按照协议执行对应操作。
 */
typedef unsigned long(OES_API* _OESV4_Verify)(OES_HANDLE iSession,
                                              unsigned char* puchDigestData,
                                              int iDigestDataLen,
                                              unsigned char* puchSignValue,
                                              int iSignValueLen,
                                              int iOnline);

/**
 * @brief 取得错误信息
 * @param[in]      iSession				入参，线程句柄
 * @param[in]      errCode
 * 入参，错误代码，获得于OES_GetErrMessage()以外的任意函数
 * @param[out]     puchErrMessage
 * 出参，错误信息，其中可包含换行符号（UTF-8编码字符串），当为NULL时，通过piErrMessageLen给出长度
 * @param[out/in]  piErrMessageLen      出参/入参，错误信息长度
 * @return  固定返回OES_OK
 */
typedef int(OES_API* _OESV4_GetErrMessage)(OES_HANDLE iSession,
                                           unsigned long errCode,
                                           unsigned char* puchErrMessage,
                                           int* piErrMessageLen);

/*******************************实现者可以不提供Pin码框,由调用者（阅读器或SDK）提供，避免OES带窗体库************************/
/**
 * @brief
 *登陆签章客户端（UKey或其他），密码收集由接口调用方（如版式软件）提供输入框
 * @param[in]        iSession			  入参，线程句柄
 * @param[in]		puchPIN
 *入参，密码组件PIN码(UTF-8编码字符串)
 * @param[in]		iPINLen				  入参，PIN码长度
 * @return
 *调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 **/
typedef int(OES_API* _OESV4_Login)(OES_HANDLE iSession,
                                   unsigned char* puchPIN,
                                   int iPINLen);

/**
 * @brief 登出签章客户端（UKey或其他）
 * @param[in]      iSession				  入参，线程句柄
 * @return
 *调用成功返回OES_OK，否则应进一步判断是否大于OES_RESERVED_MAX，如果是，可通过GetErrMessage查询详细信息
 **/
typedef int(OES_API* _OESV4_Logout)(OES_HANDLE iSession);

/**
 * @brief 获取签名算法标识 [Optional]
 * @param[in]      iSession				  入参，线程句柄
 * @param[out]     puchSignMethod
 * 出参，签名算法（UTF-8编码字符串）。当为NULL时，通过piSignMethodLen给出长度
 * @param[out/in]  piSignMethodLen        出参/入参，签名算法长度
 * @return 调用成功返回OES_OK，一般不返回错误码
 */
typedef int(OES_API* _OESV4_GetSignMethod)(OES_HANDLE iSession,
                                           unsigned char* puchSignMethod,
                                           int* piSignMethodLen);

/**
 * @brief 获取当前时间 [Optional]
 * @param[in]      iSession				 入参，线程句柄
 * @param[out]     puchSignDateTime
 * 出参，时间（字符时用UTF-8编码，如“20180101115959Z”）。当为NULL时，通过piSignDateTimeLen给出长度
 * @param[out/in]  piSignDateTimeLen     出参/入参，时间长度
 * @return 调用成功返回OES_OK，一般不返回错误码
 */
typedef int(OES_API* _OESV4_GetDateTime)(OES_HANDLE iSession,
                                         unsigned char* puchSignDateTime,
                                         int* piSignDateTimeLen);

/**
 * @brief 询问操作所需的额外参数的名称
 * @param[in]      iSession				 入参，线程句柄
 * @param[out]     puchParamName
 * 出参，所需参数的名称（UTF-8编码字符串，多个参数时用;分隔），当为NULL时，通过piParamNameLen给出长度
 * @param[out/in]  piParamNameLen		 出参/入参，所需参数的名称长度
 * @return 固定返回OES_OK
 */
typedef int(OES_API* _OESV4_GetParamNameList)(OES_HANDLE iSession,
                                              unsigned char* puchParamName,
                                              int* piParamNameLen);

/**
 * @brief
 * 设置操作所需的额外参数，这些数据可能是指文档的属性、当前操作者的信息，也有可能是所在环境的信息，还有可能是文档的特定内容
 *        设置文档内容时，可用“行业标识.内容标识”来表示，例如"fp.NativeSign"
 * @param[in]       iSession			 入参，线程句柄
 * @param[in]       puchParamName
 * 入参，参数或者参数集名称（UTF-8编码字符串）.一般对于简单类型的参数用集合名称，例如doc、usr或者env
 * @param[in]       iParamNameLen        入参，参数或者参数集名称长度
 * @param[in]       puchParamValue
 * 入参，参数或者参数集取值（二进制值，字符串时取值使用UTF-8编码）.例如:{"env.SoftwareType":"OFDReader";"env.IP":"127.0.0.1"}
 * @param[in]       iParamValueLen       入参，参数或者参数集取值长度
 * @return 调用成功返回OES_OK，一般不返回错误码
 */
typedef int(OES_API* _OESV4_SetExtendParam)(OES_HANDLE iSession,
                                            unsigned char* puchParamName,
                                            int iParamNameLen,
                                            unsigned char* puchParamValue,
                                            int iParamValueLen);

/**
 * @brief 获取当前时间的时间戳形式 [Optional]
 * @param[in]       puchContent         入参，原文（二进制值）
 * @param[in]		iContentLen			入参，原文的长度
 * @param[out]      puchTimeStamp       出参，时间戳（二进制值，GB/T
 * 20520-2006）。当为NULL时，通过piTimeStampLen给出长度
 * @param[out/in]   piTimeStampLen      出参/入参，时间戳长度
 * @return 调用成功返回OES_OK，否则是错误代码
 */
typedef int(OES_API* _OESV4_GetTimeStamp)(OES_HANDLE iSession,
                                          unsigned char* puchContent,
                                          int iContentLen,
                                          unsigned char* puchTimeStamp,
                                          int* piTimeStampLen);

/**
 * @brief 验证时间戳形式 [Optional]
 * @param[in]       puchContent         入参，原文（二进制值）
 * @param[in]		iContentLen			入参，原文的长度
 * @param[in]       puchTimeStamp       入参，时间戳（二进制值，GB/T
 * 20520-2006）。当为NULL时，通过piTimeStampLen给出长度
 * @param[in]       piTimeStampLen      入参，时间戳长度
 * @return 调用成功返回OES_OK，否则是错误代码
 */
typedef int(OES_API* _OESV4_VerifyTimeStamp)(OES_HANDLE iSession,
                                             unsigned char* puchContent,
                                             int iContentLen,
                                             unsigned char* puchTimeStamp,
                                             int piTimeStampLen);

/**
 * @brief 代理解析时间戳 [Optional]
 * @param[in]		puchTimeStamp		入参，时间戳时间（二进制值）
 * @param[in]		iTimeStampLen		入参，时间戳时间长度
 * @param[out]		puchContent
 * 出参，原文（二进制值），当为NULL时，通过piContentLen给出长度
 * @param[out/in]	piContentLen		出参/入参，原文的长度
 * @param[out]		puchDateTime
 * 出参，时间（UTF-8编码字符串，如“20180101115959Z”）。当为NULL时，通过piDateTimeLen给出长度
 * @param[out/in]	piDateTimeLen		出参/入参，时间长度
 * @param[out]		puchCert
 * 出参，签名证书（二进制值）。当为NULL时，通过piCertLen给出长度
 * @param[out/in]	piCertLen			出参/入参，签名证书长度
 * @return 调用成功返回OES_OK，否则是错误代码
 */
typedef int(OES_API* _OESV4_GetTimeStampInfo)(OES_HANDLE iSession,
                                              unsigned char* puchTimeStamp,
                                              int iTimeStampLen,
                                              unsigned char* puchContent,
                                              int* piContentLen,
                                              unsigned char* puchDateTime,
                                              int* piDateTimeLen,
                                              unsigned char* puchCert,
                                              int* piCertLen);

typedef int(OES_API* _OESV4_CancelSeal)(OES_HANDLE iSession,
                                        unsigned char* puchSignValue,
                                        int iSignValueLen);

#ifdef __cplusplus
}
#endif

typedef struct oes_interface_s oes_interface;

struct oes_interface_s {
  _OES_GetProviderInfo get_provider_info;
  _OES_GetSealList get_seal_list;
  _OES_GetSeal get_seal;
  _OES_GetSealImage get_seal_image;
  _OES_GetSignImage get_sign_image;
  _OES_GetSignDateTime get_sign_date_time;
  _OES_GetSignMethod get_sign_method;
  _OES_GetDigestMethod get_digest_method;
  _OES_Digest digest;
  _OES_GetSealInfo get_seal_info;
  _OES_Sign sign;
  _OES_Verify verify;
  _OES_CancelSeal cancel_seal;
  _OES_GetErrMessage get_error_msg;


  OES_HANDLE oesctx;  // OESV4上下文
  _OESV4_GetProviderInfo pOESV4_GetProviderInfo;
  _OESV4_GetProtocolVersion pOESV4_GetProtocolVersion;
  _OESV4_OpenSession pOESV4_OpenSession;
  _OESV4_CloseSession pOESV4_CloseSession;
  _OESV4_GetSealList pOESV4_GetSealList;
  _OESV4_GetCertList pOESV4_GetCertList;
  _OESV4_GetSealImageById pOESV4_GetSealImageById;
  _OESV4_GetSealInfoById pOESV4_GetSealInfoById;
  _OESV4_GetCertById pOESV4_GetCertById;
  _OESV4_GetSignImage pOESV4_GetSignImage;
  _OESV4_GetSealImage pOESV4_GetSealImage;
  _OESV4_GetSignInfo pOESV4_GetSignInfo;
  _OESV4_GetRawSignInfo pOESV4_GetRawSignInfo;
  _OESV4_GetDigestMethod pOESV4_GetDigestMethod;
  _OESV4_Digest pOESV4_Digest;
  _OESV4_Digest_Init pOESV4_Digest_Init;
  _OESV4_Digest_Update pOESV4_Digest_Update;
  _OESV4_Digest_Final pOESV4_Digest_Final;
  _OESV4_Sign pOESV4_Sign;
  _OESV4_RawSign pOESV4_RawSign;
  _OESV4_Verify pOESV4_Verify;
  _OESV4_GetErrMessage pOESV4_GetErrMessage;
  _OESV4_Login pOESV4_Login;
  _OESV4_Logout pOESV4_Logout;
  _OESV4_GetSignMethod pOESV4_GetSignMethod;
  _OESV4_GetDateTime pOESV4_GetDateTime;
  _OESV4_GetParamNameList pOESV4_GetParamNameList;
  _OESV4_SetExtendParam pOESV4_SetExtendParam;
  _OESV4_GetTimeStamp pOESV4_GetTimeStamp;
  _OESV4_VerifyTimeStamp pOESV4_VerifyTimeStamp;
  _OESV4_GetTimeStampInfo pOESV4_GetTimeStampInfo;
  _OESV4_CancelSeal pOESV4_CancelSeal;
};
class COESLoader
{
  int m_errcode;

 public:
  oes_interface_s m_si;

 private:
  void loadOESFun(HANDLE hDLL, oes_interface* oes);

 public:
  COESLoader();
  ~COESLoader();

public:
  COESLoader(const COESLoader &) = delete;
  COESLoader(COESLoader &&) = delete;
  COESLoader &operator=(const COESLoader &) = delete;
  COESLoader &operator=(COESLoader &&) = delete;

public:
  static COESLoader *Instance();
 void CopyOESFnPointer(oes_interface* si);
  int ErrorCode() { return m_errcode; }
};

}  // namespace OES
#endif

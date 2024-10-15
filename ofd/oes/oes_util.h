#ifndef XILOU_OFD_OES_UTIL_HEADER_H
#define XILOU_OFD_OES_UTIL_HEADER_H


namespace OES{
typedef void*				HANDLE;
typedef void*				LPVOID;

 bool util_get_env_variable(char* lpszName, char* pszBuffer, int nBufSize);
 HANDLE util_library_load_local(const char* pszLibPath);
 LPVOID util_library_get_function_ptr(HANDLE hLib, const char* pszFunctionName);
 bool util_library_free(HANDLE hLib);
 char* util_utf8_to_gbk(char* pszUTF8);
 char* util_gbk_to_utf8( const char* pszGBK );
 bool util_is_utf8(const char* pszStr);
}
#endif//__PLUGIN_UTIL_H__

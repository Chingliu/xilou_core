
#include "ofd/oes/oes_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory.h>

#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h> 
#endif
namespace OES{
bool util_get_env_variable( char* lpszName, char* pszBuffer, int nBufSize )
{

	if(lpszName == NULL || lpszName[0] == 0 || pszBuffer == NULL || nBufSize <= 0)
	{

		return false;
	}
	
#ifdef WIN32

	int nRet = GetEnvironmentVariableA(lpszName, pszBuffer, nBufSize);	

	if(nRet != 0 && nRet < nBufSize)
	{
		return true;
	}
  //auto errcode = GetLastError();
#elif UNIX
	char* pszValue = NULL;
	pszValue = getenv(lpszName);	
	if((pszValue == NULL) || (nBufSize < strlen(pszValue)))
	{
		return false;
	}

	if (pszBuffer)
	{
		strcpy(pszBuffer, pszValue);
		//delete[] pszValue;	//getenv返回的指针不需要释放
		return true;
	}
#endif
	return false;
}

HANDLE util_library_load_local(const char* pszLibPath)
{
	if(pszLibPath == NULL)
		return 0;	

#ifdef WIN32
	HMODULE hDLL = LoadLibraryA(pszLibPath);
	if(hDLL == NULL)
	{
		int x  = GetLastError();
    printf("[util_library_load_local] %d\r\n", x);
	}
	return (HANDLE)hDLL;
#else
	void* pHandle = dlopen(pszLibPath, RTLD_LAZY);
	if(pHandle == NULL)
	{
		//const char* err=dlerror();
	}

//	printf("dlopen[%s]...hLib=%x\n", pszLibPath, pHandle);

	return (HANDLE)pHandle;
#endif

}

LPVOID util_library_get_function_ptr(HANDLE hLib, const char* pszFunctionName)
{
	if(hLib == 0 || pszFunctionName == NULL || pszFunctionName[0] == 0)
		return NULL;

#ifdef WIN32
	void* FuncAddr = (void *)GetProcAddress((HMODULE)hLib, pszFunctionName);
	if (FuncAddr == NULL)
	{
		//int nErr = GetLastError();
	}
	return FuncAddr;

#elif UNIX
	return dlsym(hLib, pszFunctionName);
#endif

}

bool util_library_free(HANDLE hLib)
{
	if(hLib == NULL)
		return false;

#ifdef WIN32
	FreeLibrary((HMODULE)hLib);	
#elif UNIX
	dlclose((void*)hLib);
#endif

	return true;
}

// convert utf8 to gbk
char* util_utf8_to_gbk( char* pszUTF8 )
{
	if(pszUTF8 == NULL)
	{
		return NULL;
	}

#ifdef WIN32
	// UTF8 to UNICODE
	DWORD dwSize = MultiByteToWideChar(CP_UTF8, 0, pszUTF8, -1, NULL, 0);
	DWORD dwDataLength = (dwSize + 1) * sizeof(WCHAR);
	WCHAR* ptszData = (WCHAR*)malloc(dwDataLength);
	if(ptszData == NULL)
	{
		return NULL;
	}
	memset(ptszData, 0, dwDataLength);
	MultiByteToWideChar(CP_UTF8, 0, pszUTF8, -1, ptszData, dwDataLength);

	// UNICODE to ANSI
	dwSize = WideCharToMultiByte(CP_ACP, 0, ptszData, -1, NULL, 0, NULL, NULL);
	char* pszData = (char*)malloc(dwSize+1);
	if(pszData == NULL)
	{
		free(ptszData);
		return NULL;
	}
	memset(pszData, 0, dwSize+1);
	WideCharToMultiByte(CP_ACP, 0, ptszData, -1, pszData, dwSize, NULL, NULL);
	free(ptszData);
	return pszData;	
#else
        return pszUTF8;
#endif

	
}

// convert gbk to utf8
char* util_gbk_to_utf8( const char* pszGBK )
{
	if(pszGBK == NULL)
	{
		return NULL;
	}

#ifdef WIN32
	// ASNI to UNICODE
	DWORD dwSize = MultiByteToWideChar(CP_ACP, 0, pszGBK, -1, NULL, 0);
	DWORD dwDataLength = (dwSize + 1) * sizeof(WCHAR);
	WCHAR* ptszData = (WCHAR*)malloc(dwDataLength);
	if(ptszData == NULL)
	{
		return NULL;
	}
	memset(ptszData, 0, dwDataLength);
	MultiByteToWideChar(CP_ACP, 0, pszGBK, -1, ptszData, dwDataLength);

	// UNICODE to utf-8
	dwSize = WideCharToMultiByte(CP_UTF8, 0, ptszData, -1, NULL, 0, NULL, NULL);
	char* pszData = (char*)malloc(dwSize+1);
	if(pszData == NULL)
	{
		free(ptszData);
		return NULL;
	}
	memset(pszData, 0, dwSize+1);
	WideCharToMultiByte(CP_UTF8, 0, ptszData, -1, pszData, dwSize, NULL, NULL);
	free(ptszData);
	return pszData;	

#elif UNIX
	return (char*)pszGBK;
#endif

	return NULL;
}

bool util_is_utf8(const char* pszStr)
{
	unsigned int nBytes = 0;
	unsigned char chr = *pszStr;
	bool bAllAscii = true;
	for (unsigned int i = 0; pszStr[i] != '\0'; ++i){
		chr = *(pszStr + i);
		//判断是否ASCII编码,如果不是,说明有可能是UTF8,ASCII用7位编码,最高位标记为0,0xxxxxxx 
		if (nBytes == 0 && (chr & 0x80) != 0){
			bAllAscii = false;
		}
		if (nBytes == 0) {
			//如果不是ASCII码,应该是多字节符,计算字节数  
			if (chr >= 0x80) {

				if (chr >= 0xFC && chr <= 0xFD){
					nBytes = 6;
				}
				else if (chr >= 0xF8){
					nBytes = 5;
				}
				else if (chr >= 0xF0){
					nBytes = 4;
				}
				else if (chr >= 0xE0){
					nBytes = 3;
				}
				else if (chr >= 0xC0){
					nBytes = 2;
				}
				else{
					return false;
				}
				nBytes--;
			}
		}
		else{
			//多字节符的非首字节,应为 10xxxxxx 
			if ((chr & 0xC0) != 0x80){
				return false;
			}
			//减到为零为止
			nBytes--;
		}
	}
	//违返UTF8编码规则 
	if (nBytes != 0)  {
		return false;
	}
	if (bAllAscii){ //如果全部都是ASCII, 也是UTF8
		return true;
	}
	return true;
}
}

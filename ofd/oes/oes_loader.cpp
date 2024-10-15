#include "ofd/oes/oes_loader.h"
#include "ofd/oes/oes_util.h"
#include <string.h>
#include <string>
#include "third_party/spdlog/include/spdlog/spdlog.h"
#ifdef WIN32
#include <Windows.h>

#endif
using namespace OES;
#define OES_GET_FUN_NOT_CONTINUE(name)\
{\
    oes->p##name = (_##name)util_library_get_function_ptr(hDLL,(char*)(const char*)#name);\
    if (oes->p##name == NULL)\
    {\
        printf("util_library_get_function_ptr (%s)\n", (const char*)#name);\
    }\
}

COESLoader::COESLoader()
{
  m_errcode = OES::OES_E_NO_ENV;
  char szBuffer[260] = { 0 };
  HANDLE hDLL = NULL;
  if (util_get_env_variable((char*)(const char*)"OESV2_HOME", szBuffer, sizeof(szBuffer)) && strlen(szBuffer) > 0)
  {
    spdlog::debug("[COESLoader] found OESV2_HOME env, {0}", szBuffer);
    char *token = strtok(szBuffer, ";");
    if (token)
    {
      std::string oes_path(token);
#ifdef WIN32
      oes_path += "\\oes.dll";
#else
      oes_path += "/liboes.so";
#endif
      hDLL = util_library_load_local(oes_path.c_str());
      spdlog::debug("[COESLoader] load oes from{0} handle is {1}",oes_path, hDLL);
    }
  }
  if (hDLL == NULL || hDLL == 0) {
    m_errcode = OES::OES_E_LOAD_ERR;
#ifdef WIN32
    char szPath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, szPath, MAX_PATH);
    std::string selfoes(szPath);
    selfoes = selfoes.substr(0, selfoes.find_last_of("\\"));
    selfoes.append("\\oes.dll");
    hDLL = util_library_load_local(selfoes.c_str());
    spdlog::debug("[COESLoader] load self oes from{0} handle is {1}", selfoes, hDLL);
#endif
  }
  if (hDLL == NULL || hDLL == 0) {
    spdlog::error("[COESLoader] load oes error");
    m_errcode = OES::OES_E_LOAD_ERR;
    return;
  }
  memset(&m_si, 0, sizeof(m_si));
  loadOESFun(hDLL, &m_si);
  if (m_si.verify && m_si.pOESV4_GetSignImage) {
    if (m_si.pOESV4_OpenSession) {
      m_si.pOESV4_OpenSession(&m_si.oesctx);
    }
    m_errcode = 0;
    spdlog::debug("[COESLoader] oes loaded");
    return;
  } else {
    spdlog::error("[COESLoader] check oes verify and pOESV4_GetSignImage error ");
    m_errcode = OES::OES_E_LOAD_ERR;
    return;
  }
}

COESLoader::~COESLoader()
{
  if (m_si.pOESV4_CloseSession && m_si.oesctx) {
    m_si.pOESV4_CloseSession(m_si.oesctx);
  }
}
COESLoader* COESLoader::Instance()
{
  static COESLoader& oes = *new COESLoader();
  return &oes;
}

void COESLoader::CopyOESFnPointer(oes_interface* si) {
  memcpy(si, &m_si, sizeof(oes_interface));
}
void COESLoader::loadOESFun(HANDLE hDLL, oes_interface* oes) {
  if (!hDLL || !oes)
    return;

  oes->oesctx = nullptr;

  oes->get_provider_info = (_OES_GetProviderInfo)util_library_get_function_ptr(hDLL, "OES_GetProviderInfo");
  oes->get_seal_list = (_OES_GetSealList)util_library_get_function_ptr(hDLL, "OES_GetSealList");
  oes->get_seal = (_OES_GetSeal)util_library_get_function_ptr(hDLL, "OES_GetSeal");
  oes->get_seal_image = (_OES_GetSealImage)util_library_get_function_ptr(hDLL, "OES_GetSealImage");
  oes->get_sign_date_time = (_OES_GetSignDateTime)util_library_get_function_ptr(hDLL, "OES_GetSignDateTime");
  oes->get_sign_method = (_OES_GetSignMethod)util_library_get_function_ptr(hDLL, "OES_GetSignMethod");
  oes->get_digest_method = (_OES_GetDigestMethod)util_library_get_function_ptr(hDLL, "OES_GetDigestMethod");
  oes->digest = (_OES_Digest)util_library_get_function_ptr(hDLL, "OES_Digest");
  oes->get_seal_info = (_OES_GetSealInfo)util_library_get_function_ptr(hDLL, "OES_GetSealInfo");
  oes->sign = (_OES_Sign)util_library_get_function_ptr(hDLL, "OES_Sign");
  oes->verify = (_OES_Verify)util_library_get_function_ptr(hDLL, "OES_Verify");
  oes->cancel_seal = (_OES_CancelSeal)util_library_get_function_ptr(hDLL, "OES_CancelSeal");
  oes->get_error_msg = (_OES_GetErrMessage)util_library_get_function_ptr(hDLL, "OES_GetErrMessage");


  OES_GET_FUN_NOT_CONTINUE(OESV4_GetProviderInfo);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetProtocolVersion);
  OES_GET_FUN_NOT_CONTINUE(OESV4_OpenSession);
  OES_GET_FUN_NOT_CONTINUE(OESV4_CloseSession);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetSealList);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetCertList);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetSealImageById);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetSealInfoById);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetCertById);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetSignImage);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetSealImage);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetSignInfo);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetRawSignInfo);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetDigestMethod);
  OES_GET_FUN_NOT_CONTINUE(OESV4_Digest);
  OES_GET_FUN_NOT_CONTINUE(OESV4_Digest_Init);
  OES_GET_FUN_NOT_CONTINUE(OESV4_Digest_Update);
  OES_GET_FUN_NOT_CONTINUE(OESV4_Digest_Final);
  OES_GET_FUN_NOT_CONTINUE(OESV4_Sign);
  OES_GET_FUN_NOT_CONTINUE(OESV4_RawSign);
  OES_GET_FUN_NOT_CONTINUE(OESV4_Verify);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetErrMessage);
  OES_GET_FUN_NOT_CONTINUE(OESV4_Login);
  OES_GET_FUN_NOT_CONTINUE(OESV4_Logout);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetSignMethod);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetDateTime);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetParamNameList);
  OES_GET_FUN_NOT_CONTINUE(OESV4_SetExtendParam);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetTimeStamp);
  OES_GET_FUN_NOT_CONTINUE(OESV4_VerifyTimeStamp);
  OES_GET_FUN_NOT_CONTINUE(OESV4_GetTimeStampInfo);
  OES_GET_FUN_NOT_CONTINUE(OESV4_CancelSeal);


}

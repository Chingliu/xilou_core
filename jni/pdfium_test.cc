// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <mutex>
#if defined(PDF_ENABLE_SKIA) && !defined(_SKIA_SUPPORT_)
#define _SKIA_SUPPORT_
#endif

#if defined(PDF_ENABLE_SKIA_PATHS) && !defined(_SKIA_SUPPORT_PATHS_)
#define _SKIA_SUPPORT_PATHS_
#endif

#include "public/cpp/fpdf_scopers.h"
#include "public/fpdf_annot.h"
#include "public/fpdf_attachment.h"
#include "public/fpdf_dataavail.h"
#include "public/fpdf_edit.h"
#include "public/fpdf_ext.h"
#include "public/fpdf_formfill.h"
#include "public/fpdf_progressive.h"
#include "public/fpdf_structtree.h"
#include "public/fpdf_text.h"
#include "public/fpdfview.h"
#include "jni/pdfium_test_dump_helper.h"
#include "jni/pdfium_test_event_helper.h"
#include "jni/pdfium_test_write_helper.h"
#include "testing/fx_string_testhelpers.h"
#include "testing/test_loader.h"
#include "testing/utils/file_util.h"
#include "testing/utils/hash.h"
#include "testing/utils/path_service.h"


#include "jni/pdfium_jni.h"

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef ENABLE_CALLGRIND
#include <valgrind/callgrind.h>
#endif  // ENABLE_CALLGRIND

#ifdef PDF_ENABLE_V8
#include "testing/v8_initializer.h"
#include "v8/include/libplatform/libplatform.h"
#include "v8/include/v8.h"
#endif  // PDF_ENABLE_V8

#ifdef _WIN32
#define access _access
#define snprintf _snprintf
#define R_OK 4
#endif

// wordexp is a POSIX function that is only available on macOS and non-Android
// Linux platforms.
#if defined(__APPLE__) || (defined(__linux__) && !defined(__ANDROID__))
#define WORDEXP_AVAILABLE
#endif

#ifdef WORDEXP_AVAILABLE
#include <wordexp.h>
#endif  // WORDEXP_AVAILABLE


namespace {

unsigned long PrintLastError() {
  unsigned long err = FPDF_GetLastError();
  fprintf(stderr, "Load pdf docs unsuccessful: ");
  switch (err) {
    case FPDF_ERR_SUCCESS:
      fprintf(stderr, "Success");
      break;
    case FPDF_ERR_UNKNOWN:
      fprintf(stderr, "Unknown error");
      break;
    case FPDF_ERR_FILE:
      fprintf(stderr, "File not found or could not be opened");
      break;
    case FPDF_ERR_FORMAT:
      fprintf(stderr, "File not in PDF format or corrupted");
      break;
    case FPDF_ERR_PASSWORD:
      fprintf(stderr, "Password required or incorrect password");
      break;
    case FPDF_ERR_SECURITY:
      fprintf(stderr, "Unsupported security scheme");
      break;
    case FPDF_ERR_PAGE:
      fprintf(stderr, "Page not found or content error");
      break;
    default:
      fprintf(stderr, "Unknown error %ld", err);
  }
  fprintf(stderr, ".\n");
  return err;
}


//+++++jni interface+++++++++++++++++++
//+++++++++data structure +++++++++++++++++++++++++++++++++++++

class JByteArrayHelper{
  JNIEnv *_env;
  jbyteArray _data;  
public:
	jsize len;
	jbyte *jData;
  JByteArrayHelper(JNIEnv * env, jbyteArray data):_env(env),_data(data) {
    len = _env->GetArrayLength(data);
    jData = _env->GetByteArrayElements(data, JNI_FALSE);    
  }
  ~JByteArrayHelper(){
    _env->ReleaseByteArrayElements(_data, jData, 0);
  }
};

struct FPDF_FORMFILLINFO_jni final : public FPDF_FORMFILLINFO {
  // Hold a map of the currently loaded pages in order to avoid them
  // to get loaded twice.
  std::map<int, ScopedFPDFPage> loaded_pages;

  // Hold a pointer of FPDF_FORMHANDLE so that PDFium app hooks can
  // make use of it.
  FPDF_FORMHANDLE form_handle;
};

FPDF_FORMFILLINFO_jni* ToPDFiumJniFormFillInfo(
    FPDF_FORMFILLINFO* form_fill_info) {
  return static_cast<FPDF_FORMFILLINFO_jni*>(form_fill_info);
}
class Jni_Document{
public:
    Jni_Document() = default;
    Jni_Document(const Jni_Document&) = delete;  //阻止拷贝
    Jni_Document & operator = (const Jni_Document&) = delete; //阻止赋值
    ~Jni_Document(){

      for(const auto &kv: form_callbacks.loaded_pages){
        FORM_OnBeforeClosePage(kv.second.get(), form.get()); 
      }
      form_callbacks.loaded_pages.clear();
      form.reset();
      doc.reset();
      
    }
public:
    int _page_count;
    FPDF_FORMFILLINFO_jni form_callbacks;
    ScopedFPDFFormHandle form;
    //std::map<int, ScopedFPDFPage> loaded_pages;
    ScopedFPDFDocument doc;
    std::vector<uint8_t> _doc_content;
};

FPDF_PAGE GetPageForIndex(FPDF_FORMFILLINFO* param,
                          FPDF_DOCUMENT doc,
                          int index) {
  FPDF_FORMFILLINFO_jni* form_fill_info =
      ToPDFiumJniFormFillInfo(param);
  auto& loaded_pages = form_fill_info->loaded_pages;
  auto iter = loaded_pages.find(index);
  if (iter != loaded_pages.end())
    return iter->second.get();

  ScopedFPDFPage page(FPDF_LoadPage(doc, index));
  if (!page)
    return nullptr;

  // Mark the page as loaded first to prevent infinite recursion.
  FPDF_PAGE page_ptr = page.get();
  loaded_pages[index] = std::move(page);

  FPDF_FORMHANDLE& form_handle = form_fill_info->form_handle;
  FORM_OnAfterLoadPage(page_ptr, form_handle);
  FORM_DoPageAAction(page_ptr, form_handle, FPDFPAGE_AACTION_OPEN);
  return page_ptr;
}

}//namespace
//++++++++++++library interface +++++++++++++++++++
//==========================================================================
// Function:    openDocumentFromSteam
// Input:       jstring customer_font_path自定义字体文件路径
//				
//
// Output:      -
// Return:      -
// Description: -库初始化
//==========================================================================
static void _init_impl(JNIEnv * env, jobject thiz, jstring customer_font_path){
  static bool has_been_init = false;
  if(!has_been_init){
    std::unique_ptr<char, pdfium::FreeDeleter> font_path;
    FPDF_LIBRARY_CONFIG config;
    config.version = 3;
    config.m_pUserFontPaths = nullptr;
    config.m_pIsolate = nullptr;
    config.m_v8EmbedderSlot = 0;
    config.m_pPlatform = nullptr;
    const char* path_array[2] = {nullptr, nullptr};
    if(customer_font_path){
      font_path.reset(jstringToUTF8(env, customer_font_path));
      path_array[0] = font_path.get();
    }
    config.m_pUserFontPaths = path_array;
    
    FPDF_InitLibraryWithConfig(&config);
    has_been_init = true;
  }
}
JNIEXPORT void JNICALL JNI_FN(KGLibrary_Init)(JNIEnv * env, jobject thiz, jstring customer_font_path){
  return _init_impl(env, thiz, customer_font_path);
}

//==========================================================================
// Function:    openDocumentFromSteam
// Input:       jstring customer_font_path自定义字体文件路径
//				
//
// Output:      -
// Return:      -
// Description: -库初始化
//==========================================================================
JNIEXPORT void JNICALL JNI_FN(KGLibrary_Destroy)(JNIEnv * env, jobject thiz){
  FPDF_DestroyLibrary();
}

//++++++++++++++++++++document interface +++++++++++++++++++++++++++++++++++++++


//==========================================================================
// Function:    openDocumentFromSteam
// Input:       jbyteArray data 输入文件流
//				
//
// Output:      -krc_document *doc
// Return:      -
// Description: -输入文件流，返回文档句柄
//==========================================================================

JNIEXPORT jlong JNICALL
JNI_FN(KGDocument_openDocumentFromSteam)(JNIEnv *env, jobject thiz, jbyteArray data){
  
  _init_impl(env, thiz, nullptr);
	JByteArrayHelper array_helper(env, data);
  Jni_Document *jni_doc = nullptr;

  jni_doc = new Jni_Document();
  if(!jni_doc){
    return KERROR_NOMEMORY;
  }
  
  if(!array_helper.jData || array_helper.len <= 0){
    return KERROR_GETBBYTEARRAYELEMENTS;
  }
  jni_doc->_doc_content.resize(array_helper.len + 10);
  memcpy(&jni_doc->_doc_content.front(), array_helper.jData, array_helper.len);
  if (jni_doc->_doc_content.empty())
  {
    return KERROR_GETBBYTEARRAYELEMENTS;
  }
  ScopedFPDFDocument doc;
  doc.reset(FPDF_LoadMemDocument(&jni_doc->_doc_content.front(), jni_doc->_doc_content.size(), nullptr));
  if (!doc) {
    unsigned long err = PrintLastError();
    return 0 - err;//错误码为负值
  }
  
  
  if(jni_doc){
    jni_doc->doc = std::move(doc);
    jni_doc->_page_count = FPDF_GetPageCount(jni_doc->doc.get());
    printf("[opendoc]doc handle is:%p\r\n", jni_doc->doc.get());
    
    jni_doc->form_callbacks.version = 1;
    jni_doc->form_callbacks.FFI_GetPage = GetPageForIndex;
    ScopedFPDFFormHandle form(
        FPDFDOC_InitFormFillEnvironment(jni_doc->doc.get(), &jni_doc->form_callbacks));

    jni_doc->form_callbacks.form_handle = form.get();
    FPDF_SetFormFieldHighlightColor(form.get(), FPDF_FORMFIELD_UNKNOWN, 0xFFE4DD);
    FPDF_SetFormFieldHighlightAlpha(form.get(), 100);
    FORM_DoDocumentJSAction(form.get());
    FORM_DoDocumentOpenAction(form.get());      
    jni_doc->form = std::move(form);
  }
 
  return jlong_cast(jni_doc);
}

//==========================================================================
// Function:    closeDocument
// Input:       jlong doc 文档句柄
//				
//
// Output:      - 
// Return:      -
// Description: -关闭文档
//==========================================================================

JNIEXPORT void JNICALL JNI_FN(KGDocument_closeDocument)(JNIEnv *env, jobject thiz, jlong doc){
  
  Jni_Document *jni_doc = (Jni_Document *)doc;
  FORM_DoDocumentAAction(jni_doc->form.get(), FPDFDOC_AACTION_WC);
  if(jni_doc){
    delete jni_doc;
  }
  
}


//==========================================================================
// Function:    getPagesCount
// Input:       jlong doc 文档句柄
//				
//
// Output:      - 文档页数
// Return:      -
// Description: -获取文档页数
//==========================================================================
JNIEXPORT jint JNICALL JNI_FN(KGDocument_getPagesCount)(JNIEnv *env, jobject thiz, jlong doc){
  Jni_Document *jni_doc = (Jni_Document *)doc;
  return jni_doc->_page_count;
}

//==========================================================================
// Function:    getPageHeight
// Input:       jlong doc 文档句柄
//				jint number 从1开始
//
// Output:      - 页高度
// Return:      -
// Description: -获取页高度
//==========================================================================
JNIEXPORT jint JNICALL JNI_FN(KGDocument_getPageHeight)(JNIEnv *env, jobject thiz, jlong doc, jint number){
  
  Jni_Document *jni_doc = (Jni_Document *)doc;
  number = number - 1;//number 从1开始
  float height = 0;
  FPDF_PAGE page_ptr = GetPageForIndex(&jni_doc->form_callbacks, jni_doc->doc.get(), number);
  height = FPDF_GetPageHeightF(page_ptr);
  printf("getPageWidth:page height: %f\r\n", height);  
  return (jint)height;
}

//==========================================================================
// Function:    getPageWidth
// Input:       jlong doc 文档句柄
//				jint number 从1开始
//
// Output:      - 页宽度
// Return:      -
// Description: -获取页宽度
//==========================================================================
JNIEXPORT jint JNICALL JNI_FN(KGDocument_getPageWidth)(JNIEnv *env, jobject thiz, jlong doc, jint number){
  
  Jni_Document *jni_doc = (Jni_Document *)doc;
  number = number - 1;//number 从1开始
  float width = 0;
  FPDF_PAGE page_ptr = GetPageForIndex(&jni_doc->form_callbacks, jni_doc->doc.get(), number);
  width = FPDF_GetPageWidthF(page_ptr);  
  return (jint)width;
}

//==========================================================================
// Function:    getPageImage
// Input:       jlong doc 文档句柄
//				jint number 从1开始
//              jfloatArray area 区域，其值是相对于页左上角的坐标
//				jfloat zoom  缩放比，取值 0.1 ~ 64，超出此范围会被修正到最大或最小值
//              jfloat rotate 旋转角度，取值 0 ~ 359
//              jint type 1 png 2 jpg
// Output:      - 图片流
// Return:      -
// Description: -获取页指定格式图片流
//==========================================================================
JNIEXPORT jbyteArray JNICALL JNI_FN(KGDocument_getPageImage)(JNIEnv *env, jobject thiz, jlong doc, jint number, jfloatArray area, jfloat zoom,jfloat rotate, jint type){
  
  jbyteArray image_data = nullptr;
  Jni_Document *jni_doc = (Jni_Document *)doc;
  number = number - 1;//number 从1开始

  jfloat *karea = NULL;
	karea = (jfloat *)env->GetPrimitiveArrayCritical(area, NULL);
	if (karea == NULL)
	{
		return image_data;
	}

	int start_x = karea[0];
	int start_y = karea[1];
	int width = karea[2];
	int height = karea[3];

	env->ReleasePrimitiveArrayCritical(area, karea, 0);
  printf("getPageImage:startx= %d;starty=%d;endx=%d;endy=%d, zoom = %f, rotate = %f\r\n", start_x, start_y, width, height, zoom, rotate);

  FPDF_PAGE page_ptr = GetPageForIndex(&jni_doc->form_callbacks, jni_doc->doc.get(), number);
  if (width == 0 || height == 0)
  {
    width = static_cast<int>(FPDF_GetPageWidthF(page_ptr));
    height = static_cast<int>(FPDF_GetPageHeightF(page_ptr));
  }
  
  width = static_cast<int>(width * zoom);
  height = static_cast<int>(height * zoom);
  int alpha = FPDFPage_HasTransparency(page_ptr) ? 1 : 0;
  ScopedFPDFBitmap bitmap(FPDFBitmap_Create(width, height, alpha));

  if (bitmap) {
    printf("getPageImage:to render page\r\n");
    FPDF_DWORD fill_color = alpha ? 0x00000000 : 0xFFFFFFFF;
    FPDFBitmap_FillRect(bitmap.get(), 0, 0, width, height, fill_color);
    FPDF_RenderPageBitmap(bitmap.get(), page_ptr, start_x, start_y, width, height, rotate, FPDF_ANNOT);
    FPDF_FFLDraw(jni_doc->form.get(), bitmap.get(), page_ptr, start_x, start_y, width, height, rotate, FPDF_ANNOT);
    FORM_DoPageAAction(page_ptr, jni_doc->form.get(), FPDFPAGE_AACTION_CLOSE);
    
    int stride = FPDFBitmap_GetStride(bitmap.get());
    void* buffer = FPDFBitmap_GetBuffer(bitmap.get());
    auto input =
        pdfium::make_span(static_cast<uint8_t*>(buffer), stride * height);
    std::vector<uint8_t> png_encoding =
        EncodePng(input, width, height, stride, FPDFBitmap_BGRA);
    printf("encoded\r\n");        
    if (png_encoding.empty()) {
      fprintf(stderr, "Failed to convert bitmap to PNG\n");
    }else{   
#if 1
      int image_size = png_encoding.size();
      printf("getPageImage:page[%ld] size = %d\r\n", number, image_size); 
      image_data = env->NewByteArray((jsize)image_size);
      if(image_data){
        env->SetByteArrayRegion(image_data, 0, image_size, (const jbyte *)&png_encoding.front());
      }else{
        fprintf(stderr, "Failed to NewByteArray\n");
      }
#endif      
    }   
  }else{
    printf("getPageImage:failed to create bitmap\r\n");
  }

  return image_data;
}

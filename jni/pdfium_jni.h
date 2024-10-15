#include "jni.h"


#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define MY_JNI_VERSION JNI_VERSION_1_8

#define JNI_FN(A) Java_com_kinggrid_krc_ ## A

#define JNULL (jlong)0L

#define DEFAULT_DPI (72.0f)
#define TRANSFORM_FACTOR (25.4f)
#define JNI_FLT_EPSILON      1.192092896e-07F

#define RUNTIME_EXCEPTION "java/lang/RuntimeException"

#ifdef __cplusplus
extern "C" {
#endif

////////////////////////////////////////////////////////////////////////////
//							    Function	 							  //
////////////////////////////////////////////////////////////////////////////
static inline jlong jlong_cast(const void *p)
{
	return (jlong)(intptr_t)p;
}

static inline int float_cmp(float a, float b)
{
	if (fabs(a - b) < JNI_FLT_EPSILON)
		return 0;

	if (a > b)
		return 1;
	
	if (a < b)
		return -1;
	return -1;
}

// UTF8 -> jstring
static inline jstring strToJstring(JNIEnv *env, const char *pStr) {
	int strLen;
	jclass jstrObj;
	jmethodID methodId;
	jbyteArray byteArray;
	jstring encode;
    if (NULL == pStr){
        return env->NewStringUTF("");
	}
	strLen = strlen(pStr);			
	jstrObj = env->FindClass("java/lang/String");
	methodId = env->GetMethodID(jstrObj, "<init>", "([BLjava/lang/String;)V");
	byteArray = env->NewByteArray(strLen);
	encode = env->NewStringUTF("utf-8");
	env->SetByteArrayRegion(byteArray, 0, strLen, (jbyte *)pStr);
	return (jstring)env->NewObject(jstrObj, methodId, byteArray, encode);
}

// jstring -> UTF-8
static inline char *jstringToUTF8(JNIEnv *env, jstring jstr) 
{
	jclass clsstring = env->FindClass("java/lang/String");
	jstring strencode = env->NewStringUTF("utf-8");
	jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
	jbyteArray barr = (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
	jsize alen = env->GetArrayLength(barr);
	jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
	char *rtn = NULL;

	if (alen > 0) 
	{
		rtn = (char *)malloc(alen + 1);
		memcpy(rtn, ba, alen);
		rtn[alen] = 0;
	}

	env->ReleaseByteArrayElements(barr, ba, 0);

	return rtn;
}

//+++++++++++++++++++++++++++++Library interface

//==========================================================================
// Function:    openDocumentFromSteam
// Input:       jstring customer_font_path自定义字体文件路径
//				
//
// Output:      -
// Return:      -
// Description: -库初始化
//==========================================================================
JNIEXPORT void JNICALL JNI_FN(KGLibrary_Init)(JNIEnv * env, jobject thiz, jstring customer_font_path);

//==========================================================================
// Function:    openDocumentFromSteam
// Input:       jstring customer_font_path自定义字体文件路径
//				
//
// Output:      -
// Return:      -
// Description: -库释放
//==========================================================================
JNIEXPORT void JNICALL JNI_FN(KGLibrary_Destroy)(JNIEnv * env, jobject thiz);


//+++++++++++++++++++++++++++++document interface

//==========================================================================
// Function:    openDocumentFromSteam
// Input:       jbyteArray data 输入文件流
//				
//
// Output:      -krc_document *doc
// Return:      -
// Description: -输入文件流，返回文档句柄, 0表示错误， 负值为错误码
//==========================================================================
JNIEXPORT jlong JNICALL JNI_FN(KGDocument_openDocumentFromSteam)(JNIEnv * env, jobject thiz, jbyteArray data);

//==========================================================================
// Function:    closeDocument
// Input:       jlong doc 文档句柄
//				
//
// Output:      - 
// Return:      -
// Description: -关闭文档
//==========================================================================

JNIEXPORT void JNICALL JNI_FN(KGDocument_closeDocument)(JNIEnv * env, jobject thiz, jlong doc);


//==========================================================================
// Function:    getPagesCount
// Input:       jlong doc 文档句柄
//				
//
// Output:      - 文档页数
// Return:      -
// Description: -获取文档页数
//==========================================================================
JNIEXPORT jint JNICALL JNI_FN(KGDocument_getPagesCount)(JNIEnv * env, jobject thiz, jlong doc);

//==========================================================================
// Function:    getPageHeight
// Input:       jlong doc 文档句柄
//				jint number 从1开始
//
// Output:      - 页高度
// Return:      -
// Description: -获取页高度
//==========================================================================
JNIEXPORT jint JNICALL JNI_FN(KGDocument_getPageHeight)(JNIEnv * env, jobject thiz, jlong doc, jint number);

//==========================================================================
// Function:    getPageWidth
// Input:       jlong doc 文档句柄
//				jint number 从1开始
//
// Output:      - 页宽度
// Return:      -
// Description: -获取页宽度
//==========================================================================
JNIEXPORT jint JNICALL JNI_FN(KGDocument_getPageWidth)(JNIEnv * env, jobject thiz, jlong doc, jint number);

//==========================================================================
// Function:    getPageImage
// Input:       jlong doc 文档句柄
//				jint number 从1开始
//              jfloatArray area 区域，
//          start_x     -   Left pixel position of the display area in
//                          bitmap coordinates.
//          start_y     -   Top pixel position of the display area in bitmap
//                          coordinates.
//          size_x      -   Horizontal size (in pixels) for displaying the page.
//          size_y      -   Vertical size (in pixels) for displaying the page.
//				jfloat zoom  缩放比，取值 0.1 ~ 64，超出此范围会被修正到最大或最小值
//              jfloat rotate 
//          rotate      -   Page orientation:
//                            0 (normal)
//                            1 (rotated 90 degrees clockwise)
//                            2 (rotated 180 degrees)
//                            3 (rotated 90 degrees counter-clockwise)
//              jint type 1 png 2 jpg, for now just support png
// Output:      - 图片流
// Return:      -
// Description: -获取页指定格式图片流
//==========================================================================
JNIEXPORT jbyteArray JNICALL JNI_FN(KGDocument_getPageImage)(JNIEnv * env, jobject thiz, jlong doc, jint number, jfloatArray area, jfloat zoom,jfloat rotate, jint type);

//+++++++++++++++++++++++Error Number
//错误码为负值
#define KERROR_BEGIN		-10000L
#define KERROR_GETBBYTEARRAYELEMENTS	(KERROR_BEGIN + 1)
#define KERROR_LOADPAGE					(KERROR_BEGIN + 2)
#define KERROR_NOMEMORY					(KERROR_BEGIN + 3)
#ifdef __cplusplus
}
#endif

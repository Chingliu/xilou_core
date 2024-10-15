// 下列 ifdef 块是创建使从 DLL 导出更简单的
// 宏的标准方法。此 DLL 中的所有文件都是用命令行上定义的 HBSUBSET_EXPORTS
// 符号编译的。在使用此 DLL 的
// 任何项目上不应定义此符号。这样，源文件中包含此文件的任何其他项目都会将
// HBSUBSET_API 函数视为是从 DLL 导入的，而此 DLL 则将用此宏定义的
// 符号视为是被导出的。
#ifdef HBSUBSET_EXPORTS
#define HBSUBSET_API __declspec(dllexport)
#else
#define HBSUBSET_API __declspec(dllimport)
#endif

// 此类是从 dll 导出的
class HBSUBSET_API Chbsubset {
public:
	Chbsubset(void);
	// TODO: 在此处添加方法。
};

extern HBSUBSET_API int nhbsubset;

extern "C" {
    // 成功返回 hb_face_t* new_face.
//失败返回空
HBSUBSET_API void * fnhbsubset(const char *fontname, const char *utf8_strings);

HBSUBSET_API void fnhb_release_face(void* face);

    // 成功返回 hb_blob_t *.
//失败返回空
HBSUBSET_API void* fnhb_getblob_from_face(void* face);

HBSUBSET_API void fnhb_release_blob(void* blob);
// 成功返回 char *., 
//失败返回空
HBSUBSET_API const char* fnhb_getdata_from_blob(void* blob,
                                                unsigned int* outsize);
}

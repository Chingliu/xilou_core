// hbsubset.cpp : 定义 DLL 的导出函数。
//

#include "pch.h"
#include "framework.h"
#include "../../hbsubset.h"
#include "hb.h"


// 这是导出变量的一个示例
HBSUBSET_API int nhbsubset=0;

namespace {
enum {
  UTFmax = 4,         /* maximum bytes per rune */
  Runesync = 0x80,    /* cannot represent part of a UTF sequence (<) */
  Runeself = 0x80,    /* rune and UTF sequences are the same (<) */
  Runeerror = 0xFFFD, /* decoding error in UTF */
  Runemax = 0x10FFFF, /* maximum rune value */
};
enum {
  Bit1 = 7,
  Bitx = 6,
  Bit2 = 5,
  Bit3 = 4,
  Bit4 = 3,
  Bit5 = 2,

  T1 = ((1 << (Bit1 + 1)) - 1) ^ 0xFF, /* 0000 0000 */
  Tx = ((1 << (Bitx + 1)) - 1) ^ 0xFF, /* 1000 0000 */
  T2 = ((1 << (Bit2 + 1)) - 1) ^ 0xFF, /* 1100 0000 */
  T3 = ((1 << (Bit3 + 1)) - 1) ^ 0xFF, /* 1110 0000 */
  T4 = ((1 << (Bit4 + 1)) - 1) ^ 0xFF, /* 1111 0000 */
  T5 = ((1 << (Bit5 + 1)) - 1) ^ 0xFF, /* 1111 1000 */

  Rune1 = (1 << (Bit1 + 0 * Bitx)) - 1, /* 0000 0000 0111 1111 */
  Rune2 = (1 << (Bit2 + 1 * Bitx)) - 1, /* 0000 0111 1111 1111 */
  Rune3 = (1 << (Bit3 + 2 * Bitx)) - 1, /* 1111 1111 1111 1111 */
  Rune4 = (1 << (Bit4 + 3 * Bitx)) - 1, /* 0001 1111 1111 1111 1111 1111 */

  Maskx = (1 << Bitx) - 1, /* 0011 1111 */
  Testx = Maskx ^ 0xFF,    /* 1100 0000 */

  Bad = Runeerror,
};
int
chartorune (int *rune, const char *str)
{
  int c, c1, c2, c3;
  long l;

  /*
   * one character sequence
   *	00000-0007F => T1
   */
  c = *(const unsigned char *) str;
  if (c < Tx)
  {
    *rune = c;
    return 1;
  }

  /*
   * two character sequence
   *	0080-07FF => T2 Tx
   */
  c1 = *(const unsigned char *) (str + 1) ^ Tx;
  if (c1 & Testx) goto bad;
  if (c < T3)
  {
    if (c < T2) goto bad;
    l = ((c << Bitx) | c1) & Rune2;
    if (l <= Rune1) goto bad;
    *rune = l;
    return 2;
  }

  /*
   * three character sequence
   *	0800-FFFF => T3 Tx Tx
   */
  c2 = *(const unsigned char *) (str + 2) ^ Tx;
  if (c2 & Testx) goto bad;
  if (c < T4)
  {
    l = ((((c << Bitx) | c1) << Bitx) | c2) & Rune3;
    if (l <= Rune2) goto bad;
    *rune = l;
    return 3;
  }

  /*
   * four character sequence (21-bit value)
   *	10000-1FFFFF => T4 Tx Tx Tx
   */
  c3 = *(const unsigned char *) (str + 3) ^ Tx;
  if (c3 & Testx) goto bad;
  if (c < T5)
  {
    l = ((((((c << Bitx) | c1) << Bitx) | c2) << Bitx) | c3) & Rune4;
    if (l <= Rune3) goto bad;
    *rune = l;
    return 4;
  }
  /*
   * Support for 5-byte or longer UTF-8 would go here, but
   * since we don't have that, we'll just fall through to bad.
   */

  /*
   * bad decoding
   */
bad:
  *rune = Bad;
  return 1;
}

}

extern "C" {
HBSUBSET_API void *
fnhbsubset (const char *fontname, const char *utf8_strings)
{
  hb_subset_input_t *input = NULL;
  hb_face_t *face = NULL, *new_face = NULL;
  hb_blob_t *blob = NULL;
  hb_blob_t *result = NULL;
  hb_set_t *codepoints = NULL;
  unsigned int face_index = 0;

  blob = hb_blob_create_from_file_or_fail (fontname);
  if (blob)
  {
    face = hb_face_create (blob, face_index);
    if (face)
    {
      input = hb_subset_input_create_or_fail ();
      if (input)
      {
	    {
	      codepoints = hb_subset_input_unicode_set (input);
	      if (codepoints)
	      {
	        int t;
	        int char_code = '?';
	        
	        const char *data = utf8_strings;
	        int len = strlen(data);

	        hb_set_clear (codepoints);
	        while (data && len > 0)
	        {
            t = chartorune (&char_code, data);
            hb_set_add (codepoints, char_code);
	          data += t;
	          len -= t;
	        }
	      }

	      new_face = hb_subset_or_fail (face, input);
	      if (new_face)
	      {
	        result = hb_face_reference_blob (new_face);
	      }
	    }

	    if(!result){
	      if (new_face) {
          hb_face_destroy (new_face);
        }
        new_face = nullptr;
	    }else{
        hb_blob_destroy(result);
      }

	    hb_subset_input_destroy (input);
      }
      hb_face_destroy (face);
    }
    hb_blob_destroy (blob);
  }
  return new_face;
}

HBSUBSET_API void* fnhb_getblob_from_face(void* face)
{
  if (!face)
    return nullptr;
  hb_face_t* hb_face = reinterpret_cast<hb_face_t*>(face);
  return hb_face_reference_blob(hb_face);
}

// 成功返回 char *.
//失败返回空
HBSUBSET_API const char* fnhb_getdata_from_blob(void* blob,
                                                unsigned int* outsize) {
  if (!blob)
    return nullptr;
  unsigned int size;
  const char* data =
      hb_blob_get_data(reinterpret_cast<hb_blob_t*>(blob), &size);
  if (outsize)
  {
    *outsize = size;
  }
  return data;
}
HBSUBSET_API void fnhb_release_face(void* face) {
  if (face) {
    hb_face_destroy(reinterpret_cast<hb_face_t*>(face));
  }
}


HBSUBSET_API void fnhb_release_blob(void* blob) {
  if (blob)
  {
    hb_blob_destroy(reinterpret_cast<hb_blob_t*>(blob));
  }
}



} //end of extern "C" {
// 这是已导出类的构造函数。
Chbsubset::Chbsubset()
{
    return;
}

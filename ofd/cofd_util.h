#ifndef OFD_COFD_CUTIL_HEADER_H
#define OFD_COFD_CUTIL_HEADER_H
#include <string>
#include "ofd/cofd_common.h"
#include "core/fxcrt/fx_coordinates.h"
#include "core/fxge/dib/cfx_dibitmap.h"
class vector;
namespace ofd_util {
std::string DbgWriteBitmapAsBmp(const char* name,
                                int num,
                                const RetainPtr<CFX_DIBitmap>& pBitmap);

CFX_RectF ParseSTBOX(const WideString& STBOX_string);

CFX_PointF ParseSTPoint(const WideString& STPoint_string);

void SaveBinaryFile(const char* fpath, const unsigned char* data, int datalen);

void ConvertBitmapAsBmp(const RetainPtr<CFX_DIBitmap>& pBitmap,
                        std::vector<uint8_t>& bmp);
}//namespace ofd_util


#endif  // !OFD_COFD_CUTIL_HEADER_H

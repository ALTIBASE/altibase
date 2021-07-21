#if !defined(_O_ACL_LZ4_H_)
#define _O_ACL_LZ4_H_

#include <lz4.h>
#include <acpTypes.h>

ACP_EXTERN_C_BEGIN

#define ACL_LZ4_COMPRESSBOUND(isize) LZ4_COMPRESSBOUND(isize)

#define SChar  acp_char_t
#define SInt   acp_sint32_t
SInt aclLZ4_decompress_safe ( const SChar * src, 
                              SChar       * dst, 
                              SInt          compressedSize, 
                              SInt          maxDecompressedSize );

SInt aclLZ4_compress_fast ( const SChar * src, 
                            SChar       * dst, 
                            SInt          srcSize, 
                            SInt          dstCapacity, 
                            SInt          acceleration );

#undef SInt
#undef SChar

ACP_EXTERN_C_END

#endif

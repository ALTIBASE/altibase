#include <aclLZ4.h>
//#include <lz4.h>

typedef acp_sint32_t SInt;
typedef acp_char_t   SChar;


SInt aclLZ4_decompress_safe ( const SChar * src,
                              SChar       * dst,
                              SInt          compressedSize,
                              SInt          maxDecompressedSize )
{
    return LZ4_decompress_safe( src,
                                dst,
                                compressedSize,
                                maxDecompressedSize );
}

SInt aclLZ4_compress_fast ( const SChar * src,
                            SChar       * dst,
                            SInt          srcSize,
                            SInt          dstCapacity,
                            SInt          acceleration )
{
    return LZ4_compress_fast( src,
                              dst,
                              srcSize,
                              dstCapacity,
                              acceleration );
}

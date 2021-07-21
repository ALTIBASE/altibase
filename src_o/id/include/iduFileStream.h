/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduFileStream.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#ifndef _O_IDU_FILESTREAM_H_
#define _O_IDU_FILESTREAM_H_

#include <idl.h>

class iduFileStream
{
public:
    static IDE_RC openFile( FILE** aFp,
                            SChar* aFilePath,
                            SChar* aMode );

    static IDE_RC closeFile( FILE* aFp );

    static IDE_RC getString( SChar* aStr,
                             SInt   aLength,
                             FILE*  aFp );

    static IDE_RC putString( SChar* aStr,
                             FILE*  aFp );

    static IDE_RC flushFile( FILE* aFp );

    static IDE_RC removeFile( SChar* aFilePath );

    static IDE_RC renameFile( SChar* aOldFilePath,
                              SChar* aNewFilePath,
                              idBool aOverWrite );
};

#endif // _O_IDU_FILESTREAM_H_

/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iduRunTimeInfo.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/
#ifndef _O_IDU_RUNTIMEINFO_H_
#define _O_IDU_RUNTIMEINFO_H_ 1

#include <idl.h>

#define IDU_OBJ_MAX_NAME 256

typedef void (*iduStatusObject)(SChar* a_strBuffer);

typedef struct runTimeInfo
{
    SChar              name[IDU_OBJ_MAX_NAME];
    iduStatusObject    statusObjectFunc;
} runTimeInfo;

class iduRunTimeInfo
{
public:
    iduRunTimeInfo();
     ~iduRunTimeInfo();

    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC add(SChar *strName,
                      iduStatusObject* statusObjectFunc);
};

#endif	// _O_RUNTIMEINFO_H_

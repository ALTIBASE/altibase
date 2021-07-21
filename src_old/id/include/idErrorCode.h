/***********************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idErrorCode.h 66405 2014-08-13 07:15:26Z djin $
 **********************************************************************/

/***********************************************************************
 *	File Name 		:	idErrorCode.h
 *
 *  Modify			:	Sung-Jin, Kim
 *
 *	Related Files	:
 *
 *	Description		:	ID 에러 코드 화일
 *
 *
 **********************************************************************/

#include <idl.h>

#ifndef ID_ERROR_CODE_H
#define ID_ERROR_CODE_H  1

#ifndef BUILD_FOR_UTIL
typedef enum _id_err_code
{
#include "idErrorCode.ih"

    ID_MAX_ERROR_CODE


} ID_ERR_CODE;


/* ------------------------------------------------
 *  Trace Code
 * ----------------------------------------------*/

IDL_EXTERN_C const SChar *gIDTraceCode[];

#include "ID_TRC_CODE.ih"

#endif

#endif


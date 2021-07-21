/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

#ifndef _O_DKO_LINK_INFO_H_
#define _O_DKO_LINK_INFO_H_ 1

#ifdef ALTIBASE_PRODUCT_XDB

    /* nothing to do */

#else

/* ----------------------------------------------------------
 * DK module link object satus 
 * ---------------------------------------------------------*/
typedef enum
{
    
    DKO_LINK_OBJ_NON  = 0,
    DKO_LINK_OBJ_CREATED,
    DKO_LINK_OBJ_META,
    DKO_LINK_OBJ_READY,
    DKO_LINK_OBJ_TOUCH,
    DKO_LINK_OBJ_ACTIVE
    
} dkoLinkInfoStatus;

/* Define DB-Link linformation for performance view */
typedef struct dkoLinkInfo
{
    UInt               mId;        /* 이 데이터베이스 링크의 식별자        */
    dkoLinkInfoStatus  mStatus;    /* 링크 객체의 상태                     */
    UInt               mRefCnt;    /* 링크 객체를 참조하는 session 의 개수 */
} dkoLinkInfo;

IDE_RC dkoLinkInfoInitialize( void );
IDE_RC dkoLinkInfoFinalize( void );

IDE_RC dkoLinkInfoCreate( UInt aLinkId );
IDE_RC dkoLinkInfoDestroy( UInt aLinkId );

UInt dkoLinkInfoGetLinkInfoCount( void );
IDE_RC dkoLinkInfoGetForPerformanceView( dkoLinkInfo ** aInfoArray, UInt * aInfoCount );

IDE_RC dkoLinkInfoSetStatus( UInt aLinkId, dkoLinkInfoStatus aStatus );
IDE_RC dkoLinkInfoGetStatus( UInt aLinkId, dkoLinkInfoStatus *aStatus );

IDE_RC dkoLinkInfoIncresaseReferenceCount( UInt aLinkId );
IDE_RC dkoLinkInfoDecresaseReferenceCount( UInt aLinkId );
IDE_RC dkoLinkInfoGetReferenceCount( UInt aLinkId, UInt * aReferenceCount );

#endif /* ALTIBASE_PRODUCT_XDB */

#endif /* _O_DKO_LINK_INFO_H_ */

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
 

/***********************************************************************
 * $id$
 **********************************************************************/

#ifndef _O_DKO_DEF_H_
#define _O_DKO_DEF_H_ 1


#include <idTypes.h>
#include <idkAtomic.h>

#include <dkDef.h>

#define DKO_LINK_USER_MODE_PUBLIC           (0)
#define DKO_LINK_USER_MODE_PRIVATE          (1)

#define DKO_GET_USER_SESSION_ID( aSession ) ((aSession)->mUserId)

/* Define DK module link object */
typedef struct dkoLink
{
    UInt            mId;        /* 이 데이터베이스 링크의 식별자        */
    UInt            mUserId;    /* 링크를 생성한 user 의 id             */
    SInt            mUserMode;  /* 링크 객체가 public/private 객체인지  */
    SInt            mLinkType;  /* heterogeneous/homogeneous link 인지  */
    smOID           mLinkOID;   /* SM 에서 부여하는 object id           */
    SChar           mLinkName[DK_NAME_LEN + 1];     /* 링크 객체의 이름 */
    SChar           mTargetName[DK_NAME_LEN + 1];   /* target 원격서버  */
    SChar           mRemoteUserId[DK_NAME_LEN + 1]; /* 원격서버 접속 id */
    SChar           mRemoteUserPasswd[DK_NAME_LEN + 1];/* 접속 password */
} dkoLink;

#endif /* _O_DKO_DEF_H_ */


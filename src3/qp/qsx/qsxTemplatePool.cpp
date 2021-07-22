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
 * $Id: qsxTemplatePool.cpp 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#include <idl.h>
#include <ide.h>
#include <qcg.h>
#include <qsParseTree.h>
#include <qsxTemplatePool.h>

void qsxTemplatePool::addToHead  (
    qsxTemplatePoolInfo  * aPoolInfo,
    qsxTemplateInfo      * aTmplInfo)
{
    #define IDE_FN "qsxTemplatePool::addToHead"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aTmplInfo->prev           = NULL;
    aTmplInfo->next           = aPoolInfo->poolHead;

    if (aPoolInfo->poolHead != NULL )
    {
        aPoolInfo->poolHead->prev = aTmplInfo;
    }

    if (aPoolInfo->poolTail == NULL )
    {
        aPoolInfo->poolTail = aTmplInfo;
    }
    
    aPoolInfo->poolHead       = aTmplInfo;
    
    #undef IDE_FN
}

void qsxTemplatePool::addToTail  (
    qsxTemplatePoolInfo  * aPoolInfo,
    qsxTemplateInfo      * aTmplInfo)
{
    #define IDE_FN "qsxTemplatePool::addToTail"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aTmplInfo->prev           = aPoolInfo->poolTail;
    aTmplInfo->next           = NULL;

    if (aPoolInfo->poolTail != NULL )
    {
        aPoolInfo->poolTail->next = aTmplInfo;
    }

    if (aPoolInfo->poolHead == NULL )
    {
        aPoolInfo->poolHead = aTmplInfo;
    }
    
    aPoolInfo->poolTail       = aTmplInfo;
    
    #undef IDE_FN
}

void qsxTemplatePool::remove     (
    qsxTemplatePoolInfo  * aPoolInfo,
    qsxTemplateInfo      * aRemTmplInfo)
{
    #define IDE_FN "qsxTemplatePool::remove"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxTemplateInfo * sPrev ;
    qsxTemplateInfo * sNext ;
    
    sPrev = aRemTmplInfo->prev ;
    sNext = aRemTmplInfo->next ;

    if ( aRemTmplInfo == aPoolInfo->poolHead )
    {
        aPoolInfo->poolHead = aRemTmplInfo->next ;
    }
    
    if ( aRemTmplInfo == aPoolInfo->poolTail )
    {
        aPoolInfo->poolTail = aRemTmplInfo->prev ;
    }
    
    if (sPrev != NULL )
    {
        sPrev->next = sNext;
    }

    if (sNext != NULL )
    {
        sNext->prev = sPrev;
    }
    
    #undef IDE_FN
}

IDE_RC qsxTemplatePool::grow       (
    qsxTemplatePoolInfo  * aPoolInfo,
    UInt                   aCount )
{
    #define IDE_FN "qsxTemplatePool::grow"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxTemplateInfo * sClonedTmpl;
    
    while ( aCount-- > 0 )
    {
        IDE_TEST( clone( aPoolInfo, & sClonedTmpl )
                  != IDE_SUCCESS );
        addToHead( aPoolInfo, sClonedTmpl );
    }
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

    #undef IDE_FN
}

IDE_RC qsxTemplatePool::clone    (
    qsxTemplatePoolInfo   * aPoolInfo,
    qsxTemplateInfo      ** aTmplInfo)
{
    #define IDE_FN "qsxTemplatePool::clone"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxTemplateInfo   * sNewTmplInfo;

    IDE_TEST(aPoolInfo->poolMem->cralloc( idlOS::align8((UInt) ID_SIZEOF( qsxTemplateInfo ) ),
                                          (void**)&sNewTmplInfo)
             != IDE_SUCCESS);
    
    IDE_TEST( qcg::cloneAndInitTemplate( aPoolInfo->poolMem,
                                         aPoolInfo->procTemplate,
                                         & sNewTmplInfo->tmplate )
              != IDE_SUCCESS );

    *aTmplInfo = sNewTmplInfo;
    
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

    #undef IDE_FN
}


IDE_RC qsxTemplatePool::initialize(
    qsxTemplatePoolInfo  * aPoolInfo,
    qsOID                  aProcOID,
    qcTemplate           * aProcTemplate,
    iduMemory            * aPoolMem,
    UInt                   aInitPoolCount,
    UInt                   aExtendPoolCount )
{
    #define IDE_FN "qsxTemplatePool::initialize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    aPoolInfo->procOID       = aProcOID;
    aPoolInfo->procTemplate  = aProcTemplate;
    aPoolInfo->poolMem       = aPoolMem;
    aPoolInfo->extPoolCnt    = aExtendPoolCount;
    aPoolInfo->poolHead      = NULL;
    aPoolInfo->poolTail      = NULL;

    IDE_ASSERT( aInitPoolCount > 0 );
    
    IDE_TEST( grow ( aPoolInfo, aInitPoolCount ) != IDE_SUCCESS );
 
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

    #undef IDE_FN
}

// get head if unused. otherwise make pool to grow.
IDE_RC qsxTemplatePool::checkout  (
    qsxTemplatePoolInfo  * aPoolInfo,
    qsxTemplateInfo     ** aTmplInfo)
{
    #define IDE_FN "qsxTemplatePool::checkout"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    qsxTemplateInfo * sCheckedOutTmplInfo;
    
    if ( aPoolInfo->poolHead == NULL )
    {
        IDE_TEST( grow( aPoolInfo, aPoolInfo->extPoolCnt )
                  != IDE_SUCCESS );
    }

    sCheckedOutTmplInfo = aPoolInfo->poolHead;

    IDE_ASSERT( sCheckedOutTmplInfo != NULL );
    
    remove ( aPoolInfo, sCheckedOutTmplInfo );

    *aTmplInfo = sCheckedOutTmplInfo ;
 
    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

    #undef IDE_FN
}

// move checked in items (unused items to head)
void qsxTemplatePool::checkin   (
    qsxTemplatePoolInfo  * aPoolInfo,
    qsxTemplateInfo      * aTmplInfo)
{
    #define IDE_FN "qsxTemplatePool::checkin"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    addToHead( aPoolInfo, aTmplInfo );
    
    #undef IDE_FN
}


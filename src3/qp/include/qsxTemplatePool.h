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
 
#ifndef _O_QSX_TEMPLATE_POOL_H_
#define _O_QSX_TEMPLATE_POOL_H_ 1

#include <qc.h>
#include <qsxProc.h>

typedef struct qsxTemplateInfo
{
    qcTemplate         tmplate;
    qsxTemplateInfo  * next ;
    qsxTemplateInfo  * prev ;
} qsxTemplateInfo ;


typedef struct qsxTemplatePoolInfo
{
    qsOID              procOID;
    qcTemplate       * procTemplate;
    iduMemory        * poolMem;
    UInt               extPoolCnt;
    qsxTemplateInfo  * poolHead;
    qsxTemplateInfo  * poolTail;
} qsxTemplatePoolInfo ;

/* unused items always exist at the head. */
class qsxTemplatePool
{
private :
    static void   addToHead  ( qsxTemplatePoolInfo  * aPoolInfo,
                               qsxTemplateInfo      * aTmplInfo);
    static void   addToTail  ( qsxTemplatePoolInfo  * aPoolInfo,
                               qsxTemplateInfo      * aTmplInfo);
    static void   remove     ( qsxTemplatePoolInfo  * aPoolInfo,
                               qsxTemplateInfo      * aTmplInfo);
    static IDE_RC grow       ( qsxTemplatePoolInfo  * aPoolInfo,
                               UInt                   aCount );
    static IDE_RC clone      ( qsxTemplatePoolInfo  * aPoolInfo,
                               qsxTemplateInfo     ** aTmplInfo);
public :
    static IDE_RC initialize( qsxTemplatePoolInfo  * aPoolInfo,
                              qsOID                  aProcOID,
                              qcTemplate           * aProcTemplate,
                              iduMemory            * aPoolMem,
                              UInt                   aInitPoolCount,
                              UInt                   aExtendPoolCount );

    // get head if unused. otherwise make pool to grow.
    static IDE_RC checkout  ( qsxTemplatePoolInfo  * aPoolInfo,
                              qsxTemplateInfo     ** aTmplInfo);
    // move checked in items (unused items to head)
    static void   checkin   ( qsxTemplatePoolInfo  * aPoolInfo,
                              qsxTemplateInfo      * aTmplInfo);
};

#endif /* _O_QSX_TEMPLATE_POOL_H_ */









/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
/*******************************************************************************
 * $Id$
 ******************************************************************************/

#if !defined(_O_IDR_SHM_TX_PENDING_OP_H_)
#define _O_IDR_SHM_TX_PENDING_OP_H_ 1

#include <idv.h>
#include <idTypes.h>
#include <iduShmDef.h>
#include <idrDef.h>

typedef enum idrPendingOpType
{
    IDR_PENDING_OPERATION_NULL,
    IDR_PENDING_OPERATION_LOGFILE_OPEN,
    IDR_PENDING_OPERATION_MAX
} idrPendingOpType;

#define IDR_PENDING_OP_PARAM_CNT    2
#define IDR_PENDING_OP_PARAM_SIZE   (ID_SIZEOF(ULong) * IDR_PENDING_OP_PARAM_CNT)

/******************************************************************************
 * Description : Shared Memory Tx의 Pending Operation 수행을 위한 구조체
 *****************************************************************************/
typedef struct idrStPendingOp
{
    // Pending Operation간 Node 연결을 위한 ShmList Node
    iduShmListNode      mNode;
    // Pending Operation 구분
    idrPendingOpType    mPendingOpType;
    // Pending Operation 수행시 전달될 Parameter들의 사이즈
    UInt                mParamSize;
    // Pending Operation 수행시 전달될 Parameter
    ULong               mParam[IDR_PENDING_OP_PARAM_SIZE / ID_SIZEOF(ULong)];
} idrPendingOp;

typedef IDE_RC (*idrDoPendingOpFunc)( idvSQL*,
                                      iduShmTxInfo *,
                                      idrSVP *,
                                      idrStPendingOp * );


class idrShmTxPendingOp
{
public:
    static IDE_RC initialize();
    static IDE_RC destroy();

    static IDE_RC registerPendingOpFunc( idrPendingOpType    aPendingOpType,
                                         idrDoPendingOpFunc  aPendingOpFunc );

    static void initializePendingOpList( iduShmTxInfo * aShmTxInfo );

    static IDE_RC addStPendingOp( idvSQL           * aStatistics,
                                  iduShmTxInfo     * aShmTxInfo,
                                  idrPendingOpType   aPendingOpType,
                                  void             * aParam,
                                  UInt               aParamSize,
                                  idrStPendingOp  ** aPendingOp );

    static IDE_RC removeStPendingOp( iduShmTxInfo   * aShmTxInfo,
                                     idrStPendingOp * aPendingOp );

    static IDE_RC freeStPendingOp( idvSQL         * aStatistics,
                                   iduShmTxInfo   * aShmTxInfo,
                                   idrStPendingOp * aPendingOp );

    static IDE_RC executePendingOp( idvSQL         * aStatistics,
                                    iduShmTxInfo   * aShmTxInfo,
                                    idrSVP         * aVSavepoint,
                                    idrStPendingOp * aPendingOp );

    static IDE_RC executePendingOpAll( idvSQL       * aStatistics,
                                       iduShmTxInfo * aShmTxInfo );

    static IDE_RC freeAllStFreePendingOpList( idvSQL       * aStatistics,
                                              iduShmTxInfo * aShmTxInfo );
};


#endif /* _O_IDR_SHM_TX_PENDING_OP_H_ */

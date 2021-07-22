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
 * $Id$
 **********************************************************************/

#ifndef _O_SDF_H_
#define _O_SDF_H_ 1

#include <idl.h>
#include <mt.h>
#include <sdi.h>

#define SDF_QUERY_LEN (1024)
#define SDF_DIGEST_LEN (64)

typedef enum
{
    SDF_REPL_CREATE,
    SDF_REPL_ADD,
    SDF_REPL_START,
    SDF_REPL_STOP,
    SDF_REPL_FLUSH,
    SDF_REPL_DROP
} sdfReplicationFunction;

typedef struct sdfArgument
{
    SChar       * mArgument1; // partition name or value
    SChar       * mArgument2; // node name
    idBool        mIsCheck;   // irregular option check
    idBool        mIsFlush;   // use SET_SHARD_TABLE_SHARDKEY()
    sdfArgument * mNext;
} sdfArgument;

typedef enum
{
    SDF_REPL_SENDER,
    SDF_REPL_RECEIVER
}sdfReplDirectionalRole;


class sdf
{
public:
    /* shard meta enable시에만 동작하는 함수들 */
    static const mtfModule * extendedFunctionModules[];

    /* data node에서 항상 동작하는 함수들 */
    static const mtfModule * extendedFunctionModules2[];

    static IDE_RC checkShardObjectSchema( qcStatement * aStatement,
                                          SChar       * aSqlStr );
    
    static IDE_RC makeArgumentList( qcStatement    * aStatement,
                                    SChar          * aArgumentListStr,
                                    sdfArgument   ** aArgumentList,
                                    UInt           * aArgumentCnt,
                                    SChar          * aDefaultPartitionNameStr );
    
    static IDE_RC stringTrim( SChar * aDestString,
                              SChar * aSrcString );

    static IDE_RC stringRemoveCrlf( SChar * aDestString,
                                    SChar * aSrcString );
    
    static IDE_RC validateNode( qcStatement    * aStatement,
                                sdfArgument    * aArgumentList,
                                SChar          * aNodeName,
                                sdiSplitMethod   aSplitMethod );
    
    static IDE_RC makeReplName( SChar * aDestReplName,
                                SChar * aSrcReplName );
    
    static IDE_RC executeRemoteSQLWithNewTrans( qcStatement * aStatement,
                                                SChar       * aNodeName,
                                                SChar       * aSqlStr,
                                                idBool        aIsDDLTimeOut );

    static IDE_RC remoteSQLWithNewTrans( qcStatement * aStatement,
                                         SChar       * aSqlStr );
    
    static IDE_RC createReplicationForInternal( qcStatement * aStatement,
                                                SChar       * aNodeName,
                                                SChar       * aReplName,
                                                SChar       * aHostIP,
                                                UInt          aPortNo );

    static IDE_RC startReplicationForInternal( qcStatement * aStatement,
                                               SChar       * aNodeName,
                                               SChar       * aReplName,
                                               SInt          aReplParallelCnt );

    static IDE_RC stopReplicationForInternal( qcStatement * aStatement,
                                              SChar       * aNodeName,
                                              SChar       * aReplName );

    static IDE_RC dropReplicationForInternal( qcStatement * aStatement,
                                              SChar       * aNodeName,
                                              SChar       * aReplName );
    
    static IDE_RC searchReplicaSet( qcStatement            * aStatement,
                                    iduList                * aNodeInfoList,
                                    sdiReplicaSetInfo      * aReplicaSetInfo,
                                    SChar                  * aNodeNameStr,
                                    SChar                  * aUserNameStr,
                                    SChar                  * aTableNameStr,
                                    SChar                  * aPartNameStr,
                                    SInt                     aReplParallelCnt,
                                    sdfReplicationFunction   aReplicationFunction,
                                    sdfReplDirectionalRole   aRole,
                                    UInt                     aNth );

    static IDE_RC getCurrentAccessMode( qcStatement     * aStatement,
                                        SChar           * aTableNameStr,
                                        UInt            * aAccessMode );

    static IDE_RC runRemoteQuery( qcStatement * aStatement,
                                  SChar       * aSqlStr );
    
};

#endif /* _O_SDF_H_ */

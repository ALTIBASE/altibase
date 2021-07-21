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
 * $Id: qmxShard.h 88285 2020-08-05 05:49:37Z bethy $
 **********************************************************************/

#ifndef _O_QMX_SHARD_H_
#define _O_QMX_SHARD_H_ 1

#include <idl.h>
#include <idu.h>
#include <ide.h>
#include <qci.h>
#include <sdi.h>
#include <sdlStatement.h>

/*************************************************************************
 * PROJ-2728 Server-side Sharding LOB
 *************************************************************************/
class qmxShard
{
public:

    static IDE_RC initializeLobInfo( qcStatement  * aStatement,
                                     qmxLobInfo  ** aLobInfo,
                                     UShort         aSize );

    static void initLobInfo( qmxLobInfo  * aLobInfo );

    static void clearLobInfo( qmxLobInfo  * aLobInfo );

    static IDE_RC addLobInfoForCopy( qmxLobInfo    * aLobInfo,
                                      smLobLocator   aLocator,
                                      UShort         aBindId );

    static IDE_RC addLobInfoForOutBind( qmxLobInfo   * aLobInfo,
                                        UShort         aBindId );

    static IDE_RC addLobInfoForPutLob( qmxLobInfo   * aLobInfo,
                                       UShort         aBindId );

    static IDE_RC addLobInfoForOutBindNonLob( qmxLobInfo   * aLobInfo,
                                              UShort         aBindId );

    static IDE_RC copyAndOutBindLobInfo( qcStatement    * aStatement,
                                         sdiConnectInfo * aConnectInfo,
                                         UShort           aIndex,
                                         qmxLobInfo     * aLobInfo,
                                         sdiDataNode    * aDataNode );

    static IDE_RC closeLobLocatorForCopy( idvSQL         * aStatistics,
                                          qmxLobInfo     * aLobInfo);

private:

    static IDE_RC copyChar2Lob( mtdCharType      * aCharValue,
                                mtdLobType       * aLobValue,
                                UInt               aDataSize );

    static IDE_RC copyBinary2Lob( mtdBinaryType  * aBinaryValue,
                                  mtdLobType     * aLobValue,
                                  UInt             aDataSize );

};

#endif /* _O_QMX_SHARD_H_ */

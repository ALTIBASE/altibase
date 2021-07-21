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
 * $Id: smcSequence.h 88753 2020-10-05 00:19:19Z khkwak $
 **********************************************************************/

#ifndef _O_SMC_SEQUENCE_H_
#define _O_SMC_SEQUENCE_H_ 1


#include <smDef.h>
#include <smcDef.h>

class smcSequence
{
public:

    static IDE_RC createSequence( void*              aTrans,
                                  SLong              aStartSequence,
                                  SLong              aIncSequence,
                                  SLong              aSyncInterval,
                                  SLong              aMinSequence,
                                  SLong              aMaxSequence,
                                  UInt               aFlag,
                                  smcTableHeader**   aTable );

    static IDE_RC alterSequence( void*             aTrans,
                                 smcTableHeader*   aTableHeader,
                                 SLong             aIncSequence,
                                 SLong             aSyncInterval,
                                 SLong             aMaxSequence,
                                 SLong             aMinSequence,
                                 UInt              aFlag,
                                 idBool            aIsRestart,
                                 SLong             aStartSequence,
                                 SLong*            aLastSyncSeq );

    /* BUG-45929 */
    static IDE_RC resetSequence( void                * aTrans,
                                 smcTableHeader      * aTableHeader );

    static IDE_RC readSequenceCurr( smcTableHeader  * aTableHeader,
                                    SLong           * aValue );

    static IDE_RC setSequenceCurr(  void*             aTrans,
                                    smcTableHeader  * aTableHeader,
                                    SLong             aValue );

    static IDE_RC readSequenceNext( void*                aTrans,
                                    smcTableHeader*      aTableHeader,
                                    SLong*               aValue,
                                    smiSeqTableCallBack* aCallBack );

    static IDE_RC refineSequence( smcTableHeader * aTableHeader );
};

#endif /* _O_SMC_SEQUENCE_H_ */


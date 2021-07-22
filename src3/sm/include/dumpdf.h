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
 * $Id: dumpdf.h 82075 2018-01-17 06:39:52Z jina.kim $
 **********************************************************************/

#ifndef O_dumpci_H
#define O_dumpci_H 1

#include <idl.h>
#include <ideErrorMgr.h>

#include <smp.h>
#include <smr.h>
#include <smuUtility.h>

SChar     *gDbf = NULL;
smOID      gOID = SM_NULL_OID;
scPageID   gPID = SM_NULL_PID;
UInt       gPL = 0;
UInt       gLS = 0;
SChar     *gJOB = NULL;
UInt       gDisplay = 0;

UChar    **gPages = NULL;
smmMemBase gMB;
scOffset   gPos;

void   dumpMeta();
IDE_RC dumpMB();
IDE_RC dumpCT();
IDE_RC dumpPLE( smcTableHeader *aCatTblHdr );
IDE_RC dumpPH( scPageID aPid, SChar aType, SInt aFlag = 0 );
IDE_RC dumpPage( scPageID aPid, idBool aFlag );
IDE_RC dumpTH( smcTableHeader *aTH, idBool aFlag = ID_FALSE );
IDE_RC dumpSH( smpSlotHeader *aSH );

IDE_RC allocDbf( smmTBSNode * aTBSNode, smmDatabaseFile *aDb, scPageID aPid );
IDE_RC destroyDbf( smmDatabaseFile *aDb );

IDE_RC allocPages();
IDE_RC destroyPages();

IDE_RC readPage( smmTBSNode * aTBSNode, SInt aPid );
IDE_RC readMB(smmTBSNode * aTBSNode);

IDE_RC verifyDbf(smmMemBase *aMembase);

void usage();
IDE_RC loadProperty();
void parseArgs( int &argc, char **&argv );

smpPersPage* pid_ptr( smmTBSNode * aTBSNode, SInt aPid );

void initForScan( smcTableHeader    * aCatTblHdr,
                  SChar             * a_pRow,
                  smpPersPage      *& a_pPage,
                  SChar            *& a_pRowPtr );

IDE_RC nextOIDall( smcTableHeader   * aCatTblHdr,
                   SChar            * a_pCurRow,
                   SChar           ** a_pNxtRow,
                   idBool             a_bInit);

#define LSP "((((((((((((((("
#define RSP ")))))))))))))))"

#endif

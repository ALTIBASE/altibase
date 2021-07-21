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
 * $Id: 
 **********************************************************************/

#ifndef _O_SMI_LEGACY_TRANS_H_
#define _O_SMI_LEGACY_TRANS_H_ 1

class smiLegacyTrans
{
public:
    static IDE_RC removeLegacyTrans( smiStatement * aStmtListHead,
                                     void         * aSmxTrans );

    static IDE_RC makeLegacyStmt( void          * aLeagcyTrans,
                                  smiStatement  * aOrgStmtListHead );
};

#endif /* _O_SMI_LEGACY_TRANS_H_ */

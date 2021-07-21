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
 * $Id: stm.h 15866 2006-05-26 08:16:16Z laufer $
 **********************************************************************/

#ifndef _O_STM_H_
#define _O_STM_H_ 1

#include    <idl.h>
#include    <qcm.h>

/**************************************************************
                       BASIC DEFINITIONS
 **************************************************************/
#define STM_MAX_META_INDICES        QCM_MAX_META_INDICES


/**************************************************************
                       META TABLE NAMES
 **************************************************************/
/* BUG-47809 USER_SRS meta handle */
#define STM_USER_SRS                  "USER_SRS_"


/**************************************************************
                         INDEX ORDER
 **************************************************************/
/*
      The index order should be exactly same with
      sCrtMetaSql array of qcmCreate::runDDLforMETA
 */
/* BUG-47809 USER_SRS meta handle */
#define STM_USER_SRS_SRID_IDX_ORDER                   (0)
#define STM_USER_SRS_AUTH_SRID_IDX_ORDER              (1)


/**************************************************************
                         COLUMN ORDERS
 **************************************************************/
/* BUG-47809 USER_SRS meta handle */
#define STM_USER_SRS_SRID_COL_ORDER                  (0)
#define STM_USER_SRS_AUTH_NAME_COL_ORDER             (1)
#define STM_USER_SRS_AUTH_SRID_COL_ORDER             (2)
#define STM_USER_SRS_SRTEXT_COL_ORDER                (3)
#define STM_USER_SRS_PROJ4TEXT_COL_ORDER             (4)

/**************************************************************
                        TABLE HANDLES
 **************************************************************/
/* BUG-47809 USER_SRS meta handle */
extern const void * gStmUserSrs;


/**************************************************************
                        INDEX HANDLES
 **************************************************************/
/* BUG-47809 USER_SRS meta handle */
extern const void * gStmUserSrsIndex               [STM_MAX_META_INDICES];


class stm
{
private:
    
public:
    static IDE_RC initializeGlobalVariables( void );
    static IDE_RC initMetaHandles( smiStatement * aSmiStmt );
    static IDE_RC makeAndSetStmColumnInfo( void * aSmiStmt,
                                           void * aTableInfo,
                                           UInt aIndex );
};


#endif /* _O_STM_H_ */

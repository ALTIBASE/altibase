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

#ifndef _O_DKO_LINK_OBJ_MGR_H_
#define _O_DKO_LINK_OBJ_MGR_H_ 1


#include <dko.h>
#include <dkaLinkerProcessMgr.h>


class dkoLinkObjMgr
{
    
public:
    static IDE_RC       initializeStatic();
    static IDE_RC       finalizeStatic();

    /* Link object create/init/destroy */
    static IDE_RC       createLinkObjectsFromMeta( idvSQL *aStatistics );

    static IDE_RC       isExistLinkObject( idvSQL   *aStatistics,
                                           SChar    *aLinkName,
                                           idBool   *aIsExist );

    static IDE_RC       findLinkObject( idvSQL      *aStatistics,
                                        SChar       *aLinkName,
                                        dkoLink     *aLinkObj );

    static IDE_RC       createLinkObject( void       *aQcStatement,
                                          SInt        aUserId,
                                          UInt        aNewDblinkId,
                                          SInt        aLinkType,
                                          SInt        aUserMode,
                                          SChar      *aLinkName,
                                          SChar      *aTargetName,
                                          SChar      *aRemoteUserId,
                                          SChar      *aRemoteUserPasswd,
                                          idBool      aPublicFlag );

    static void         initCopiedLinkObject( dkoLink     *aLinkObj );

    static void         copyLinkObject( dkoLink     *aSrcLinkObj, 
                                        dkoLink     *aDestLinkObj );

    static IDE_RC       destroyLinkObject( void      *aQcStatement,
                                           dkoLink   *aLinkObj );
    
    static IDE_RC       destroyLinkObjectByUserId( idvSQL   *aStatistics,
                                                   void     *aQcStatement,
                                                   UInt      aUserId );
        
    /* Validate link object */
    static IDE_RC       validateLinkObject( void            *aQcStatement,
                                            dksDataSession  *aSession,
                                            dkoLink         *aLinkObj );

    /* Validate link object user */
    static IDE_RC       validateLinkObjectUser( void        *aQcStatement,
                                                dkoLink     *aLinkObj,
                                                idBool      *aIsValid );

    /* Check public database link */
    static inline idBool    isPublicLink( dkoLink   *aLinkObj );

    /* Get user id from link object */
    static inline UInt  getLinkUserId( dkoLink   *aLinkObj );

private:
    static void validateLinkObjectInternal( void        *aQcStatement,
                                            dkoLink     *aLinkObj,
                                            idBool      *aIsValid );
};

inline idBool   dkoLinkObjMgr::isPublicLink( dkoLink    *aLinkObj )
{
    if ( aLinkObj->mUserMode == DKO_LINK_USER_MODE_PUBLIC )
    {
        return ID_TRUE; /* Public */
    }
    else
    {
        return ID_FALSE; /* Private */
    }
}

inline UInt  dkoLinkObjMgr::getLinkUserId( dkoLink   *aLinkObj )
{
    return aLinkObj->mUserId;
}

#endif /* _O_DKO_LINK_OBJ_MGR_H_ */

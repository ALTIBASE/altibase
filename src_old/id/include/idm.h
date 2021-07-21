/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idm.h 67796 2014-12-03 08:39:33Z donlet $
 **********************************************************************/

#ifndef _O_IDM_H_
# define _O_IDM_H_ 1

# include <idmDef.h>

# define IDM_USE_MASK       (0x00000001)
# define IDM_USE_FOR_SERVER (0x00000001)
# define IDM_USE_FOR_CLIENT (0x00000000)

class idm
{
 private:
    static idmModule* root;
    
    static IDE_RC sortChild( idmModule* aModule );
    
    static IDE_RC initializeModule( idmModule*  aModule,
                                    idmModule** aLast,
                                    UInt        aDepth,
                                    UInt        aFlag );
    
    static IDE_RC finalizeModule( idmModule*  aModule,
                                  UInt        aFlag );

    static IDE_RC searchId( const SChar* aAttribute,
                            idmId*       aId,
                            UInt         aIdMaximum,
                            idmModule*   aModule );
    
    static IDE_RC searchName( SChar*      aAttribute,
                              UInt        aAttributeMaximum,
                              idmModule*  aModule,
                              const oid*  aId,
                              UInt        aIdLength );
    
    static idmModule* search( idmModule*  aChildren,
                              const oid*  aId,
                              UInt        aIdLength );
    
 public:
    static IDE_RC initialize( idmModule** aModules,
                              UInt        aFlag );
    
    static IDE_RC finalize( UInt aFlag );
    
    static idBool matchId( const idmModule* aModule,
                           const idmId*     aId );
    
    static IDE_RC makeId( const idmModule* aModule,
                          idmId*           aId,
                          UInt             aIdMaximum );
    
    static IDE_RC translate( const SChar* aAttribute,
                             idmId*       aId,
                             UInt         aIdMaximum );
    
    static IDE_RC name( SChar*       aAttribute,
                        UInt         aAttributeMaximum,
                        const idmId* aId );
    
    static IDE_RC get( const idmId* aId,
                       UInt*        aType,
                       void*        aValue,
                       UInt*        aLength,
                       UInt         aMaximum );
    
    static IDE_RC getNext( const idmId* aPreviousId,
                           idmId*       aId,
                           UInt         aIdMaximum,
                           UInt*        aType,
                           void*        aValue,
                           UInt*        aLength,
                           UInt         aMaximum );
    
    static IDE_RC set( const idmId* aId,
                       UInt         aType,
                       const void*  aValue,
                       UInt         aLength );

    static IDE_RC initDefault( idmModule* aModule );
    
    static IDE_RC finalDefault( idmModule* aModule );
    
    static IDE_RC getNextIdDefault( const idmModule* aModule,
                                    const idmId*     aPreviousId,
                                    idmId*           aId,
                                    UInt             aIdMaximum );
    
    static IDE_RC unsupportedGet( const idmModule* aModule,
                                  const idmId*     aId,
                                  UInt*            aType,
                                  void*            aValue,
                                  UInt*            aLength,
                                  UInt             aMaximum );
    
    static IDE_RC unsupportedSet( idmModule*   aModule,
                                  const idmId* aId,
                                  UInt         aType,
                                  const void*  aValue,
                                  UInt         aLength );
};

#endif /* _O_MTC_H_ */

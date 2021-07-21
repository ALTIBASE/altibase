/***********************************************************************
 * Copyright 1999-2020, Altibase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/
 
 /***********************************************************************
 * $Id: stfProj4.h 88007 2020-07-10 00:26:34Z cory.chae $
 **********************************************************************/
 
#ifndef _O_STF_PROJ4_H_
#define _O_STF_PROJ4_H_      1

#if defined(ST_ENABLE_PROJ4_LIBRARY)

#include <idTypes.h>
#include <mtdTypes.h>

#include <proj_api.h>

class stfProj4
{
public:
    static IDE_RC doInit ( mtdCharType  * aFromProj4Text,
                           mtdCharType  * aToProj4Text,
                           projPJ       * aFromPJ,
                           projPJ       * aToPJ,
                           projCtx      * aProjCtx );
                              
    static void doFinalize ( projPJ   aFromPJ,
                             projPJ   aToPJ,
                             projCtx  aProjCtx );
                                
    static IDE_RC doTransformInternal ( SDouble  * aU,
                                        SDouble  * aV,
                                        projPJ     aFromPJ,
                                        projPJ     aToPJ,
                                        projCtx    aProjCtx );

    static IDE_RC doTransform( mtcStack     * aStack,
                               mtdCharType  * aFromProj4Text,
                               mtdCharType  * aToProj4Text );
};

#endif /* ST_ENABLE_PROJ4_LIBRARY */

#endif /* _O_STF_PROJ4_H_ */

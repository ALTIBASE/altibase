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
 * $Id: sti.cpp 17938 2006-09-05 00:54:03Z leekmo $
 **********************************************************************/

#include <qci.h>
#include <sti.h>
#include <ste.h>
#include <stix.h>
#include <stfBasic.h>
#include <stuProperty.h>

IDE_RC
sti::addExtSM_Recovery ( void )
{
    return stix::addExtSM_Recovery ();
}

IDE_RC
sti::addExtSM_Index ( void )
{
    return stix::addExtSM_Index ();
}

IDE_RC
sti::addExtMT_Module ( void )
{
    return stix::addExtMT_Module();
}

IDE_RC
sti::addExtQP_Callback ( void )
{
    return stix::addExtQP_Callback ();
}

extern iduFixedTableDesc gDumpMemRTreeKeyTableDesc;
extern iduFixedTableDesc gDumpVolRTreeKeyTableDesc;
extern iduFixedTableDesc gMemRTreeHeaderDesc;
extern iduFixedTableDesc gDumpMemRTreeStructureTableDesc;
extern iduFixedTableDesc gDumpVolRTreeStructureTableDesc;
extern iduFixedTableDesc gMemRTreeNodePoolDesc;
extern iduFixedTableDesc gMemRTreeStatDesc;

IDE_RC 
sti::initSystemTables( void )
{
    // initialize fixed table
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gDumpMemRTreeKeyTableDesc );
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gDumpVolRTreeKeyTableDesc );
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gMemRTreeHeaderDesc );
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gDumpMemRTreeStructureTableDesc );
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gDumpVolRTreeStructureTableDesc );
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gMemRTreeNodePoolDesc );
    IDU_FIXED_TABLE_DEFINE_RUNTIME( gMemRTreeStatDesc );

    // initialize performance view for ST
    SChar * sPerfViewTable[] = { ST_PERFORMANCE_VIEWS, NULL };
    SInt    i = 0;

    while(sPerfViewTable[i] != NULL)
    {
        qciMisc::addPerformanceView ( sPerfViewTable[i] );
        i++;
    }

    return IDE_SUCCESS;
}

// Proj-2059 DB Upgrade
// Geometry타입 확인을 위해 Tool에서 Insert가능한 Text형태로 출력할
// 수 있어야 합니다.

IDE_RC sti::getTextFromGeometry(
    void*               aObj,
    UChar*              aBuf,
    UInt                aMaxSize,
    UInt*               aOffset,
    IDE_RC*             aReturn)
{
    return stfBasic::getText( (stdGeometryHeader*)aObj,
                              aBuf,
                              aMaxSize,
                              aOffset,
                              aReturn);
}

IDE_RC sti::initialize( void )
{
    IDE_TEST( stuProperty::load() != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

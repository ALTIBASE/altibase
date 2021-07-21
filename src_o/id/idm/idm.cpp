/***********************************************************************
 * Copyright 1999-2001, ALTIBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: idm.cpp 67796 2014-12-03 08:39:33Z donlet $
 **********************************************************************/

#include <ide.h>
#include <idl.h>
#include <idm.h>

#define IDM_SORT_MAXTABLE (100)
#define IDM_ID_MAXMUM     (100)

idmModule* idm::root = NULL;


IDL_EXTERN_C_BEGIN

static int idmCompareId( const void* aModule1, const void* aModule2 )
{
    const idmModule** sModule1;
    const idmModule** sModule2;

    sModule1 = (const idmModule**)aModule1;
    sModule2 = (const idmModule**)aModule2;

    if( (*sModule1)->id > (*sModule2)->id )
    {
        return 1;
    }
    if( (*sModule1)->id < (*sModule2)->id )
    {
        return -1;
    }

    return 0;
}

IDL_EXTERN_C_END


IDE_RC idm::sortChild( idmModule* aModule )
{
    idmModule* sChild;
    UInt       sCount;
    UInt       sIterator;
    idmModule* sTable[IDM_SORT_MAXTABLE];

    if( aModule->child != NULL )
    {
        for( sCount  = 0,
             sChild  = aModule->child;
             sChild != NULL;
             sChild  = sChild->brother )
        {
            IDE_TEST_RAISE( sCount >= IDM_SORT_MAXTABLE,
                            ERR_SORT_TABLE_SHORTAGE );
            sTable[sCount] = sChild;
            sCount++;
        }

        idlOS::qsort( sTable, sCount, sizeof(idmModule*), idmCompareId );

        aModule->child = sTable[0];
        for( sIterator = 1; sIterator < sCount; sIterator++ )
        {
            sTable[sIterator-1]->brother = sTable[sIterator];
            IDE_TEST_RAISE( sTable[sIterator-1]->id == sTable[sIterator]->id,
                            ERR_INVALID_IDMMODULE );
        }
        sTable[sCount-1]->brother = NULL;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_SORT_TABLE_SHORTAGE)
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_idm_Sort_Table_Shortage));
    }
    IDE_EXCEPTION(ERR_INVALID_IDMMODULE)
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_idm_Invalid_idmModule));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idm::initializeModule( idmModule*  aModule,
                              idmModule** aLast,
                              UInt        aDepth,
                              UInt        aFlag )
{
    idmModule* sChild;

    aModule->depth = aDepth;

    if( ( aFlag & IDM_USE_MASK ) == IDM_USE_FOR_SERVER )
    {
        IDE_TEST( aModule->init( aModule ) != IDE_SUCCESS );
    }

    if( *aLast != NULL )
    {
        (*aLast)->next = aModule;
    }
    *aLast = aModule;

    IDE_TEST( sortChild( aModule ) != IDE_SUCCESS );

    for( sChild = aModule->child; sChild != NULL; sChild = sChild->brother )
    {
        IDE_TEST( initializeModule( sChild, aLast, aDepth + 1, aFlag )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idm::finalizeModule( idmModule*  aModule,
                            UInt        aFlag )
{
    if( aModule != NULL )
    {
        IDE_TEST( finalizeModule( aModule->next, aFlag ) != IDE_SUCCESS );
        if( ( aFlag & IDM_USE_MASK ) == IDM_USE_FOR_SERVER )
        {
            IDE_TEST( aModule->final( aModule ) != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idm::searchId( const SChar* aAttribute,
                      idmId*       aId,
                      UInt         aIdMaximum,
                      idmModule*   aModule )
{
    UInt       sLength;
    UInt       sIsNumber;
    UInt       sIterator;
    idmModule* sModule;

    if( aAttribute[0] != '\0' )
    {
        IDE_TEST_RAISE( aId->length >= aIdMaximum, ERR_ID_NOT_FOUND );

        IDE_TEST_RAISE( aAttribute[0] != '.', ERR_ID_NOT_FOUND );
        aAttribute++;

        for( sLength = 0, sIsNumber = 1;
             aAttribute[sLength] != '.' && aAttribute[sLength] != '\0';
             sLength++ )
        {
            if( aAttribute[sLength] < '0' || aAttribute[sLength] > '9' )
            {
                sIsNumber = 0;
            }
        }

        if( sIsNumber == 1 )
        {
            aId->id[aId->length]  = 0;
            for( sIterator = 0; sIterator < sLength; sIterator++ )
            {
                aId->id[aId->length] = aId->id[aId->length] * 10
                                     + aAttribute[sIterator] - '0';
            }
            if( aModule->child != NULL )
            {
                for( sModule = aModule->child;
                     sModule != NULL;
                     sModule = sModule->brother )
                {
                    if( sModule->id == aId->id[aId->length] )
                    {
                        break;
                    }
                }
                IDE_TEST_RAISE( sModule == NULL, ERR_ID_NOT_FOUND );
            }
            else
            {
                IDE_TEST_RAISE( ( aModule->flag & IDM_FLAG_HAVE_CHILD_MASK ) ==
                                IDM_FLAG_HAVE_CHILD_FALSE,
                                ERR_ID_NOT_FOUND );
                sModule = aModule;
            }
            aId->length++;
        }
        else
        {
            for( sModule = aModule->child;
                 sModule != NULL;
                 sModule = sModule->brother )
            {
                if( idlOS::strCaselessMatch(                    sModule->name,
                                    idlOS::strlen((const char*)sModule->name),
                                                                   aAttribute,
                                                                      sLength )
                    == 0 )
                {
                    break;
                }
            }
            IDE_TEST_RAISE( sModule == NULL, ERR_ID_NOT_FOUND );

            aId->id[aId->length] = sModule->id;
            aId->length++;
        }

        aAttribute += sLength;

        IDE_TEST( searchId( aAttribute, aId, aIdMaximum, sModule )
                  != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ID_NOT_FOUND)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Id_Not_Found));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idm::searchName( SChar*      aAttribute,
                        UInt        aAttributeMaximum,
                        idmModule*  aModule,
                        const oid*  aId,
                        UInt        aIdLength )
{
    idmModule* sModule;
    UInt       sLength;
    SChar      sNumber[32];

    if( aIdLength > 0 )
    {

        IDE_TEST_RAISE( aAttributeMaximum < 1, ERR_ID_NOT_FOUND );

        idlOS::strcpy( aAttribute, "." );

        aAttribute++;
        aAttributeMaximum--;

        if( aModule->child != NULL )
        {
            for( sModule = aModule->child;
                 sModule != NULL;
                 sModule = sModule->brother )
            {
                if( sModule->id == *aId )
                {
                    break;
                }
            }
            IDE_TEST_RAISE( sModule == NULL, ERR_ID_NOT_FOUND );
            sLength = idlOS::strlen( (char*)sModule->name );
            IDE_TEST_RAISE( sLength >= aAttributeMaximum, ERR_ID_NOT_FOUND );
            idlOS::strcpy( aAttribute, (char*)sModule->name );

            IDE_TEST( searchName( aAttribute + sLength,
                                  aAttributeMaximum - sLength,
                                  sModule,
                                  aId + 1,
                                  aIdLength - 1 )
                      != IDE_SUCCESS );
        }
        else
        {
            IDE_TEST_RAISE( ( aModule->flag & IDM_FLAG_HAVE_CHILD_MASK ) ==
                            IDM_FLAG_HAVE_CHILD_FALSE,
                            ERR_ID_NOT_FOUND );
            idlOS::snprintf( sNumber, ID_SIZEOF(sNumber), "%d", *aId );
            sLength = idlOS::strlen( sNumber );

            IDE_TEST_RAISE( sLength >= aAttributeMaximum, ERR_ID_NOT_FOUND );
            idlOS::strcpy( aAttribute, sNumber );

            IDE_TEST( searchName( aAttribute + sLength,
                                  aAttributeMaximum - sLength,
                                  aModule,
                                  aId + 1,
                                  aIdLength - 1 )
                      != IDE_SUCCESS );
        }
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ID_NOT_FOUND)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Id_Not_Found));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idmModule* idm::search( idmModule*  aChildren,
                        const oid*  aId,
                        UInt        aIdLength )
{
    idmModule* sModule;

    if( aIdLength > 0 )
    {
        for( ; aChildren != NULL; aChildren = aChildren->brother )
        {
            if( aId[0] == aChildren->id )
            {
                sModule = search( aChildren->child,
                                  aId + 1,
                                  aIdLength - 1 );
                return ( sModule != NULL ) ? sModule : aChildren;
            }
        }
    }

    return NULL;
}

/*
 * Name:
 *     idm::initialize : SNMP지원 모듈을 초기화 합니다.
 *
 * Arguments:
 *     aModules : 초기화시 사용되는 모듈들
 *
 */

IDE_RC idm::initialize( idmModule** aModules,
                        UInt        aFlag )
{
    idmModule** sModule1;
    idmModule** sModule2;
    idmModule*  sLastChild;
    idmModule*  sLast;
    idmModule*  sRoot;

    sRoot = NULL;

    for( sModule1 = aModules; *sModule1 != NULL; sModule1++ )
    {
        (*sModule1)->child   = NULL;
        (*sModule1)->brother = NULL;
        (*sModule1)->next    = NULL;
    }

    for( sModule1 = aModules; *sModule1 != NULL; sModule1++ )
    {
        if( (*sModule1)->parent == NULL )
        {
            IDE_TEST_RAISE( sRoot != NULL, ERR_INVALID_IDMMODULE );
            sRoot = *sModule1;
        }
        IDE_TEST_RAISE( (*sModule1)->parent == *sModule1,
                        ERR_INVALID_IDMMODULE );
        for( sModule2 = aModules; *sModule2 != NULL; sModule2++ )
        {
            if( sModule1 != sModule2 )
            {
                IDE_TEST_RAISE( *sModule1 == *sModule2,
                                ERR_INVALID_IDMMODULE );
                IDE_TEST_RAISE( idlOS::strCaselessMatch( (*sModule1)->name,
                                                         (*sModule2)->name )
                                == 0, ERR_INVALID_IDMMODULE );
                if( (*sModule1)->parent == *sModule2 )
                {
                    if( (*sModule2)->child == NULL )
                    {
                        (*sModule2)->child = *sModule1;
                    }
                    else
                    {
                        for( sLastChild = (*sModule2)->child;
                             sLastChild->brother != NULL;
                             sLastChild = sLastChild->brother ) ;
                        sLastChild->brother = *sModule1;
                    }
                    break;
                }
            }
        }
        IDE_TEST_RAISE( *sModule2 == NULL && *sModule1 != sRoot,
                        ERR_INVALID_IDMMODULE );
        for( ; *sModule2 != NULL; sModule2++ )
        {
            if( sModule1 != sModule2 )
            {
                IDE_TEST_RAISE( *sModule1 == *sModule2,
                                ERR_INVALID_IDMMODULE );
                IDE_TEST_RAISE( idlOS::strCaselessMatch( (*sModule1)->name,
                                                         (*sModule2)->name )
                                == 0, ERR_INVALID_IDMMODULE );
            }
        }
    }

    sLast = NULL;

    IDE_TEST_RAISE( sRoot == NULL, ERR_INVALID_IDMMODULE );

    IDE_TEST( initializeModule( sRoot, &sLast, 1, aFlag ) != IDE_SUCCESS );

    root = sRoot;

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_INVALID_IDMMODULE)
    {
        IDE_SET(ideSetErrorCode(idERR_FATAL_idm_Invalid_idmModule));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Name:
 *     idm::finalize : SNMP지원 모듈을 종료합니다.
 */
IDE_RC idm::finalize( UInt aFlag )
{
    IDE_TEST( finalizeModule( root, aFlag ) != IDE_SUCCESS );

    root = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

idBool idm::matchId( const idmModule* aModule,
                     const idmId*     aId )
{
    const idmModule* sParent;
    UInt             sIterator;

    if( aModule->depth <= aId->length )
    {
        for( sParent = aModule, sIterator = aModule->depth - 1;
             sParent != NULL;
             sParent = sParent->parent, sIterator-- )
        {
            if( aId->id[sIterator] != sParent->id )
            {
                break;
            }
        }
        if( sParent == NULL )
        {
            return ID_TRUE;
        }
    }

    return ID_FALSE;
}

IDE_RC idm::makeId( const idmModule* aModule,
                    idmId*           aId,
                    UInt             aIdMaximum )
{
    const idmModule* sParent;
    UInt             sIterator;

    IDE_TEST_RAISE( aModule->depth > aIdMaximum, ERR_ID_NOT_FOUND );
    aId->length = aModule->depth;
    for( sParent = aModule, sIterator = aId->length - 1;
         sParent != NULL;
         sParent = sParent->parent, sIterator-- )
    {
        aId->id[sIterator] = sParent->id;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ID_NOT_FOUND)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Id_Not_Found));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Name:
 *     idm::translate : 문자로 된 이름을 ID로 변경합니다.
 *
 * Arguments:
 *     aAttribute: 문자로 이루어진 이름
 *     aId:        출력받고자 하는 ID
 *     aIdMaximum: ID의 최대 크기
 *
 */
IDE_RC idm::translate( const SChar* aAttribute,
                       idmId*       aId,
                       UInt         aIdMaximum )
{
    UInt       sLength;
    UInt       sIsNumber;
    UInt       sIterator;
    idmModule* sModule;

    for( sLength = 0, sIsNumber = 1;
         aAttribute[sLength] != '.' && aAttribute[sLength] != '\0';
         sLength++ )
    {
        if( aAttribute[sLength] < '0' || aAttribute[sLength] > '9' )
        {
            sIsNumber = 0;
        }
    }

    if( sIsNumber == 1 )
    {
        IDE_TEST_RAISE( aIdMaximum < 1, ERR_ID_NOT_FOUND );
        aId->length = 1;
        aId->id[0]  = 0;
        for( sIterator = 0; sIterator < sLength; sIterator++ )
        {
            aId->id[0] = aId->id[0] * 10 + aAttribute[sIterator] - '0';
        }
        IDE_TEST_RAISE( root->id != aId->id[0], ERR_ID_NOT_FOUND );
        sModule = root;
    }
    else
    {
        for( sModule = root; sModule != NULL; sModule = sModule->next )
        {
            if( idlOS::strCaselessMatch(                        sModule->name,
                                    idlOS::strlen((const char*)sModule->name),
                                                                   aAttribute,
                                                                      sLength )
                == 0 )
            {
                break;
            }
        }
        IDE_TEST_RAISE( sModule == NULL, ERR_ID_NOT_FOUND );
        IDE_TEST( makeId( sModule, aId, aIdMaximum ) != IDE_SUCCESS );
    }

    aAttribute += sLength;

    IDE_TEST( searchId( aAttribute, aId, aIdMaximum, sModule )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ID_NOT_FOUND)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Id_Not_Found));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Name:
 *     idm::name : ID를 이름으로 변경합니다.
 *
 * Arguments:
 *     aAttribute:        이름을 출력받고자 하는 버퍼
 *     aAttributeMaximum: 버퍼 최대 크기
 *     aId:               ID
 *
 */
IDE_RC idm::name( SChar*       aAttribute,
                  UInt         aAttributeMaximum,
                  const idmId* aId )
{
    idmModule* sModule;
    UInt       sLength;

    idlOS::strcpy( aAttribute, "" );

    sModule = search( root, aId->id, aId->length );

    IDE_TEST_RAISE( sModule == NULL, ERR_ID_NOT_FOUND );

    sLength = idlOS::strlen( (char*)sModule->name );

    IDE_TEST_RAISE( sLength >= aAttributeMaximum, ERR_ID_NOT_FOUND );

    idlOS::strcpy( aAttribute, (char*)sModule->name );

    IDE_TEST( searchName( aAttribute + sLength,
                          aAttributeMaximum - sLength,
                          sModule,
                          aId->id + sModule->depth,
                          aId->length - sModule->depth )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ID_NOT_FOUND)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Id_Not_Found));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Name:
 *     idm::get : 특정 ID의 값을 가져옵니다.
 *
 * Arguments:
 *     aId:               ID
 *     aType:             값의 데이터 형
 *     aValue:            값을 가져오고자 하는 버퍼
 *     aLength:           값의 길이
 *     aMaximum:          버퍼의 최대 크기
 */
IDE_RC idm::get( const idmId* aId,
                 UInt*        aType,
                 void*        aValue,
                 UInt*        aLength,
                 UInt         aMaximum )
{
    idmModule* sModule;

    sModule = search( root, aId->id, aId->length );

    IDE_TEST_RAISE( sModule == NULL, ERR_ID_NOT_FOUND );

    IDE_TEST_RAISE( sModule->depth != aId->length &&
                    ( sModule->flag & IDM_FLAG_HAVE_CHILD_MASK ) ==
                    IDM_FLAG_HAVE_CHILD_FALSE,
                    ERR_ID_NOT_FOUND );

    IDE_TEST( sModule->get( sModule,
                            aId,
                            aType,
                            aValue,
                            aLength,
                            aMaximum )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ID_NOT_FOUND)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Id_Not_Found));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Name:
 *     idm::getNext : 특정 ID의 다음 ID의 값을 가져옵니다.
 *
 * Arguments:
 *     aPreviousId:       값을 가져오고자 하는 ID의 이전 ID
 *     aId:               가져온 값의 ID
 *     aIdMaximum:        ID의 최대 길이
 *     aType:             값의 데이터 형
 *     aValue:            값을 가져오고자 하는 버퍼
 *     aLength:           값의 길이
 *     aMaximum:          버퍼의 최대 크기
 */
IDE_RC idm::getNext( const idmId* aPreviousId,
                     idmId*       aId,
                     UInt         aIdMaximum,
                     UInt*        aType,
                     void*        aValue,
                     UInt*        aLength,
                     UInt         aMaximum )
{
    idmModule* sModule;

    sModule = search( root, aPreviousId->id, aPreviousId->length );

    IDE_TEST_RAISE( sModule == NULL, ERR_ID_NOT_FOUND );

    IDE_TEST( sModule->getNextId( sModule,
                                  aPreviousId,
                                  aId,
                                  aIdMaximum )
              != IDE_SUCCESS );

    IDE_TEST( get( aId, aType, aValue, aLength, aMaximum ) != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ID_NOT_FOUND)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Id_Not_Found));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

/*
 * Name:
 *     idm::set : 특정 ID의 값을 변경합니다.
 *
 * Arguments:
 *     aId:               변경 대상 ID
 *     aType:             값의 데이터 형
 *     aValue:            저장하고자 하는 값
 *     aLength:           값의 길이
 */
IDE_RC idm::set( const idmId* aId,
                 UInt         aType,
                 const void*  aValue,
                 UInt         aLength )
{
    idmModule* sModule;

    sModule = search( root, aId->id, aId->length );

    IDE_TEST_RAISE( sModule == NULL, ERR_ID_NOT_FOUND );

    IDE_TEST_RAISE( sModule->depth != aId->length &&
                    ( sModule->flag & IDM_FLAG_HAVE_CHILD_MASK ) ==
                    IDM_FLAG_HAVE_CHILD_FALSE,
                    ERR_ID_NOT_FOUND );

    IDE_TEST( sModule->set( sModule,
                            aId,
                            aType,
                            aValue,
                            aLength )
              != IDE_SUCCESS );

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ID_NOT_FOUND)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Id_Not_Found));
    }
    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idm::initDefault( idmModule* )
{
    return IDE_SUCCESS;
}

IDE_RC idm::finalDefault( idmModule* )
{
    return IDE_SUCCESS;
}

IDE_RC idm::getNextIdDefault( const idmModule* aModule,
                              const idmId*     aPreviousId,
                              idmId*           aId,
                              UInt             aIdMaximum )
{
    IDE_TEST_RAISE( aModule->next == NULL, ERR_ID_NOT_FOUND );

    aModule = aModule->next;

    if( aModule->getNextId != getNextIdDefault ||
        aModule->get       == unsupportedGet    )
    {
        IDE_TEST( aModule->getNextId( aModule,
                                      aPreviousId,
                                      aId,
                                      aIdMaximum ) != IDE_SUCCESS );
    }
    else
    {
        IDE_TEST( makeId( aModule, aId, aIdMaximum ) != IDE_SUCCESS );
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION(ERR_ID_NOT_FOUND)
    {
        IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Id_Not_Found));
    }

    IDE_EXCEPTION_END;

    return IDE_FAILURE;
}

IDE_RC idm::unsupportedGet( const idmModule*,
                            const idmId*,
                            UInt*,
                            void*,
                            UInt*,
                            UInt )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Get_Attribute));
    return IDE_FAILURE;
}

IDE_RC idm::unsupportedSet( idmModule*,
                            const idmId*,
                            UInt,
                            const void*,
                            UInt )
{
    IDE_SET(ideSetErrorCode(idERR_ABORT_idm_Unable_To_Set_Attribute));
    return IDE_FAILURE;
}

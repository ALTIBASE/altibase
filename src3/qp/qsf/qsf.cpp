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
 * $Id: qsf.cpp 89988 2021-02-15 02:01:46Z khkwak $
 *
 * Description :
 *     여기에는 PSM에서 사용되는 extended 함수가 정의되어 있다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/
#include <qsf.h>

extern mtfModule qsfPrintModule;

// PROJ-1371 PSM File Handling
extern mtfModule qsfFOpenModule;
extern mtfModule qsfFGetLineModule;
extern mtfModule qsfFPutModule;
extern mtfModule qsfFCloseModule;
extern mtfModule qsfFFlushModule;
extern mtfModule qsfFRemoveModule;
extern mtfModule qsfFRenameModule;
extern mtfModule qsfFCopyModule;
extern mtfModule qsfFCloseAllModule;
extern mtfModule qsfSendmsgModule;
// PROJ-1335 RAISE_APPLICATION_ERROR
extern mtfModule qsfRaiseAppErrModule;
extern mtfModule qsfUserIDModule;
extern mtfModule qsfUserNameModule;

// BUG-47861 INVOKE_USER_ID, INVOKE_USER_NAME function
extern mtfModule qsfInvokeUserIDModule;
extern mtfModule qsfInvokeUserNameModule;

// PROJ-1075
// array type variable의 member functions
extern mtfModule qsfMCountModule;
extern mtfModule qsfMDeleteModule;
extern mtfModule qsfMExistsModule;
extern mtfModule qsfMFirstModule;
extern mtfModule qsfMLastModule;
extern mtfModule qsfMNextModule;
extern mtfModule qsfMPriorModule;

// PROJ-1596
extern mtfModule qsfSleepModule;

// BUG-19041
extern mtfModule qsfSessionIDModule;

// BUG-42464 dbms_alert package
extern mtfModule qsfRegisterModule;
extern mtfModule qsfRemoveModule;
extern mtfModule qsfRemoveAllModule;
extern mtfModule qsfSetDefaultsModule;
extern mtfModule qsfSignalModule;
extern mtfModule qsfWaitAnyModule;
extern mtfModule qsfWaitOneModule;

// BUG-25999
extern mtfModule qsfRemoveXidModule;

extern mtfModule qsfMutexLockModule;
extern mtfModule qsfMutexUnlockModule;

/* TASK-4990 */
extern mtfModule qsfGatherSystemStatsModule;
extern mtfModule qsfGatherTableStatsModule;
extern mtfModule qsfGatherIndexStatsModule;
extern mtfModule qsfSetSystemStatsModule;
extern mtfModule qsfSetTableStatsModule;
extern mtfModule qsfSetIndexStatsModule;
extern mtfModule qsfSetColumnStatsModule;

/* BUG-40119 */
extern mtfModule qsfGetSystemStatsModule;
extern mtfModule qsfGetTableStatsModule;
extern mtfModule qsfGetIndexStatsModule;
extern mtfModule qsfGetColumnStatsModule;

/* BUG-38236 */
extern mtfModule qsfDeleteSystemStatsModule;
extern mtfModule qsfDeleteTableStatsModule;
extern mtfModule qsfDeleteColumnStatsModule;
extern mtfModule qsfDeleteIndexStatsModule;

/* PROJ-1715 */
extern mtfModule qsfConnectByRootModule;
extern mtfModule qsfSysConnectByPathModule;

/* PROJ-2209 DBTIMEZONE */
extern mtfModule qsfSessionTimezoneModule;
extern mtfModule qsfDBTimezoneModule;

/* PROJ-1353 */
extern mtfModule qsfGroupingModule;
extern mtfModule qsfGroupingIDModule;

// BUG-38129
extern mtfModule qsfLastModifiedProwidModule;

/* BUG-38430 */
extern mtfModule qsfRowCountModule;

/* Project 2408 */
extern mtfModule qsfMemoryDumpModule;

/* PROJ-2451 Concurrent Execute Package */
extern mtfModule qsfConcExecModule;
extern mtfModule qsfConcInitializeModule;
extern mtfModule qsfConcFinalizeModule;
extern mtfModule qsfConcWaitModule;
extern mtfModule qsfConcGetErrMsgModule;
extern mtfModule qsfConcGetErrCodeModule;
extern mtfModule qsfConcGetErrCntModule;
extern mtfModule qsfConcGetErrSeqModule;
extern mtfModule qsfConcGetTextModule;

/* BUG-40494 damo hash function */
extern mtfModule qsfSecureHashB64Module;
extern mtfModule qsfSecureHashStrModule;

// BUG-40854
extern mtfModule qsfOpenConnectModule;
extern mtfModule qsfCloseConnectModule;
extern mtfModule qsfCloseAllConnectModule;
extern mtfModule qsfWriterawModule;
extern mtfModule qsfIsConnectModule;

// BUG-41311 table function
extern mtfModule qsfTableFuncElementModule;
// BUG-41452 Array Binding Info.
extern mtfModule qsfIsArrayBoundModule;
extern mtfModule qsfIsFirstArrayBoundModule;
extern mtfModule qsfIsLastArrayBoundModule;

/* BUG-41307 User Lock 지원 */
extern mtfModule qsfUserLockRequestModule;
extern mtfModule qsfUserLockReleaseModule;

extern mtfModule qsfSetClientInfoModule;
extern mtfModule qsfSetModuleModule;

// BUG-41248 DBMS_SQL package
extern mtfModule qsfOpenCursorModule;
extern mtfModule qsfIsOpenModule;
extern mtfModule qsfParseModule;
extern mtfModule qsfBindVariableVarcharModule;
extern mtfModule qsfBindVariableCharModule;
extern mtfModule qsfBindVariableIntegerModule;
extern mtfModule qsfBindVariableBigintModule;
extern mtfModule qsfBindVariableSmallintModule;
extern mtfModule qsfBindVariableDoubleModule;
extern mtfModule qsfBindVariableRealModule;
extern mtfModule qsfBindVariableDateModule;
extern mtfModule qsfBindVariableNumericModule;
extern mtfModule qsfExecuteCursorModule;
extern mtfModule qsfFetchRowsModule;
extern mtfModule qsfColumnValueVarcharModule;
extern mtfModule qsfColumnValueCharModule;
extern mtfModule qsfColumnValueIntegerModule;
extern mtfModule qsfColumnValueBigintModule;
extern mtfModule qsfColumnValueSmallintModule;
extern mtfModule qsfColumnValueDoubleModule;
extern mtfModule qsfColumnValueRealModule;
extern mtfModule qsfColumnValueDateModule;
extern mtfModule qsfColumnValueNumericModule;
extern mtfModule qsfCloseCursorModule;
// BUG-44856
extern mtfModule qsfLastErrorPositionModule;

// BUG-42322
extern mtfModule qsfFormatCallStackModule;
extern mtfModule qsfFormatErrorBacktraceModule;

/* BUG-43605 [mt] random함수의 seed 설정을 session 단위로 변경해야 합니다. */
extern mtfModule qsfRandomModule;

/* PROJ-2657 UTL_SMTP 지원 */
extern mtfModule qsfWriteRawValueModule;
extern mtfModule qsfSendTextModule;
extern mtfModule qsfRecvTextModule;
extern mtfModule qsfCheckConnectStateModule;
extern mtfModule qsfCheckConnectReplyModule;

// BUG-46137
extern mtfModule qsfKeepPlanModule;
extern mtfModule qsfUnkeepPlanModule;

//PROJ-2689
extern mtfModule qsfSetPrevMetaVerModule;
extern mtfModule qsfAddUserSrsModule;
extern mtfModule qsfDeleteUserSrsModule;

// BUG-46074 Multiple trigger event
extern mtfModule qsfInsertingModule;
extern mtfModule qsfUpdatingModule;
extern mtfModule qsfDeletingModule;

// TASK-7244 DBMS_SHARD_GET_DIAGNOSTICS package
extern mtfModule qsfGetErrorCodeModule;
extern mtfModule qsfGetErrorMessageModule;
extern mtfModule qsfGetErrorCountModule;
extern mtfModule qsfGetErrorNodeIDModule;
extern mtfModule qsfGetErrorSeqNumModule;

//-------------------------------------------------------------------
// [qsfExtFuncModules]
// 서버 구동 시, MT에서는 mtfModule(연산자 모듈)에 대한 초기화를
// 수행하게 된다.  이 때, 서버 구동과 함께 초기화되어야 하는
// 외부 연산자 모듈을 여기에 등록한다.
// 물론, 서버 구동과 함께 초기화가 필요하지 않은 연산자 모듈은
// 여기에 등록할 필요가 없다.
//-------------------------------------------------------------------

static SInt qsfCompareByName( const mtfNameIndex* aIndex1,
                             const mtfNameIndex* aIndex2 )
{
#define IDE_FN "int qsfCompareByName"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    return idlOS::strCompare( aIndex1->name->string,
                              aIndex1->name->length,
                              aIndex2->name->string,
                              aIndex2->name->length );

#undef IDE_FN
}

const mtfModule* qsf::extendedFunctionModules[] =
{
    &qsfPrintModule,
    &qsfFOpenModule,
    &qsfFGetLineModule,
    &qsfFPutModule,
    &qsfFCloseModule,
    &qsfFFlushModule,
    &qsfFRemoveModule,
    &qsfFRenameModule,
    &qsfFCopyModule,
    &qsfFCloseAllModule,
    &qsfSendmsgModule,
    &qsfRaiseAppErrModule,
    &qsfUserIDModule,
    &qsfUserNameModule,
    &qsfSleepModule,
    &qsfSessionIDModule,
// BUG-42464 dbms_alert package
    &qsfRegisterModule,
    &qsfRemoveModule,
    &qsfRemoveAllModule,
    &qsfSetDefaultsModule,
    &qsfSignalModule,
    &qsfWaitAnyModule,
    &qsfWaitOneModule,
//BUG-25999
    &qsfRemoveXidModule,
    &qsfMutexLockModule,
    &qsfMutexUnlockModule,
/*TASK-4990*/
    &qsfGatherSystemStatsModule,
    &qsfGatherTableStatsModule,
    &qsfGatherIndexStatsModule,
    &qsfSetSystemStatsModule,
    &qsfSetTableStatsModule,
    &qsfSetIndexStatsModule,
    &qsfSetColumnStatsModule,
/* BUG-40019 */
    &qsfGetSystemStatsModule,
    &qsfGetTableStatsModule,
    &qsfGetIndexStatsModule,
    &qsfGetColumnStatsModule,
/* BUG-38236 */
    &qsfDeleteSystemStatsModule,
    &qsfDeleteTableStatsModule,
    &qsfDeleteIndexStatsModule,
    &qsfDeleteColumnStatsModule,
/* PROJ-1715 */
    &qsfConnectByRootModule,
    &qsfSysConnectByPathModule,
/* PROJ-2209 DBTIMEZONE */
    &qsfSessionTimezoneModule,
    &qsfDBTimezoneModule,
    &qsfGroupingModule, /* PROJ-1353 Rollup, Cube, Grouping */
    &qsfGroupingIDModule,
    &qsfLastModifiedProwidModule,
    &qsfRowCountModule, /* BUG-38430 */
    &qsfMemoryDumpModule, /* PROJ-2408 Memory Dump */
/* PROJ-2451 Concurrent Execute Package */
    &qsfConcExecModule,
    &qsfConcInitializeModule,
    &qsfConcFinalizeModule,
    &qsfConcWaitModule,
    &qsfConcGetErrCodeModule,
    &qsfConcGetErrMsgModule,
    &qsfConcGetErrCntModule,
    &qsfConcGetErrSeqModule,
    &qsfConcGetTextModule,
/* BUG-40494 damo hash function */
    &qsfSecureHashB64Module,
    &qsfSecureHashStrModule,
// BUG-40854
    &qsfOpenConnectModule,
    &qsfCloseConnectModule,
    &qsfCloseAllConnectModule,
    &qsfWriterawModule,
    &qsfIsConnectModule,
// BUG-41311 table function
    &qsfTableFuncElementModule,
/* BUG-41307 User Lock 지원 */
    &qsfUserLockRequestModule,
    &qsfUserLockReleaseModule,
// BUG-41452 Array Binding Info.
    &qsfIsArrayBoundModule,
    &qsfIsFirstArrayBoundModule,
    &qsfIsLastArrayBoundModule,
    &qsfSetClientInfoModule,
    &qsfSetModuleModule,
    &qsfOpenCursorModule,
    &qsfIsOpenModule,
    &qsfParseModule,
    &qsfBindVariableVarcharModule,
    &qsfBindVariableCharModule,
    &qsfBindVariableIntegerModule,
    &qsfBindVariableBigintModule,
    &qsfBindVariableSmallintModule,
    &qsfBindVariableDoubleModule,
    &qsfBindVariableRealModule,
    &qsfBindVariableNumericModule,
    &qsfBindVariableDateModule,
    &qsfExecuteCursorModule,
    &qsfFetchRowsModule,
    &qsfColumnValueVarcharModule,
    &qsfColumnValueCharModule,
    &qsfColumnValueIntegerModule,
    &qsfColumnValueBigintModule,
    &qsfColumnValueSmallintModule,
    &qsfColumnValueDoubleModule,
    &qsfColumnValueRealModule,
    &qsfColumnValueNumericModule,
    &qsfColumnValueDateModule,
    &qsfCloseCursorModule,
    &qsfLastErrorPositionModule,
    &qsfFormatCallStackModule,
    &qsfFormatErrorBacktraceModule,
    /* BUG-43605 [mt] random함수의 seed 설정을 session 단위로 변경해야 합니다. */
    &qsfRandomModule,
    /* PROJ-2657 UTL_SMTP 지원 */
    &qsfWriteRawValueModule,    
    &qsfSendTextModule,
    &qsfRecvTextModule, 
    &qsfCheckConnectStateModule, 
    &qsfCheckConnectReplyModule,
    /* PROJ-2689 */
    &qsfSetPrevMetaVerModule,
    // BUG-46137
    &qsfKeepPlanModule,
    &qsfUnkeepPlanModule,
    &qsfAddUserSrsModule,
    &qsfDeleteUserSrsModule,
    // BUG-46074 Multiple trigger event
    &qsfInsertingModule,
    &qsfUpdatingModule,
    &qsfDeletingModule,
    // BUG-47861 INVOKE_USER_ID, INVOKE_USER_NAME function
    &qsfInvokeUserIDModule,
    &qsfInvokeUserNameModule,
    // TASK-7244 DBMS_SHARD_GET_DIAGNOSTICS package
    &qsfGetErrorCodeModule,
    &qsfGetErrorMessageModule,
    &qsfGetErrorCountModule,
    &qsfGetErrorNodeIDModule,
    &qsfGetErrorSeqNumModule,
    NULL
};

const mtfModule* qsf::mFuncModules[] =
{
    &qsfMCountModule,
    &qsfMDeleteModule,
    &qsfMExistsModule,
    &qsfMFirstModule,
    &qsfMLastModule,
    &qsfMNextModule,
    &qsfMPriorModule,
    NULL
};

UInt qsfNumberOfModulesByName;

mtfNameIndex * qsfModulesByName;

IDE_RC qsf::initialize()
{
#define IDE_FN "qsf::initialize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    UInt               sStage = 0;
    const mtfModule ** sModule;
    const mtcName    * sName;

    for( sModule = mFuncModules,
             qsfNumberOfModulesByName = 0;
         *sModule != NULL;
         sModule++ )
    {
        IDE_TEST( (*sModule)->initialize() != IDE_SUCCESS );
        for( sName = (*sModule)->names; sName != NULL; sName = sName->next )
        {
            qsfNumberOfModulesByName++;
        }
    }

    IDU_FIT_POINT( "qsf::initialize::malloc::qsfModulesByName" );
    IDE_TEST(iduMemMgr::malloc(IDU_MEM_MT,
                               ID_SIZEOF(mtfNameIndex) * qsfNumberOfModulesByName,
                               (void**)&qsfModulesByName)
             != IDE_SUCCESS);

    sStage = 1;

    for( sModule   = mFuncModules,
             qsfNumberOfModulesByName = 0;
         *sModule != NULL;
         sModule++ )
    {
        for( sName  = (*sModule)->names;
             sName != NULL;
             sName  = sName->next, qsfNumberOfModulesByName++ )
        {
            qsfModulesByName[qsfNumberOfModulesByName].name   = sName;
            qsfModulesByName[qsfNumberOfModulesByName].module = *sModule;
        }
    }

    idlOS::qsort( qsfModulesByName, qsfNumberOfModulesByName,
                  ID_SIZEOF(mtfNameIndex), (PDL_COMPARE_FUNC)qsfCompareByName );

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    switch( sStage )
    {
        case 1:
            (void)iduMemMgr::free(qsfModulesByName);
            qsfModulesByName = NULL;

        default:
            break;
    }

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsf::finalize()
{
#define IDE_FN "qsf::finalize"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    const mtfModule** sModule;

    for( sModule   = mFuncModules; *sModule != NULL; sModule++ )
    {
        IDE_TEST( (*sModule)->finalize() != IDE_SUCCESS );
    }

    IDE_TEST(iduMemMgr::free(qsfModulesByName) != IDE_SUCCESS);

    qsfModulesByName = NULL;

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsf::moduleByName( const mtfModule** aModule,
                          idBool*           aExist,
                          const void*       aName,
                          UInt              aLength )
{
#define IDE_FN "IDE_RC qsf::moduleByName"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(""));

    mtfNameIndex        sIndex;
    mtcName             sName;
    const mtfNameIndex* sFound;

    sName.length = aLength;
    sName.string = (void*)aName;
    sIndex.name  = &sName;

    sFound = (const mtfNameIndex*)idlOS::bsearch(                    &sIndex,
                                                                     qsfModulesByName,
                                                                     qsfNumberOfModulesByName,
                                                                     ID_SIZEOF(mtfNameIndex),
                                                                     (PDL_COMPARE_FUNC)qsfCompareByName );

    if ( sFound == NULL )
    {
        *aExist = ID_FALSE;
    }
    else
    {
        *aExist = ID_TRUE;
        
        *aModule = sFound->module;
    }
    
    return IDE_SUCCESS;

#undef IDE_FN
}

IDE_RC qsf::makeFilePath( iduMemory   * aMem,
                          SChar      ** aFilePath,
                          SChar       * aPath,
                          mtdCharType * aFilename )
{

#define IDE_FN "qsf::makeFilePath"
    IDE_MSGLOG_FUNC(IDE_MSGLOG_BODY(IDE_FN));

    SInt sPathLen = idlOS::strlen( aPath );

    IDE_DASSERT( sPathLen >= 1 );

    if( aPath[sPathLen-1] == IDL_FILE_SEPARATOR )
    {
        IDU_FIT_POINT( "qsf::makeFilePath::alloc::FilePath1" );
        IDE_TEST( aMem->alloc(
                      sPathLen + aFilename->length + 1,
                      (void**)aFilePath )
                  != IDE_SUCCESS );

        idlOS::strncpy( *aFilePath,
                        aPath,
                        sPathLen );
        idlOS::strncpy( *aFilePath + sPathLen,
                        (SChar*)aFilename->value,
                        aFilename->length );
        *( *aFilePath + sPathLen + aFilename->length ) = '\0';
    }
    else
    {
        IDU_FIT_POINT( "qsf::makeFilePath::alloc::FilePath2" );
        IDE_TEST( aMem->alloc(
                      sPathLen + aFilename->length + 2,
                      (void**)aFilePath )
                  != IDE_SUCCESS );

        idlOS::strncpy( *aFilePath,
                        aPath,
                        sPathLen );

        *( *aFilePath + sPathLen ) = IDL_FILE_SEPARATOR;

        idlOS::strncpy( *aFilePath + sPathLen + 1,
                        (SChar*)aFilename->value,
                        aFilename->length );
        *( *aFilePath + sPathLen + aFilename->length + 1 ) = '\0';
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION_END;

    return IDE_FAILURE;

#undef IDE_FN
}

IDE_RC qsf::getArg( mtcStack   * aStack,
                    UInt         aIdx,
                    idBool       aNotNull,
                    qsInOutType  aInOutType,
                    void      ** aRetPtr )
{
    idBool sIsNull;

    sIsNull = aStack[ aIdx ].column->module->isNull( aStack[ aIdx ].column,
                                                     aStack[ aIdx ].value );

    IDE_TEST_RAISE( ( aNotNull == ID_TRUE ) && 
                    ( sIsNull  == ID_TRUE ),
                    ERR_ARGUMENT_NOT_APPLICABLE );

    if( sIsNull == ID_TRUE )
    {
        // BUG-40119
        // OUT 모드일때는 값을 저장할수 있는 포인터를 리턴해주어야 한다.
        if ( aInOutType == QS_OUT )
        {
            *aRetPtr = (void*)aStack[ aIdx ].value;
        }
        else
        {
            *aRetPtr = NULL;
        }
    }
    else
    {
        *aRetPtr = (void*)aStack[ aIdx ].value;
    }

    return IDE_SUCCESS;

    IDE_EXCEPTION( ERR_ARGUMENT_NOT_APPLICABLE );
    {
        IDE_SET(ideSetErrorCode(mtERR_ABORT_ARGUMENT_NOT_APPLICABLE));
    }
    IDE_EXCEPTION_END;
    
    return IDE_FAILURE;
}

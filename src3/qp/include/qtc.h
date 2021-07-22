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
 * $Id: qtc.h 88800 2020-10-06 10:29:32Z hykim $
 *
 * Description :
 *     QP layer와 MT layer의 중간에 위치하는 layer로
 *     개념상 MT interface layer 역할을 한다.
 *
 *     여기에는 QP 전체에 걸쳐 공통적으로 사용되는 함수가 정의되어 있다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QTC_H_
# define _O_QTC_H_ 1

# include <mt.h>
# include <qtcDef.h>
# include <qtcRid.h>
# include <qtcCache.h>
# include <qmsParseTree.h>

class qtc
{
public:
    // Zero Dependencies를 Global하게 생성하여
    // 필요할 때마다, Zero Dependencies를 만들지 않도록 한다.
    static qcDepInfo         zeroDependencies;

    static smiCallBackFunc   rangeFuncs[];
    static mtdCompareFunc    compareFuncs[];
    
    /*************************************************************
     * Meta Table 관리를 위한 interface
     *************************************************************/

    // Minimum KeyRange CallBack
    static IDE_RC rangeMinimumCallBack4Mtd( idBool     * aResult,
                                            const void * aRow,
                                            void       *, /* aDirectKey */
                                            UInt        , /* aDirectKeyPartialSize */
                                            const scGRID aRid,
                                            void       * aData );

    static IDE_RC rangeMinimumCallBack4GEMtd( idBool     * aResult,
                                              const void * aColVal,
                                              void       *, /* aDirectKey */
                                              UInt        , /* aDirectKeyPartialSize */
                                              const scGRID aRid,
                                              void       * aData );

    static IDE_RC rangeMinimumCallBack4GTMtd( idBool     * aResult,
                                              const void * aColVal,
                                              void       *, /* aDirectKey */
                                              UInt        , /* aDirectKeyPartialSize */
                                              const scGRID aRid,
                                              void       * aData );

    static IDE_RC rangeMinimumCallBack4Stored( idBool     * aResult,
                                               const void * aColVal,
                                               void       *, /* aDirectKey */
                                               UInt        , /* aDirectKeyPartialSize */
                                               const scGRID aRid,
                                               void       * aData );

    // Maximum KeyRange CallBack
    static IDE_RC rangeMaximumCallBack4Mtd( idBool     * aResult,
                                            const void * aRow,
                                            void       *, /* aDirectKey */
                                            UInt        , /* aDirectKeyPartialSize */
                                            const scGRID aRid,
                                            void       * aData );

    static IDE_RC rangeMaximumCallBack4LEMtd( idBool     * aResult,
                                              const void * aColVal,
                                              void       *, /* aDirectKey */
                                              UInt        , /* aDirectKeyPartialSize */
                                              const scGRID aRid,
                                              void       * aData );

    static IDE_RC rangeMaximumCallBack4LTMtd( idBool     * aResult,
                                              const void * aColVal,
                                              void       *, /* aDirectKey */
                                              UInt        , /* aDirectKeyPartialSize */
                                              const scGRID aRid,
                                              void       * aData );

    static IDE_RC rangeMaximumCallBack4Stored( idBool     * aResult,
                                               const void * aColVal,
                                               void       *, /* aDirectKey */
                                               UInt        , /* aDirectKeyPartialSize */
                                               const scGRID aRid,
                                               void       * aData );

    /* PROJ-2433 */
    static IDE_RC rangeMinimumCallBack4IndexKey( idBool     * aResult,
                                                 const void * aRow,
                                                 void       * aDirectKey,
                                                 UInt         aDirectKeyPartialSize,
                                                 const scGRID aRid,
                                                 void       * aData );

    static IDE_RC rangeMaximumCallBack4IndexKey( idBool     * aResult,
                                                 const void * aRow,
                                                 void       * aDirectKey,
                                                 UInt         aDirectKeyPartialSize,
                                                 const scGRID aRid,
                                                 void       * aData );
    /* PROJ-2433 end */
    static SInt compareMinimumLimit( mtdValueInfo * aValueInfo1,
                                     mtdValueInfo * /* aValueInfo2 */ );
    
    static SInt compareMaximumLimit4Mtd( mtdValueInfo * aValueInfo1,
                                         mtdValueInfo * /* aValueInfo2 */ );

    static SInt compareMaximumLimit4Stored( mtdValueInfo * aValueInfo1,
                                            mtdValueInfo * /* aValueInfo2 */ );

private:

    /*************************************************************
     * Conversion Node 생성 및 관리를 위한 interface
     *************************************************************/

    // intermediate conversion node를 생성
    static IDE_RC initConversionNodeIntermediate( mtcNode** aConversionNode,
                                                  mtcNode*  aNode,
                                                  void*     aInfo );

    static IDE_RC appendDefaultArguments( qtcNode*     aNode,
                                          mtcTemplate* /*aTemplate*/,
                                          mtcStack*    /*aStack*/,
                                          SInt         /*aRemain*/,
                                          mtcCallBack* aCallBack );

    static IDE_RC addLobValueFuncForSP( qtcNode      * aNode,
                                        mtcTemplate  * aTemplate,
                                        mtcStack     * aStack,
                                        SInt           aRemain,
                                        mtcCallBack  * aCallBack );

    /*************************************************************
     * Estimate 를 위한 내부 interface
     *************************************************************/

    // Node Tree 전체에 대한 estimate를 수행함.
    static IDE_RC estimateInternal( qtcNode*     aNode,
                                    mtcTemplate* aTemplate,
                                    mtcStack*    aStack,
                                    SInt         aRemain,
                                    mtcCallBack* aCallBack );

    // PROJ-1762 analytic function을 위한 over절에 대한
    // estimate를 수행함
    static IDE_RC estimate4OverClause( qtcNode*     aNode,
                                       mtcTemplate* aTemplate,
                                       mtcStack*    aStack,
                                       SInt         aRemain,
                                       mtcCallBack* aCallBack );

    static IDE_RC estimateAggregation( qtcNode     * aNode,
                                       mtcCallBack * aCallBack );


    // 하위 Node의 정보를 참조하여 자신의 estimate 를 수행함.
    static IDE_RC estimateNode( qtcNode*     aNode,
                                mtcTemplate* aTemplate,
                                mtcStack*    aStack,
                                SInt         aRemain,
                                mtcCallBack* aCallBack );

    /*************************************************************
       TODO - estimateNode() 함수를 private function으로.
       개념상 Private Function으로 구현된 함수이다.
       현재, qtc::XXX() 외부에서도 호출하고 있으며,
       이는 Error 발생의 위험성을 내포하고 있다.
       이미 사용하고 있다면, 다음 두 함수 중 하나를 골라 써야 한다.
           - qtc::estimateNodeWithoutArgument()
           - qtc::estimateNodeWithArgument()
    *************************************************************/
    static IDE_RC estimateNode( qcStatement* aStatement,
                                       qtcNode*     aNode );

    // Constant Expression인지의 검사
    static IDE_RC isConstExpr( qcTemplate  * aTemplate,
                               qtcNode     * aNode,
                               idBool      * aResult );

    // Constant Expression을 수행
    static IDE_RC runConstExpr( qcStatement * aStatement,
                                qcTemplate  * aTemplate,
                                qtcNode     * aNode,
                                mtcTemplate * aMtcTemplate,
                                mtcStack    * aStack,
                                SInt          aRemain,
                                mtcCallBack * aCallBack,
                                qtcNode    ** aResultNode );

    // Constant Expression의 사전 처리를 위한 Constant Column영역의 회득
    static IDE_RC getConstColumn( qcStatement * aStatement,
                                  qcTemplate  * aTemplate,
                                  qtcNode     * aNode );

    static idBool isSameModule( qtcNode* aNode1,
                                qtcNode* aNode2 );

    static idBool isSameModuleByName( qtcNode * aNode1,
                                      qtcNode * aNode2 );

    // BUG-28223 CASE expr WHEN .. THEN .. 구문의 expr에 subquery 사용시 에러발생 
    static mtcNode* getCaseSubExpNode( mtcNode* aNode );

    // BUG-41243 Check if name-based argument passing is used
    static idBool  hasNameArgumentForPSM( qtcNode * aNode );
    
public:

    /*************************************************************
     * QP를 위한 추가 Module
     *************************************************************/

    static mtfModule assignModule;
    static mtfModule columnModule;
    static mtfModule indirectModule;
    static mtdModule skipModule;
    static mtfModule dnf_notModule;
    static mtfModule subqueryModule;
    static mtfModule valueModule;
    static mtfModule passModule;
    static mtfModule hostConstantWrapperModule;
    static mtfModule subqueryWrapperModule;

    /*************************************************************
     * Stored Procedure 를 위한 추가 Module
     *************************************************************/

    static mtfModule spFunctionCallModule;
    static mtdModule spCursorModule;
    static mtfModule spCursorFoundModule;
    static mtfModule spCursorNotFoundModule;
    static mtfModule spCursorIsOpenModule;
    static mtfModule spCursorIsNotOpenModule;
    static mtfModule spCursorRowCountModule;
    static mtfModule spSqlCodeModule;
    static mtfModule spSqlErrmModule;

    // PROJ-1075
    // row/record/array data type 모듈 추가.
    static mtdModule spRowTypeModule;
    static mtdModule spRecordTypeModule;
    static mtdModule spArrTypeModule;

    // PROJ-1386
    // REF CURSOR type 모듈 추가.
    static mtdModule spRefCurTypeModule;

    /*************************************************************
     * Tuple Set의 Tuple 종류에 따른 Flag 정의
     *************************************************************/

    static const ULong templateRowFlags[MTC_TUPLE_TYPE_MAXIMUM];

    // BUG-44382 clone tuple 성능개선
    static void setTupleColumnFlag( mtcTuple * aTuple,
                                    idBool     aCopyColumn,
                                    idBool     aMemsetRow );

    /*************************************************************
     * Dependencies 관련 함수
     *************************************************************/

    static void dependencySet( UShort aTupleID,
                               qcDepInfo * aOperand1 );

    // PROJ-1502 PARTITIONED DISK TABLE
    // partition에 대한 selection graph는
    // partition tuple에 대한 dependency를 가지도록
    // 변경해야 한다.
    static void dependencyChange( UShort aSourceTupleID,
                                  UShort aDestTupleID,
                                  qcDepInfo * aOperand1,
                                  qcDepInfo * aResult );

    // dependencies를 설정함.
    static void dependencySetWithDep( qcDepInfo * aOperand1,
                                      qcDepInfo * aOperand2 );

    // AND dependencies를 구함
    static void dependencyAnd( qcDepInfo * aOperand1,
                               qcDepInfo * aOperand2,
                               qcDepInfo * aResult );

    // OR dependencies를 구함.
    static IDE_RC dependencyOr( qcDepInfo * aOperand1,
                                qcDepInfo * aOperand2,
                                qcDepInfo * aResult );

    // PROJ-1413
    // aOperand1에서 aTupleID의 dependency를 제거
    static void dependencyRemove( UShort      aTupleID,
                                  qcDepInfo * aOperand1,
                                  qcDepInfo * aResult );
    
    // dependencies를 초기화함.
    static void dependencyClear( qcDepInfo * aOperand1 );

    // dependencies가 동일한 지를 판단
    static idBool dependencyEqual( qcDepInfo * aOperand1,
                                   qcDepInfo * aOperand2 );

    // dependencies가 존재하는 지를 판단
    static idBool haveDependencies( qcDepInfo * aOperand1 );

    // dependencies가 포함되는지의 여부를 판단
    static idBool dependencyContains( qcDepInfo * aSubject,
                                      qcDepInfo * aTarget );

    // MINUS dependencies를 구함
    static void dependencyMinus( qcDepInfo * aOperand1,
                                 qcDepInfo * aOperand2,
                                 qcDepInfo * aResult );

    // Dependencies에 포함된 Dependency 개수 획득
    static SInt getCountBitSet( qcDepInfo * aOperand1 );

    // Dependencies에 포함된 Join Operator 개수 획득
    static SInt getCountJoinOperator( qcDepInfo * aOperand );

    // Dependencies에 존재하는 최초 depenency를 획득한다.
    static SInt getPosFirstBitSet( qcDepInfo * aOperand1 );

    // 현재 위치로부터 다음에 존재하는 dependency를 획득한다.
    static SInt getPosNextBitSet( qcDepInfo * aOperand1,
                                  UInt   aPos );

    // Set If Join Operator Exists
    static void dependencyAndJoinOperSet( UShort      aTupleID,
                                          qcDepInfo * aOperand1 );

    // AND of (Dependencies & Join Oper)를 구함
    static void dependencyJoinOperAnd( qcDepInfo * aOperand1,
                                       qcDepInfo * aOperand2,
                                       qcDepInfo * aResult );

    // Dependencies & Join Operator Status 가 모두 정확히 동일한 지를 판단
    static idBool dependencyJoinOperEqual( qcDepInfo * aOperand1,
                                           qcDepInfo * aOperand2 );

    // dep 정보중 join operator 의 유무가 반대인 dep 를 리턴
    static void getJoinOperCounter( qcDepInfo  * aOperand1,
                                    qcDepInfo  * aOperand2 );

    // 하나의 table 에 대해 join operator 가 both 인 것
    static idBool isOneTableOuterEachOther( qcDepInfo   * aDepInfo );

    // dependency table tuple id 값을 리턴
    static UInt getDependTable( qcDepInfo  * aDepInfo,
                                UInt         aIdx );

    // dependency join operator 값을 리턴
    static UChar getDependJoinOper( qcDepInfo  * aDepInfo,
                                    UInt         aIdx );

    /*************************************************************
     * 기타 지원 함수
     *************************************************************/

    // CallBack 정보를 이용한 Memory 할당 함수
    static IDE_RC alloc( void* aInfo,
                         UInt  aSize,
                         void** aMemPtr);

    // PROJ-1362
    static IDE_RC getOpenedCursor( mtcTemplate*     aTemplate,
                                   UShort           aTableID,
                                   smiTableCursor** aCursor,
                                   UShort         * aOrgTableID,
                                   idBool*          aFound );

    // BUG-40427 최초로 Open한 LOB Cursor인 경우, qmcCursor에 기록
    static IDE_RC addOpenedLobCursor( mtcTemplate  * aTemplate,
                                      smLobLocator   aLocator );

    /* PROJ-1530 PSM/Trigger에서 LOB 데이타 타입 지원 */
    static idBool isBaseTable( mtcTemplate * aTemplate,
                               UShort        aTable );

    static IDE_RC closeLobLocator( smLobLocator  aLocator );

    //PROJ-1583 large geometry 
    static UInt   getSTObjBufSize( mtcCallBack * aCallBack );

    // 새로운 Tuple Row를 할당받는다.
    static IDE_RC nextRow( iduVarMemList * aMemory, //fix PROJ-1596
                           qcStatement   * aStatement,
                           qcTemplate    * aTemplate,
                           ULong           aFlag );

    // PROJ-1583 large geometry
    static IDE_RC nextLargeConstColumn( iduVarMemList * aMemory,
                                        qtcNode       * aNode,
                                        qcStatement   * aStatement,
                                        qcTemplate    * aTemplate,
                                        UInt            aSize );

    // 새로운 Column을 할당받는다.
    static IDE_RC nextColumn( iduVarMemList * aMemory, //fix PROJ-1596
                              qtcNode       * aNode,
                              qcStatement   * aStatement,
                              qcTemplate    * aTemplate,
                              ULong           aFlag,
                              UInt            aColumns );

    // Table을 위한 공간을 할당받는다.
    static IDE_RC nextTable( UShort          *aRow,
                             qcStatement     *aStatement,
                             qcmTableInfo    *aTableInfo,
                             idBool           aIsDiskTable,
                             UInt             aIsNullableFlag); // PR-13597

    // PROJ-1358 Internal Tuple의 공간을 확장한다.
    static IDE_RC increaseInternalTuple ( qcStatement * aStatement,
                                          UShort        aIncreaseCount );

    // PROJ-1473 Tuple내 columnLocate멤버에 대한 초기화
    static IDE_RC allocAndInitColumnLocateInTuple( iduVarMemList * aMemory,
                                                   mtcTemplate   * aTemplate,
                                                   UShort          aRowID );
    
    /*************************************************************
     * Filter 처리 함수
     *************************************************************/

    // Filter 처리를 위한 CallBack 함수의 기본 단위
    static IDE_RC smiCallBack( idBool       * aResult,
                               const void   * aRow,
                               void         *, /* aDirectKey */
                               UInt          , /* aDirectKeyPartialSize */
                               const scGRID   aRid,
                               void         * aData );

    // Filter 처리를 위한 CallBack으로 CallBack의 AND 처리를 수행
    static IDE_RC smiCallBackAnd( idBool       * aResult,
                                  const void   * aRow,
                                  void         *, /* aDirectKey */
                                  UInt          , /* aDirectKeyPartialSize */
                                  const scGRID   aRid,
                                  void         * aData );

    // 하나의 Filter를 위한 CallBack정보를 구성
    static void setSmiCallBack( qtcSmiCallBackData* aData,
                                qtcNode*            aNode,
                                qcTemplate*         aTemplate,
                                UInt                aTable );

    // 여러개의 Filter CallBack을 조합
    static void setSmiCallBackAnd( qtcSmiCallBackDataAnd* aData,
                                   qtcSmiCallBackData*    aArgu1,
                                   qtcSmiCallBackData*    aArgu2,
                                   qtcSmiCallBackData*    aArgu3 );

    /*************************************************************
     * STRING Value 생성 함수
     *************************************************************/

    // 주어진 String으로부터 CHAR type value를 생성
    static void setCharValue( mtdCharType* aValue,
                              mtcColumn*   aColumn,
                              SChar*       aString );

    // 주어진 String으로부터 VARCHAR type value를 생성
    static void setVarcharValue( mtdCharType* aValue,
                                 mtcColumn*   aColumn,
                                 SChar*       aString,
                                 SInt         aLength );

    /*************************************************************
     * Meta를 위한 Key Range 생성 함수
     *************************************************************/

    // Meta KeyRange를 초기화
    static void initializeMetaRange( smiRange * aRange,
                                     UInt       aCompareType );

    // 한 컬럼에 대한 Meta KeyRange를 생성함.
    static void setMetaRangeColumn( qtcMetaRangeColumn* aRangeColumn,
                                    const mtcColumn*    aColumnDesc,
                                    const void*         aValue,
                                    UInt                aOrder,
                                    UInt                aColumnIdx );

    // PROJ-1502 PARTITIONED DISK TABLE
    // keyrange column을 변경한다.
    static void changeMetaRangeColumn( qtcMetaRangeColumn* aRangeColumn,
                                       const mtcColumn*    aColumnDesc,
                                       UInt                aColumnIdx );

    static void setMetaRangeIsNullColumn(qtcMetaRangeColumn* aRangeColumn,
                                         const mtcColumn*    aColumnDesc,
                                         UInt                aColumnIdx );

    // 한 컬럼에 대한 KeyRange를 전체 KeyRange에 추가함.
    static void addMetaRangeColumn( smiRange*           aRange,
                                    qtcMetaRangeColumn* aRangeColumn );

    // 모든 Meta KeyRange 완료 후 처리를 수행함.
    static void fixMetaRange( smiRange* aRange );

    /*************************************************************
     * Parsing 단계에서의 처리
     *************************************************************/

    // Parsing 완료 후의 처리를 수행
    static IDE_RC fixAfterParsing( qcTemplate    * aTemplate );


    // Column의 Data Type 정보를 설정함.
    static IDE_RC createColumn( qcStatement*    aStatement,
                                qcNamePosition* aPosition,
                                mtcColumn**     aColumn,
                                UInt            aArguments,
                                qcNamePosition* aPrecision,
                                qcNamePosition* aScale,
                                SInt            aScaleSign,
                                idBool          aIsForSP );

    // Column의 Data Type 정보를 설정함(Module정보는 그대로 이용함).
    static IDE_RC createColumn( qcStatement*    aStatement,
                                mtdModule  *    aMtdModule,
                                mtcColumn**     aColumn,
                                UInt            aArguments,
                                qcNamePosition* aPrecision,
                                qcNamePosition* aScale,
                                SInt            aScaleSign );

    static IDE_RC createColumn4TimeStamp( qcStatement*    aStatement,
                                          mtcColumn**     aColumn );

    /* PROJ-2422 srid */
    static IDE_RC changeColumn4SRID( qcStatement             * aStatement,
                                     struct qdExtColumnAttr  * aColumnAttr,
                                     mtcColumn               * aColumn );
    
    // PROJ-2002 Column Security
    // encrypt column으로의 변환
    static IDE_RC changeColumn4Encrypt( qcStatement             * aStatement,
                                        struct qdExtColumnAttr  * aColumnAttr,
                                        mtcColumn               * aColumn );
    
    // decrypt column으로의 변환
    static IDE_RC changeColumn4Decrypt( mtcColumn    * aColumn );
 
    /*  PROJ-1584 DML Return Clause */
    static IDE_RC changeColumn4Decrypt( mtcColumn    * aSrcColumn,
                                        mtcColumn    * aDestColumn );
    
    // OR Node를 생성함
    static IDE_RC addOrArgument( qcStatement* aStatement,
                                 qtcNode**    aNode,
                                 qtcNode**    aArgument1,
                                 qtcNode**    aArgument2 );

    // AND Node를 생성함
    static IDE_RC addAndArgument( qcStatement* aStatement,
                                  qtcNode**    aNode,
                                  qtcNode**    aArgument1,
                                  qtcNode**    aArgument2 );

    // BUG-36125 LNNVL을 처리함
    static IDE_RC lnnvlNode( qcStatement* aStatement,
                             qtcNode*     aNode );

    // NOT을 처리하기 위해 하위 Node들의 Counter 연산자로 대체함.
    static IDE_RC notNode( qcStatement* aStatement,
                           qtcNode**    aNode );

    // PRIOR keyword를 처리함.
    static IDE_RC priorNode( qcStatement* aStatement,
                             qtcNode**    aNode );

    // String으로부터 연산자를 위한 Node를 생성함.
    static IDE_RC makeNode( qcStatement*    aStatement,
                            qtcNode**       aNode,
                            qcNamePosition* aPosition,
                            const UChar*    aOperator,
                            UInt            aOperatorLength );

    // PROJ-1075 array type member function을 고려한 makeNode
    static IDE_RC makeNodeForMemberFunc( qcStatement     * aStatement,
                                         qtcNode        ** aNode,
                                         qcNamePosition  * aPositionStart,
                                         qcNamePosition  * aPositionEnd,
                                         qcNamePosition  * aUserNamePos,
                                         qcNamePosition  * aTableNamePos,
                                         qcNamePosition  * aColumnNamePos,
                                         qcNamePosition  * aPkgNamePos );

    // Module 정보로부터 Node를 생성함.
    static IDE_RC makeNode( qcStatement*    aStatement,
                            qtcNode**       aNode,
                            qcNamePosition* aPosition,
                            mtfModule*      aModule );

    // Node에 Argument를 추가함.
    static IDE_RC addArgument( qtcNode** aNode,
                               qtcNode** aArgument );
    
    // Node에 within argument를 추가함.
    static IDE_RC addWithinArguments( qcStatement  * aStatement,
                                      qtcNode     ** aNode,
                                      qtcNode     ** aArguments );
    
    // Quantify Expression을 위해 생성된 잉여 Node를 제거함
    static IDE_RC modifyQuantifiedExpression( qcStatement* aStatement,
                                              qtcNode**    aNode );
    // Value를 위한 Node를 생성함.
    static IDE_RC makeValue( qcStatement*    aStatement,
                             qtcNode**       aNode,
                             const UChar*    aTypeName,
                             UInt            aTypeLength,
                             qcNamePosition* aPosition,
                             const UChar*    aValue,
                             UInt            aValueLength,
                             mtcLiteralType  aLiteralType );

    // BUG-38952 type null
    static IDE_RC makeNullValue( qcStatement     * aStatement,
                                 qtcNode        ** aNode,
                                 qcNamePosition  * aPosition );

    // Procedure 변수를 위한 Node를 생성함.
    static IDE_RC makeProcVariable( qcStatement*     aStatement,
                                    qtcNode**        aNode,
                                    qcNamePosition*  aPosition,
                                    mtcColumn*       aColumn,
                                    UInt             aProcVarOp );

    // To Fix PR-11391
    // Internal Procedure 변수를 위한 Node를 생성함.
    static IDE_RC makeInternalProcVariable( qcStatement*    aStatement,
                                            qtcNode**       aNode,
                                            qcNamePosition* aPosition,
                                            mtcColumn*      aColumn,
                                            UInt            aProcVarOp );

    // Host 변수를 위한 Node를 생성함.
    static IDE_RC makeVariable( qcStatement*    aStatement,
                                qtcNode**       aNode,
                                qcNamePosition* aPosition );

    // Column을 위한 Node를 생성함.
    static IDE_RC makeColumn( qcStatement*    aStatement,
                              qtcNode**       aNode,
                              qcNamePosition* aUserPosition,
                              qcNamePosition* aTablePosition,
                              qcNamePosition* aColumnPosition,
                              qcNamePosition* aPkgPosition );

    // Asterisk Target Column을 위한 Node를 생성함.
    static IDE_RC makeTargetColumn( qtcNode* aNode,
                                    UShort   aTable,
                                    UShort   aColumn );

    // 해당 Tuple의 최대 Offset을 재조정한다.
    static void resetTupleOffset( mtcTemplate* aTemplate, UShort aTupleID );

    // Intermediate Tuple을 위한 공간을 할당
    static IDE_RC allocIntermediateTuple( qcStatement* aStatement,
                                          mtcTemplate* aTemplate,
                                          UShort       aTupleID,
                                          SInt         aColCount );

    // List Expression을 위한 정보를 재조정한다.
    static IDE_RC changeNode( qcStatement*    aStatement,
                              qtcNode**       aNode,
                              qcNamePosition* aPosition,
                              qcNamePosition* aPositionEnd );

    // Module로 부터 List Expression을 위한 정보를 재조정한다.
    static IDE_RC changeNodeByModule( qcStatement*    aStatement,
                                      qtcNode**       aNode,
                                      mtfModule*      aModule,
                                      qcNamePosition* aPosition,
                                      qcNamePosition* aPositionEnd );

    // PROJ-2533 array 변수를 고려한 changeNode
    static IDE_RC changeNodeForArray( qcStatement*    aStatement,
                                      qtcNode**       aNode,
                                      qcNamePosition* aPosition,
                                      qcNamePosition* aPositionEnd,
                                      idBool          aIsBracket );

    // PROJ-1075 array type member function을 고려한 changeNode
    static IDE_RC changeNodeForMemberFunc( qcStatement    * aStatement,
                                           qtcNode       ** aNode,
                                           qcNamePosition * aPositionStart,
                                           qcNamePosition * aPositionEnd,
                                           qcNamePosition * aUserNamePos,
                                           qcNamePosition * aTableNamePos,
                                           qcNamePosition * aColumnNamePos,
                                           qcNamePosition * aPkgNamePos,
                                           idBool           aIsBracket /* PROJ-2533 */ );
    // PROJ-2533 column을 위한 changeNode
    static IDE_RC changeColumn( qtcNode        ** aNode,
                                qcNamePosition  * aPositionStart,
                                qcNamePosition  * aPositionEnd,
                                qcNamePosition  * aUserNamePos,
                                qcNamePosition  * aTableNamePos,
                                qcNamePosition  * aColumnNamePos,
                                qcNamePosition  * aPkgNamePos);

    /* BUG-40279 lead, lag with ignore nulls */
    static IDE_RC changeIgnoreNullsNode( qtcNode     * aNode,
                                         idBool      * aChanged );

    // BUG-41631 RANK() Within Group Aggregation.
    static IDE_RC changeWithinGroupNode( qcStatement * aStatement,
                                         qtcNode     * aNode,
                                         qtcOver     * aOver );

    // BUG-41631 RANK() Within Group Aggregation.
    static idBool canHasOverClause( qtcNode * aNode );

    // BIGINT Value를 생성한다.
    static IDE_RC getBigint( SChar*          aStmtText,
                             SLong*          aValue,
                             qcNamePosition* aPosition );

    /*************************************************************
     * Validation 단계에서의 처리
     *************************************************************/

    // SET의 Target등의 표현을 위해 internal column을 생성함.
    static IDE_RC makeInternalColumn( qcStatement* aStatement,
                                      UShort       aTable,
                                      UShort       aColumn,
                                      qtcNode**    aNode);

    // Invalid View에 대한 처리
    static void fixAfterValidationForCreateInvalidView (
        qcTemplate* aTemplate );

    // Validation 완료 후의 처리
    static IDE_RC fixAfterValidation( iduVarMemList * aMemory, //fix PROJ-1596
                                      qcTemplate    * aTemplate );

    // Argument를 고려하지 않은 estimate 처리
    static IDE_RC estimateNodeWithoutArgument( qcStatement* aStatement,
                                               qtcNode*     aNode );

    // 바로 하위 Arguemnt만을 고려한 estimate 처리
    static IDE_RC estimateNodeWithArgument( qcStatement* aStatement,
                                            qtcNode*     aNode );

    // PROJ-1718 Window function을 위한 estimate 처리
    static IDE_RC estimateWindowFunction( qcStatement * aStatement,
                                          qmsSFWGH    * aSFWGH,
                                          qtcNode     * aNode );

    // 전체 Node Tree를 Traverse하며 estimate 처리
    static IDE_RC estimate( qtcNode*     aNode,
                            qcTemplate*  aTemplate,
                            qcStatement* aStatement,
                            qmsQuerySet* aQuerySet,
                            qmsSFWGH*    aSFWGH,
                            qmsFrom*     aFrom );

    /* PROJ-1353 */
    static IDE_RC estimateNodeWithSFWGH( qcStatement * aStatement,
                                         qmsSFWGH    * aSFWGH,
                                         qtcNode     * aNode );

    // Conversion Node를 생성한다.
    static IDE_RC makeConversionNode( qtcNode*         aNode,
                                      qcStatement*     aStatement,
                                      qcTemplate*      aTemplate,
                                      const mtdModule* aModule );

    // ORDER BY indicator에 대한 정보를 설정함.
    static IDE_RC getSortColumnPosition( qmsSortColumns* aSortColumn,
                                         qcTemplate*     aTemplate );

    // 두 qtcNode가 같은 expression을 표현하는지 검사
    static IDE_RC isEquivalentExpression( qcStatement     * aStatement,
                                          qtcNode         * aNode1,
                                          qtcNode         * aNode2,
                                          idBool          * aIsTrue);

    // 두 predicate이 같은 predicate인지 검사
    static IDE_RC isEquivalentPredicate( qcStatement * aStatement,
                                         qtcNode     * aPredicate1,
                                         qtcNode     * aPredicate2,
                                         idBool      * aResult );

    static IDE_RC isEquivalentExpressionByName( qtcNode * aNode1,
                                                qtcNode * aNode2,
                                                idBool  * aIsEquivalent );

    // column node를 stored function node로 변환
    static IDE_RC changeNodeFromColumnToSP( qcStatement * aStatement,
                                            qtcNode     * aNode,
                                            mtcTemplate * aTemplate,
                                            mtcStack    * aStack,
                                            SInt          aRemain,
                                            mtcCallBack * aCallBack,
                                            idBool      * aFindColumn );

    // BUG-16000
    // Equal연산이 가능한 타입인지 검사
    static idBool isEquiValidType( qtcNode     * aNode,
                                   mtcTemplate * aTemplate );

    // PROJ-1413
    static IDE_RC registerTupleVariable( qcStatement    * aStatement,
                                         qcNamePosition * aPosition );

    // Constant Expression의 사전 처리
    static IDE_RC preProcessConstExpr( qcStatement * aStatement,
                                       qtcNode     * aNode,
                                       mtcTemplate * aTemplate,
                                       mtcStack    * aStack,
                                       SInt          aRemain,
                                       mtcCallBack * aCallBack );

    // column 수행 위치 설정 ( Target, Group By, Having ... )
    static IDE_RC setColumnExecutionPosition( mtcTemplate * aTemplate,
                                              qtcNode     * aNode,
                                              qmsSFWGH    * aColumnSFWGH,
                                              qmsSFWGH    * aCurrentSFWGH );

    /* PROJ-2462 ResultCache */
    static void checkLobAndEncryptColumn( mtcTemplate * aTemplate,
                                          mtcNode     * aNode );

    // BUG-45745
    static idBool isSameType( mtcColumn * aColumn1,
                              mtcColumn * aColumn2 );
    
    /*************************************************************
     * Optimization 단계에서의 처리
     *************************************************************/

    // Optimization 완료 후의 처리
    static IDE_RC fixAfterOptimization( qcStatement * aStatement );
    
    // PRIOR Node의 Table ID를 변경함.
    static IDE_RC priorNodeSetWithNewTuple( qcStatement* aStatement,
                                            qtcNode**    aNode,
                                            UShort       aOriginalTable,
                                            UShort       aTable );

    // Assign을 위한 Node를 생성함.
    static IDE_RC makeAssign( qcStatement * aStatement,
                              qtcNode     * aNode,
                              qtcNode     * aArgument );

    // Indirect 처리를 위한 Node를 생성함.
    static IDE_RC makeIndirect( qcStatement* aStatement,
                                qtcNode*     aNode,
                                qtcNode*     aArgument );

    // Pass Node를 생성함
    static IDE_RC makePassNode( qcStatement * aStatement,
                                qtcNode     * aCurrentNode,
                                qtcNode     * aArgumentNode,
                                qtcNode    ** aPassNode );

    // Constant Wrapper Node를 생성함.
    static IDE_RC makeConstantWrapper( qcStatement * aStatement,
                                       qtcNode     * aNode );

    // Node Tree내에서 Host Constant Expression을 찾고,
    // 이에 대하여 Constant Wrapper Node를 생성함.
    static IDE_RC optimizeHostConstExpression( qcStatement * aStatement,
                                               qtcNode     * aNode );

    // Subquery Wrapper Node를 생성함.
    static IDE_RC makeSubqueryWrapper( qcStatement * aStatement,
                                       qtcNode     * aSubqueryNode,
                                       qtcNode    ** aWrapperNode );

    // PROJ-1502 PARTITIONED DISK TABLE
    // Node Tree를 복사하면서 partitioned table의 노드를
    // partition의 노드로 변경함.
    static IDE_RC cloneQTCNodeTree4Partition(
        iduVarMemList * aMemory,
        qtcNode*        aSource,
        qtcNode**       aDestination,
        UShort          aSourceTable,
        UShort          aDestTable,
        idBool          aContainRootsNext );

    // Node Tree를 복사함.
    static IDE_RC cloneQTCNodeTree( iduVarMemList * aMemory, //fix PROJ-1596
                                    qtcNode       * aSource,
                                    qtcNode      ** aDestination,
                                    idBool          aContainRootsNext,
                                    idBool          aConversionClear,
                                    idBool          aConstCopy,
                                    idBool          aConstRevoke );

    static IDE_RC copyNodeTree( qcStatement *   aStatement,
                                qtcNode*        aSource,
                                qtcNode**       aDestination,
                                idBool          aContainRootsNext,
                                idBool          aSubqueryCopy );

    static IDE_RC copySubqueryNodeTree( qcStatement  * aStatement,
                                        qtcNode      * aSource,
                                        qtcNode     ** aDestination );
    
    // DNF Filter를 위해 필요한 Node를 복사함.
    static IDE_RC copyAndForDnfFilter( qcStatement* aStatement,
                                       qtcNode*     aSource,
                                       qtcNode**    aDestination,
                                       qtcNode**    aLast );

    // PROJ-1486
    // 두 Node의 수행 결과의 Data Type을 일치시킴
    static IDE_RC makeSameDataType4TwoNode( qcStatement * aStatement,
                                            qtcNode     * aNode1,
                                            qtcNode     * aNode2 );

    // PROJ-2582 recursive with
    static IDE_RC makeLeftDataType4TwoNode( qcStatement * aStatement,
                                            qtcNode     * aNode1,
                                            qtcNode     * aNode2 );

    // PROJ-2582 recursive with
    static IDE_RC makeRecursiveTargetInfo( qcStatement * aStatement,
                                           qtcNode     * aWithNode,
                                           qtcNode     * aViewNode,
                                           qcmColumn   * aColumnInfo );
    
    // template의 현재 dataSize값을 얻어오고 새로운 size만큼 갱신한다.
    static IDE_RC getDataOffset( qcStatement * aStatement,
                                 UInt          aSize,
                                 UInt        * aOffsetPtr );

    // PROJ-1446 Host variable을 포함한 질의 최적화
    // qcStatement에 scan decision factor를 add, remove 하는 함수들
    static IDE_RC addSDF( qcStatement           * aStatement,
                          qmoScanDecisionFactor * aSDF );

    static IDE_RC removeSDF( qcStatement           * aStatement,
                             qmoScanDecisionFactor * aSDF );

    // PROJ-1405 ROWNUM
    // 숫자형 constant value의 값을 가져온다.
    static idBool getConstPrimitiveNumberValue( qcTemplate  * aTemplate,
                                                qtcNode     * aNode,
                                                SLong       * aNumberValue );

    /*************************************************************
     * Binding 단계에서의 처리
     *************************************************************/

    // Binding 후의 처리 작업
    static IDE_RC setVariableTupleRowSize( qcTemplate    * aTemplate );

    // aNode가 상수 node tree인지 아닌지 알아온다. (ORDER BY용)
    static idBool isConstNode4OrderBy( qtcNode * aNode );

    // BUG-25594
    // aNode가 상수 node tree인지 아닌지 알아온다. (like pattern용)
    static idBool isConstNode4LikePattern( qcStatement * aStatement,
                                           qtcNode     * aNode,
                                           qcDepInfo   * aOuterDependencies );

    /*************************************************************
     * Execution 단계에서의 처리
     *************************************************************/

    // Aggregation Node의 초기화
    static IDE_RC initialize( qtcNode*    aNode,
                              qcTemplate* aTemplate );

    // Aggregation Node의 연산 수행
    static IDE_RC aggregate( qtcNode*    aNode,
                             qcTemplate* aTemplate );

    // Aggregation Node의 연산 수행
    static IDE_RC aggregateWithInfo( qtcNode*    aNode,
                                     void*       aInfo,
                                     qcTemplate* aTemplate );

    // Aggregation Node의 종료 작업 수행
    static IDE_RC finalize( qtcNode*    aNode,
                            qcTemplate* aTemplate );

    // 해당 Node의 연산을 수행
    static IDE_RC calculate( qtcNode*    aNode,
                             qcTemplate* aTemplate );

    // 해당 Node의 연산 결과의 논리값을 판단
    static IDE_RC judge( idBool*     aResult,
                         qtcNode*    aNode,
                         qcTemplate* aTemplate );

    /* PROJ-2209 DBTIMEZONE */
    static IDE_RC setDatePseudoColumn( qcTemplate * aTemplate );

    // 수행 시점의 System Time을 획득함. fix BUG-14394
    static IDE_RC sysdate( mtdDateType* aDate );

    // Subquery Node의 Plan 초기화
    static IDE_RC initSubquery( mtcNode*     aNode,
                                mtcTemplate* aTemplate );

    // Subquery Node의 Plan 수행
    static IDE_RC fetchSubquery( mtcNode*     aNode,
                                 mtcTemplate* aTemplate,
                                 idBool*      aRowExist );

    // Subquery Node의 Plan 종료
    static IDE_RC finiSubquery( mtcNode*     aNode,
                                mtcTemplate* aTemplate );

    /* PROJ-2248 Subquery caching */
    static IDE_RC setCalcSubquery( mtcNode     * aNode,
                                   mtcTemplate * aTemplate );

    static IDE_RC setSubAggregation( qtcNode * aNode );

    // PROJ-1502 PARTITIONED DISK TABLE
    static IDE_RC estimateConstExpr( qtcNode     * aNode,
                                     qcTemplate  * aTemplate,
                                     qcStatement * aStatement );

    // PROJ-1502 PARTITIONED DISK TABLE
    static idBool isConstValue( qcTemplate  * aTemplate,
                                qtcNode     * aNode );

    static idBool isHostVariable( qcTemplate  * aTemplate,
                                  qtcNode     * aNode );
    
    // BUG-15746
    // ColumnNode를 통해 칼럼 정보를 획득하여
    // BindNode에 대응되는 Bind Param 정보를 설정함
    // ( setDescribeParamInfo4Where()에서 이 함수를 호출 ) 
    static IDE_RC setBindParamInfoByNode( qcStatement * aStatement,
                                          qtcNode     * aColumnNode,
                                          qtcNode     * aBindNode );
    
    // where절에서 (칼럼)(비교연산자)(bind) 형태를 찾아
    // bindParamInfo에 칼럼 정보를 설정함 
    static IDE_RC setDescribeParamInfo4Where( qcStatement * aStatement,
                                              qtcNode     * aNode );

    // BUG-34231
    static idBool isQuotedName( qcNamePosition * aPosition );

    /* PROJ-2208 Multi Currency */
    static IDE_RC getNLSCurrencyCallback( mtcTemplate * aTemplate,
                                          mtlCurrency * aCurrency );

    // loop count
    static IDE_RC getLoopCount( qcTemplate  * aTemplate,
                                qtcNode     * aLoopNode,
                                SLong       * aCount );

    static IDE_RC addKeepArguments( qcStatement  * aStatement,
                                    qtcNode     ** aNode,
                                    qtcKeepAggr  * aKeep );

    static IDE_RC changeKeepNode( qcStatement  * aStatement,
                                  qtcNode     ** aNode );

    /* PROJ-2632 */
    static IDE_RC estimateSerializeFilter( qcStatement          * aStatement,
                                           mtcNode              * aNode,
                                           UInt                 * aCount );

    static IDE_RC allocateSerializeFilter( qcStatement          * aStatement,
                                           UInt                   aCount,
                                           mtxSerialFilterInfo ** aInfo );

    static IDE_RC recursiveSerializeFilter( qcStatement          * aStatement,
                                            mtcNode              * aNode,
                                            mtxSerialFilterInfo  * aFense,
                                            mtxSerialFilterInfo ** aInfo );

    static IDE_RC recursiveConversionSerializeFilter( qcStatement          * aStatement,
                                                      mtcNode              * aNode,
                                                      mtxSerialFilterInfo  * aFense,
                                                      mtxSerialFilterInfo ** aInfo );

    static IDE_RC checkSerializeFilterType( qcTemplate           * aTemplate,
                                            mtcNode              * aNode,
                                            mtcNode             ** aArgument,
                                            mtxSerialExecuteFunc * aExecute,
                                            UChar                * aEntryType );

    static IDE_RC checkNodeModuleForNormal( qcTemplate           * aTemplate,
                                            mtcNode              * aNode,
                                            mtcNode             ** aArgument,
                                            mtxSerialExecuteFunc * aExecute,
                                            UChar                * aEntryType );

    static IDE_RC checkNodeModuleForEtc( qcTemplate           * aTemplate,
                                         mtcNode              * aNode,
                                         mtcNode             ** aArgument,
                                         mtxSerialExecuteFunc * aExecute,
                                         UChar                * aEntryType );

    static IDE_RC checkConversionSerializeFilterType( qcTemplate           * aTemplate,
                                                      mtcNode              * aNode,
                                                      mtcNode              * aArgument,
                                                      mtxSerialExecuteFunc * aExecute,
                                                      UChar                * aEntryType );

    static IDE_RC adjustSerializeFilter( UChar                  aType,
                                         mtxSerialFilterInfo  * aPrev,
                                         mtxSerialFilterInfo ** aInfo );

    static IDE_RC setConversionSerializeFilter( UChar                  aType,
                                                mtcNode              * aNode,
                                                mtxSerialExecuteFunc   aExecute,
                                                mtxSerialFilterInfo  * aFense,
                                                mtxSerialFilterInfo ** aInfo );

    static IDE_RC setParentSerializeFilter( UChar                  aType,
                                            UChar                  aCount,
                                            mtcNode              * aNode,
                                            mtxSerialExecuteFunc   aExecute,
                                            mtxSerialFilterInfo  * aLeft,
                                            mtxSerialFilterInfo  * aRight,
                                            mtxSerialFilterInfo  * aFense,
                                            mtxSerialFilterInfo ** aInfo );

    static IDE_RC setSerializeFilter( UChar                  aType,
                                      UChar                  aCount,
                                      mtcNode              * aNode,
                                      mtxSerialExecuteFunc   aExecute,
                                      mtxSerialFilterInfo  * aLeft,
                                      mtxSerialFilterInfo  * aRight,
                                      mtxSerialFilterInfo  * aFense,
                                      mtxSerialFilterInfo ** aInfo );
    
};

class qtcNodeI
{
/***********************************************************************
 *
 * qtcNode에 대한 각종 접근자 함수들
 *
 *      by kumdory, 2005-04-25
 ***********************************************************************/

public:

    static inline idBool isListNode( qtcNode * aNode )
    {
        return ( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
                 == MTC_NODE_OPERATOR_LIST ) ? ID_TRUE : ID_FALSE;
    }

    static inline void linkNode( qtcNode * aPrev, qtcNode * aNext )
    {
        if( aNext == NULL )
        {
            aPrev->node.next = NULL;
        }
        else
        {
            aPrev->node.next = (mtcNode *)&(aNext->node);
        }
    }

    static inline qtcNode* nextNode( qtcNode * aNode )
    {
        return (qtcNode *)(aNode->node.next);
    }

    static inline qtcNode* argumentNode( qtcNode * aNode )
    {
        return (qtcNode *)(aNode->node.arguments);
    }

    static inline idBool isOneColumnSubqueryNode( qtcNode * aNode )
    {
        if( aNode == NULL || aNode->node.arguments == NULL )
        {
            return ID_FALSE;
        }
        else if( aNode->node.arguments->next == NULL )
        {
            return ID_TRUE;
        }
        else
        {
            return ID_FALSE;
        }
    }

    static inline idBool isColumnArgument( qcStatement * aStatement,
                                           qtcNode     * aNode )
    {
        if( ( aNode->node.lflag & MTC_NODE_OPERATOR_MASK )
              == MTC_NODE_OPERATOR_LIST )
        {
            aNode = (qtcNode *)(aNode->node.arguments);
            while( aNode != NULL )
            {
                if( QTC_IS_COLUMN( aStatement, aNode ) == ID_TRUE )
                {
                    // Nothing To Do
                }
                else
                {
                    return ID_FALSE;
                }
                aNode = (qtcNode *)(aNode->node.next);
            }
            return ID_TRUE;
        }
        else
        {
            if( QTC_IS_COLUMN( aStatement, aNode ) == ID_TRUE )
            {
                return ID_TRUE;
            }
            else
            {
                return ID_FALSE;
            }
        }
    }
};

#endif /* _O_QTC_H_ */

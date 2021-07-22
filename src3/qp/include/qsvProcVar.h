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
 * $Id: qsvProcVar.h 84378 2018-11-19 09:55:58Z ahra.cho $
 **********************************************************************/

#ifndef _Q_QSV_PROC_VAR_H_
#define _Q_QSV_PROC_VAR_H_ 1

#include <iduVarMemList.h>
#include <qc.h>
#include <qsParseTree.h>

class qsvProcVar
{
public:
    static IDE_RC validateParaDef( qcStatement      * aStatement,
                                   qsVariableItems  * aParaDecls );

    static IDE_RC validateLocalVariable( qcStatement * aStatement,
                                         qsVariables * aVariable );

    static IDE_RC checkAttributeColType( qcStatement * aStatement,
                                         qsVariables * aVariable );

    static IDE_RC checkAttributeRowType( qcStatement * aStatement,
                                         qsVariables * aVariable);

    static IDE_RC searchArrayVar( qcStatement  * aStatement,
                                  qtcNode      * aArrNode,
                                  idBool       * aIsFound,
                                  qsVariables ** aFoundVariable );

    static IDE_RC searchVarAndPara(
        qcStatement     * aStatement,
        qtcNode         * aVarNode,
        idBool            aSearchForLValue,
        idBool          * aIsFound,
        qsVariables    ** aVariable );

    // PROJ-2533 array에 관한 variable 탐색
    static IDE_RC searchVarAndParaForArray(
        qcStatement      * aStatement,
        qtcNode          * aVarNode,
        idBool           * aIsFound,
        qsVariables     ** aVariable,
        const mtfModule ** aModule );

    static IDE_RC searchCursor(
        qcStatement     * aStatement,
        qtcNode         * aNode,
        idBool          * aIsFound,
        qsCursors      ** aCursorDef);

    static IDE_RC setParamModules(
        qcStatement      * aStatement,
        UInt               aParaDeclCount,
        qsVariableItems  * aParaDecls,
        mtdModule      *** aModules,
        mtcColumn       ** aParaColumns);

    static IDE_RC copyParamModules(
        qcStatement      * aStatement,
        UInt               aParaDeclCount,
        mtcColumn        * aSrcParaColumn,
        mtdModule      *** aDstModules,
        mtcColumn       ** aDstParaColumns );

    // PROJ-1075 record/array 변수 생성
    static IDE_RC makeUDTVariable(
        qcStatement     * aStatement,
        qsVariables     * aVariable );

    // PROJ-1075 record/rowtype 변수 생성
    static IDE_RC makeRecordVariable(
        qcStatement     * aStatement,
        qsTypes         * aType,
        qsVariables     * aVariable );

    // PROJ-1075 array 변수 생성
    static IDE_RC makeArrayVariable(
        qcStatement     * aStatement,
        qsTypes         * aType,
        qsVariables     * aVariable );

    // PROJ-1386 ref cursor 변수 생성
    static IDE_RC makeRefCurVariable(
        qcStatement     * aStatement,
        qsTypes         * aType,
        qsVariables     * aVariable );
    
    // PROJ-1075 trigger rowtype변수 생성
    static IDE_RC makeTriggerRowTypeVariable(
        qcStatement     * aStatement,
        qcmTableInfo    * aTableInfo,
        qsVariables     * aVariable );

    // PROJ-1075 rowtype 변수 생성
    static IDE_RC makeRowTypeVariable(
        qcStatement     * aStatement,
        qcmTableInfo    * aTableInfo,
        qsVariables     * aVariable);
    
    static SChar * getValidSQL( qcStatement * aStatement );

    // PROCJ-1073 Package
    static IDE_RC checkPkgVarType(
        qcStatement * aStatement,
        qsVariables * aVariable ,
        idBool * aValidVariable,
        mtcColumn ** aColumn );

    static IDE_RC searchNodeFromPkg(
        qcStatement  * aStatement,
        qsxPkgInfo   * aPlanTree,
        qtcNode      * aQtcNode,
        idBool       * aValidVariable,
        mtcColumn   ** aColumn);

    static IDE_RC searchVarFromPkg(
        qcStatement  * aStatement,
        qsxPkgInfo   * aPlanTree,
        qsVariables  * aVariable,
        idBool       * aValidVariable,
        mtcColumn   ** aColumn);

    static IDE_RC searchVariableFromPkg(
        qcStatement   * aStatement,
        qtcNode       * aVarNode,
        idBool        * aIsFound,
        qsVariables  ** aVariable );

    // PROJ-2533 array에 관한 variable 탐색
    static IDE_RC searchVariableFromPkgForArray(
        qcStatement      * aStatement,
        qtcNode          * aVarNode,
        idBool           * aIsFound,
        qsVariables     ** aVariable,
        const mtfModule ** aModule );

    static IDE_RC searchPkgVariable(
        qcStatement   * aStatement,
        qsxPkgInfo    * aPkgInfo,
        qtcNode       * aVarNode,
        idBool        * aIsFound,
        qsVariables  ** aVariable );

    // BUG-46032
    static IDE_RC makeInternalArrayVariable( qcStatement      * aStatement, 
                                             qtcNode          * aNode,      
                                             qsVariables      * aParentArray,
                                             qsVariables     ** aChildArray );

    // BUG-46174
    static IDE_RC makeRecordColumnByName( qcStatement      * aStatement,
                                          qtcNode          * aRecord,
                                          qcNamePosition   * aFieldName,
                                          qtcNode         ** aColumn );

private:

    static IDE_RC makeArgumentsForRowTypeNode(
        qcStatement     * aStatement,
        qsTypes         * aType,
        qtcNode         * aRowNode,
        UShort            aTable );
    
    static IDE_RC searchVarType(
        qcStatement * aStatement,
        qsVariables * aVariable,
        idBool      * aIsFound,
        mtcColumn  ** aColumn );

    static IDE_RC searchVariableNonArg(
        qcStatement     * aStatement,
        qtcNode         * aVarNode,
        idBool          * aIsFound,
        qsVariables    ** aVariable );

    static IDE_RC searchParameterNonArg(
        qcStatement     * aStatement,
        qtcNode         * aVarNode,
        idBool            aSearchForLValue,
        idBool          * aIsFound,
        qsVariables    ** aVariable );
    
    static IDE_RC searchVariableWithArg(
        qcStatement     * aStatement,
        qtcNode         * aVarNode,
        idBool          * aIsFound,
        qsVariables    ** aVariable );

    // PROJ-2533 array에 관한 variable 탐색
    static IDE_RC searchVariableForArray(
        qcStatement      * aStatement,
        qtcNode          * aVarNode,
        idBool           * aIsFound,
        qsVariables     ** aVariable,
        const mtfModule ** aModule );

    static IDE_RC searchParameterWithArg(
        qcStatement     * aStatement,
        qtcNode         * aVarNode,
        idBool          * aIsFound,
        qsVariables    ** aVariable );
 
    // PROJ-2533 array에 관한 variable 탐색
    static IDE_RC searchParameterForArray(
        qcStatement      * aStatement,
        qtcNode          * aVarNode,
        idBool           * aIsFound,
        qsVariables     ** aVariable,
        const mtfModule ** aModule );
   
    static IDE_RC searchVariableItems(
        qsVariableItems * aVariableItems,
        qsVariableItems * aCurrDeclItem,
        qcNamePosition  * aVarName,
        idBool          * aIsFound,
        qsVariables    ** aFoundVariable );

    static IDE_RC searchFieldTypeOfRecord(
        qsVariables     * aRecordVariable,
        qcNamePosition  * aFieldName,
        idBool          * aIsFound,
        qcTemplate      * aTemplate,
        mtcColumn      ** aColumn );

    static IDE_RC searchFieldOfRecord(
        qcStatement     * aStatement,
        qsTypes         * aType,
        qtcNode         * aRecordVarNode,
        qtcNode         * aVariable,
        idBool            aSearchForLValue,
        idBool          * aIsFound);       

    static IDE_RC setPrimitiveDataType(
        qcStatement     * aStatement,
        mtcColumn       * aColumn,
        qsVariables     * aVariable);

    // PROJ-1073 Package
    static IDE_RC searchPkgLocalVarType(
        qcStatement * aStatement,
        qsVariables * aVariable,
        idBool      * aIsFound,
        mtcColumn  ** aColumn );

    static IDE_RC searchPkgLocalVarNonArg(
        qcStatement    * aStatement,
        qsPkgParseTree * aPkgParseTree,
        qtcNode        * aVarNode,
        idBool         * aIsFound,
        qsVariables   ** aVariable );

    static IDE_RC searchPkgLocalVarWithArg(
        qcStatement    * aStatement,
        qsPkgParseTree * aPkgParseTree,
        qtcNode        * aVarNode,
        idBool         * aIsFound,
        qsVariables   ** aVariable );

    // PROJ-2533 array에 관한 variable 탐색
    static IDE_RC searchPkgLocalVarForArray(
        qcStatement      * aStatement,
        qsPkgParseTree   * aPkgParseTree,
        qtcNode          * aVarNode,
        idBool           * aIsFound,
        qsVariables     ** aVariable,
        const mtfModule ** aModule );

    static IDE_RC searchPkgVarNonArg(
        qcStatement  * aStatement,
        qsxPkgInfo   * aPkgInfo,
        qtcNode      * aVarNode,
        idBool       * aIsFound,
        qsVariables  ** aVariable );

    static IDE_RC searchPkgVarWithArg(
        qcStatement  * aStatement,
        qsxPkgInfo   * aPkgInfo,
        qtcNode      * aVarNode,
        idBool       * aIsFound,
        qsVariables ** aVariable );

    // BUG-38243 support array method in package
    static IDE_RC searchPkgLocalArrayVarInternal(
        qcStatement     * aStatement,
        qsPkgParseTree  * aPkgParseTree,
        qsVariableItems * aVariableItems,
        qtcNode         * aArrNode,
        idBool          * aIsFound,
        qsVariables    ** aFoundVariable );

    static IDE_RC searchPkgLocalArrayVar(
        qcStatement     * aStatement,
        qtcNode         * aArrNode,
        idBool          * aIsFound,
        qsVariables    ** aFoundVariable );

    static IDE_RC searchPkgArrayVar(
        qcStatement  * aStatement,
        qtcNode      * aArrNode,
        idBool       * aIsFound,
        qsVariables ** aFoundVariable );

    /* BUG-41847
       package local에 있는 type에 대해 찾을 수 있어야 합니다. */
    static IDE_RC searchPkgLocalVarTypeInternal( qcStatement     * aStatement,
                                                 qsPkgParseTree  * aPkgParseTree,
                                                 qtcNode         * aVarNode,
                                                 qcTemplate      * aTemplate,
                                                 idBool          * aIsFound,
                                                 mtcColumn      ** aColumn );
};

#endif  // _Q_QSV_PROC_VAR_H_

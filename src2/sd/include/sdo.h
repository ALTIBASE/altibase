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
 * $Id: sdo.h 90192 2021-03-12 02:01:03Z jayce.park $
 **********************************************************************/

#ifndef _O_SDO_H_
#define _O_SDO_H_ 1

#include <sdi.h>

/* TASK-7219 Shard Transformer Refactoring */
class sdo
{
public:
    /* */
    static IDE_RC allocAndInitQuerySetList( qcStatement * aStatement );

    static IDE_RC makeAndSetQuerySetList( qcStatement * aStatement,
                                          qmsQuerySet * aQuerySet );

    static IDE_RC setQuerySetListState( qcStatement  * aStatement,
                                        qmsParseTree * aParseTree,
                                        idBool       * aIsChanged );

    static IDE_RC unsetQuerySetListState( qcStatement * aStatement,
                                          idBool        aIsChanged );

    /* */
    static IDE_RC makeAndSetAnalysis( qcStatement * aSrcStatement,
                                      qcStatement * aDstStatement,
                                      qmsQuerySet * aDstQuerySet );

    static IDE_RC allocAndCopyTableInfoList( qcStatement       * aStatement,
                                             sdiTableInfoList  * aSrcTableInfoList,
                                             sdiTableInfoList ** aDstTableInfoList );

    /* */
    static IDE_RC makeAndSetAnalyzeInfoFromStatement( qcStatement * aStatement );

    static IDE_RC makeAndSetAnalyzeInfoFromParseTree( qcStatement     * aStatement,
                                                      qcParseTree     * aParseTree,
                                                      sdiAnalyzeInfo ** aAnalyzeInfo );

    static IDE_RC makeAndSetAnalyzeInfoFromQuerySet( qcStatement     * aStatement,
                                                     qmsQuerySet     * aQuerySet,
                                                     sdiAnalyzeInfo ** aAnalyzeInfo );

    static IDE_RC makeAndSetAnalyzeInfoFromObjectInfo( qcStatement     * aStatement,
                                                       sdiObjectInfo   * aShardObjInfo,
                                                       sdiAnalyzeInfo ** aAnalyzeInfo );

    /* */
    static IDE_RC isShardParseTree( qcParseTree * aParseTree,
                                    idBool      * aIsShardParseTree );

    static IDE_RC isShardQuerySet( qmsQuerySet * aQuerySet,
                                   idBool      * aIsShardQuerySet,
                                   idBool      * aIsTransformAble );

    static IDE_RC isShardObject( qcParseTree * aParseTree,
                                 idBool      * aIsShardObject );

    static IDE_RC isSupported( qmsQuerySet * aQuerySet,
                               idBool      * aIsSupported );

    /* */
    static IDE_RC setStatementFlagForShard( qcStatement * aStatement,
                                            UInt          aFlag );

    static IDE_RC isTransformNeeded( qcStatement * aStatement,
                                     idBool      * aIsTransformNeeded );

    static IDE_RC setPrintInfoFromTransformAble( qcStatement * aStatement );

    static IDE_RC raiseInvalidShardQueryError( qcStatement * aStatement,
                                               qcParseTree * aParseTree );

    /* */
    static IDE_RC preAnalyzeQuerySet( qcStatement * aStatement,
                                      qmsQuerySet * aQuerySet,
                                      ULong         aSMN );

    static IDE_RC preAnalyzeSubquery( qcStatement * aStatement,
                                      qtcNode     * aNode,
                                      ULong         aSMN );

    static IDE_RC setShardPlanStmtVariable( qcStatement    * aStatement,
                                            qcShardStmtType  aStmtType,
                                            qcNamePosition * aStmtPos );

    static IDE_RC setShardPlanCommVariable( qcStatement    * aStatement,
                                            sdiAnalyzeInfo * aAnalyzeInfo,
                                            UShort           aParamCount,
                                            UShort           aParamOffset,
                                            qcShardParamInfo * aParamInfo );

    static IDE_RC setShardStmtType( qcStatement * aStatement,
                                    qcStatement * aViewStatement );
private:

    /* */
    static IDE_RC makeAndSetAnalysisStatement( qcStatement * aDstStatement,
                                               qcStatement * aSrcStatement );

    static IDE_RC makeAndSetAnalysisPartialStatement( qcStatement * aDstStatement,
                                                      qmsQuerySet * aDstQuerySet,
                                                      qcStatement * aSrcStatement );

    static IDE_RC allocAndCopyAnalysisQuerySet( qcStatement * aStatement,
                                                qmsQuerySet * aDstQuerySet,
                                                qmsQuerySet * aSrcQuerySet );

    static IDE_RC allocAndCopyAnalysisParseTree( qcStatement * aStatement,
                                                 qcParseTree * aDstParseTree,
                                                 qcParseTree * aSrcParseTree );

    static IDE_RC allocAndCopyAnalysisPartialStatement( qcStatement * aStatement,
                                                        qmsQuerySet * aDstQuerySet,
                                                        qcParseTree * aSrcParseTree );

    static IDE_RC getQuerySetAnalysis( qmsQuerySet       * aQuerySet,
                                       sdiShardAnalysis ** aAnalysis );

    static IDE_RC allocAndCopyAnalysis( qcStatement       * aStatement,
                                        sdiShardAnalysis  * aSrcAnalysis,
                                        sdiShardAnalysis ** aDstAnalysis );

    static IDE_RC allocAndCopyKeyInfo( qcStatement * aStatement,
                                       sdiKeyInfo  * aSrcKeyInfo,
                                       sdiKeyInfo ** aDstKeyInfo );

    static IDE_RC allocAndCopyKeyTarget( qcStatement  * aStatement,
                                         UInt           aSrcCount,
                                         UShort       * aSrcTarget,
                                         UInt         * aDstCount,
                                         UShort      ** aDstTarget );

    static IDE_RC allocAndCopyKeyColumn( qcStatement        * aStatement,
                                         UInt                 aSrcCount,
                                         sdiKeyTupleColumn  * aSrcolumn,
                                         UInt               * aDstCount,
                                         sdiKeyTupleColumn ** aDstColumn );

    static IDE_RC allocAndCopyShardInfo( qcStatement  * aStatement,
                                         sdiShardInfo * aSrcShardInfo,
                                         sdiShardInfo * aDstShardInfo );

    /* */
    static IDE_RC makeAndSetAnalyzeInfoFromNodeInfo( qcStatement     * aStatement,
                                                     qcShardNodes    * aNodeInfo,
                                                     sdiAnalyzeInfo ** aAnalyzeInfo );

    /* */
    static IDE_RC setPrintInfoFromAnalyzeInfo( qcStatement    * aStatement,
                                               sdiAnalyzeInfo * aAnalyzeInfo );
};
#endif /* _O_SDO_H_ */

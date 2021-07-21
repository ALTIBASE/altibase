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
 * $Id: qcmPerformanceView.h 83394 2018-07-02 03:28:18Z hykim $
 *
 * Description :
 *
 *     FT 테이블들에 대하여
 *     CREATE VIEW V$VIEW AS SELECT * FROM X$TABLE;
 *     과 같이 view를 정의하여 일반 사용자에게는  이 view에 대한 연산만을
 *     개방하도록 한다.
 *
 * 용어 설명 :
 *
 * 약어 :
 *
 **********************************************************************/

#ifndef _O_QCM_PERFORMANCEVIEW_H_
#define _O_QCM_PERFORMANCEVIEW_H_ 1

#include    <smiDef.h>
#include    <qc.h>
#include    <qtc.h>
#include    <qdbCommon.h>
#include    <qdbCreate.h>
#include    <qdv.h>
#include    <qcmTableInfo.h>

extern SChar * gQcmPerformanceViews[];

// PROJ-1726
// qcmPerformanceViewManager
// 용도 : 동적모듈화가 진행되면서 기존의 정적 모듈이 정적으로
// performance view를 등록하는 방식(qcm/qcmPerformanceView.cpp
// 의 gQcmPerformanceViews 배열에 추가) 외에 동적 모듈 내에서
// 런타임 중 추가되는 performance view 가 존재한다.
// 이 두가지 방식의 performance view 를 처리하기 위한,
// 기존의 gQcmPerformanceViews 배열에 대한 일종의 wrapper class.

// 동적모듈화가 되지 않은 모듈의 경우 performance view는
// 기존방식대로 gQcmPerformanceViews 에 추가하며,
// 동적모듈화가 진행된 모듈의 경우 <모듈>i.h 에 매크로로 정의 뒤
// <모듈>im.h :: initSystemTables 에서 
// qciMisc::addPerformanceViews(aQueryStr) 를 이용, 등록한다.
// 사용 예는 rpi.h, rpim.cpp 등을 참조한다.
class qcmPerformanceViewManager
{
public:
    static IDE_RC   initialize();
    static IDE_RC   finalize();

    // 총 performance view 의 수를 얻는다.
    static SInt     getTotalViewCount( qcmPVType aType )
    {
        SInt sCount = 0;

        switch( aType )
        {
            case QCM_PV_TYPE_NORMAL:
                sCount = mNumOfPreViews + mNumOfAddedViews;
                break;
            case QCM_PV_TYPE_SHARD:
                sCount = mShardViewCount;
                break;
            default:
                IDE_DASSERT(0);
                break;
        }

        return sCount;
    }

    // 동적으로 performance view 를 추가
    static IDE_RC   add(SChar* aViewStr);

    // performance view 를 리턴. 기존의 gQcmPerformanceViews의
    // 배열 접근 방식과 같은 인터페이스 제공.
    static SChar *  get( int aIdx, qcmPVType aType );

private:
    static SChar *  getFromAddedViews(int aIdx);

    static SChar ** mPreViews;
    static SInt     mNumOfPreViews;

    static SInt     mNumOfAddedViews;
    static iduList  mAddedViewList;

    static SChar ** mShardViews;
    static SInt     mShardViewCount;
};

class qcmPerformanceView
{

private:

    static IDE_RC runDDLforPV( idvSQL * aStatistics, qcmPVType aType );

    static IDE_RC executeDDL( qcStatement * aStatement,
                              SChar       * aText );

    static IDE_RC registerOnIduFixedTableDesc( qdTableParseTree * aParseTree );

public:

    static IDE_RC makeParseTreeForViewInSelect(
            qcStatement   * aStatement,
            qmsTableRef   * aTableRef );


    static IDE_RC registerPerformanceView( idvSQL * aStatistics, qcmPVType aType );

    // parse
    static IDE_RC parseCreate( qcStatement * aStatement );

    // validate
    static IDE_RC validateCreate( qcStatement * aStatement );

    // execute
    static IDE_RC executeCreate( qcStatement * aStatement );

    // null buildRecord function
    static IDE_RC nullBuildRecord( idvSQL *,
                                   void * ,
                                   void * ,
                                   iduFixedTableMemory *aMemory);

};

#endif /* _O_QCM_PERFORMANCEVIEW_H_ */

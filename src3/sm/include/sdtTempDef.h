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
 * $Id: sdtTempDef.h 24767 2008-01-08 07:42:33Z newdaily $
 **********************************************************************/

#ifndef _O_SDT_TEMP_DEF_H_
# define _O_SDT_TEMP_DEF_H_ 1

#include <sdtDef.h>

/*****************************************************************************
 * PROJ-2201 Innovation in sorting and hashing(temp)
 *****************************************************************************/

/*********************************************************************
 *
 * - SortTemp의 State Transition Diagram
 *      +------------------------------------------------------+
 *      |                                                      |
 * +------------+                                         +----------+
 * |InsertNSort |----<sort()>--+                          |InsertOnly|
 * +------------+   +----------+-------------------+      +----------+
 *      |           |          |                   |           |
 *      |   +------------+ +-------+ +---------+   |           |
 *   sort() |ExtractNSort|-| Merge |-|MakeIndex| sort()      scan()
 *      |   +------------+ +-------+ +---------+   |           |
 *      |                      |          |        |           |
 * +------------+        +---------+ +---------+   |      +----------+
 * |InMemoryScan|        |MergeScan| |IndexScan|---+      |   Scan   |
 * +------------+        +---------+ +---------+          +----------+
 *
 * 위 상태에 따라 Group들이 구분되며, Group에 따라 아래 Copy 연산이 달라진다.
 *
 *
 *
 * 1. InsertNSort
 *  InsertNSort는 최로로 데이터가 삽입되는 상태이다. 다만 삽입하면서 동시에
 * 정렬도 한다.
 *  그리고 이 상태는 Limit,MinMax등 InMemoryOnly 상태이냐, Range이냐, 그외냐
 *  에 따라 Group의 모양이 달라진다.
 *
 * 1) InsertNSort( InMemoryOnly )
 *  Limit, MinMax등 반드시 페이지 부족이 일어나지 않는다는 보장이 있을 경우
 *  InMemoryOnly로 수행한다.
 * +-------+ +-------+
 * | Sort1 |-| Sort2 |
 * +-------+ +-------+
 *     |         |
 *     +<-SWAP->-+
 * 두개의 SortGroup이 있고, 한 SortGroup이 가득 찰때마다 Compaction용도로
 * SWAP을 수행한다.
 *
 * 2) InsertNSort ( Normal )
 *  일반적인 형태로 Merge등을 하는 형태이다.
 * +-------+ +-------+
 * | Sort  |-| Flush |
 * +-------+ +-------+
 *     |          |
 *     +-NORMAL->-+
 *
 * 3) ExtractNSort ( RangeScan )
 *  RangeScan이 필요해 Index를 만드는 형태이다. Normal과 같지만,
 * Key와 Row를 따로 저장한다. Row에는 모든 칼럼이 들어있고, Key는 IndexColumn
 * 만 들어간다. 이후 Merge등의 과정을 Key를 가지고 처리한다.
 * +-------+ +-------+ +--------+
 * | Sort  |-| Flush |-|SubFlush|
 * +-------+ +-------+ +--------+
 *     |           |        |
 *     +-MAKE_KEY--+        |
 *     |                    |
 *     +-<-<-<-EXTRACT_ROW--+
 *
 *
 *
 *  2. ExtractNSort
 *  InsertNSort의 1.3과 동일하지만, insert는 데이터를 QP로부터 받는데반해
 *  Extract는 1.3 과정에서 SubFlush에 복사해둔 Row를 다시 꺼내어 정렬한다는
 *  점이 특징이다.
 * +-------+ +-------+ +--------+
 * | Sort  |-| Flush |-|SubFlush|
 * +-------+ +-------+ +--------+
 *     |           |        |
 *     +-MAKE_KEY->+        |
 *     |                    |
 *     +-<-<--<-NORMAL------+
 *
 *
 *
 *  3. Merge
 *  SortGroup에 Heaq을 구성한 후 FlushGroup으로 복사하며, FlushGroup으로
 *  Run을 구성한다. 즉 InsertNSort(Normal) 1.2와 동일하다.
 *
 *
 *
 *  4. MakeIndex
 *  Merge와 유사하지만, ExtraPage라는 것으로 Row를 쪼갠다. 왜냐하면 index의
 *  한 Node에는 2개의 Key가 들어가야 안정적이며, 따라서 2개 이상의 Key가
 *  들어갈 수 있도록, Key가 4000Byte를 넘으면 남는 Row가 4000Byte가 되도록
 *  4000Byte 이후의 것들을 ExtraPage에 저장시킨다.
 *
 *  1) LeafNode 생성
 * +-------+ +-------+ +--------+
 * | Sort  |-| Flush |-|SubFlush|
 * +-------+ +-------+ +--------+
 *   |        |              | |
 *   |        +-<-MAKE_LNODE-+ |
 *   |                         |
 *   +-(copyExtraRow())-->->->-+
 *
 *  2) InternalNode 생성
 *  LeafNode는 Depth를 아래에서 위로 올라가며 만드는 방식이다. 현재는
 *  BreadthFirst로 진행하여 속도가 느리니 주의.
 * +-------+ +-------+ +--------+
 * | Sort  |-| Flush |-|SubFlush|
 * +-------+ +-------+ +--------+
 *            |              |
 *            +-<-MAKE_INODE-+
 *
 *
 *
 *  5. InMemoryScan
 *  InsertNSort(Normal) 1.2와 동일하다.
 *
 *
 *
 *  6. MergeScan
 *  Merge와 동일하다.
 *
 *
 *
 *  7.IndexScan
 *  Copy는 없다.
 *  +-------+ +-------+
 *  | INode |-| LNode |
 *  +-------+ +-------+
 *
 * 8. InsertOnly
 *  InsertOnly 최로로 데이터가 삽입되는 상태이다. 다만 삽입하면서
 *   정렬을 하지 않는것이 InsertNSort 와 차이점.
 *
 * 1) InsertOnly( InMemoryOnly )
 *   반드시 페이지 부족이 일어나지 않는다는 보장이 있을 경우
 *  InMemoryOnly로 수행한다.
 *
 * 2) InsertOnly ( Normal )
 *  일반적인 형태로 저장된 런의 순서로 읽음.
 */

typedef enum sdtCopyPurpose
{
    SDT_COPY_NORMAL,
    SDT_COPY_SWAP,
    SDT_COPY_MAKE_KEY,
    SDT_COPY_MAKE_LNODE,
    SDT_COPY_MAKE_INODE,
    SDT_COPY_EXTRACT_ROW,
} sdtCopyPurpose;

/* make MergeRun
 * +------+------+------+------+------+
 * | Size |0 PID |0 Slot|1 PID |1 Slot|
 * +------+------+------+------+------+
 * MergeRun은 위와같은 모양의 Array로, MergeRunCount * 2 + 1 개로 구성된다.
 */
typedef struct sdtTempMergeRunInfo
{
    /* Run번호.
     * Run은 Last부터 왼쪽으로 배치되며, 하나의 Run은 MaxRowPageCount의 크기를
     * 갖기 때문에, LastWPID - MaxRowPageCnt * No 하면 Run의 WPID를 알 수
     * 있다. */
    UInt    mRunNo;
    UInt    mPIDSeq;  /*Run내 PageSequence */
    SShort  mSlotNo;  /*Page의 Slot번호 */
} sdtTempMergeRunInfo;

#define SDT_TEMP_RUNINFO_NULL ( ID_UINT_MAX )

#define SDT_TEMP_MERGEPOS_SIZEIDX     ( 0 )
#define SDT_TEMP_MERGEPOS_PIDIDX(a)   ( a * 2 + 1 )
#define SDT_TEMP_MERGEPOS_SLOTIDX(a)  ( a * 2 + 2 )

#define SDT_TEMP_MERGEPOS_SIZE(a)     ( a * 2 + 1 )

typedef scPageID sdtTempMergePos;


/* make ScanRun
 * +------+-----+-------+-------+------+
 * | Size | pin | 0 PID | 1 PID |
 * +------+-----+-------+-------+------+
 *  ScanRund 은  위와 같은 모양의 array로 RunCount +2  로 구성된다.
 */
#define SDT_TEMP_SCANPOS_SIZEIDX     ( 0 )
#define SDT_TEMP_SCANPOS_PINIDX      ( 1 )
#define SDT_TEMP_SCANPOS_HEADERIDX   ( 2 )

#define SDT_TEMP_SCANPOS_PIDIDX(a)   ( a + 2 )
#define SDT_TEMP_SCANPOS_SIZE(a)     ( a + 2 )

typedef scPageID sdtTempScanPos;

#endif /* _O_SDT_TEMP_DEF_H_ */

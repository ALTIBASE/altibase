/*
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

package Altibase.jdbc.driver.datatype;

import java.sql.SQLException;
import java.util.List;

  // PROJ-2368
  /**
   * @author pss4you
   * ListBufferHandle 과 RowHandle 의 상위 Interface
   * 두 객체 사이에 공통 되는 부분을 여기서 Define 하여 Code 를 간결하게 만들기 위함
   */
public interface BatchDataHandle
{
    /**
     * DataHandle 에 저장된 작업 숫자를 반환 한다.
     * 
     * @return 현재 까지 저장된 작업 숫자
     */
    int size();

    /**
     * DataHandle 에 Binding 된 작업을 저장한다.
     */
    void store() throws SQLException;

    /**
     * DataHandle 을 초기화 한다.
     */
    void initToStore();

    /**
     * DataHandle 에 Binding 할 Column 들을 setting 한다.
     */
    void setColumns(List<Column> aColumns);

    /**
     * 기존 Bind Column에 새로운 Type으로 바인딩이 된 경우 해당하는 값의 type으로 bind column type을 변경한다.
     * @param aIndex 컬럼인덱스 (base 0)
     * @param aColumn Column 정보
     * @param aColumnInfo Column 메타정보
     * @param aInOutType InOutType 정보
     */
    void changeBindColumnType(int aIndex, Column aColumn, ColumnInfo aColumnInfo, byte aInOutType);
}

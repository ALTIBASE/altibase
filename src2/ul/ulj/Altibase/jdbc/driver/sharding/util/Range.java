/*
 * Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

package Altibase.jdbc.driver.sharding.util;

import java.io.Serializable;
import java.util.Comparator;

/**
 * 범위를 나타내는 클래스<br>
 * 이 클래스를 사용하려면 반드시 Comparator를 구현해야 한다.
 */
public final class Range<T> implements Serializable
{
    private static final long   serialVersionUID = 8140790962014453934L;
    private final Comparator<T> mComparator;
    private final T             mMinimum;
    private final T             mMaximum;
    private transient int       mHashCode;
    private transient String    mToString;

    /**
     * mMinimum값과 mMaximum값 사이에 해당하는 Range객체를 생성한다.
     *
     * @param <T> 범위에 들어가는 엘리멘트 타입
     * @param aFrom 범위의 시작을 나타내는 값
     * @param aTo 범위의 끝을 나타내는 값
     * @return 범위 객체
     * @throws IllegalArgumentException 엘리먼트가 null인 경우
     * @throws ClassCastException 엘리먼트가 Comparable을 구현하고 있지 않은 경우
     */
    public static <T extends Comparable<T>> Range<T> between(T aFrom, T aTo)
    {
        return between(aFrom, aTo, null);
    }

    /**
     * 첫번째 Range의 max값과 mMaximum값 사이에 해당하는 Range객체를 생성한다.
     *
     * @param <T> 범위에 들어가는 엘리멘트 타입
     * @param aFrom 범위의 시작을 나타내는 Range객체
     * @param aTo 범위의 끝을 나타내는 값
     * @return 범위 객체
     * @throws IllegalArgumentException 엘리먼트가 null인 경우
     * @throws ClassCastException 엘리먼트가 Comparable을 구현하고 있지 않은 경우
     */
    public static <T extends Comparable<T>> Range<T> between(Range<T> aFrom, T aTo)
    {
        if (aFrom.getMaximum() != null && aFrom.isEndedBy(aTo))
        {
            return between(aFrom.getMinimum(), aFrom.getMaximum());
        }
        return between(aFrom.getMaximum(), aTo, null);
    }

    public static Range getNullRange()
    {
        return between(null, null, null);
    }

    /**
     * 매개변수로 지정된 Comparator를 사용해 Range객체를 생성한다.
     *
     * @param <T> 범위에 들어가는 엘리멘트 타입
     * @param aFrom 범위의 시작을 나타내는 값
     * @param aTo 범위의 끝을 나타내는 값
     * @param aComparator 정렬을 위해 사용할 Comparator객체, null인 경우에는 natural ordering을 사용한다.
     * @return 범위 객체
     * @throws IllegalArgumentException 엘리먼트가 null인 경우
     * @throws ClassCastException 엘리먼트가 natural ordering을 사용하고 Comparable을 구현하고 있지 않은 경우.
     */
    public static <T> Range<T> between(T aFrom, T aTo, Comparator<T> aComparator)
    {
        return new Range<T>(aFrom, aTo, aComparator);
    }

    /**
     * Range객체 생성
     *
     * @param aFrom 첫번째 범위값
     * @param aTo 두번째 범위값
     * @param aComparator Comparator 객체, null인 경우에는 natural ordering
     */
    @SuppressWarnings("unchecked")
    private Range(T aFrom, T aTo, Comparator<T> aComparator)
    {
        if (aComparator == null)
        {
            aComparator = ComparableComparator.INSTANCE;
        }
        if (aFrom != null)
        {
            if (aComparator.compare(aFrom, aTo) < 1)
            {
                this.mMinimum = aFrom;
                this.mMaximum = aTo;
            }
            else
            {
                this.mMinimum = aTo;
                this.mMaximum = aFrom;
            }
        }
        else
        {
            this.mMinimum = null;
            this.mMaximum = aTo;
        }

        this.mComparator = aComparator;
    }

    /**
     * 범위의 최소값을 리턴한다.
     *
     * @return 범위 최소값
     */
    public T getMinimum()
    {
        return mMinimum;
    }

    /**
     * 범위의 최대값을 리턴한다.
     *
     * @return 범위 최대값
     */
    public T getMaximum()
    {
        return mMaximum;
    }

    /**
     * 해당 엘리먼트가 범위에 속하는지 여부를 리턴한다.<br>
     * 범위의 최소값과 최대값을 포함해서 판단한다.(inclusive)
     * @param aElement 비교할 엘리멘트값
     * @return 해당 엘리먼트가 범위에 포함되어 있는지 여부
     */
    public boolean contains(T aElement)
    {
        if (aElement == null)
        {
            return false;
        }
        return (mComparator.compare(aElement, mMinimum) > -1) &&
               (mComparator.compare(aElement, mMaximum) < 1);
    }

    /**
     * 해당 엘리먼트가 범위에 속하는지 여부를 리턴한다.<br>
     * 범위의 최소값은 비교하지 않고 최대값 보다 작은지만 판단한다.(exclusive)
     * @param aElement 비교할 엘리멘트값
     * @return 해당 엘리먼트가 범위에 포함되어 있는지 여부
     */
    public boolean containsEndedBy(T aElement)
    {
        if (aElement == null)
        {
            return false;
        }

        return mComparator.compare(aElement, mMaximum) < 0;
    }

    /**
     * 해당 엘리먼트가 범위에 속하는지 여부를 리턴한다.<br>
     * 범위의 최소값보다는 크거나 같고(inclusive) 최대값 보다는 작은지 판단한다.(exclusive)
     * @param aElement 비교할 엘리멘트값
     * @return 해당 엘리먼트가 범위에 포함되어 있는지 여부
     */
    public boolean containsEqualAndLessThan(T aElement)
    {
        if (aElement == null)
        {
            return false;
        }

        if (mMinimum == null) // min값이 null인 경우에는 max만 비교
        {
            return mComparator.compare(aElement, mMaximum) < 0;
        }

        return (mComparator.compare(aElement, mMinimum) > -1) &&
               (mComparator.compare(aElement, mMaximum) < 0);
    }

    /**
     * 해당 값이 범위의 최소값 보다 작은지 여부를 판단한다(exclusive).
     * @param element 검사할 객체
     * @return true 해당 엘리먼트가 범위보다 작은 경우
     */
    public boolean isAfter(T element)
    {
        if (element == null)
        {
            return false;
        }
        return mComparator.compare(element, mMinimum) < 0;
    }

    /**
     * 해당 객체로 범위가 시작되는지 여부를 판단한다(inclusive).
     *
     * @param element 검사할 객체
     * @return true 범위가 해당 값으로 시작하는 경우
     */
    public boolean isStartedBy(T element)
    {
        if (element == null)
        {
            return false;
        }
        return mComparator.compare(element, mMinimum) == 0;
    }

    /**
     * 해당 객체로 범위가 끝나는지 여부를 판단한다(inclusive).
     *
     * @param aElement 검사할 객체
     * @return true 범위가 해당 값으로 끝나는 경우
     */
    public boolean isEndedBy(T aElement)
    {
        if (aElement == null)
        {
            return false;
        }
        return mComparator.compare(aElement, mMaximum) == 0;
    }

    /**
     * 해당 값이 범위의 최대값 보다 큰 여부를 판단한다(exclusive).
     * @param element 검사할 객체
     * @return true 해당 엘리먼트가 범위보다 큰 경우
     */
    public boolean isBefore(T element)
    {
        if (element == null)
        {
            return false;
        }
        return mComparator.compare(element, mMaximum) > 0;
    }

    /**
     * 선택된 범위가 Range에 포함되는지 여부를 리턴한다(inclusive).
     *
     * @param aRange 체크할 Range객체
     * @return true 체크할 Range객체가 포함되는 경우
     * @throws RuntimeException 범위를 비교할 수 없는 경우
     */
    public boolean containsRange(Range<T> aRange)
    {
        if (aRange == null)
        {
            return false;
        }
        return contains(aRange.mMinimum) && contains(aRange.mMaximum);
    }

    /**
     * 범위가 aRange의 범위보다 후인지 확인한다.
     *
     * @param aRange 확인할 범위 변수
     * @return true 범위의 최소값이 aRange의 범위 max값보다 클때
     * @throws RuntimeException 범위를 비교할 수 없는 경우
     */
    public boolean isAfterRange(Range<T> aRange)
    {
        if (aRange == null)
        {
            return false;
        }
        return isAfter(aRange.mMaximum);
    }

    /**
     * 해당 범위가 매개변수로 들어온 범위에 오버랩 되는지 여부를 판단한다.<br>
     * (두 범위에 적어도 하나의 요소가 포함되는 경우)
     *
     * @param aOtherRange 확인할 범위 객체
     * @return true 지정된 범위가 해당 범위와 오버랩 되는 경우
     * @throws RuntimeException 범위를 비교할 수 없는 경우
     */
    public boolean isOverlappedBy(Range<T> aOtherRange)
    {
        if (aOtherRange == null)
        {
            return false;
        }
        return aOtherRange.contains(mMinimum) || aOtherRange.contains(mMaximum) ||
               contains(aOtherRange.mMinimum);
    }

    /**
     * 범위가 지정된 범위보다 완전하게 앞에 있는지 여부를 판단한다.
     *
     * @param aOtherRange 확인할 범위 객체
     * @return true 해당 범위가 지정된 범위보다 앞에 있을 경우
     * @throws RuntimeException 범위를 비교할 수 없는 경우
     */
    public boolean isBeforeRange(Range<T> aOtherRange)
    {
        if (aOtherRange == null)
        {
            return false;
        }
        return isBefore(aOtherRange.mMinimum);
    }

    /**
     * 두개의 객체가 동일하려면 최소값과 최대값이 동일해야하며 이 때 Comparator의 차이점은 무시된다.
     *
     * @param aObject 비교할 객체
     * @return true 두개 객체가 동일할때
     */
    @Override
    public boolean equals(Object aObject)
    {
        if (aObject == this)
        {
            return true;
        }
        else if (aObject == null || aObject.getClass() != getClass())
        {
            return false;
        }
        else
        {
            @SuppressWarnings("unchecked")
            Range<T> range = (Range<T>)aObject;
            if (mMinimum == null)
            {
                return mMaximum.equals(range.mMaximum);
            }
            return mMinimum.equals(range.mMinimum) && mMaximum.equals(range.mMaximum);
        }
    }

    /**
     * 범위객체에 적절한 해쉬코드를 가져온다.
     *
     * @return 해쉬코드 값
     */
    @Override
    public int hashCode()
    {
        int sResult = mHashCode;
        if (mHashCode == 0)
        {
            sResult = 17;
            sResult = 37 * sResult + getClass().hashCode();
            sResult = 37 * sResult + mMinimum.hashCode();
            sResult = 37 * sResult + mMaximum.hashCode();
            mHashCode = sResult;
        }
        return sResult;
    }

    @Override
    public String toString()
    {
        String sResult = mToString;
        if (sResult == null)
        {
            StringBuilder sBuf = new StringBuilder();
            sBuf.append('[');
            sBuf.append(mMinimum);
            sBuf.append("..");
            sBuf.append(mMaximum);
            sBuf.append(']');
            sResult = sBuf.toString();
            mToString = sResult;
        }
        return sResult;
    }


    @SuppressWarnings({ "rawtypes", "unchecked" })
    private enum ComparableComparator implements Comparator
    {
        INSTANCE;
        /**
         * Comparable 베이스 구현(싱글턴으로 생성하기 위해 enum사용)
         *
         * @param aObj1 비교대상값 1
         * @param aObj2 비교대상값 2
         * @return negative  aObj1 < aObj2
         *         0         aObj1 = aObj2
         *         positive  aObj1 > aObj2
         */
        public int compare(Object aObj1, Object aObj2)
        {
            return ((Comparable)aObj1).compareTo(aObj2);
        }
    }

}

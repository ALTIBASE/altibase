/** 
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
 
/***********************************************************************
 * $Id:
 **********************************************************************/

#ifndef _O_IDU_FATALCALLBACK_H_
#define _O_IDU_FATALCALLBACK_H_ 1

#define IDU_FATAL_INFO_CALLBACK_ARR_SIZE (16)

typedef void ( *iduCallbackFunction )( void );

class iduFatalCallback
{
public:
    static IDE_RC initializeStatic();
    static IDE_RC destroyStatic();

    static void   setCallback( iduCallbackFunction aCallbackFunction );
    static void   unsetCallback( iduCallbackFunction aCallbackFunction );
    static void   doCallback();

private:
    static IDE_RC lock()   { return mMutex.lock( NULL ); };
    static IDE_RC unlock() { return mMutex.unlock(); };

    static iduMutex            mMutex;
    static iduCallbackFunction mCallbackFunction[IDU_FATAL_INFO_CALLBACK_ARR_SIZE];
    static idBool              mInit;
};

#endif  // _O_IDU_FATALCALLBACK_H_

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
 
#ifndef _O_IDU_RTM_H_
#define _O_IDU_RTM_H_ 1

#include <idTypes.h>
#include <iduMutex.h>

#define IDURTM_SUCCESS  ((SInt)(-1))
#define IDURTM_EXPLICIT (1 << 0)
#define IDURTM_RETRY    (1 << 1)
#define IDURTM_CONFLICT (1 << 2)
#define IDURTM_CAPACITY (1 << 3)
#define IDURTM_DEBUG    (1 << 4)
#define IDURTM_NESTED   (1 << 5)

extern SInt idurtmavailable(void);
extern SInt idurtmbegin(void);
extern void idurtmend(void);
extern void idurtmabort(void);
extern SInt idurtmtest(void);

/*
 * checks if rtm is available
 */
inline idBool   iduRtmAvailable(void)
{
    return idurtmavailable()==0? ID_FALSE:ID_TRUE;
}

inline SInt     iduRtmXBegin(void) { return idurtmbegin(); }
inline void     iduRtmXEnd(void)   { return idurtmend();   }
inline void     iduRtmXAbort(void) { return idurtmabort(); }
inline SInt     iduRtmXTest(void)  { return idurtmtest();  }

#endif /* _O_IDU_RTM_H_ */

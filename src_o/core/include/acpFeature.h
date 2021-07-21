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
 
/*******************************************************************************
 * $Id: acpFeature.h 11162 2010-06-01 05:43:55Z djin $
 ******************************************************************************/


#if !defined(_O_ACP_FEATURE_H_)
#define _O_ACP_FEATURE_H_

#include <acpTypes.h>
#include <acpError.h>



/*
 * Feature list
 */
typedef enum acp_feature_t
{
    ACP_FEATURE_TYPE_ATOMIC = 100, /* meaningless number */
    ACP_FEATURE_TYPE_CALLSTACK,
    ACP_FEATURE_TYPE_IPV6   
} acp_feature_t;


/*
 * Individual Features Definition
 */

/*
 * Define feature macros. Features can be defined at compilation
 * command: gcc -DACP_ISOC90=1. If the macro is not defined at
 * compilation command, it is defined in this file.
 */

#if !(defined ACP_FEATURE_NATIVE_ATOMIC)
# if (defined ALTI_CFG_CPU_PARISC)
#  define ACP_FEATURE_NATIVE_ATOMIC ACP_FALSE
# else
#  define ACP_FEATURE_NATIVE_ATOMIC ACP_TRUE
# endif
#endif

#if !(defined ACP_FEATURE_CALLSTACK)
# if (defined ALTI_CFG_OS_DARWIN)
#  define ACP_FEATURE_CALLSTACK ACP_FALSE
# else
#  define ACP_FEATURE_CALLSTACK ACP_TRUE
# endif
#endif

#if !(defined ACP_FEATURE_IPV6)
#define ACP_FEATURE_IPV6 ACP_FALSE
#endif


ACP_EXPORT acp_rc_t acpFeatureStatus(acp_feature_t aRequest, ...);

#endif

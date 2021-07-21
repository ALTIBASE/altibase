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
 * $Id: acpPset.c 5245 2009-04-14 02:02:23Z jykim $
 ******************************************************************************/

#include <acp.h>
#include <acpSys.h>
#include <acpPset.h>

/* ------------------------------------------------
 *  Why did I dedine gAcpPsetInitControl and gAcpPsetFunction,
 *  Because of minimizing the calling costs of acpPsetGetConf().
 *
 *  acpPsetGetConf() is very expensive and have a lots of system call
 *  which can effect to the other process or threads CPU binds.
 *  so, I decide to store the system cpu informationto
 *  global memrory(gAcpPsetSysCache).
 *
 *  but, acp_thr_once_t doesn't support of by-passing of error code(only void).
 *  therefore, I have to define function pointer of init, success, failure.
 *
 *  so far, you can understand it.
 *
 *  Additionally, gAcpPsetSysCache is used in a unbind() API for a platform
 *  which doesn't support an unbinding API like Linux.
 * ----------------------------------------------*/

static acp_thr_once_t gAcpPsetInitControl = ACP_THR_ONCE_INITIALIZER;
static acp_pset_t     gAcpPsetSysCache;
static acp_rc_t       gErrorCode          = ACP_RC_SUCCESS;
static acp_rc_t     (*gAcpPsetFunction)(acp_pset_t *aPset);

static acp_rc_t acpPsetGetConfInit(acp_pset_t *aPset);

static acp_rc_t acpPsetGetConfError(acp_pset_t *aPset)
{
    ACP_UNUSED(aPset);

    return gErrorCode; /* previous error code */
}

static acp_rc_t acpPsetGetConfSuccess(acp_pset_t *aPset)
{
    acpMemCpy(aPset, &gAcpPsetSysCache, sizeof(acp_pset_t));

    return ACP_RC_SUCCESS;
}

static void acpPsetInitOnce(void)
{
    acp_rc_t sRC;

    sRC = acpPsetGetConfInit(&gAcpPsetSysCache);

    if (sRC == ACP_RC_SUCCESS)
    {
        gAcpPsetFunction = acpPsetGetConfSuccess;
    }
    else
    {
        gErrorCode = sRC;
        gAcpPsetFunction = acpPsetGetConfError;
    }
}

# if defined(ALTI_CFG_OS_LINUX)
# include <acpPset-LINUX.c>
# define  ACP_PSET_SUPPORT_BINDING      ACP_TRUE
# define  ACP_PSET_SUPPORT_MCPU_BINDING ACP_TRUE

# elif defined(ALTI_CFG_OS_AIX)
# include <acpPset-AIX.c>
# define  ACP_PSET_SUPPORT_BINDING      ACP_TRUE
# define  ACP_PSET_SUPPORT_MCPU_BINDING ACP_FALSE

# elif defined(ALTI_CFG_OS_HPUX)
# include <acpPset-HPUX.c>
# define  ACP_PSET_SUPPORT_BINDING      ACP_TRUE
# define  ACP_PSET_SUPPORT_MCPU_BINDING ACP_FALSE

# elif defined(ALTI_CFG_OS_SOLARIS)
# include <acpPset-SOLARIS.c>
# define  ACP_PSET_SUPPORT_BINDING      ACP_TRUE
# define  ACP_PSET_SUPPORT_MCPU_BINDING ACP_FALSE

# elif defined(ALTI_CFG_OS_TRU64)
# include <acpPset-TRU64.c>
# define  ACP_PSET_SUPPORT_BINDING      ACP_TRUE
# define  ACP_PSET_SUPPORT_MCPU_BINDING ACP_TRUE

# elif defined(ALTI_CFG_OS_WINDOWS)
# include <acpPset-WINDOWS.c>
# define  ACP_PSET_SUPPORT_BINDING      ACP_TRUE
# define  ACP_PSET_SUPPORT_MCPU_BINDING ACP_TRUE

# else
# warning Unknown OS
# define  ACP_PSET_SUPPORT_BINDING      ACP_FALSE
# define  ACP_PSET_SUPPORT_MCPU_BINDING ACP_FALSE
# endif


/**
 * Get the cpu information as a bitSets(acp_pset_t)
 *
 * When you call this function at the first time,
 * the cpu bind state of your current process may become a unbind state.
 * So, use this function in a early stage, or
 * you need to rebind current process again.
 *
 * but, in second calling of this function,
 * the cached info will be used.
 * therefore, the bind info will not be changed.
 *
 * @param BitArray Sets of the CPUs
 * @param Is support of CPU binding itself  or NOT
 * @param Is support of Binding Multiple CPUS sets to a process or thread
 * @return result code
 */

ACP_EXPORT acp_rc_t acpPsetGetConf(acp_pset_t *aSystemPset,
                                   acp_bool_t *aIsSupportBind,
                                   acp_bool_t *aIsSupportMultipleBind)
{
    (void)acpThrOnce(&gAcpPsetInitControl, acpPsetInitOnce);

    if (aIsSupportBind != NULL)
    {
        *aIsSupportBind = ACP_PSET_SUPPORT_BINDING;
    }
    else
    {
        /* NULL passed : skip */
    }

    if (aIsSupportMultipleBind != NULL)
    {
        *aIsSupportMultipleBind = ACP_PSET_SUPPORT_MCPU_BINDING;
    }
    else
    {
        /* NULL passed : skip */
    }

    return (*gAcpPsetFunction)(aSystemPset);
}

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
 * $Id: aclConf.h 9884 2010-02-03 06:50:04Z gurugio $
 ******************************************************************************/

#if !defined(_O_ACL_CONF_H_)
#define _O_ACL_CONF_H_

/**
 * @file
 * @ingroup CoreEnv
 */

#include <acpStr.h>


ACP_EXTERN_C_BEGIN


#define ACL_CONF_DEFVAL_BOOLEAN(aVal) { (aVal), 0, 0, 0, 0, NULL }
#define ACL_CONF_DEFVAL_SINT32(aVal)  { ACP_FALSE, (aVal), 0, 0, 0, NULL }
#define ACL_CONF_DEFVAL_UINT32(aVal)  { ACP_FALSE, 0, (aVal), 0, 0, NULL }
#define ACL_CONF_DEFVAL_SINT64(aVal)  { ACP_FALSE, 0, 0, (aVal), 0, NULL }
#define ACL_CONF_DEFVAL_UINT64(aVal)  { ACP_FALSE, 0, 0, 0, (aVal), NULL }
#define ACL_CONF_DEFVAL_STRING(aVal)  { ACP_FALSE, 0, 0, 0, 0, (aVal) }

#define ACL_CONF_CHKVAL_SINT32(aVal)  { (aVal), 0, 0, 0 }
#define ACL_CONF_CHKVAL_UINT32(aVal)  { 0, (aVal), 0, 0 }
#define ACL_CONF_CHKVAL_SINT64(aVal)  { 0, 0, (aVal), 0 }
#define ACL_CONF_CHKVAL_UINT64(aVal)  { 0, 0, 0, (aVal) }

#define ACL_CONF_DEF_PRIVATE_MEMBER_INITIALIZER  0
#define ACL_CONF_UNDEFINED_KEY                  -1

/**
 * defines a key for boolean value
 * @param aKey null-terminated C style string of key
 * @param aID integer id to identify the key (eg, enumeration value)
 * @param aDefault default value
 * @param aCallback callback function to assign the value
 */
#define ACL_CONF_DEF_BOOLEAN(aKey,                                          \
                             aID,                                           \
                             aDefault,                                      \
                             aCallback) { (aKey),                           \
            NULL,                                                           \
            NULL,                                                           \
            (aID),                                                          \
            1,                                                              \
            ACL_CONF_TYPE_BOOLEAN,                                          \
            ACL_CONF_DEFVAL_BOOLEAN((aDefault)),                            \
            ACL_CONF_CHKVAL_SINT32(0),                                      \
            ACL_CONF_CHKVAL_SINT32(0),                                      \
            (aCallback),                                                    \
            ACL_CONF_DEF_PRIVATE_MEMBER_INITIALIZER                         \
            }

/**
 * defines a key for signed 32-bit integer value
 * @param aKey null-terminated C style string of key
 * @param aID integer id to identify the key (eg, enumeration value)
 * @param aDefault default value
 * @param aMin minimum value
 * @param aMax maximum value
 * @param aMultiplicity multiplicity of the value
 * @param aCallback callback function to assign the value
 *
 * if @a aMin and @a aMax is zero, the validation will be skipped.
 */
#define ACL_CONF_DEF_SINT32(aKey,                                          \
                             aID,                                           \
                             aDefault,                                      \
                             aMin,                                          \
                             aMax,                                          \
                             aMultiplicity,                                 \
                             aCallback) { (aKey),                           \
        NULL,                                                               \
        NULL,                                                               \
        (aID),                                                              \
        (aMultiplicity),                                                    \
        ACL_CONF_TYPE_SINT32,                                               \
        ACL_CONF_DEFVAL_SINT32((aDefault)),                                 \
        ACL_CONF_CHKVAL_SINT32((aMin)),                                     \
        ACL_CONF_CHKVAL_SINT32((aMax)),                                     \
        (aCallback),                                                        \
        ACL_CONF_DEF_PRIVATE_MEMBER_INITIALIZER                             \
            }

/**
 * defines a key for unsigned 32-bit integer value
 * @param aKey null-terminated C style string of key
 * @param aID integer id to identify the key (eg, enumeration value)
 * @param aDefault default value
 * @param aMin minimum value
 * @param aMax maximum value
 * @param aMultiplicity multiplicity of the value
 * @param aCallback callback function to assign the value
 *
 * if @a aMin and @a aMax is zero, the validation will be skipped.
 */
#define ACL_CONF_DEF_UINT32(aKey,                                           \
                            aID,                                            \
                            aDefault,                                       \
                            aMin,                                           \
                            aMax,                                           \
                            aMultiplicity,                                  \
                            aCallback) { (aKey),                            \
            NULL,                                                           \
            NULL,                                                           \
            (aID),                                                          \
            (aMultiplicity),                                                \
            ACL_CONF_TYPE_UINT32,                                           \
            ACL_CONF_DEFVAL_UINT32((aDefault)),                             \
            ACL_CONF_CHKVAL_UINT32((aMin)),                                 \
            ACL_CONF_CHKVAL_UINT32((aMax)),                                 \
            (aCallback),                                                    \
            ACL_CONF_DEF_PRIVATE_MEMBER_INITIALIZER                         \
            }

/**
 * defines a key for signed 64-bit integer value
 * @param aKey null-terminated C style string of key
 * @param aID integer id to identify the key (eg, enumeration value)
 * @param aDefault default value
 * @param aMin minimum value
 * @param aMax maximum value
 * @param aMultiplicity multiplicity of the value
 * @param aCallback callback function to assign the value
 *
 * if @a aMin and @a aMax is zero, the validation will be skipped.
 */
#define ACL_CONF_DEF_SINT64(aKey,                                           \
                            aID,                                            \
                            aDefault,                                       \
                            aMin,                                           \
                            aMax,                                           \
                            aMultiplicity,                                  \
                            aCallback) { (aKey),                            \
            NULL,                                                           \
            NULL,                                                           \
            (aID),                                                          \
            (aMultiplicity),                                                \
            ACL_CONF_TYPE_SINT64,                                           \
            ACL_CONF_DEFVAL_SINT64((aDefault)),                             \
            ACL_CONF_CHKVAL_SINT64((aMin)),                                 \
            ACL_CONF_CHKVAL_SINT64((aMax)),                                 \
            (aCallback),                                                    \
            ACL_CONF_DEF_PRIVATE_MEMBER_INITIALIZER                         \
            }

/**
 * defines a key for unsigned 64-bit integer value
 * @param aKey null-terminated C style string of key
 * @param aID integer id to identify the key (eg, enumeration value)
 * @param aDefault default value
 * @param aMin minimum value
 * @param aMax maximum value
 * @param aMultiplicity multiplicity of the value
 * @param aCallback callback function to assign the value
 *
 * if @a aMin and @a aMax is zero, the validation will be skipped.
 */
#define ACL_CONF_DEF_UINT64(aKey,                                           \
                            aID,                                            \
                            aDefault,                                       \
                            aMin,                                           \
                            aMax,                                           \
                            aMultiplicity,                                  \
                            aCallback) { (aKey),                            \
            NULL,                                                           \
            NULL,                                                           \
            (aID),                                                          \
            (aMultiplicity),                                                \
            ACL_CONF_TYPE_UINT64,                                           \
            ACL_CONF_DEFVAL_UINT64((aDefault)),                             \
            ACL_CONF_CHKVAL_UINT64((aMin)),                                 \
            ACL_CONF_CHKVAL_UINT64((aMax)),                                 \
            (aCallback),                                                    \
            ACL_CONF_DEF_PRIVATE_MEMBER_INITIALIZER                         \
            }

/**
 * defines a key for string value
 * @param aKey null-terminated C style string of key
 * @param aID integer id to identify the key (eg, enumeration value)
 * @param aDefault default value as null-terminated C style string
 * @param aMultiplicity multiplicity of the value
 * @param aCallback callback function to assign the value
 */
#define ACL_CONF_DEF_STRING(aKey,                                           \
                            aID,                                            \
                            aDefault,                                       \
                            aMultiplicity,                                  \
                            aCallback) { (aKey),                            \
            NULL,                                                           \
            NULL,                                                           \
            (aID),                                                          \
            (aMultiplicity),                                                \
            ACL_CONF_TYPE_STRING,                                           \
            ACL_CONF_DEFVAL_STRING((aDefault)),                             \
            ACL_CONF_CHKVAL_SINT32(0),                                      \
            ACL_CONF_CHKVAL_SINT32(0),                                      \
            (aCallback),                                                    \
            ACL_CONF_DEF_PRIVATE_MEMBER_INITIALIZER                         \
            }

/**
 * defines a key containing sub keys and values
 * @param aKey null-terminated C style string of key
 * @param aID integer id to identify the key (eg, enumeration value)
 * @param aDef array of sub-key definitions (#acl_conf_def_t *)
 * @param aMultiplicity multiplicity of the value
 *
 * if @a aKey is null, an unnamed container will be defined.
 */
#define ACL_CONF_DEF_CONTAINER(aKey,                                        \
                               aID,                                         \
                               aDef,                                        \
                               aMultiplicity) { (aKey),                     \
            NULL,                                                           \
            (aDef),                                                         \
            (aID),                                                          \
            (aMultiplicity),                                                \
            ACL_CONF_TYPE_CONTAINER,                                        \
            ACL_CONF_DEFVAL_SINT32(0),                                      \
            ACL_CONF_CHKVAL_SINT32(0),                                      \
            ACL_CONF_CHKVAL_SINT32(0),                                      \
            NULL,                                                           \
            ACL_CONF_DEF_PRIVATE_MEMBER_INITIALIZER                         \
            }

/**
 * specifies end of key definitions
 */
#define ACL_CONF_DEF_END() { NULL,                                             \
            NULL,                                                              \
            NULL,                                                              \
            0,                                                                 \
            0,                                                                 \
            ACL_CONF_TYPE_NONE,                                                \
            ACL_CONF_DEFVAL_SINT32(0),                                         \
            ACL_CONF_CHKVAL_SINT32(0),                                         \
            ACL_CONF_CHKVAL_SINT32(0),                                         \
            NULL,                                                              \
            ACL_CONF_DEF_PRIVATE_MEMBER_INITIALIZER                            \
            }


typedef enum acl_conf_type_t
{
    ACL_CONF_TYPE_NONE = 0,
    ACL_CONF_TYPE_BOOLEAN,
    ACL_CONF_TYPE_SINT32,
    ACL_CONF_TYPE_UINT32,
    ACL_CONF_TYPE_SINT64,
    ACL_CONF_TYPE_UINT64,
    ACL_CONF_TYPE_STRING,
    ACL_CONF_TYPE_CONTAINER,
    ACL_CONF_TYPE_UNNAMED_CONTAINER,
    ACL_CONF_TYPE_MAX
} acl_conf_type_t;

typedef struct acl_conf_defval_t
{
    acp_bool_t        mBoolean;
    acp_sint32_t      mSInt32;
    acp_uint32_t      mUInt32;
    acp_sint64_t      mSInt64;
    acp_uint64_t      mUInt64;
    const acp_char_t *mString;
} acl_conf_defval_t;

typedef struct acl_conf_chkval_t
{
    acp_sint32_t      mSInt32;
    acp_uint32_t      mUInt32;
    acp_sint64_t      mSInt64;
    acp_uint64_t      mUInt64;
} acl_conf_chkval_t;


/**
 * structure for configuration key information
 *
 * this will be passed to the callback function
 */
typedef struct acl_conf_key_t
{
    acp_str_t         mKey;   /**< key string (constant string object)     */
    acp_sint32_t      mID;    /**< key identifier                          */
    acp_sint32_t      mIndex; /**< multiplicity index of the value for key */
} acl_conf_key_t;

/**
 * structure for configuration key definitions
 */
typedef struct acl_conf_def_t acl_conf_def_t;

/**
 * prototype of callback function to assign value
 * @param aDepth number of keys
 * @param aKey array of keys
 * @param aLineNumber line number of a key
 * @param aValue pointer to the value
 * @param aContext opaque data passed into aclConfLoad()
 * @return 0 to continue parsing, non-zero to stop parsing
 *
 * if the callback function returns non-zero,
 * aclConfLoad() will stop parsing and return #ACP_RC_ECANCELED.
 *
 * type of @a aValue is described in the following table;
 * <table border="1" cellspacing="1" cellpadding="4">
 * <tr>
 * <td align="center">&nbsp;</td>
 * <td align="center">BOOLEAN</td>
 * <td align="center">SINT32</td>
 * <td align="center">UINT32</td>
 * <td align="center">SINT64</td>
 * <td align="center">UINT64</td>
 * <td align="center">STRING</td>
 * <td align="center">CONTAINER</td>
 * </tr>
 * <tr>
 * <td nowrap align="center"><i>type of aValue</i></td>
 * <td nowrap align="center">#acp_bool_t *</td>
 * <td nowrap align="center">#acp_sint32_t *</td>
 * <td nowrap align="center">#acp_uint32_t *</td>
 * <td nowrap align="center">#acp_sint64_t *</td>
 * <td nowrap align="center">#acp_uint64_t *</td>
 * <td nowrap align="center">#acp_str_t *</td>
 * <td nowrap align="center">N/A</td>
 * </tr>
 * </table>
 */
typedef acp_sint32_t acl_conf_set_callback_t(acp_sint32_t    aDepth,
                                             acl_conf_key_t *aKey,
                                             acp_sint32_t    aLineNumber,
                                             void           *aValue,
                                             void           *aContext);

/**
 * prototype of callback function to report error
 * @param aPath path of the configuration file
 * @param aLine error line number
 * @param aError error description
 * @param aContext opaque data passed into aclConfLoad()
 * @return 0 to continue parsing, non-zero to stop parsing
 */
typedef acp_sint32_t acl_conf_err_callback_t(acp_str_t    *aPath,
                                             acp_sint32_t  aLine,
                                             acp_str_t    *aError,
                                             void         *aContext);


struct acl_conf_def_t
{
    const acp_char_t        *mKey;
    acl_conf_def_t          *mSuperDef;
    acl_conf_def_t          *mSubDef;
    acp_sint32_t             mID;
    acp_sint32_t             mMultiplicity;
    acl_conf_type_t          mType;
    acl_conf_defval_t        mDefault;
    acl_conf_chkval_t        mMin;
    acl_conf_chkval_t        mMax;
    acl_conf_set_callback_t *mCallback;
    acp_sint32_t             mLineFound;
};


ACP_EXPORT acp_rc_t aclConfLoad(acp_str_t               *aPath,
                                acl_conf_def_t          *aDef,
                                acl_conf_set_callback_t *aDefaultCallback,
                                acl_conf_err_callback_t *aErrorCallback,
                                acp_sint32_t            *aCallbackResult,
                                void                    *aContext);


ACP_EXTERN_C_END


#endif

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

#ifndef _O_CMP_MARSHAL_CLIENT_H_
#define _O_CMP_MARSHAL_CLIENT_H_ 1


/*
 * Macro Functions for CMI
 */

#define CMP_MARSHAL_STATE_INITIALIZE(aMarshalState)                         \
    do                                                                      \
    {                                                                       \
        (aMarshalState).mFieldLeft = 0;                                     \
        (aMarshalState).mSizeLeft  = 0;                                     \
    } while (0)

#define CMP_MARSHAL_STATE_IS_COMPLETE(aMarshalState)                        \
    (((aMarshalState).mFieldLeft == 0) ? ACP_TRUE : ACP_FALSE)


/*
 * Macro Functions for Implement Marshal Functions
 */

#define CMP_MARSHAL_BEGIN(aFieldCount)                                      \
    acp_bool_t sSuccess;                                                    \
                                                                            \
    if (aMarshalState->mFieldLeft == 0)                                     \
    {                                                                       \
        aMarshalState->mFieldLeft = aFieldCount;                            \
    }                                                                       \
    if (aMarshalState->mFieldLeft > 0)                                      \
    {                                                                       \
        CMP_MARSHAL_GOTO ## aFieldCount;                                    \
    }                                                                       \
marshal_stage_ ## aFieldCount:

#define CMP_MARSHAL_STAGE(aFieldCount)                                      \
    ACE_DASSERT(aMarshalState->mFieldLeft == aFieldCount + 1);              \
    aMarshalState->mFieldLeft = aFieldCount;                                \
marshal_stage_ ## aFieldCount:


#define CMP_MARSHAL_READ_SCHAR(aValue)                                      \
    ACI_TEST(cmbBlockReadSChar(aBlock,                                      \
                               &(aValue),                                   \
                               &sSuccess) != ACI_SUCCESS);               \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_UCHAR(aValue)                                      \
    ACI_TEST(cmbBlockReadUChar(aBlock,                                      \
                               &(aValue),                                   \
                               &sSuccess) != ACI_SUCCESS);               \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_SSHORT(aValue)                                     \
    ACI_TEST(cmbBlockReadSShort(aBlock,                                     \
                                &(aValue),                                  \
                                &sSuccess) != ACI_SUCCESS);              \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_USHORT(aValue)                                     \
    ACI_TEST(cmbBlockReadUShort(aBlock,                                     \
                                &(aValue),                                  \
                                &sSuccess) != ACI_SUCCESS);              \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_SINT(aValue)                                       \
    ACI_TEST(cmbBlockReadSInt(aBlock,                                       \
                              &(aValue),                                    \
                              &sSuccess) != ACI_SUCCESS);                \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_UINT(aValue)                                       \
    ACI_TEST(cmbBlockReadUInt(aBlock,                                       \
                              &(aValue),                                    \
                              &sSuccess) != ACI_SUCCESS);                \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_SLONG(aValue)                                      \
    ACI_TEST(cmbBlockReadSLong(aBlock,                                      \
                               &(aValue),                                   \
                               &sSuccess) != ACI_SUCCESS);               \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_ULONG(aValue)                                      \
    ACI_TEST(cmbBlockReadULong(aBlock,                                      \
                               &(aValue),                                   \
                               &sSuccess) != ACI_SUCCESS);               \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_SFLOAT(aValue)                                     \
    ACI_TEST(cmbBlockReadSFloat(aBlock,                                     \
                                &(aValue),                                  \
                                &sSuccess) != ACI_SUCCESS);              \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_SDOUBLE(aValue)                                    \
    ACI_TEST(cmbBlockReadSDouble(aBlock,                                    \
                                 &(aValue),                                 \
                                 &sSuccess) != ACI_SUCCESS);             \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_DATETIME(aValue)                                   \
    ACI_TEST(cmbBlockReadDateTime(aBlock,                                   \
                                  &(aValue),                                \
                                  &sSuccess) != ACI_SUCCESS);            \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_NUMERIC(aValue)                                    \
    ACI_TEST(cmbBlockReadNumeric(aBlock,                                    \
                                 &(aValue),                                 \
                                 &sSuccess) != ACI_SUCCESS);             \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_VARIABLE(aValue)                                   \
    ACI_TEST(cmbBlockReadVariable(aBlock,                                   \
                                  &(aValue),                                \
                                  &sSuccess) != ACI_SUCCESS);            \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_COLLECTION(aValue)                                 \
    ACI_TEST(cmbBlockReadCollection(aBlock,                                 \
                                    &(aValue),                              \
                                    &sSuccess) != ACI_SUCCESS);          \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_ANY(aValue)                                        \
    ACI_TEST(cmbBlockReadAny(aBlock,                                        \
                             &(aValue),                                     \
                             &sSuccess) != ACI_SUCCESS);                 \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }


#define CMP_MARSHAL_WRITE_SCHAR(aValue)                                     \
    ACI_TEST(cmbBlockWriteSChar(aBlock,                                     \
                                aValue,                                     \
                                &sSuccess) != ACI_SUCCESS);              \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_UCHAR(aValue)                                     \
    ACI_TEST(cmbBlockWriteUChar(aBlock,                                     \
                                aValue,                                     \
                                &sSuccess) != ACI_SUCCESS);              \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_SSHORT(aValue)                                    \
    ACI_TEST(cmbBlockWriteSShort(aBlock,                                    \
                                 aValue,                                    \
                                 &sSuccess) != ACI_SUCCESS);             \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_USHORT(aValue)                                    \
    ACI_TEST(cmbBlockWriteUShort(aBlock,                                    \
                                 aValue,                                    \
                                 &sSuccess) != ACI_SUCCESS);             \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_SINT(aValue)                                      \
    ACI_TEST(cmbBlockWriteSInt(aBlock,                                      \
                               aValue,                                      \
                               &sSuccess) != ACI_SUCCESS);               \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_UINT(aValue)                                      \
    ACI_TEST(cmbBlockWriteUInt(aBlock,                                      \
                               aValue,                                      \
                               &sSuccess) != ACI_SUCCESS);               \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_SLONG(aValue)                                     \
    ACI_TEST(cmbBlockWriteSLong(aBlock,                                     \
                                aValue,                                     \
                                &sSuccess) != ACI_SUCCESS);              \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_ULONG(aValue)                                     \
    ACI_TEST(cmbBlockWriteULong(aBlock,                                     \
                                aValue,                                     \
                                &sSuccess) != ACI_SUCCESS);              \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_SFLOAT(aValue)                                    \
    ACI_TEST(cmbBlockWriteSFloat(aBlock,                                    \
                                 aValue,                                    \
                                 &sSuccess) != ACI_SUCCESS);             \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_SDOUBLE(aValue)                                   \
    ACI_TEST(cmbBlockWriteSDouble(aBlock,                                   \
                                  aValue,                                   \
                                  &sSuccess) != ACI_SUCCESS);               \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_DATETIME(aValue)                                  \
    ACI_TEST(cmbBlockWriteDateTime(aBlock,                                  \
                                   aValue,                                  \
                                   &sSuccess) != ACI_SUCCESS);           \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_NUMERIC(aValue)                                   \
    ACI_TEST(cmbBlockWriteNumeric(aBlock,                                   \
                                  aValue,                                   \
                                  &sSuccess) != ACI_SUCCESS);            \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_VARIABLE(aValue)                                  \
    ACI_TEST(cmbBlockWriteVariable(aBlock,                                  \
                                   &(aValue),                               \
                                   &aMarshalState->mSizeLeft,               \
                                   &sSuccess) != ACI_SUCCESS);           \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_COLLECTION(aValue)                                \
    ACI_TEST(cmbBlockWriteCollection(aBlock,                                \
                                     &(aValue),                             \
                                     &aMarshalState->mSizeLeft,             \
                                     &sSuccess) != ACI_SUCCESS);         \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_ANY(aValue)                                       \
    ACI_TEST(cmbBlockWriteAny(aBlock,                                       \
                              &(aValue),                                    \
                              &aMarshalState->mSizeLeft,                    \
                              &sSuccess) != ACI_SUCCESS);                \
    if (sSuccess != ACP_TRUE)                                               \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }


/*
 * Internal Macro Functions (Never Use These Macros in Marshal Functions)
 */

#define CMP_MARSHAL_GOTO0

#define CMP_MARSHAL_GOTO1                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO2                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO3                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO4                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO5                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO6                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO7                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO8                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO9                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO10                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        default: ACE_ASSERT(0);                                             \
    }

/*
 * PROJ-1502 Partitioned Disk Table
 */
#define CMP_MARSHAL_GOTO11                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO12                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO13                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO14                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO15                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO16                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        case 16: goto marshal_stage_16;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO17                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        case 16: goto marshal_stage_16;                                     \
        case 17: goto marshal_stage_17;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO18                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        case 16: goto marshal_stage_16;                                     \
        case 17: goto marshal_stage_17;                                     \
        case 18: goto marshal_stage_18;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO19                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        case 16: goto marshal_stage_16;                                     \
        case 17: goto marshal_stage_17;                                     \
        case 18: goto marshal_stage_18;                                     \
        case 19: goto marshal_stage_19;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO20                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        case 16: goto marshal_stage_16;                                     \
        case 17: goto marshal_stage_17;                                     \
        case 18: goto marshal_stage_18;                                     \
        case 19: goto marshal_stage_19;                                     \
        case 20: goto marshal_stage_20;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO21                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        case 16: goto marshal_stage_16;                                     \
        case 17: goto marshal_stage_17;                                     \
        case 18: goto marshal_stage_18;                                     \
        case 19: goto marshal_stage_19;                                     \
        case 20: goto marshal_stage_20;                                     \
        case 21: goto marshal_stage_21;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO22                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        case 16: goto marshal_stage_16;                                     \
        case 17: goto marshal_stage_17;                                     \
        case 18: goto marshal_stage_18;                                     \
        case 19: goto marshal_stage_19;                                     \
        case 20: goto marshal_stage_20;                                     \
        case 21: goto marshal_stage_21;                                     \
        case 22: goto marshal_stage_22;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO23                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        case 16: goto marshal_stage_16;                                     \
        case 17: goto marshal_stage_17;                                     \
        case 18: goto marshal_stage_18;                                     \
        case 19: goto marshal_stage_19;                                     \
        case 20: goto marshal_stage_20;                                     \
        case 21: goto marshal_stage_21;                                     \
        case 22: goto marshal_stage_22;                                     \
        case 23: goto marshal_stage_23;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO24                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        case 16: goto marshal_stage_16;                                     \
        case 17: goto marshal_stage_17;                                     \
        case 18: goto marshal_stage_18;                                     \
        case 19: goto marshal_stage_19;                                     \
        case 20: goto marshal_stage_20;                                     \
        case 21: goto marshal_stage_21;                                     \
        case 22: goto marshal_stage_22;                                     \
        case 23: goto marshal_stage_23;                                     \
        case 24: goto marshal_stage_24;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO25                                                  \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        case 6:  goto marshal_stage_6;                                      \
        case 7:  goto marshal_stage_7;                                      \
        case 8:  goto marshal_stage_8;                                      \
        case 9:  goto marshal_stage_9;                                      \
        case 10: goto marshal_stage_10;                                     \
        case 11: goto marshal_stage_11;                                     \
        case 12: goto marshal_stage_12;                                     \
        case 13: goto marshal_stage_13;                                     \
        case 14: goto marshal_stage_14;                                     \
        case 15: goto marshal_stage_15;                                     \
        case 16: goto marshal_stage_16;                                     \
        case 17: goto marshal_stage_17;                                     \
        case 18: goto marshal_stage_18;                                     \
        case 19: goto marshal_stage_19;                                     \
        case 20: goto marshal_stage_20;                                     \
        case 21: goto marshal_stage_21;                                     \
        case 22: goto marshal_stage_22;                                     \
        case 23: goto marshal_stage_23;                                     \
        case 24: goto marshal_stage_24;                                     \
        case 25: goto marshal_stage_25;                                     \
        default: ACE_ASSERT(0);                                             \
    }

#endif

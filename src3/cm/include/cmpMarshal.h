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

#ifndef _O_CMP_MARSHAL_H_
#define _O_CMP_MARSHAL_H_ 1


/*
 * Macro Functions for CMI
 */

#define CMP_MARSHAL_STATE_INITIALIZE(aMarshalState)                         \
    do                                                                      \
    {                                                                       \
        (aMarshalState).mFieldLeft = 0;                                     \
        (aMarshalState).mSizeLeft  = 0;                                     \
    } while (0)

#define CMP_MARSHAL_STATE_IS_COMPLETE(aMarshalState)            \
    (((aMarshalState).mFieldLeft == 0) ? ID_TRUE : ID_FALSE)


/*
 * Macro Functions for Implement Marshal Functions
 */

#define CMP_MARSHAL_BEGIN(aFieldCount)                                      \
    idBool sSuccess;                                                        \
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
    IDE_DASSERT(aMarshalState->mFieldLeft == aFieldCount + 1);              \
    aMarshalState->mFieldLeft = aFieldCount;                                \
marshal_stage_ ## aFieldCount:


#define CMP_MARSHAL_READ_SCHAR(aValue)                                      \
    IDE_TEST(cmbBlockReadSChar(aBlock,                                      \
                               &(aValue),                                   \
                               &sSuccess) != IDE_SUCCESS);                  \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_UCHAR(aValue)                                      \
    IDE_TEST(cmbBlockReadUChar(aBlock,                                      \
                               &(aValue),                                   \
                               &sSuccess) != IDE_SUCCESS);                  \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_SSHORT(aValue)                                     \
    IDE_TEST(cmbBlockReadSShort(aBlock,                                     \
                                &(aValue),                                  \
                                &sSuccess) != IDE_SUCCESS);                 \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_USHORT(aValue)                                     \
    IDE_TEST(cmbBlockReadUShort(aBlock,                                     \
                                &(aValue),                                  \
                                &sSuccess) != IDE_SUCCESS);                 \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_SINT(aValue)                                       \
    IDE_TEST(cmbBlockReadSInt(aBlock,                                       \
                              &(aValue),                                    \
                              &sSuccess) != IDE_SUCCESS);                   \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_UINT(aValue)                                       \
    IDE_TEST(cmbBlockReadUInt(aBlock,                                       \
                              &(aValue),                                    \
                              &sSuccess) != IDE_SUCCESS);                   \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_SLONG(aValue)                                      \
    IDE_TEST(cmbBlockReadSLong(aBlock,                                      \
                               &(aValue),                                   \
                               &sSuccess) != IDE_SUCCESS);                  \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_ULONG(aValue)                                      \
    IDE_TEST(cmbBlockReadULong(aBlock,                                      \
                               &(aValue),                                   \
                               &sSuccess) != IDE_SUCCESS);                  \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_SFLOAT(aValue)                                     \
    IDE_TEST(cmbBlockReadSFloat(aBlock,                                     \
                                &(aValue),                                  \
                                &sSuccess) != IDE_SUCCESS);                 \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_SDOUBLE(aValue)                                    \
    IDE_TEST(cmbBlockReadSDouble(aBlock,                                    \
                                 &(aValue),                                 \
                                 &sSuccess) != IDE_SUCCESS);                \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_DATETIME(aValue)                                   \
    IDE_TEST(cmbBlockReadDateTime(aBlock,                                   \
                                  &(aValue),                                \
                                  &sSuccess) != IDE_SUCCESS);               \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_NUMERIC(aValue)                                    \
    IDE_TEST(cmbBlockReadNumeric(aBlock,                                    \
                                 &(aValue),                                 \
                                 &sSuccess) != IDE_SUCCESS);                \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_VARIABLE(aValue)                                   \
    IDE_TEST(cmbBlockReadVariable(aBlock,                                   \
                                  &(aValue),                                \
                                  &sSuccess) != IDE_SUCCESS);               \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_COLLECTION(aValue)                                 \
    IDE_TEST(cmbBlockReadCollection(aBlock,                                 \
                                    &(aValue),                              \
                                    &sSuccess) != IDE_SUCCESS);             \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_READ_ANY(aValue)                                        \
    IDE_TEST(cmbBlockReadAny(aBlock,                                        \
                             &(aValue),                                     \
                             &sSuccess) != IDE_SUCCESS);                    \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }


#define CMP_MARSHAL_WRITE_SCHAR(aValue)                                     \
    IDE_TEST(cmbBlockWriteSChar(aBlock,                                     \
                                aValue,                                     \
                                &sSuccess) != IDE_SUCCESS);                 \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_UCHAR(aValue)                                     \
    IDE_TEST(cmbBlockWriteUChar(aBlock,                                     \
                                aValue,                                     \
                                &sSuccess) != IDE_SUCCESS);                 \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_SSHORT(aValue)                                    \
    IDE_TEST(cmbBlockWriteSShort(aBlock,                                    \
                                 aValue,                                    \
                                 &sSuccess) != IDE_SUCCESS);                \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_USHORT(aValue)                                    \
    IDE_TEST(cmbBlockWriteUShort(aBlock,                                    \
                                 aValue,                                    \
                                 &sSuccess) != IDE_SUCCESS);                \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_SINT(aValue)                                      \
    IDE_TEST(cmbBlockWriteSInt(aBlock,                                      \
                               aValue,                                      \
                               &sSuccess) != IDE_SUCCESS);                  \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_UINT(aValue)                                      \
    IDE_TEST(cmbBlockWriteUInt(aBlock,                                      \
                               aValue,                                      \
                               &sSuccess) != IDE_SUCCESS);                  \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_SLONG(aValue)                                     \
    IDE_TEST(cmbBlockWriteSLong(aBlock,                                     \
                                aValue,                                     \
                                &sSuccess) != IDE_SUCCESS);                 \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_ULONG(aValue)                                     \
    IDE_TEST(cmbBlockWriteULong(aBlock,                                     \
                                aValue,                                     \
                                &sSuccess) != IDE_SUCCESS);                 \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_SFLOAT(aValue)                                    \
    IDE_TEST(cmbBlockWriteSFloat(aBlock,                                    \
                                 aValue,                                    \
                                 &sSuccess) != IDE_SUCCESS);                \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_SDOUBLE(aValue)                                   \
    IDE_TEST(cmbBlockWriteSDouble(aBlock,                                   \
                                  aValue,                                   \
                                  &sSuccess) != IDE_SUCCESS);               \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_DATETIME(aValue)                                  \
    IDE_TEST(cmbBlockWriteDateTime(aBlock,                                  \
                                   aValue,                                  \
                                   &sSuccess) != IDE_SUCCESS);              \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_NUMERIC(aValue)                                   \
    IDE_TEST(cmbBlockWriteNumeric(aBlock,                                   \
                                  aValue,                                   \
                                  &sSuccess) != IDE_SUCCESS);               \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_VARIABLE(aValue)                                  \
    IDE_TEST(cmbBlockWriteVariable(aBlock,                                  \
                                   &(aValue),                               \
                                   &aMarshalState->mSizeLeft,               \
                                   &sSuccess) != IDE_SUCCESS);              \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_COLLECTION(aValue)                                \
    IDE_TEST(cmbBlockWriteCollection(aBlock,                                \
                                     &(aValue),                             \
                                     &aMarshalState->mSizeLeft,             \
                                     &sSuccess) != IDE_SUCCESS);            \
    if (sSuccess != ID_TRUE)                                                \
    {                                                                       \
        goto marshal_stage_0;                                               \
    }

#define CMP_MARSHAL_WRITE_ANY(aValue)                                       \
    IDE_TEST(cmbBlockWriteAny(aBlock,                                       \
                              &(aValue),                                    \
                              &aMarshalState->mSizeLeft,                    \
                              &sSuccess) != IDE_SUCCESS);                   \
    if (sSuccess != ID_TRUE)                                                \
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
        default: IDE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO2                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        default: IDE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO3                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        default: IDE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO4                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        default: IDE_ASSERT(0);                                             \
    }

#define CMP_MARSHAL_GOTO5                                                   \
    switch (aMarshalState->mFieldLeft)                                      \
    {                                                                       \
        case 1:  goto marshal_stage_1;                                      \
        case 2:  goto marshal_stage_2;                                      \
        case 3:  goto marshal_stage_3;                                      \
        case 4:  goto marshal_stage_4;                                      \
        case 5:  goto marshal_stage_5;                                      \
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
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
        default: IDE_ASSERT(0);                                             \
    }

#endif

/*******************************************************************************
 * Copyright 1999-2007, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 ******************************************************************************/

/*******************************************************************************
 * $ID$
 ******************************************************************************/

#include <act.h>
#include <qc.h>
#include <qcmFixedTable.h>

void testvalidateTableName()
{
    qcStatement    sStatement;
    qcTemplate     sTemplate1;
    qcNamePosition sTableName;

    sStatement.sharedPlan.sTmplate = &sTemplate1;
    sStatement.myPlan = &(sStatement.sharedPlan);

    sTableName.stmtText = "X$Table";
    sTableName.offset   = 0;
    sTableName.size     = 7;

    sStatement.myPlan->sTmplate->fixedTableAutomataStatus = 0;
    qcmFixedTable::validateTableName(&sStatement, sTableName);
    ACT_CHECK( sStatement.myPlan->sTmplate->fixedTableAutomataStatus == 1);

    sStatement.myPlan->sTmplate->fixedTableAutomataStatus = 1;
    qcmFixedTable::validateTableName(&sStatement, sTableName);
    ACT_CHECK( sStatement.myPlan->sTmplate->fixedTableAutomataStatus == 1);

    sStatement.myPlan->sTmplate->fixedTableAutomataStatus = 2;
    qcmFixedTable::validateTableName(&sStatement, sTableName);
    ACT_CHECK( sStatement.myPlan->sTmplate->fixedTableAutomataStatus == 3);


    sTableName.stmtText = "x$Table";
    sTableName.offset   = 0;
    sTableName.size     = 7;

    sStatement.myPlan->sTmplate->fixedTableAutomataStatus = 0;
    qcmFixedTable::validateTableName(&sStatement, sTableName);
    ACT_CHECK( sStatement.myPlan->sTmplate->fixedTableAutomataStatus == 2);

    sStatement.myPlan->sTmplate->fixedTableAutomataStatus = 1;
    qcmFixedTable::validateTableName(&sStatement, sTableName);
    ACT_CHECK( sStatement.myPlan->sTmplate->fixedTableAutomataStatus == 3);

    sStatement.myPlan->sTmplate->fixedTableAutomataStatus = 2;
    qcmFixedTable::validateTableName(&sStatement, sTableName);
    ACT_CHECK( sStatement.myPlan->sTmplate->fixedTableAutomataStatus == 2);
}

void testmakeTrimmedName()
{
    UInt   i;
    SChar  sDst[12]    = {0,0,0,0,0,0,0,0,0,0,0,0};
    SChar  sResult[12] = {'T','a','b','l','e',0,0,0,0,0,0,0};

    qcmFixedTable::makeTrimmedName(sDst, "Table    ");

    for(i=0; i < 10; ++i)
    {
        ACT_CHECK( sDst[i] == sResult[i]);
    }

    qcmFixedTable::makeTrimmedName(sDst, "Table Name");

    for(i=0; i < 10; ++i)
    {
        ACT_CHECK( sDst[i] == sResult[i]);
    }
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    testvalidateTableName();
    testmakeTrimmedName();

    ACT_TEST_END();

    return 0;
}


/*******************************************************************************
 * Copyright 1999-2011, ALTIBASE Corporation or its subsidiaries.
 * All rights reserved.
 ******************************************************************************/

/*******************************************************************************
 * $id$
 ******************************************************************************/

#include <act.h>
#include <qc.h>
#include <qtc.h>

void testdependencySet()
{
    qcDepInfo sOperand;

    qtc::dependencySet( 1, &sOperand);

    ACT_CHECK( sOperand.depCount == 1);
    ACT_CHECK( sOperand.depend[0] == 1);
}

void testdependencySetWithDep()
{
    qcDepInfo sOperand1;
    qcDepInfo sOperand2;

    qtc::dependencySet( 1, &sOperand1);
    qtc::dependencySet( 0, &sOperand2);

    qtc::dependencySetWithDep(&sOperand1, &sOperand2);

    ACT_CHECK( sOperand1.depCount == sOperand2.depCount);
    ACT_CHECK( sOperand1.depend[0] == sOperand2.depend[0]);

}

void testdependencyAnd()
{
    qcDepInfo sOperand1;
    qcDepInfo sOperand2;
    qcDepInfo sResult;

    qtc::dependencySet( 1, &sOperand1);
    qtc::dependencySet( 0, &sOperand2);
    qtc::dependencyAnd(&sOperand1, &sOperand2, &sResult);

    ACT_CHECK( sResult.depCount == 0);

    qtc::dependencySet( 1, &sOperand1);
    qtc::dependencySet( 1, &sOperand2);
    qtc::dependencyAnd(&sOperand1, &sOperand2, &sResult);

    ACT_CHECK( sResult.depCount == 1);
    ACT_CHECK( sResult.depend[0] == 1);

    sOperand1.depCount   = 2;
    sOperand1.depend[0]  = 0;
    sOperand1.depend[1]  = 1;

    sOperand2.depCount   = 2;
    sOperand2.depend[0]  = 1;
    sOperand2.depend[1]  = 2;

    qtc::dependencyAnd(&sOperand1, &sOperand2, &sResult);

    ACT_CHECK( sResult.depCount == 1);
    ACT_CHECK( sResult.depend[0] == 1);
}

void testdependencyOr()
{
    UInt      i;
    qcDepInfo sOperand1;
    qcDepInfo sOperand2;
    qcDepInfo sResult;


    qtc::dependencySet( 1, &sOperand1);
    qtc::dependencySet( 0, &sOperand2);
    ACT_CHECK(qtc::dependencyOr(&sOperand1, &sOperand2, &sResult) == IDE_SUCCESS);
    ACT_CHECK( sResult.depCount == 2);
    ACT_CHECK( sResult.depend[0] == 0);
    ACT_CHECK( sResult.depend[1] == 1);


    qtc::dependencySet( 1, &sOperand1);
    qtc::dependencySet( 1, &sOperand2);
    ACT_CHECK(qtc::dependencyOr(&sOperand1, &sOperand2, &sResult) == IDE_SUCCESS);
    ACT_CHECK( sResult.depCount == 1);
    ACT_CHECK( sResult.depend[0] == 1);


    sOperand1.depCount = QC_MAX_REF_TABLE_CNT;
    for(i = 0; i < QC_MAX_REF_TABLE_CNT; ++i)
    {
        sOperand1.depend[i] = i;
    }
    qtc::dependencySet( 1, &sOperand2);
    ACT_CHECK(qtc::dependencyOr(&sOperand1, &sOperand2, &sResult) == IDE_SUCCESS);
    ACT_CHECK( sResult.depCount == QC_MAX_REF_TABLE_CNT);
    for(i = 0; i < QC_MAX_REF_TABLE_CNT; ++i)
    {
        ACT_CHECK( sResult.depend[i] == i);
    }


    qtc::dependencySet( 1, &sOperand1);
    sOperand2.depCount = QC_MAX_REF_TABLE_CNT;
    for(i = 0; i < QC_MAX_REF_TABLE_CNT; ++i)
    {
        sOperand2.depend[i] = i;
    }
    ACT_CHECK(qtc::dependencyOr(&sOperand1, &sOperand2, &sResult) == IDE_SUCCESS);
    ACT_CHECK( sResult.depCount == QC_MAX_REF_TABLE_CNT);
    for(i = 0; i < QC_MAX_REF_TABLE_CNT; ++i)
    {
        ACT_CHECK( sResult.depend[i] == i);
    }


    sOperand1.depCount = QC_MAX_REF_TABLE_CNT;
    for(i = 0; i < QC_MAX_REF_TABLE_CNT; ++i)
    {
        sOperand1.depend[i] = i+2;
    }
    sOperand2.depCount = QC_MAX_REF_TABLE_CNT;
    for(i = 0; i < QC_MAX_REF_TABLE_CNT; ++i)
    {
        sOperand2.depend[i] = i;
    }
    ACT_CHECK(qtc::dependencyOr(&sOperand1, &sOperand2, &sResult) == IDE_FAILURE);
}

void testdependencyClear()
{
    qcDepInfo sOperand;

    qtc::dependencySet(1, &sOperand);

    qtc::dependencyClear(&sOperand);

    ACT_CHECK( sOperand.depCount == 0);
}

acp_sint32_t main(acp_sint32_t aArgc, acp_char_t *aArgv[])
{
    ACP_UNUSED(aArgc);
    ACP_UNUSED(aArgv);

    ACT_TEST_BEGIN();

    testdependencySet();
    testdependencySetWithDep();
    testdependencyAnd();
    testdependencyOr();
    testdependencyClear();

    ACT_TEST_END();

    return 0;
}


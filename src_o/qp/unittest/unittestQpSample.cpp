#include <idl.h>
#include <act.h>

/*
 * Function to demonstrate UnitTest framework
 */
SInt unittestQpGetOne()
{
    return 1;
}

/*
 * ACT_CHECK and ACT_CHECK_DESC demonstration function
 */
void unittestQpTestCheck()
{
    SInt sRet;

    ACT_CHECK(unittestQpGetOne() == 1);

    sRet = unittestQpGetOne();
    ACT_CHECK_DESC(sRet == 1, ("unittestQpGetOne() should return 1 but %d", sRet));
}

/*
 * ACT_ERROR demonstration function
 */
void unittestQpTestError()
{
    if (unittestQpGetOne() == 1)
    {
        ACT_CHECK(unittestQpGetOne() == 1);
    }
    else
    {
        ACT_ERROR(("Error: unittestQpGetOne() should return 1 but %d", unittestQpGetOne()));
    }
}

/*
 * ACT_ASSERT and ACT_ASSERT_DESC demonstration function
 */
void unittestQpTestAssert()
{
    SInt sRet;

    ACT_ASSERT(unittestQpGetOne() == 1);

    sRet = unittestQpGetOne();
    ACT_ASSERT_DESC(sRet == 1, ("unittestQpGetOne() should return 1 but %d", sRet));
}

/*
 * ACT_DUMP and ACT_DUMP_DESC demonstration function
 */
void unittestQpTestDump()
{
    UChar sBuf[] = {'A', 'B', 'C', 'D', 'E',
                    'A', 'B', 'C', 'D', 'E',
                    'A', 'B', 'C', 'D', 'E',
                    'A', 'B', 'C', 'D', 'E',
                    'A', 'B', 'C', 'D', 'E',
                    'A', 'B', 'C', 'D', 'E',
                   };

    if (unittestQpGetOne() != 1)
    {
        ACT_DUMP(&sBuf, sizeof(sBuf));

        ACT_DUMP_DESC(&sBuf, sizeof(sBuf), ("Dump description."));
    }
}

int main()
{
    ACT_TEST_BEGIN();

    /* Run function to check ACT_CHECK ACT_CHECK_DESC use */
    unittestQpTestCheck();

    /* Run function to check ACT_ERROR use */
    unittestQpTestError();

    /* Run function to check ACT_ASSERT ACT_ASSERT_DESC use */
    unittestQpTestAssert();

    /* Run function to check ACT_DUMP ACT_DUMP_DESC use */
    unittestQpTestDump();

    ACT_TEST_END();

    return 0;
}

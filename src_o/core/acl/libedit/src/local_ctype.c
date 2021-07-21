#include <ctype.h>

int local_isalnum(int c)
{
    if (c > 0x7f)
        return 1;
    else
        return isalnum(c);
}

int local_isalpha(int c)
{
    if (c > 0x7f)
        return 1;
    else
        return isalpha(c);
}

int local_isgraph(int c)
{
    if (c > 0x7f)
        return 1;
    else
        return isgraph(c);
}

int local_isprint(int c)
{
    if (c > 0x7f)
        return 1;
    else
        return isprint(c);
}

#undef isalnum
#undef isalpha
#undef isgraph
#undef isprint

#define isalnum(c) (local_isalnum(c))
#define isalpha(c) (local_isalpha(c))
#define isgraph(c) (local_isgraph(c))
#define isprint(c) (local_isprint(c))

int local_isalnum(int c);
int local_isalpha(int c);
int local_isgraph(int c);
int local_isprint(int c);

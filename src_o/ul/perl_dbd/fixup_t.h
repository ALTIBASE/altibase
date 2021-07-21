
/* fix up types for driver managers that don't have them	*/
/* (basically iODBC)										*/

#ifndef SQLCHAR
#define SQLCHAR char
#endif

#ifndef SQLSMALLINT
#define SQLSMALLINT I16
#endif

#ifndef SQLUINTEGER
#define SQLUINTEGER U32
#endif

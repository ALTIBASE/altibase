
/* fix up constants and other fundamentals that some driver managers	*/
/* don't define (basically iODBC)										*/

#ifndef SQL_API_ALL_FUNCTIONS
#define SQL_API_ALL_FUNCTIONS	0
#endif

#ifndef SQL_SQLSTATE_SIZE
#define SQL_SQLSTATE_SIZE	5
#endif

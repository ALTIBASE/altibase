/***********************************************************************
 * ACS Primitive Types
 ***********************************************************************/

#define ACS_NULL_ENV           (0)

typedef void *    ACSHENV;
    
typedef enum tagACSRETURN
{
    ACS_SUCCESS              =  0,
    ACS_ERROR                = (-1),
    ACS_INVALID_HANDLE       = (-2)
} ACSRETURN;


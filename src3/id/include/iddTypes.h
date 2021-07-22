/***********************************************************************
 * Copyright 2000, AltiBase Corporation or its subsidiaries.
 * All rights reserved.
 **********************************************************************/

/***********************************************************************
 * $Id: iddTypes.h 26440 2008-06-10 04:02:48Z jdlee $
 **********************************************************************/

#ifndef _O_IDDTYPES_H_
# define _O_IDDTYPES_H_

enum iddOpType
{
  IDDO_XX =  0,  /* error */
  IDDO_GT =  1,  /* greater than */
  IDDO_GE =  2,  /* greater or equal */
  IDDO_LT =  3,  /* less than */
  IDDO_LE =  4,  /* less or equal */
  IDDO_EQ =  5,  /* equal */
  IDDO_NE =  6,  /* not equal */
  IDDO_IN =  7,  /* in */
  IDDO_NN =  8,  /* not in */
  IDDO_LK =  9,  /* like */
  IDDO_NL = 10,  /* not like */
  IDDO_MX = 11   /* maximum limit */
};

enum iddDataType
{
  IDDT_ERR =  0,  /* error */
  IDDT_INT =  1,  /* integer */
  IDDT_STR =  2,  /* string */
  IDDT_BYT =  3,  /* bytes */
  IDDT_CHA =  4,  /* char */
  IDDT_VAR =  5,  /* varchar */
  IDDT_BIT =  6,  /* bit */
  IDDT_DAT =  7,  /* date */
  IDDT_TSP =  8,  /* timestamp */
  IDDT_NUM =  9,  /* numeric */
  IDDT_TNU = 10,  /* tnumeric */
  IDDT_NIB = 11,  /* nibble */
  IDDT_LIM = 11,  /* limit of real datatype */
  IDDT_REF = 12,  /* ref */
  IDDT_BOO = 13,  /* bool */
  IDDT_LIS = 14,  /* list */
  IDDT_NUL = 15,  /* null */
  IDDT_MAX = 16   /* max */
};

#endif /* _O_IDDTYPES_H_ */

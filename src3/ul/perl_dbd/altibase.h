/*
 * Copyright (c) 1994,1995,1996,1997  Tim Bunce
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Artistic License, as specified in the Perl README file.
 */

#include "dbdodbc.h"	/* Get SQL_* defs *before* loading DBIXS.h	*/

#define NEED_DBIXS_VERSION 9

#include <DBIXS.h>	/* from DBI. Load this after dbdodbc.h */

#include "dbivport.h" /* copied from DBI to maintain compatibility */

#include "dbdimp.h"

#include <dbd_xsh.h>	/* from DBI. Load this after dbdodbc.h */

SV      *altibase_get_info _((SV *dbh, int ftype));
int      altibase_get_type_info _((SV *dbh, SV *sth, int ftype));
SV	*altibase_col_attributes _((SV *sth, int colno, int desctype));
SV	*altibase_cancel _((SV *sth));
int	 altibase_describe_col _((SV *sth, int colno,
	    char *ColumnName, I16 BufferLength, I16 *NameLength,
	    I16 *DataType, U32 *ColumnSize,
	    I16 *DecimalDigits, I16 *Nullable));
int	 altibase_db_columns _((SV *dbh, SV *sth,
	    char *catalog, char *schema, char *table, char *column));

int  altibase_st_tables _((SV *dbh, SV *sth, char *catalog, char *schema, char *table, char *table_type));
int  altibase_st_primary_keys _((SV *dbh, SV *sth, char *catalog, char *schema, char *table));
int  altibase_get_statistics _((SV *dbh, SV *sth, char *CatalogName, char *SchemaName, char *TableName, int Unique));
int  altibase_get_special_columns _((SV *dbh, SV *sth, int Identifier, char *CatalogName, char *SchemaName, char *TableName, int Scope, int Nullable));
int  altibase_get_foreign_keys _((SV *dbh, SV *sth, char *PK_CatalogName, char *PK_SchemaName, char *PK_TableName, char *FK_CatalogName, char *FK_SchemaName, char *FK_TableName));
void dbd_error _((SV *h, RETCODE err_rc, char *what));
void dbd_error2 _((SV *h, RETCODE err_rc, char *what, HENV henv, HDBC hdbc, HSTMT hstmt));
int dbd_db_execdirect _(( SV *dbh, char *statement ));


/* end of altibase.h */

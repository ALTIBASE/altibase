/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 

/***********************************************************************
 * $Id: qcpll.l 84793 2019-01-24 04:12:50Z donovan.seo $
 **********************************************************************/

%option c++
%option 8bit
%option caseless
%option never-interactive
%option noreject
%option noyymore
%option noyywrap
%option nostdinit
%option prefix="qcpl"
%option yyclass="qcplLexer"

%{
#include <idl.h>
#include <idn.h>
#include <mtuProperty.h>
#include <qcuError.h>
#include <qcphManager.h>
#include <qdParseTree.h>
#include <qmmParseTree.h>
#include <qmsParseTree.h>
#include <qriParseTree.h>
#include <qsParseTree.h>
#include <qspDef.h>
#include <qdnTrigger.h>
#include <qlParseTree.h> // PROJ-1685
#include <qdkParseTree.h>
#include <qdk.h>
#include <qdcAudit.h>
#include <qdcJob.h>
#include "qcplx.h"

#if defined(BISON_POSTFIX_HPP)
#include "qcply.hpp"
#else  /* BISON_POSTFIX_CPP_H */
#include "qcply.cpp.h"
#endif

#include "qcpll.h"

// To Fix PR-11793 To Remove Warning
// Linux의 경우 /usr/include/bits/termios.h 에 ECHO가 정의되어 있음.'
// Flex가 ECHO를 define하여 사용하므로, undef해야 함.
// 참조) Flex가 생성하는 qcpll.cpp 에는 다음과 같은 Code가 생성됨.
//       #ifndef ECHO
//       #define ECHO LexerOutput( yytext, yyleng )
//       #endif
#undef ECHO

# define QCPL_ERROR_SQL_MAX_SIZE (256)

int qcpllex(YYSTYPE* lvalp, void* param );

idnCharFeature gCharType = IDN_CF_UNKNOWN;
int            gPrevState = 0;

/* TR_ Reserved Word.    */
/* TK_ Keyword.          */
/* TO_ Oracle Keyword.   */
/* TA_ Altibase Keyword. */
/* TI_ Identifier.       */
/* TL_ Literal.          */
/* TS_ Symbol.           */


/* NEG_TRAIL : 2byte character whose leading charater is always negative and
               trailing character is also negative.
               ex) KSC5601, UTF-8, EUC-JP,...    */

/* POS_TRAIL : 2byte character whose leading charater is always negative and
               trailing character can be positive
               ex) BIG5, GB231280, MS949, MS936  */

/* SJIS      : some of negative leading character is 1 byte. and
               trailing character of 2byte character can be positive
               ex) SHIFT-JIS, MS932              */
               

%}


NEG_TRAIL_CH     [\x80-\xFF]

POS_TRAIL_CH     [\x81-\xFE][\x40-\x7E\x80-\xFE]

SJIS_HWKATA      [\xA0-\xDF]
SJIS_JIS97       [\x81-\x9F\xE0-\xFC][\x40-\x7E\x80-\xFC]
SJIS_ALL         ({SJIS_HWKATA})|({SJIS_JIS97})

%s NEG_TRAIL
%s POS_TRAIL
%s SJIS

%x comment
%x hint

/* PROJ-2550
   qcpll.l에 특수기호 token이 추가되면,
   ut/altiWrap/src/altiWrapll.l에 token 추가하고,
   ut/altiWrap/src/altiWraply.y의 token_list에 추가한다. */

%%

ADD               { return TR_ADD;               }
AFTER             { return TR_AFTER;             }
AGER              { return TR_AGER;              }
ALL               { return TR_ALL;               }
ALTER             { return TR_ALTER;             }
ALTER[ \t\r\n]+USER             { return TR_ALTER_USER; }   // BUG-36769
AND               { return TR_AND;               }
ANY               { return TR_ANY;               }
ARCHIVE           { return TR_ARCHIVE;           }
ARCHIVELOG        { return TR_ARCHIVELOG;        }
AS                { return TR_AS;                }
ASC               { return TR_ASC;               }
AT                { return TR_AT;                } // PROJ-1502 PARTITIONED DISK TABLE
AUTHID            { return TR_AUTHID;            }      /* BUG-45306 PSM AUTHID */
BACKUP            { return TR_BACKUP;            }
BEFORE            { return TR_BEFORE;            }
BEGIN             { return TR_BEGIN;             }
BODY              { return TR_BODY;              } // PROJ-1073 Package
BY                { return TR_BY;                }
CASCADE           { return TR_CASCADE;           }
CASE              { return TR_CASE;              }
CAST              { return TR_CAST;              }
CHECK[ \t\n\r]*"(" { return TR_CHECK_OPENING_PARENTHESIS; } /* PROJ-1107 Check Constraint 지원 */
CLOSE             { return TR_CLOSE;             }
COALESCE          { return TR_COALESCE;          } // PROJ-1502 PARTITIONED DISK TABLE
COLUMN            { return TR_COLUMN;            }
COMPILE           { return TR_COMPILE;           }
COMMIT            { return TR_COMMIT;            }
CONNECT           { return TR_CONNECT;           }
CONSTRAINT        { return TR_CONSTRAINT;        }
CONSTRAINTS       { return TR_CONSTRAINTS;       }
CONTINUE          { return TR_CONTINUE;          }
CREATE            { return TR_CREATE;            }
CREATE[ \t\r\n]+USER            { return TR_CREATE_USER ; }  // BUG-36769
CURRENT_USER      { return TR_CURRENT_USER; }      /* BUG-45306 PSM AUTHID */
CURSOR            { return TR_CURSOR;            }
CYCLE             { return TR_CYCLE;             }
DECLARE           { return TR_DECLARE;           }
DEFAULT           { return TR_DEFAULT;           }
DEFINER           { return TR_DEFINER;           }       /* BUG-45306 PSM AUTHID */
DELETE            { return TR_DELETE;            }
DEQUEUE           { return TR_DEQUEUE;           }
DESC              { return TR_DESC;              }
DETERMINISTIC     { return TR_DETERMINISTIC;     }
DISABLE           { return TR_DISABLE;           }
DISASTER          { return TR_DISASTER;          }      // BUG-37372
DIRECTORY         { return TR_DIRECTORY;         }
DISCONNECT        { return TR_DISCONNECT;        }
DISTINCT          { return TR_DISTINCT;          }
DROP              { return TR_DROP;              }
DROP[ \t\r\n]+USER              { return TR_DROP_USER; }     // BUG-36769
EACH              { return TR_EACH;              }
ELSE              { return TR_ELSE;              }
ELSEIF            { return TR_ELSEIF;            }
ENABLE            { return TR_ENABLE;            }
END               { return TR_END;               }
ENQUEUE           { return TR_ENQUEUE;           }
ESCAPE            { return TR_ESCAPE;            }
EXCEPTION         { return TR_EXCEPTION;         }
EXEC              { return TR_EXEC;              }
EXECUTE           { return TR_EXECUTE;           }
EXIT              { return TR_EXIT;              }
FALSE             { return TR_FALSE;             }
FETCH             { return TR_FETCH;             }
FIFO              { return TR_FIFO;              }
FLUSH             { return TR_FLUSH;             }
FLUSHER           { return TR_FLUSHER;           } // PROJ-1568 BUFFER MANAGER RENEWAL
FOR               { return TR_FOR;               }
FOREIGN           { return TR_FOREIGN;           }
FROM              { return TR_FROM;              }
FULL              { return TR_FULL;              }
FUNCTION          { return TR_FUNCTION;          }
GOTO              { return TR_GOTO;              }
GRANT             { return TR_GRANT;             }
GROUP             { return TR_GROUP;             }
HAVING            { return TR_HAVING;            }
IF                { return TR_IF;                }
IN                { return TR_IN;                }
INNER             { return TR_INNER;             }
INITRANS          { return TR_INITRANS;          } // PROJ-1704 MVCC Renewal
INSERT            { return TR_INSERT;            }
INSTEAD           { return TR_INSTEAD;           } // PROJ-1888 INSTEAD OF TRIGGER
INTERSECT         { return TR_INTERSECT;         }
INTO              { return TR_INTO;              }
IS                { return TR_IS;                }
ISOLATION         { return TR_ISOLATION;         }
JOIN              { return TR_JOIN;              }
KEY               { return TR_KEY;               }
LEFT              { return TR_LEFT;              }
LESS              { return TR_LESS;              } // PROJ-1502 PARTITIONED DISK TABLE
LEVEL             { return TR_LEVEL;             }
LIFO              { return TR_LIFO;              }
LIKE              { return TR_LIKE;              }
LIMIT             { return TR_LIMIT;             }
LOCAL             { return TR_LOCAL;             } // PROJ-1502 PARTITIONED DISK TABLE
LOCALUNIQUE       { return TR_LOCALUNIQUE;       } // fix BUG-18937
LOGANCHOR         { return TR_LOGANCHOR;         }
LOOP              { return TR_LOOP;              }
MAXTRANS          { return TR_MAXTRANS;          } // PROJ-1704 MVCC Renewal
MERGE             { return TR_MERGE;             } // PROJ-1502 PARTITIONED DISK TABLE
MOVE              { return TR_MOVE;              }
MOVEMENT          { return TR_MOVEMENT;          } // PROJ-1502 PARTITIONED DISK TABLE
NEW               { return TR_NEW;               }
NOARCHIVELOG      { return TR_NOARCHIVELOG;      }
NOCOPY            { return TR_NOCOPY;            }
NOCYCLE           { return TR_NOCYCLE;           }
NOT               { return TR_NOT;               }
NULL              { return TR_NULL;              }
OF                { return TR_OF;                }
OFF               { return TR_OFF;               }
OLD               { return TR_OLD;               }
ON                { return TR_ON;                }
OPEN              { return TR_OPEN;              }
OR                { return TR_OR;                }
ORDER             { return TR_ORDER;             }
OUT               { return TR_OUT;               }
OUTER             { return TR_OUTER;             }
PACKAGE           { return TR_PACKAGE;           } // PROJ-1073 Package
PARTITION         { return TR_PARTITION;         } // PROJ-1502 PARTITIONED DISK TABLE
PARTITIONS        { return TR_PARTITIONS;        } // PROJ-1502 PARTITIONED DISK TABLE
PRAGMA[ \t\r\n]+AUTONOMOUS_TRANSACTION { return TR_PRAGMA_AUTONOMOUS_TRANSACTION; } // BUG-38509 autonomous transaction
PRAGMA[ \t\r\n]+EXCEPTION_INIT         { return TR_PRAGMA_EXCEPTION_INIT; }         // BUG-41240 EXCEPTION_INIT Pragma
PRAGMA[ \t\r\n]+RESTRICT_REFERENCES    { return TR_PRAGMA_RESTRICT_REFERENCES; }
PRIMARY           { return TR_PRIMARY;           }
PRIOR             { return TR_PRIOR;             }
PRIVILEGES        { return TR_PRIVILEGES;        }
PROCEDURE         { return TR_PROCEDURE;         }
QUEUE             { return TR_QUEUE;             }
READ              { return TR_READ;              }
REBUILD           { return TR_REBUILD;           } // PROJ-1502 PARTITIONED DISK TABLE
RECOVER           { return TR_RECOVER;           }
REFERENCES        { return TR_REFERENCES;        }
REFERENCING       { return TR_REFERENCING;       }
REMOVE            { return TR_REMOVE;            } // BUG-37372
RETURN            { return TR_RETURN;            }
RETURNING         { return TR_RETURNING;         } // PRJ-1584 DML Return Clause
REVOKE            { return TR_REVOKE;            }
RIGHT             { return TR_RIGHT;             }
ROLLBACK          { return TR_ROLLBACK;          }
ROW               { return TR_ROW;               }
ROWNUM            { return TR_ROWNUM;            } // PROJ-1405 ROWNUM
SAVEPOINT         { return TR_SAVEPOINT;         }
SELECT            { return TR_SELECT;            }
SEQUENCE          { return TR_SEQUENCE;          }
SESSION           { return TR_SESSION;           }
SET               { return TR_SET;               }
SOME              { return TR_SOME;              }
SPECIFICATION     { return TR_SPECIFICATION;     } // PROJ-1073 Package
START             { return TR_START;             }
STATEMENT         { return TR_STATEMENT;         }
TABLE             { return TR_TABLE;             }
TEMPORARY         { return TR_TEMPORARY;         }
THAN              { return TR_THAN;              } // PROJ-1502 PARTITIONED DISK TABLE
THEN              { return TR_THEN;              }
TO                { return TR_TO;                }
TRUE              { return TR_TRUE;              }
TRIGGER           { return TR_TRIGGER;           }
TYPE              { return TR_TYPE;              }  // PROJ-1075 UDT
TYPESET           { return TR_TYPESET;           }  // PROJ-1075 TYPESET
UNION             { return TR_UNION;             }
UNIQUE            { return TR_UNIQUE;            }
UNTIL             { return TR_UNTIL;             }
UPDATE            { return TR_UPDATE;            }
USING             { return TR_USING;             }
VALUES            { return TR_VALUES;            }
VARIABLE          { return TR_VARIABLE;          }
VARIABLE_LARGE      { return TR_VARIABLE_LARGE;      }
VIEW              { return TR_VIEW;              }
VOLATILE          { return TR_VOLATILE;          }
WHEN              { return TR_WHEN;              }
WHERE             { return TR_WHERE;             }
WHILE             { return TR_WHILE;             }
WITH              { return TR_WITH;              }
WORK              { return TR_WORK;              }
WRAPPED           { return TR_WRAPPED;           }  /* PROJ-2550 PSM Encryption */
WRITE             { return TR_WRITE;             }
_PROWID           { return TR__PROWID;           }  // PROJ-1789 PROWID
PRECEDING         { return TR_PRECEDING;         }  /* PROJ-1805 */
FOLLOWING         { return TR_FOLLOWING;         }  /* PROJ-1805 */
CURRENT[ \t\r\n]+ROW { return TR_CURRENT_ROW;    }  /* PROJ-1805 */
LINK              { return TR_LINK;              }  /* PROJ-1832 */
WHENEVER          { return TR_WHENEVER;          }  // PROJ-2223 audit
WITHIN            { return TR_WITHIN;            }  // PROJ-2527 WITHIN GROUP AGGR

BETWEEN           { return TK_BETWEEN;           }
EXISTS            { return TK_EXISTS;            }

CONSTANT          { return TO_CONSTANT;          }
IDENTIFIED        { return TO_IDENTIFIED;        }
INDEX             { return TO_INDEX;             }
MINUS             { return TO_MINUS;             }
MODE              { return TO_MODE;              }
OTHERS            { return TO_OTHERS;            }
RAISE             { return TO_RAISE;             }
RENAME            { return TO_RENAME;            }
REPLACE           { return TO_REPLACE;           }
ROWTYPE           { return TO_ROWTYPE;           }
WAIT              { return TO_WAIT;              }
EXTENT            { return TO_EXTENT;            }
SEGMENT           { return TO_SEGMENT;           }
STORAGE           { return TO_STORAGE;           }
PIVOT             { return TO_PIVOT;             }
UNPIVOT           { return TO_UNPIVOT;           } /* PROJ-2523 Unpivot clause */
MATERIALIZED      { return TO_MATERIALIZED;      }
CONNECT_BY_ROOT   { return TO_CONNECT_BY_ROOT;   }     /* PROJ-1715 */
CONNECT[ \t\r\n]+BY[ \t\r\n]+NOCYCLE { return TO_CONNECT_BY_NOCYCLE; } /* PROJ-1715 */
LIBRARY           { return TO_LIBRARY;           } // PROJ-1685
LANGUAGE          { return TO_LANGUAGE;          } // PROJ-1685
PARAMETERS        { return TO_PARAMETERS;        } // PROJ-1685
AUDIT             { return TO_AUDIT;             } // PROJ-2223 audit
NOAUDIT           { return TO_NOAUDIT;           } // PROJ-2223 audit
DELAUDIT          { return TO_DELAUDIT;          } // BUG-39074 audit
ACCESS            { return TO_ACCESS;            } // PROJ-2223 audit
NULLS             { return TO_NULLS;             } // PROJ-2435 order by nulls first/last
IGNORE[ \t\r\n]+NULLS { return TO_IGNORE_NULLS;  } /* BUG-40279 lead, lag with ignore nulls */
PURGE             { return TO_PURGE;             } /* PROJ-2441 flashback */
FLASHBACK         { return TO_FLASHBACK;         }
VC2COLL           { return TO_VC2COLL;           } // BUG-41310
KEEP              { return TO_KEEP;              } // PROJ-2528 Keep Aggregation

DATABASE          { return TR_DATABASE;          }
ELSIF             { return TA_ELSIF;             }
EXTENTSIZE        { return TA_EXTENTSIZE;        }
FIXED             { return TA_FIXED;             }
LOCK              { return TA_LOCK;              }
UNLOCK            { return TA_UNLOCK;            } /* PROJ-2207 Password policy support */
MAXROWS           { return TA_MAXROWS;           }
ONLINE            { return TA_ONLINE;            }
OFFLINE           { return TA_OFFLINE;           }
REPLICATION       { return TA_REPLICATION;       }
ROWCOUNT          { return TA_ROWCOUNT;          }
SQLCODE           { return TA_SQLCODE;           }
SQLERRM           { return TA_SQLERRM;           }
STEP              { return TA_STEP;              }
TABLESPACE        { return TA_TABLESPACE;        }
TRUNCATE          { return TA_TRUNCATE;          }
DECRYPT           { return TA_DECRYPT;           } // PROJ-2002
BULK              { return TA_BULK;              } // PROJ-1584 DML Return Clause
LINKER            { return TA_LINKER;            } /* PROJ-1832 */
IN[ \t\r\n]+REVERSE { return TA_IN_REVERSE;      } // BUG-34421
REMOTE_TABLE      { return TA_REMOTE_TABLE;      } /* PROJ-1832 */
REMOTE_TABLE_STORE { return TA_REMOTE_TABLE_STORE; } /* BUG-37077 */
SHARD             { return TA_SHARD;             } /* PROJ-2638 */
COMPRESS          { return TA_COMPRESS;          } // PROJ-2264 Dictionary table
REORGANIZE        { return TA_REORGANIZE;        } // PROJ-2264 Dictionary table
DISJOIN           { return TA_DISJOIN;           } /* PROJ-1810 Partition Exchange */
CONJOIN           { return TA_CONJOIN;           } /* BUG-42468 JOIN TO CONJOIN */
ANALYSIS[ \t\r\n]+PROPAGATION   { return TA_ANALYSIS_PROPAGATION; } // BUG-46095

LATERAL           { return TR_LATERAL;           } // PROJ-2418 Lateral View
CROSS             { return TR_CROSS;             } // PROJ-2418 Lateral View
APPLY             { return TR_APPLY;             } // PROJ-2418 Lateral View
SYNONYM           { return TR_SYNONYM;           }
PARALLEL          { return TR_PARALLEL;          } // to fix BUG-13127
NOPARALLEL        { return TR_NOPARALLEL;        } // to fix BUG-13127
LOB               { return TR_LOB;               } // PROJ-1362
STORE             { return TR_STORE;             } // PROJ-1362
SPLIT             { return TR_SPLIT;             } // PROJ-1548
CHECKPOINT        { return TR_CHECKPOINT;        } // PROJ-1548
AUTOEXTEND        { return TR_AUTOEXTEND;        } // PROJ-1548
LOGGING           { return TR_LOGGING;           } // PROJ-1469
NOLOGGING         { return TR_NOLOGGING;         } // PROJ-1469
COMPRESSED        { return TR_COMPRESSED;        } // TASK-2398 Log Compress
UNCOMPRESSED      { return TR_UNCOMPRESSED;      } // TASK-2398 Log Compress
SUPPLEMENTAL      { return TR_SUPPLEMENTAL;      } // PROJ-1723 [MDW/INTEGRATOR] Altibase Plugin 개발
OVER              { return TR_OVER;              } // PROJ-1762
MODIFY            { return TR_MODIFY;            } // PROJ-1877
COMMENT           { return TR_COMMENT;           } // BUG-21387 Comment 기능 지원
SHRINK_MEMPOOL    { return TR_SHRINK_MEMPOOL;    } // PROJ-2065
DUMP_CALLSTACKS   { return TR_DUMP_CALLSTACKS;   } // BUG-45182
ROLLUP            { return TR_ROLLUP;            } // PROJ-1353
CUBE              { return TR_CUBE;              } // PROJ-1353
GROUPING[ \t\r\n]+SETS { return TR_GROUPING_SETS;     } // PROJ-1353
TOP               { return TM_TOP;               } /* BUG-36580 supported TOP */
TOP[ \t\r\n]+"100"[ \t\r\n]+PERCENT  {
                    return TM_TOP_ALL;           }
NODE[ \t\r\n]*"["[ \t\r\n]*META[ \t\r\n]*"]" {
                    return TA_NODE_META;         } /* BUG-45359 */
NODE[ \t\r\n]*"["[ \t\r\n]*DATA {
                    return TA_NODE_DATA;         } /* BUG-45359 */
WITH[ \t\r\n]+ROLLUP { return TC_WITH_ROLLUP;    } /* BUG-46702 */

<NEG_TRAIL>([_A-Z]|({NEG_TRAIL_CH}))([_0-9A-Z$#]|({NEG_TRAIL_CH}))*   {
                                         return TI_NONQUOTED_IDENTIFIER; }
<POS_TRAIL>([_A-Z]|({POS_TRAIL_CH}))([_0-9A-Z$#]|({POS_TRAIL_CH}))*   {
                                         return TI_NONQUOTED_IDENTIFIER; }
<SJIS>([_A-Z]|({SJIS_ALL}))([_0-9A-Z$#]|({SJIS_ALL}))*   {
                                         return TI_NONQUOTED_IDENTIFIER; }


"\""[^'"]+"\""                         { return TI_QUOTED_IDENTIFIER; }


("BIGINT"|"BINARY"|"BIT"|"BLOB"|"BLOB_LOCATOR"|"BOOLEAN"|"BYTE"|"CHAR"|"CLOB"|"CLOB_LOCATOR"|"DATE"|"DECIMAL"|"DOUBLE"|"ECHAR"|"EVARCHAR"|"FILETYPE"|"FLOAT"|"GEOMETRY"|"INT"|"INTEGER"|"INTERVAL"|"LIST"|"NCHAR"|"NIBBLE"|"NUMBER"|"NUMERIC"|"NULL"|"NVARCHAR"|"REAL"|"SMALLINT"|"VARBIT"|"VARCHAR"|"VARCHAR2"|"VARBYTE"|"RAW")"'"("''"*[^']*)*"'" { return TL_TYPED_LITERAL;   }


N"'"("''"*[^']*)*"'"                      { return TL_NCHAR_LITERAL;   }


U"'"("''"*[^']*)*"'"                      { return TL_UNICODE_LITERAL;   }


"'"("''"*[^']*)*"'"                       { return TL_LITERAL;         }



":"[_A-Z][_0-9A-Z]*                   { return TI_HOSTVARIABLE; }
[0-9]+                                { return TL_INTEGER;      }
[0-9]+("."[0-9]*)?([Ee][+-]?[0-9]+)?  |
"."[0-9]+([Ee][+-]?[0-9]+)?           { return TL_NUMERIC;      }

"/*+"[^*/]*"*/" {
                      gPrevState = YYSTATE;
                      BEGIN(INITIAL);
                      qcphManager::parseIt (
                          mStatement,
                          (mBufferLast-mBuffer)+(YYText()-mBufferInput) + 3,
                          (SChar*)( YYText() + 3 ),
                          YYLeng() - 5 );
                      BEGIN(gPrevState);
                      return TX_HINTS;
                  }
("--"|"//").*     {}

"/*" { gPrevState = YYSTATE; BEGIN(comment); }
<comment>[^*\n]*   {}
<comment>"*"+[^*/\n]*   {}
<comment>\n             {}
<comment>"*"+"/" { BEGIN(gPrevState); }

"||"  { return TS_CONCATENATION_SIGN;  }
".."  { return TS_DOUBLE_PERIOD;  }

"!"   { return TS_EXCLAMATION_POINT;   }
"%"   { return TS_PERCENT_SIGN;        }
"("   { return TS_OPENING_PARENTHESIS; }
")"   { return TS_CLOSING_PARENTHESIS; }
"["   { return TS_OPENING_BRACKET;     }
"]"   { return TS_CLOSING_BRACKET;     }
"*"   { return TS_ASTERISK;            }
"+"   { return TS_PLUS_SIGN;           }
","   { return TS_COMMA;               }
"-"   { return TS_MINUS_SIGN;          }
"."   { return TS_PERIOD;              }
"/"   { return TS_SLASH;               }
":"   { return TS_COLON;               }
";"   { return TS_SEMICOLON;           }
"<"   { return TS_LESS_THAN_SIGN;      }
"="   { return TS_EQUAL_SIGN;          }
">"   { return TS_GREATER_THAN_SIGN;   }
"?"   { return TS_QUESTION_MARK;       }
"(+)" { return TS_OUTER_JOIN_OPERATOR; }    // PROJ-1653 Outer Join Operator (+)
"@"   { return TS_AT_SIGN;             }    /* PROJ-1832 New database link */
"=>"  { return TS_NAMED_ASSOCIATE_SIGN;}    /* BUG-41243 Name-based Argument Passing */

"\n\r" |
"\n"   |
"\r"     { }

[ \t]+   { }

.        { return E_ERROR; }

%%



qcplLexer::qcplLexer( SChar* aBuffer,
                      UInt   aBufferLength,
                      SInt   aStart,
                      SInt   aSize,
                      SInt   aFirstToken ) 
{ 
    mBuffer       = aBuffer;
    mBufferCursor = mBuffer + aStart;

    mBufferLength = aBufferLength;
    mBufferRemain = aSize;

    mFirstToken   = aFirstToken;

    mLastTokenPos[0].stmtText = aBuffer;
    mLastTokenPos[0].offset   = -1;  // empty
    mLastTokenPos[0].size     = -1;  // empty

    mLastTokenPos[1].stmtText = aBuffer;
    mLastTokenPos[1].offset   = -1;  // empty
    mLastTokenPos[1].size     = -1;  // empty

    if (mtl::mDBCharSet != NULL)
    {
        mMtlModule = mtl::mDBCharSet;
    }
    else
    {
        mMtlModule = mtl::defaultModule();
    }
}

SChar * qcplLexer::getLexLastError( SChar * aMessage )
{
    qcNamePosition sPosition;
    UInt           sLineOffset;
    UInt           sLineNo;
    UInt           sBufferIterator;
    UInt           sMessageIterator;
    UInt           sMessageFence;
    UInt           i = 0;
    UInt           sBufferFence = 0;
    UInt           sPositionLength = 0;
    UInt           sLineStartOffset = 0;
    UInt           sLineEndOffset = 0;
    idBool         sNeedBeforeReduction = ID_FALSE;
    idBool         sNeedAfterReduction = ID_FALSE;

    getPosition( &sPosition );

    sLineOffset = 0 ;
    sLineNo     = 1;
    for( sBufferIterator = 0;
         sBufferIterator < (UInt)(sPosition.offset);
         sBufferIterator++ )
    {
        if( mBuffer[sBufferIterator] == '\n' )
        {
            sLineStartOffset = sBufferIterator + 1;
            sLineNo++;
        }
    }

    sBufferIterator += sPosition.size;

    while ( ( sBufferIterator < mBufferLength ) &&
            ( mBuffer[sBufferIterator] != '\n' ) )
    {
        sBufferIterator++;
    }

    sLineEndOffset = sBufferIterator;

    sLineOffset = sLineStartOffset;

    if ( ( sLineEndOffset - sLineStartOffset ) > QCPL_ERROR_SQL_MAX_SIZE )
    {
        if ( ( sPosition.offset - sLineStartOffset ) > ( QCPL_ERROR_SQL_MAX_SIZE / 2 ) )
        {
            sLineOffset = sPosition.offset - ( QCPL_ERROR_SQL_MAX_SIZE / 2 );
            sNeedBeforeReduction = ID_TRUE;
        }
        else
        {
            /* do nothing */
        }

        if ( ( sLineOffset + QCPL_ERROR_SQL_MAX_SIZE ) > sLineEndOffset )
        {
            sLineOffset = sLineEndOffset - QCPL_ERROR_SQL_MAX_SIZE;
        }
        else
        {
            sNeedAfterReduction = ID_TRUE;
        }

        sBufferFence = sLineOffset + QCPL_ERROR_SQL_MAX_SIZE;
    }
    else
    {
        sBufferFence = sLineEndOffset;
    }

    /* summary info */
    idlOS::snprintf( mMessage, QCPL_BUFFER_SIZE,
                     "\n\nline %"ID_INT32_FMT": %s\n", sLineNo, aMessage );

    sMessageIterator  = idlOS::strlen( mMessage );
    sMessageFence = QCPL_BUFFER_SIZE - 8;

    if ( ( sNeedBeforeReduction == ID_TRUE ) || ( sNeedAfterReduction == ID_TRUE ) )
    {
        idlOS::snprintf( &mMessage[sMessageIterator],
                         sMessageFence - sMessageIterator,
                         "position %"ID_INT32_FMT": ",
                         sPosition.offset - sLineStartOffset + 1 );

        sPositionLength = idlOS::strlen( &mMessage[sMessageIterator] );
        sMessageIterator += sPositionLength;
    }
    else
    {
        /* do nothing */
    }

    /* copy from sql to error buffer */
    if ( sNeedBeforeReduction == ID_TRUE )
    {
        idlOS::snprintf( &mMessage[sMessageIterator],
                         sMessageFence - sMessageIterator,
                         "... " );

        sPositionLength += idlOS::strlen( &mMessage[sMessageIterator] );
        sMessageIterator += idlOS::strlen( &mMessage[sMessageIterator] );
    }
    else
    {
        /* do nothing */
    }

    for( sBufferIterator = sLineOffset;
         ( sMessageIterator < sMessageFence ) &&
         ( sBufferIterator < sBufferFence );
         sBufferIterator++, sMessageIterator++ )
    {
        mMessage[sMessageIterator] = mBuffer[sBufferIterator];
    }

    if ( sNeedAfterReduction == ID_TRUE )
    {
        idlOS::snprintf( &mMessage[sMessageIterator],
                         sMessageFence - sMessageIterator,
                        " ..." );
        sMessageIterator += idlOS::strlen( &mMessage[sMessageIterator] );
    }
    else
    {
        /* do nothing */
    }

    if( sMessageIterator < sMessageFence )
    {
        mMessage[sMessageIterator] = '\n';
        sMessageIterator++;
    }

    /* set error positioin */
    for ( i = 0; 
          ( i < sPositionLength ) && 
          ( sMessageIterator < sMessageFence ); 
          i++, sMessageIterator++ )
    {
        mMessage[sMessageIterator] = ' ';
    }

    for( sBufferIterator = sLineOffset;
         sMessageIterator < sMessageFence &&
         sBufferIterator  < mBufferLength;
         sBufferIterator++, sMessageIterator++ )
    {
        if( sBufferIterator == (UInt)(sPosition.offset)                    ||
	    sBufferIterator == (UInt)(sPosition.offset + sPosition.size - 1) )
        {
            mMessage[sMessageIterator] = '^';
        }
        else
        {
            mMessage[sMessageIterator] = ( mBuffer[sBufferIterator] == '\n' )
                                       ? '\n' : ' ' ;
        }

        if( sBufferIterator == (UInt)(sPosition.offset + sPosition.size - 1) )
        {
            sMessageIterator++;            
            break;             
        }
        else
        {
            // Nothing To Do
        }
    }

    mMessage[sMessageIterator] = '\n';
    sMessageIterator++;
    mMessage[sMessageIterator] = '\n';
    sMessageIterator++;
    mMessage[sMessageIterator] = '\0';

    return mMessage;
}

void qcplLexer::getPosition( qcNamePosition* aPosition )
{
    aPosition->stmtText = mBuffer;
    aPosition->offset   = (mBufferLast-mBuffer)+(YYText()-mBufferInput);
    aPosition->size     = YYLeng();
}

void qcplLexer::strUpper( qcNamePosition* aPosition )
{
    SChar * sIndex = mBuffer + aPosition->offset;
    SChar * sFence = sIndex + aPosition->size;

    // PRJ-1678 : For multi-byte character set strings
    for( ;sIndex < sFence; mMtlModule->nextCharPtr( (UChar**) &sIndex,
                                                 (UChar*)sFence ) )
    {
        *sIndex = idlOS::toupper(*sIndex);
    }
}

int qcplLexer::LexerInput( char* aBuffer,
                           int   aMaximum )
{
    aMaximum       = 
        ( (int)mBufferRemain < aMaximum ) ? (int)mBufferRemain : aMaximum ;
    aMaximum       = ( aMaximum > 0 ) ? aMaximum : 0 ;
    mBufferRemain -= aMaximum;

    idlOS::memcpy( aBuffer, mBufferCursor, aMaximum );

    mBufferLast    = mBufferCursor;

    mBufferCursor += aMaximum;

    mBufferInput   = aBuffer;

    return aMaximum;
}


void qcplLexer::setInitState( void )
{
    if( gCharType == IDN_CF_UNKNOWN )
    {
        switch(mMtlModule->id)
        {
            case MTL_SHIFTJIS_ID:
            /* PROJ-2590 [기능성] CP932 database character set 지원 */ 
            case MTL_MS932_ID:
            {
                gCharType = IDN_CF_SJIS;
                break;
            }
            case MTL_BIG5_ID:
            case MTL_GB231280_ID:
            /* PROJ-2414 [기능성] GBK, CP936 character set 추가 */
            case MTL_MS936_ID:
            case MTL_MS949_ID:
            {
                gCharType = IDN_CF_POS_TRAIL;
                break;
            }
            default:
            {
                gCharType = IDN_CF_NEG_TRAIL;
                break;
            }
        }
    }

    switch(gCharType)
    {
        case IDN_CF_SJIS:
        {
            BEGIN(SJIS);
            break;
        }
        case IDN_CF_POS_TRAIL:
        {
            BEGIN(POS_TRAIL);
            break;
        }
        default:
        {
            BEGIN(NEG_TRAIL);
            break;
        }
    }
}


SInt qcplLexer::getFirstToken( void )
{
    SInt sFirstToken;

    sFirstToken = mFirstToken;
    mFirstToken = 0;

    return sFirstToken;
}

void qcplLexer::updateLastTokenPosition( qcNamePosition* aPosition )
{
    mLastTokenPos[1].stmtText = mLastTokenPos[0].stmtText;
    mLastTokenPos[1].offset   = mLastTokenPos[0].offset;
    mLastTokenPos[1].size     = mLastTokenPos[0].size;

    mLastTokenPos[0].stmtText = aPosition->stmtText;
    mLastTokenPos[0].offset   = aPosition->offset;
    mLastTokenPos[0].size     = aPosition->size;
}

void qcplLexer::setLastTokenPosition( qcNamePosition* aPosition )
{
    aPosition->stmtText = mLastTokenPos[1].stmtText;
    aPosition->offset   = mLastTokenPos[1].offset;
    aPosition->size     = mLastTokenPos[1].size;
}

void qcplLexer::setEndTokenPosition( qcNamePosition* aPosition )
{
    aPosition->stmtText = mLastTokenPos[0].stmtText;
    aPosition->offset   = mLastTokenPos[0].offset;
    aPosition->size     = mLastTokenPos[0].size;
}

void qcplLexer::setNullPosition( qcNamePosition* aPosition )
{
    aPosition->stmtText = mBuffer;
    aPosition->offset   = 0;
    aPosition->size     = 0;
}

int qcpllex( YYSTYPE* lvalp,
             void*    param )
{
    int        sToken;
    qcplLexer* sLexer  = ((qcplx *)param)->mLexer;

    sLexer->setInitState();

    sToken = sLexer->getFirstToken();

    if ( sToken > 0 )
    {
        sLexer->setNullPosition( &lvalp->position );
    }
    else
    {
        sToken = sLexer->yylex();

//        sLexer->printToken();

        sLexer->getPosition( &lvalp->position );

        sLexer->updateLastTokenPosition( &lvalp->position );

        if( sToken == TI_NONQUOTED_IDENTIFIER )
        {
            sLexer->strUpper( &lvalp->position );
        }
    }

    return sToken;
}


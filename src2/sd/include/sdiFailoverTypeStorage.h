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
 * $Id$ sdiFailoverTypeStorage.h
 **********************************************************************/

#ifndef _O_SDI_FAILOVER_TYPE_STORAGE_H_
#define _O_SDI_FAILOVER_TYPE_STORAGE_H_ 1

#define SDI_CLIENT_NODE_ID_NONE (0)

typedef struct {
    UInt   mNodeId     [SDI_NODE_MAX_COUNT];
    UChar  mDestination[SDI_NODE_MAX_COUNT];
    SInt   mMaxIndex;
} sdlNodeFailoverTypeArray;

class sdiFailoverTypeStorage
{
  public:
    void                initialize();
    void                finalize();
    IDE_RC              setClientConnectionStatus( UInt aNodeId, UChar aDestination );
    sdiFailOverTarget   getClientConnectionStatus( UInt aNodeId );

  private:
    void clearReports();

  private:
    sdlNodeFailoverTypeArray mStorage;
};

#endif  // _O_SDI_FAILOVER_TYPE_STORAGE_H_

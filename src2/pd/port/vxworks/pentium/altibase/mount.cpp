/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdio.h>
#include <dosFsLib.h>
#include <drv/hdisk/ataDrv.h>

BLK_DEV      * ata_device;  /* pointer to the ATA device       */
DOS_VOL_DESC * hd_volume;  /* the dos fs volume description */
char         * devname= "/DOSA";  /* name of the new       device     */

int mount( int argc, char **argv )
{
  /* show a little bit of information */
  printf( "trying to initalize device %s\n", devname );

  /* init the ATA Device (Bus 0, Device 0 => Primary Master) */
  ata_device= ataDevCreate( 0, 0, 0, 63 );

  /*
  dosFsMkfsOptionsSet(DOS_OPT_LONGNAMES);
  hd_volume= dosFsMkfs( devname, ata_device );
  */
 
  /* init the Dos FS on the newly created device */
  hd_volume= dosFsDevInit( devname, ata_device, NULL );

  /* display the configuration data of the newly createtd device on stdio */
  dosFsShow( devname, 0 );
 
  return 0;
}

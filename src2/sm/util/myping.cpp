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
 
#include <idl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

SChar sBuffer[1024];
SInt  seq;
struct sockaddr  gTo;

#define DEST_ADDR "211.188.18.77"

int main()
{
    SInt fd;
    SInt fromlen;
    SInt tolen;
    SInt n;
    struct sockaddr    from;
    struct sockaddr_in  *to;
    struct icmp     sIcmp;

    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd == -1)
    {
        idlOS::fprintf(stderr, "socket error %d\n", errno);
        idlOS::exit(0);
    }

    idlOS::printf("PID = %d\n", idlOS::getpid());

    sIcmp.icmp_type = ICMP_ECHO;
    sIcmp.icmp_code = 0;
    sIcmp.icmp_id   = idlOS::getpid();
    sIcmp.icmp_seq  = seq++;
    sIcmp.icmp_cksum  = 0;
    
    to = 
        (struct sockaddr_in *)&gTo;

    to->sin_family = AF_INET;
    to->sin_addr.s_addr = inet_addr(DEST_ADDR);
    tolen = ID_SIZEOF(gTo);

    n = sendto(fd, &sIcmp, ID_SIZEOF(sIcmp), 0, &gTo, tolen);

    if (n < 0)
    {
        idlOS::fprintf(stderr, "sendto error %d\n", errno);
        idlOS::exit(0);
    }

    // recvfrom

    fromlen = ID_SIZEOF(from);
    n = recvfrom(fd, sBuffer, ID_SIZEOF(sBuffer), 0, &from, (socklen_t *)&fromlen);

    if (n < 0)
    {
        idlOS::fprintf(stderr, "recvfrom error %d\n", errno);
        idlOS::exit(0);
    }
    else
    {
        struct ip *ip;
        struct icmp *icmp;

        ip = (struct ip *)sBuffer;

        icmp = (struct icmp *)(sBuffer + (ip->ip_hl << 2));
        
        idlOS::printf("type is %d\n", (UInt)sIcmp.icmp_type);
        idlOS::printf("code = %d\n",  (UInt)sIcmp.icmp_code);
        idlOS::printf("id = %d\n",    (UInt)sIcmp.icmp_id);
        idlOS::printf("seq = %d\n",   (UInt)sIcmp.icmp_seq);
        
        
        idlOS::printf("size is %d\n", n);
    }

}

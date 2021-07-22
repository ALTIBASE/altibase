/*****************************************************************************
 * Copyright 1999-2000, RTBase Corporation or its subsidiaries.
 * All rights reserved.
 ****************************************************************************/

/*****************************************************************************
 * $Id: checkConnect.cpp 26440 2008-06-10 04:02:48Z jdlee $
 *
 * DESC : configure된 내용이 실제 RUNTIME 결과에 문제를 미치지 않는지 검사.
 *        by gamestar 2000/8/30
 *
 ****************************************************************************/
#include <idl.h>

#include <ida.h>


SInt connect_timed_wait(PDL_SOCKET sockfd,
                        struct sockaddr *addr,
                        int addrlen,
                        PDL_Time_Value  *tval)
{
    SInt  n;
    SChar error;
    fd_set rset, wset;
    
    if ( idlVA::setNonBlock( sockfd ) != IDE_SUCCESS )
    {
        return -1;
    }

    error = 0;
    if ( (n = idlOS::connect( sockfd, addr, addrlen)) < 0 )
    {
        if (errno != EINPROGRESS)
        {
            return -1;
        }
    }

    if ( n != 0 )
    {
        FD_ZERO(&rset);
        FD_SET(sockfd, &rset);
        wset = rset;

        if ( (n = idlOS::select(sockfd + 1, &rset, &wset, NULL, tval)) == 0)
        {
            idlOS::closesocket(sockfd);
            errno = ETIMEDOUT;
            return -1;
        }

        if (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset))
	  {
            SInt len;
            len = sizeof(error);

            if (idlOS::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
            {
                return -1;
            }
        }
        else
        {
            return -1;
        }
    }
    
    return 0;
}

int main(int argc, char *argv[])
{
    PDL_SOCKET m_newsock;
    struct sockaddr_in servaddr;
    PDL_Time_Value     to;

    to.initialize(10);

    if (argc < 3)
    {
        idlOS::fprintf(stderr, "checkConnect hostip port \n");
        idlOS::exit(0);
    }
    
    idlOS::memset(&servaddr, 0, sizeof(servaddr));

    m_newsock = idlOS::socket(AF_INET, SOCK_STREAM, 0);
    if (m_newsock == -1)
    {
        idlOS::fprintf(stderr, "error socket \n");
    }
     
    servaddr.sin_family      = AF_INET; 
    servaddr.sin_addr.s_addr = idlOS::inet_addr(argv[1]);
    servaddr.sin_port        = htons(idlOS::strtol(argv[2], NULL, 10));

    fprintf(stderr, "connecting to  %s %s ...\n", argv[1], argv[2]);
    if (connect_timed_wait(m_newsock,
                           (sockaddr *)&servaddr,
                           sizeof(servaddr),
                           &to) != 0)
    {
        idlOS::fprintf(stderr, "connect error \n");
    }
    else
    {
        idlOS::fprintf(stderr, "connect success \n");
    }
}




/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//==============================================================================
//	Includes
//==============================================================================
#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#define ADDRINFO struct addrinfo
#define INVALID_SOCKET -1
#define closesocket close
#define SD_BOTH SHUT_RDWR
#define SOCKET_ERROR -1

#endif

#include "SimpleTCPServerSocket.h"

//==============================================================================
/**
 *	CTOR
 *	Accepts a buffer size for internal storage and a flag to indicate if
 *	Recv should return fixed sized packets or not.
 */
SimpleTCPServerSocket::SimpleTCPServerSocket(long inBufferSize, bool inWaitForFull)
    : m_ServerSocket(NULL)
    , m_ClientSocket(NULL)
    , m_Data(NULL)
    , m_BytesRead(0)
    , m_BytesSent(0)
    , m_BufferSize(inBufferSize)
    , m_WaitForFull(inWaitForFull)
    , m_Initialized(false)
{
    m_Data = new char[m_BufferSize];
}

//==============================================================================
/**
 *	DTOR
 */
SimpleTCPServerSocket::~SimpleTCPServerSocket()
{
    if (m_ClientSocket)
        Disconnect();

    delete[] m_Data;
}

//==============================================================================
/**
 *	Initializes TCP communications.
 *	Needs to be called once per app.
 */
bool SimpleTCPServerSocket::Initialize()
{
#ifdef _PCPLATFORM
    WSADATA theWsaData;

    if (WSAStartup(WINSOCK_VERSION, &theWsaData)) {
        // WSAStartup failed
        return false;
    }
#endif
    return true;
}

//==============================================================================
/**
 *	Deinitializes TCP communications.
 *	Needs to be called once per app.
 */
void SimpleTCPServerSocket::DeInitialize()
{
#ifdef _PCPLATFORM
    WSACleanup();
#endif
}

//==============================================================================
/**
 *	Creates server socket, bind it and listen for incoming connection from a client.
 */
bool SimpleTCPServerSocket::Accept(const char *inServerAddress /*=NULL*/,
                                   unsigned short inServerPort /*=0*/)
{
    bool theReturn = false;
    struct sockaddr_in sockaddrServer;
    struct sockaddr_in sockaddrClient;

    if (!m_Initialized) {
        /* Create the TCP socket */
        if ((m_ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) != INVALID_SOCKET) {
            /* Construct the server sockaddr_in structure */
            memset(&sockaddrServer, 0, sizeof(sockaddrServer)); /* Clear struct */
            sockaddrServer.sin_family = AF_INET; /* Internet/IP */
            sockaddrServer.sin_addr.s_addr = htonl(INADDR_ANY); /* Incoming addr */
            sockaddrServer.sin_port = htons(inServerPort); /* server port */

            /* Bind the server socket */
            if (bind(m_ServerSocket, (struct sockaddr *)&sockaddrServer, sizeof(sockaddrServer))
                == 0)
                m_Initialized = true;
        }
    }

    /* Listen on the server socket */
    if (listen(m_ServerSocket, 1) == 0) {
        socklen_t theClientLen = sizeof(sockaddrClient);
        /* Wait for client connection */
        if ((m_ClientSocket =
                 accept(m_ServerSocket, (struct sockaddr *)&sockaddrClient, &theClientLen))
            == 0) {
            SetNonBlocking(m_ClientSocket);
            theReturn = true;
        }
    }

    return theReturn;
}

//==============================================================================
/**
 *	Sets socket to non blocking state
 */
bool SimpleTCPServerSocket::SetNonBlocking(SOCKET inSocket)
{
// Set this up to be a non-blocking socket
#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
    int theFlags = fcntl(inSocket, F_GETFL, 0);
    return fcntl(m_ClientSocket, F_SETFL, theFlags | O_NONBLOCK) == 0;
#endif

#ifdef _PCPLATFORM
    u_long theBlockMode = 1;
    return ioctlsocket(inSocket, FIONBIO, &theBlockMode) == 0;
#endif
}

//==============================================================================
/**
 *	Disconnect and closes the TCP connection.
 */
void SimpleTCPServerSocket::Disconnect(bool inShutdown /*= false*/)
{
    shutdown(m_ClientSocket, SD_BOTH);
    closesocket(m_ClientSocket);
    m_BytesRead = 0;
    m_BytesSent = 0;
    m_ClientSocket = 0;

    if (inShutdown) {
        shutdown(m_ServerSocket, SD_BOTH);
        closesocket(m_ServerSocket);
        m_ServerSocket = 0;
    }
}

//==============================================================================
/**
 *	Receives a data packet from the socket.
 *	If m_WaitForFull is true, Recv will always return 0 until it reads m_BufferSize
 *	bytes.
 *	Else, it will return the number of bytes read (if any) per call.
 */
long SimpleTCPServerSocket::Recv()
{
    long theReturnValue = 0;

    if (m_WaitForFull) {
        theReturnValue = recv(m_ClientSocket, m_Data + m_BytesRead, m_BufferSize - m_BytesRead, 0);

        if (theReturnValue > 0)
            m_BytesRead += theReturnValue;

        if (m_BytesRead < m_BufferSize)
            theReturnValue = 0;
        else {
            m_BytesRead = 0;
            theReturnValue = m_BufferSize;
        }
    } else {
        theReturnValue = recv(m_ClientSocket, m_Data, m_BufferSize, 0);
        if (theReturnValue == SOCKET_ERROR)
            theReturnValue = 0;
    }

    return theReturnValue;
}

//==============================================================================
/**
 *	Sends a data packet to the socket.
 *	If m_WaitForFull is true, Send will always return 0 until it receives m_BufferSize
 *	bytes.
 *	Else, it will return the number of bytes sent (if any) per call.
 */
long SimpleTCPServerSocket::Send()
{
    long theReturnValue = 0;

    if (m_WaitForFull) {
        theReturnValue = send(m_ClientSocket, m_Data + m_BytesSent, m_BufferSize - m_BytesSent, 0);

        if (theReturnValue > 0)
            m_BytesSent += theReturnValue;

        if (m_BytesSent < m_BufferSize)
            theReturnValue = 0;
        else {
            m_BytesSent = 0;
            theReturnValue = m_BufferSize;
        }
    } else {
        theReturnValue = send(m_ClientSocket, m_Data, m_BufferSize, 0);
        if (theReturnValue == SOCKET_ERROR)
            theReturnValue = 0;
    }

    return theReturnValue;
}

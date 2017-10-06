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

#include "SimpleTCPClientSocket.h"

//==============================================================================
/**
 *	CTOR
 *	Accepts a buffer size for internal storage and a flag to indicate if
 *	Recv should return fixed sized packets or not.
 */
SimpleTCPClientSocket::SimpleTCPClientSocket(long inBufferSize, bool inWaitForFull)
    : m_ClientSocket(NULL)
    , m_Data(NULL)
    , m_BytesRead(0)
    , m_BufferSize(inBufferSize)
    , m_WaitForFull(inWaitForFull)
{
    m_Data = new char[m_BufferSize];
}

//==============================================================================
/**
 *	DTOR
 */
SimpleTCPClientSocket::~SimpleTCPClientSocket()
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
bool SimpleTCPClientSocket::Initialize()
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
void SimpleTCPClientSocket::DeInitialize()
{
#ifdef _PCPLATFORM
    WSACleanup();
#endif
}

//==============================================================================
/**
 *	Attempts to connect at the specified address and port.
 */
bool SimpleTCPClientSocket::Connect(const char *inServerAddress, const char *inServerPort)
{
    bool theReturn = false;
    ADDRINFO theHints;
    ADDRINFO *theAddrInfo = NULL;
    ADDRINFO *theAddrInfoPtr = NULL;

    memset(&theHints, 0, sizeof(theHints));
    theHints.ai_family = PF_INET;
    theHints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(inServerAddress, inServerPort, &theHints, &theAddrInfo) == 0) {
        for (theAddrInfoPtr = theAddrInfo; theAddrInfoPtr != NULL;
             theAddrInfoPtr = theAddrInfoPtr->ai_next) {
            if ((theAddrInfoPtr->ai_family == PF_INET)
                || (theAddrInfoPtr->ai_family == PF_INET6)) // only want PF_INET or PF_INET6
            {
                m_ClientSocket = socket(theAddrInfoPtr->ai_family, theAddrInfoPtr->ai_socktype,
                                        theAddrInfoPtr->ai_protocol);

                if (m_ClientSocket != INVALID_SOCKET) {
                    if (theAddrInfoPtr->ai_socktype == SOCK_STREAM) {
                        if (connect(m_ClientSocket, theAddrInfoPtr->ai_addr,
                                    theAddrInfoPtr->ai_addrlen)
                            == 0) {
// Set this up to be a non-blocking socket
#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
                            int theFlags = fcntl(m_ClientSocket, F_GETFL, 0);
                            fcntl(m_ClientSocket, F_SETFL, theFlags | O_NONBLOCK);
#endif

#ifdef _PCPLATFORM
                            u_long theBlockMode = 1;
                            ioctlsocket(m_ClientSocket, FIONBIO, &theBlockMode);
#endif

                            theReturn = true;
                            break;
                        }

                        // Connect failed
                        closesocket(m_ClientSocket);
                    }
                }
            }
        }
    }

    freeaddrinfo(theAddrInfo);
    return theReturn;
}

//==============================================================================
/**
 *	Disconnect and closes the TCP connection.
 */
void SimpleTCPClientSocket::Disconnect()
{
    shutdown(m_ClientSocket, SD_BOTH);
    closesocket(m_ClientSocket);
    m_BytesRead = 0;
    m_ClientSocket = 0;
}

//==============================================================================
/**
 *	Receives a data packet from the socket.
 *	If m_WaitForFull is true, Recv will always return 0 until it reads m_BufferSize
 *	bytes.
 *	Else, it will return the number of bytes read (if any) per call.
 */
long SimpleTCPClientSocket::Recv()
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
long SimpleTCPClientSocket::Send()
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

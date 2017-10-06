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
#pragma once

#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
#define SOCKET int
#endif

#ifdef _PCPLATFORM
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

//==============================================================================
//	Creates a TCP socket to an address and port.
//  Can return either fixed sized packets or variable sized ones.
//==============================================================================
class SimpleTCPServerSocket
{
public:
    SimpleTCPServerSocket(long inBufferSize, bool inWaitForFull);
    ~SimpleTCPServerSocket();

public:
    static bool Initialize();
    static void DeInitialize();

    // Client side
    // bool Connect( const char* inServerAddress, const char* inServerPort );
    void Disconnect(bool inShutdown = false);

    bool Accept(const char *inServerAddress = 0, unsigned short inServerPort = 0);

    // General
    long Recv();
    long Send();

    // operator void*( ){ return m_Data; }
    void *GetData() { return m_Data; }

protected:
    bool SetNonBlocking(SOCKET inSocket);

protected:
    SOCKET m_ServerSocket;
    SOCKET m_ClientSocket;
    char *m_Data;
    long m_BytesRead;
    long m_BytesSent;
    const long m_BufferSize;
    bool m_WaitForFull;
    bool m_Initialized;
};

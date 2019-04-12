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
#include <termios.h>
#include <unistd.h>
#endif

#ifdef _PCPLATFORM
#include <conio.h>
#endif

#include <fcntl.h>
#include "TCPPerfLogServer.h"

//==============================================================================
/**
 *	CTOR
 *  Sets up listening server at specificed address and port.
 *  Opens log file to be ready for writing.
 */
TCPPerfLogServer::TCPPerfLogServer(const char *inServerAddress, unsigned short inServerPort,
                                   const char *inLogFilename)
    : m_LogFileHandle(0)
    , m_Socket(sizeof(SPerfLogData), true)
{
    m_LogFileHandle = ::fopen(inLogFilename, "w");
    m_Socket.Accept(inServerAddress, inServerPort);
}

//==============================================================================
/**
 *	DTOR
 *  Closes log file and disconnects server.
 */
TCPPerfLogServer::~TCPPerfLogServer()
{
    ::fclose(m_LogFileHandle);
    m_Socket.Disconnect(true);
}

//==============================================================================
/**
 *	Checks for any received packets and write them into the log file
 */
const char *TCPPerfLogServer::Update()
{
    if (m_Socket.Recv() > 0) {
        const char *theOutputMessage = Output();
        ::fputs(theOutputMessage, m_LogFileHandle);
        return theOutputMessage;
    }

    return NULL;
}

//==============================================================================
/**
 *	Formats receieved data into a text string.
 */
const char *TCPPerfLogServer::Output()
{
    SPerfLogData *thePerfLogData = static_cast<SPerfLogData *>(m_Socket.GetData());

    return FormatPerfLogData(thePerfLogData);
}

//==============================================================================
/**
 *	Keyboard hittest for Linux
 */
#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
int kbhit(void)
{
    return 0;
}
#else
#define kbhit _kbhit
#endif

//==============================================================================
/**
 *	Main
 */
int main(int argc, const char *argv[])
{
    const char *theServerIP = "10.0.0.1";
    const char *theLogFile = "perf.log";
    if (argc > 1)
        theServerIP = argv[1];

    if (argc > 2)
        theLogFile = argv[2];

    SimpleTCPServerSocket::Initialize();

    {
        ::printf("\nLog file: %s", theLogFile);
        ::printf("\nServer listening at: %s", theServerIP);

        ::printf("\nWaiting for client...");
        ::fflush(stdout);
        TCPPerfLogServer theServer(theServerIP, 9997, theLogFile);
        ::printf("\nClient connected...");
        ::fflush(stdout);

        while (true) {
            const char *theReceivedMessage = theServer.Update();
            if (theReceivedMessage)
                printf("\n%s", theReceivedMessage);

            if (kbhit()) {
#if defined(_LINUXPLATFORM) || defined(_INTEGRITYPLATFORM)
                char theKey = getchar();
#endif
#ifdef _PCPLATFORM
                char theKey = (char)_getch();
#endif
                if (theKey == 'q')
                    break;
            }
        }
    }

    SimpleTCPServerSocket::DeInitialize();
    return 0;
}

/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "Qt3DSCommonPrecompile.h"
#include "MsgRouter.h"
#include "StudioConst.h"
#include "Core.h"
#include "Doc.h"
#include "Views.h"

#include <QtCore/qcoreapplication.h>

static std::unique_ptr<CMsgRouter> static_theInstance;

//==============================================================================
/**
 * Constructor
 */
CMsgRouter::CMsgRouter()
{
    qApp->installEventFilter(this);
    m_eventType = QEvent::registerEventType();
}

//==============================================================================
/**
 * Destructor
 */
CMsgRouter::~CMsgRouter()
{
}

CMsgRouter *CMsgRouter::GetInstance()
{
    if (!static_theInstance) {
        static_theInstance.reset(new CMsgRouter);
    }

    return static_theInstance.get();
}

void CMsgRouter::blockMessages()
{
    qApp->removeEventFilter(this);
}

//==============================================================================
/**
 * Send a command to be executed asynchronously.
 */
void CMsgRouter::SendCommand(CCmd *inCommand, CCore *inCore)
{
    SMessageData *theMsgData = new SMessageData(m_eventType);
    theMsgData->Method = &CMsgRouter::OnCommand;
    theMsgData->Data = inCommand;
    theMsgData->Data2 = inCore;

    qApp->postEvent(qApp, theMsgData);
}

bool CMsgRouter::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == m_eventType) {
        SMessageData *theMsgData = reinterpret_cast<SMessageData *>(event);
        (this->*(theMsgData->Method))(theMsgData);
        return true;
    }

    return false;
}

//==============================================================================
/**
 * Windows entry point for processing generic messages.
 */
void CMsgRouter::OnAsyncNotification(SMessageData *inMessageData)
{
    try {
        CRoutedMessageBase *theRoutedMsg = static_cast<CRoutedMessageBase *>(inMessageData->Data);
        if (theRoutedMsg)
            theRoutedMsg->Notify();
    } catch (...) {
        // Catch crashes in case the object is gone
    }
}

//==============================================================================
/**
 * Main thread processing for processing command execution messages.
 */
void CMsgRouter::OnCommand(SMessageData *inMsgData)
{
    CCmd *theCommand = static_cast<CCmd *>(inMsgData->Data);
    CCore *theCore = static_cast<CCore *>(inMsgData->Data2);
    theCore->ExecuteCommand(theCommand, true);
}



CMsgRouter::SMessageData::SMessageData(int eventType) :
    QEvent(static_cast<QEvent::Type>(eventType))
{
}

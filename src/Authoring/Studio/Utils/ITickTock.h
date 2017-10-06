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
#pragma once
#ifndef ITICKTOCKH
#define ITICKTOCKH

namespace Q3DStudio {
class CString;
}

namespace UICDM {
class ISignalConnection;
}

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

typedef std::function<void()> TTickTockProc;

namespace Q3DStudio {
/**
 *	ITickTock is meant for a relatively small number of scheduled
 *	events that need to happen on the UI thread.  Clients can cancel
 *	any event simply by releasing the returned shared pointer.
 *	Interface is completely threadsafe and the returned signal connection
 *	may safely outlive the interface.
 */
class ITickTock
{
protected:
    virtual ~ITickTock() {}
    static ITickTock *m_Instance;

public:
    // The timer is canceled if the shared ptr deletes the signal connection.  Save to call from any
    // thread.
    // The callback, however, (inTickTockProc) will be activated solely from the UI thread.
    virtual std::shared_ptr<UICDM::ISignalConnection>
    AddTimer(unsigned long inTime, bool inIsPeriodic, TTickTockProc inTickTockProc,
             const Q3DStudio::CString &inName) = 0;

    // Called from UI thread to process all of the messages
    // that have happened in the timer thread.
    // Clients should not generally call this, it will be take care of for them.
    // In the current implementation, MainFrm.h processes the tick tock message
    // in order to call this function.
    virtual void ProcessMessages() = 0;

    friend class std::shared_ptr<ITickTock>;

    // m_Instance is set to the first tick tock created, and unset when that tick tock
    // goes away.
    static std::shared_ptr<ITickTock> CreateTickTock(long inMessageID, QWidget* inTarget);

    static ITickTock &GetInstance();
};
}

#endif

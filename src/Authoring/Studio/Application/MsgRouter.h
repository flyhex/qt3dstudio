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

//==============================================================================
// Prefix
//==============================================================================
#ifndef INCLUDED_MSG_ROUTER_H
#define INCLUDED_MSG_ROUTER_H 1
#pragma once

//==============================================================================
// Defines
//==============================================================================

//==============================================================================
// Includes
//==============================================================================
#include "StudioConst.h"

#include <QEvent>
#include <QObject>

//==============================================================================
// Forwards
//==============================================================================
class CCmd;
class CCore;

class CRoutedMessageBase
{
public:
    CRoutedMessageBase() {}
    virtual ~CRoutedMessageBase() {}

    virtual void Notify() = 0;
};

template <typename TObject, typename TData>
class CRoutedMessageImpl : public CRoutedMessageBase
{
protected:
    typedef void (TObject::*TMethod)(const TData &);

public:
    TObject *m_Object;
    TMethod m_Method;
    TData m_Data;

    CRoutedMessageImpl(TObject *inObject, TMethod inMethod, const TData &inData)
        : m_Object(inObject)
        , m_Method(inMethod)
        , m_Data(inData)
    {
    }

    virtual void Notify() { (m_Object->*m_Method)(m_Data); }
};

//==============================================================================
/**
 * Routes user-defined messages between different threads of an application.
 */
class CMsgRouter : public QObject
{
protected:
    class SMessageData;

public:
    typedef void (CMsgRouter::*TMainThreadMethod)(SMessageData *inMessageData);

public:
    static CMsgRouter *GetInstance();
    virtual ~CMsgRouter();

    void SendCommand(CCmd *inCommand, CCore *inCore);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

    CMsgRouter(); ///< This is a singleton so the constructor is not public (call GetInstance)

    class SMessageData : public QEvent
    {
    public:
        SMessageData(int eventType);

        TMainThreadMethod Method;
        void *Data;
        void *Data2;
    };

    void OnAsyncNotification(SMessageData *inMessageData);
    void OnCommand(SMessageData *inMessageData);

private:
    int m_eventType;
};

#endif // INCLUDED_MSG_ROUTER_H

/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifndef INCLUDED_TIME_EDIT_H
#define INCLUDED_TIME_EDIT_H 1

#pragma once

#include "Control.h"
#include "GenericFunctor.h"
#include "Multicaster.h"
#include "FloatEdit.h"
#include "EditInPlace.h"
#include "StringEdit.h"

#include  "IDoc.h"

GENERIC_FUNCTOR_1(CTimeEditChangeListener, OnTimeChanged, long);

class CTimeEdit : public CControl, public CCommitDataListener
{
public:
    CTimeEdit(IDoc *inDoc);
    virtual ~CTimeEdit();

    void SetMinimumTime(long inMinimumTime);
    void SetMaximumTime(long inMaximumTime);
    void SetBackgroundColor(const CColor &inColor);
    void SetTime(long inTime);
    virtual void Draw(CRenderer *inRenderer);
    long GetWidth();
    void AddTimeChangeListener(CTimeEditChangeListener *inListener);
    void RemoveTimeChangeListener(CTimeEditChangeListener *inListener);

    virtual void SetSize(CPt inSize);

    virtual void OnSetData(CControl *inControl);
    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags);
    long GetTime();

protected:
    long m_Time;
    IDoc *m_Doc;
    CEditInPlace<CFloatEdit> m_Minutes;
    CEditInPlace<CFloatEdit> m_Seconds;
    CEditInPlace<CFloatEdit> m_Millis;

    CColor m_BackgroundColor;

    long m_MinimumTime;
    long m_MaximumTime;

    CMulticaster<CTimeEditChangeListener *> m_TimeListeners;
};
#endif // INCLUDED_TIME_EDIT_H

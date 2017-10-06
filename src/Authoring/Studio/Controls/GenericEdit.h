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
//	Prefix
//==============================================================================
#ifndef INCLUDED_GENERIC_EDIT_H
#define INCLUDED_GENERIC_EDIT_H 1

//==============================================================================
//	Includes
//==============================================================================
#include "LazyFlow.h"
#include "Multicaster.h"
#include "CColor.h"

//==============================================================================
//	Forwards
//==============================================================================
class CRenderer;

//==============================================================================
//	Functors
//==============================================================================
/// Function to be called when the edit control changes
GENERIC_FUNCTOR(CGenericEditCommitListener, OnCommitData);
GENERIC_FUNCTOR(CGenericEditChangeListener, OnChangeData);
GENERIC_FUNCTOR(CGenericEditRevertListener, OnRevertData);

//==============================================================================
/**
 * Base class for most edit controls.  Provides functions for adding/removing/
 * notifying listeners when the control's value changes.  Implements some
 * common functionality across all edit controls.
 */
class CGenericEdit : public Q3DStudio::Control::CRuntimeLazyFlow
{
public:
    CGenericEdit();
    virtual ~CGenericEdit();

    void AddCommitListener(CGenericEditCommitListener *inListener);
    void RemoveCommitListener(CGenericEditCommitListener *inListener);
    void FireCommitDataEvent();

    void AddChangeListener(CGenericEditChangeListener *inListener);
    void RemoveChangeListener(CGenericEditChangeListener *inListener);
    void FireChangeDataEvent();

    void AddRevertListener(CGenericEditRevertListener *inListener);
    void RemoveRevertListener(CGenericEditRevertListener *inListener);
    void FireRevertDataEvent();

    void SetFillBackground(bool inFill);
    void SetBackgroundColor(const ::CColor &inColor);

    // CControl
    void Draw(CRenderer *inRenderer);

protected:
    CMulticaster<CGenericEditCommitListener *> m_CommitListeners;
    CMulticaster<CGenericEditChangeListener *> m_ChangeListeners;
    CMulticaster<CGenericEditRevertListener *> m_RevertListeners;
    bool m_FillBackground;
    ::CColor m_BackgroundColor;
};

#endif // INCLUDED_GENERIC_EDIT_H

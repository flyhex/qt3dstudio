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

//=============================================================================
// Prefix
//=============================================================================
#ifndef INCLUDED_OVERLAY_CONTROL_H
#define INCLUDED_OVERLAY_CONTROL_H 1

#pragma once

//=============================================================================
// Includes
//=============================================================================
#include "Control.h"

//=============================================================================
// Forwards
//=============================================================================

#include "BufferedRenderer.h"
#include "Qt3DSObjectCounter.h"

//=============================================================================
/**
 * Overlay control provides a way to a control on top of another and move it
 * around.  The drawing is handled so that you do not need to invalidate the
 * bottom control and you don't get trails to forming on the bottom control.
 * This is accomplished through the use of a buffered renderer.
 */
class COverlayControl : public CControl
{
public:
    COverlayControl();
    virtual ~COverlayControl();

    DEFINE_OBJECT_COUNTER(COverlayControl)

    virtual void SetSize(CPt inSize);
    virtual void SetPosition(CPt inPosition);
    virtual void OnDraw(CRenderer *inRenderer, CRct &inDirtyRect, bool inIgnoreValidation = false);
    virtual void BeginDrawChildren(CRenderer *inRenderer);
    virtual void SetVisible(bool inIsVisible);
    void SetAlpha(short inAlpha);
    virtual bool IsChildInvalidated() const;

protected:
    void DrawAlpha(CRenderer *inRenderer);

    CPt m_PositionOffset;
    Q3DStudio::CAutoMemPtr<CBufferedRenderer> m_BufferedRenderer;
    bool m_HasSizeChanged;
    CPt m_PreviousSize;
    bool m_WasVisible;
    short m_Alpha;
};
#endif // INCLUDED_OVERLAY_CONTROL_H

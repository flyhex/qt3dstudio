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

#ifndef INCLUDED_NAME_EDIT
#define INCLUDED_NAME_EDIT 1

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "TextEditInPlace.h"

class INamable;

//=============================================================================
/**
 * @class CNameEdit Edit control for statically displaying a "name" property.
 * Double-clicking on the text enables you to edit it.  Changes to the underlying
 * name property are automatically handled by this control.
 */
class CNameEdit : public CTextEditInPlace, public CCommitDataListener
{
public:
    CNameEdit(INamable *inNamable);
    virtual ~CNameEdit();
    void OnSetData(CControl *inControl) override;
    virtual void OnDirty();

    bool OnMouseDown(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnMouseUp(CPt inPoint, Qt::KeyboardModifiers inFlags) override;
    void OnGainFocus() override;
    void OnLoseFocus() override;

protected:
    INamable *m_Namable;
    bool m_IsSettingData;

    CPt m_LastMoustDown;
    bool m_LastFocused;
    bool m_CanEdit;
};

#endif // INCLUDED_NAME_EDIT

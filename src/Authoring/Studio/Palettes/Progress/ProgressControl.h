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
#ifndef INCLUDED_PROGRESS_CONTROL_H
#define INCLUDED_PROGRESS_CONTROL_H 1
#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "Control.h"
#include "ProgressCallback.h"

#include <QPixmap>

//==============================================================================
// Forwards
//==============================================================================
class CRenderer;
class CResImage;

//==============================================================================
/**
 * Top-level control of a loading progress window.
 */
class CProgressControl : public CControl, public IProgressCallback
{
public:
    CProgressControl();
    virtual ~CProgressControl();

    virtual CPt GetPreferredSize() override;
    virtual void Draw(CRenderer *inRenderer);
    virtual void SetActionText(const Q3DStudio::CString &inText);
    virtual void SetProgress(long inPercent);
    void SetFileName(const Q3DStudio::CString &inFileName);

protected:
    long m_Percent;
    QString m_ActionText;
    QString m_PercentString;
    QString m_FileName;
    QPixmap m_Image;
};

#endif // INCLUDED_PROGRESS_CONTROL_H

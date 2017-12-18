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
#ifndef INCLUDED_SPLASH_CONTROL_H
#define INCLUDED_SPLASH_CONTROL_H 1

#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "Control.h"

#include <QtGui/qpixmap.h>

//=============================================================================
/**
 * Class responsible for drawing contents of a splash screen at program start-up.
 */
class CSplashControl : public CControl
{
public:
    CSplashControl();
    virtual ~CSplashControl();

    void Draw(CRenderer *inRenderer) override;

protected:
    QPixmap m_Image;
    QString m_CopyrightLine1;
    QString m_CopyrightLine2;
    QString m_VersionInfo;
    long m_SpaceBetweenLines; ///< number of pixels between subsequent lines of the copyright
                              ///statement (calculated automatically)
};

#endif // INCLUDED_SPLASH_CONTROL_H

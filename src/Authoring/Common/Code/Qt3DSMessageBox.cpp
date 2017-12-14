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
#include "stdafx.h"
#include "Qt3DSMessageBox.h"

using namespace Q3DStudio;

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSMessageBox.h"

#include <QtWidgets/qmessagebox.h>

//==============================================================================
/**
 * Constructor
 */
Qt3DSMessageBox::Qt3DSMessageBox()
{
}

//==============================================================================
/**
 * Destructor
 */
Qt3DSMessageBox::~Qt3DSMessageBox()
{
}

//==============================================================================
/**
 * Displays the modal message box to the user.
 * @param inTitle Title of the message box
 * @param inText Text to be displayed on the message box
 * @param inIcon Icon to be displayed on the message box
 * @param inParentWindow window to attach this dialog to.
 */
Qt3DSMessageBox::EMessageBoxReturn
Qt3DSMessageBox::Show(const QString &inTitle, const QString &inText, EMessageBoxIcon inIcon,
                      bool inShowCancel /*false*/,
                      Qt3DSMessageBox::TPlatformWindow inParentWindow /*NULL*/)
{
    QMessageBox box(inParentWindow);
    box.setWindowTitle(inTitle);
    box.setText(inText);

    switch (inIcon) {
    case ICON_ERROR:
        box.setIcon(QMessageBox::Critical);
        break;
    case ICON_WARNING:
        box.setIcon(QMessageBox::Warning);
        break;
    case ICON_INFO:
        box.setIcon(QMessageBox::Information);
        break;
    default:
        break;
    }

    QMessageBox::StandardButtons buttons = QMessageBox::Ok;
    if (inShowCancel)
        buttons |= QMessageBox::Cancel;
    box.setStandardButtons(buttons);

    auto theButtonPressed = box.exec();
    return theButtonPressed == QMessageBox::Ok ? MSGBX_OK : MSGBX_CANCEL;
}

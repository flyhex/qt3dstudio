/****************************************************************************
**
** Copyright (C) 2006 NVIDIA Corporation.
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
//	Includes
//==============================================================================
#include "stdafx.h"
#include "GLVersionDlg.h"
#include "ui_GLVersionDlg.h"

#include <QStyle>

CGLVersionDlg::CGLVersionDlg(QWidget *pParent)
    : QDialog(pParent)
    , m_ui(new Ui::GLVersionDlg)
{
    m_ui->setupUi(this);
    setFixedSize(size());
}

CGLVersionDlg::~CGLVersionDlg()
{
}

void CGLVersionDlg::Initialize(const Q3DStudio::CString &inTitle,
                               const Q3DStudio::CString &inMessage, bool inErrorIcon)
{
    // Set title and message
    setWindowTitle(inTitle.toQString());
    m_ui->m_Message->setText(inMessage.toQString());

    // Set which icon to load
    const int size = style()->pixelMetric(QStyle::PM_LargeIconSize);
    m_ui->m_WarningIcon->setPixmap(style()->standardIcon(inErrorIcon ?
        QStyle::SP_MessageBoxCritical :
        QStyle::SP_MessageBoxWarning).pixmap(size, size));
}

bool CGLVersionDlg::GetDontShowAgain()
{
    return m_ui->m_DontShowAgain->isChecked();
}

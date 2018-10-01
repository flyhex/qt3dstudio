/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "EditPresentationIdDlg.h"
#include "ui_EditPresentationIdDlg.h"
#include "StudioApp.h"
#include "Core.h"

EditPresentationIdDlg::EditPresentationIdDlg(const QString &src, QWidget *parent)
    : QDialog(parent)
    , m_src(src)
    , m_ui(new Ui::EditPresentationIdDlg)
{
    m_ui->setupUi(this);

    m_presentationId = g_StudioApp.GetCore()->getProjectFile().getPresentationId(src);
    m_ui->lineEditPresentationId->setText(m_presentationId);
}

void EditPresentationIdDlg::accept()
{
    QString newId = m_ui->lineEditPresentationId->text();
    if (newId.isEmpty()) {
        g_StudioApp.showPresentationIdEmptyWarning();
    } else if (newId != m_presentationId) {
        if (!g_StudioApp.GetCore()->getProjectFile().isUniquePresentationId(newId, m_src)) {
            g_StudioApp.showPresentationIdUniqueWarning();
        } else {
            g_StudioApp.GetCore()->getProjectFile().writePresentationId(newId, m_src);
            QDialog::accept();
        }
    } else {
        QDialog::accept();
    }
}

EditPresentationIdDlg::~EditPresentationIdDlg()
{
    delete m_ui;
}

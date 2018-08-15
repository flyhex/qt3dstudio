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

    auto *sp = std::find_if(g_StudioApp.m_subpresentations.begin(),
                            g_StudioApp.m_subpresentations.end(),
                           [&src](const SubPresentationRecord &spr) -> bool {
                               return spr.m_argsOrSrc == src;
                           });

    if (sp != g_StudioApp.m_subpresentations.end()) {
        m_presentationId = sp->m_id;
        m_ui->lineEditPresentationId->setText(m_presentationId);
    }

    connect(m_ui->lineEditPresentationId, &QLineEdit::textEdited, this,
            &EditPresentationIdDlg::onChangePresentationId);
}

void EditPresentationIdDlg::onChangePresentationId()
{
    m_presentationId = m_ui->lineEditPresentationId->text();
}

void EditPresentationIdDlg::accept()
{
    if (!g_StudioApp.GetCore()->getProjectFile().isUniquePresentationId(m_presentationId, m_src)) {
        g_StudioApp.showPresentationIdUniqueWarning();
    } else {
        g_StudioApp.GetCore()->getProjectFile().writePresentationId(m_presentationId, m_src);
        QDialog::accept();
    }
}

EditPresentationIdDlg::~EditPresentationIdDlg()
{
    delete m_ui;
}

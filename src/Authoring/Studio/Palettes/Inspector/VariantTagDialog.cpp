/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "VariantTagDialog.h"
#include "ui_VariantTagDialog.h"
#include "Dialogs.h"
#include "StudioApp.h"
#include "Core.h"
#include "ProjectFile.h"

VariantTagDialog::VariantTagDialog(DialogType type, const QString &group, const QString &name,
                                   QWidget *parent)
    : QDialog(parent)
    , m_type(type)
    , m_group(group)
    , m_ui(new Ui::VariantTagDialog)
{
    m_ui->setupUi(this);

    m_names.first = name;

    if (type == AddGroup) {
        setWindowTitle(tr("Add new Group"));
        m_ui->label->setText(tr("Group name"));
    } else if (type == RenameGroup) {
        setWindowTitle(tr("Rename Group"));
        m_ui->label->setText(tr("Group name"));
        m_ui->lineEditTagName->setText(name);
        m_ui->lineEditTagName->selectAll();
    } else if (type == RenameTag) {
        m_ui->lineEditTagName->setText(name);
        m_ui->lineEditTagName->selectAll();
    }
}

void VariantTagDialog::accept()
{
    QString name = m_ui->lineEditTagName->text();

    if (name.isEmpty()) {
        displayWarning(EmptyWarning);
    } else if (name == m_names.first) { // no change
        QDialog::reject();
    } else if (((m_type == AddGroup || m_type == RenameGroup)
                && !g_StudioApp.GetCore()->getProjectFile().isVariantGroupUnique(name))
               || (!g_StudioApp.GetCore()->getProjectFile().isVariantTagUnique(m_group, name))) {
        displayWarning(UniqueWarning);
    } else {
        m_names.second = name;
        QDialog::accept();
    }
}

std::pair<QString, QString> VariantTagDialog::getNames() const
{
    return m_names;
}

void VariantTagDialog::displayWarning(WarningType warningType)
{
    QString warning;
    if (warningType == EmptyWarning) {
        if (m_type == AddGroup || m_type == RenameGroup)
            warning = tr("The group name must not be empty.");
        else
            warning = tr("The tag name must not be empty.");
    } else if (warningType == UniqueWarning) {
        if (m_type == AddGroup || m_type == RenameGroup)
            warning = tr("The group name must be unique.");
        else
            warning = tr("The tag name must be unique within the tag group.");
    }

    g_StudioApp.GetDialogs()->DisplayMessageBox(tr("Warning"), warning,
                                                Qt3DSMessageBox::ICON_WARNING, false);
}

VariantTagDialog::~VariantTagDialog()
{
    delete m_ui;
}

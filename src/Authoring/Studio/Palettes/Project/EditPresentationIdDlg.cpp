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
#include "Dialogs.h"

EditPresentationIdDlg::EditPresentationIdDlg(const QString &src, DialogType type, QWidget *parent)
    : QDialog(parent)
    , m_src(src)
    , m_ui(new Ui::EditPresentationIdDlg)
    , m_dialogType(type)
{
    m_ui->setupUi(this);

    switch (m_dialogType) {
    case EditPresentationId:
        m_ui->label->setText(tr("Presentation Id"));
        setWindowTitle(tr("Edit Presentation Id"));
        break;
    case EditQmlStreamId:
        m_ui->label->setText(tr("Qml Stream Id"));
        setWindowTitle(tr("Edit Qml Stream Id"));
        break;
    case EditPresentationName:
        m_ui->label->setText(tr("Presentation Name"));
        setWindowTitle(tr("Rename Presentation"));
        break;
    case EditQmlStreamName:
        m_ui->label->setText(tr("Qml Stream Name"));
        setWindowTitle(tr("Rename Qml Stream"));
        break;
    case DuplicatePresentation:
        m_ui->label->setText(tr("Presentation Name"));
        setWindowTitle(tr("Duplicate Presentation"));
        break;
    case DuplicateQmlStream:
        m_ui->label->setText(tr("Qml Stream Name"));
        setWindowTitle(tr("Duplicate Qml Stream"));
        break;
    default:
        break;
    }

    if (m_dialogType == EditPresentationId || m_dialogType == EditQmlStreamId) {
        m_presentationId = g_StudioApp.GetCore()->getProjectFile().getPresentationId(src);
        m_ui->lineEditPresentationId->setText(m_presentationId);
    } else {
        QFileInfo fi(src);
        QString initialText = fi.completeBaseName();
        if (m_dialogType == DuplicatePresentation || m_dialogType == DuplicateQmlStream)
            initialText = g_StudioApp.GetCore()->getProjectFile().getUniquePresentationName(src);
        m_ui->lineEditPresentationId->setText(initialText);
    }

    window()->setFixedSize(size());
}

void EditPresentationIdDlg::accept()
{
    QString newValue = m_ui->lineEditPresentationId->text();
    if (newValue.isEmpty()) {
        displayWarning(EmptyWarning);
    } else if (m_dialogType == EditPresentationId || m_dialogType == EditQmlStreamId) {
        if (newValue != m_presentationId) {
            if (!g_StudioApp.GetCore()->getProjectFile().isUniquePresentationId(newValue, m_src)) {
                displayWarning(UniqueWarning);
            } else {
                g_StudioApp.GetCore()->getProjectFile().writePresentationId(newValue, m_src);
                QDialog::accept();
            }
        } else {
            QDialog::accept();
        }
    } else { // editing name
        QFileInfo fi(m_src);
        QString suffix = QStringLiteral(".") + fi.suffix();
        if (!newValue.endsWith(suffix))
            newValue.append(suffix);

        if (newValue == suffix) {
            // If we are left with just the suffix, treat it as an empty name
            displayWarning(EmptyWarning);
        } else {
            int slashIndex = m_src.lastIndexOf(QLatin1Char('/'));
            if (slashIndex >= 0)
                newValue.prepend(m_src.left(slashIndex + 1));

            if (newValue != m_src) {
                bool success = false;
                if (m_dialogType == DuplicatePresentation || m_dialogType == DuplicateQmlStream) {
                    success = g_StudioApp.GetCore()->getProjectFile().duplicatePresentation(
                                m_src, newValue);
                    if (success) {
                        m_duplicateFile = g_StudioApp.GetCore()->getProjectFile()
                                .getAbsoluteFilePathTo(newValue);
                    }
                } else {
                    success = g_StudioApp.GetCore()->getProjectFile().renamePresentationFile(
                                m_src, newValue);
                }
                if (success)
                    QDialog::accept();
                else
                    displayWarning(UniqueWarning);
            } else {
                QDialog::accept();
            }
        }
    }
}

void EditPresentationIdDlg::displayWarning(WarningType warningType)
{
    QString warning;
    QString uniqueFileNote;
    if (warningType == UniqueWarning)
        uniqueFileNote = tr("The new name must be unique within its folder and a valid filename.");

    switch (m_dialogType) {
    // Presentation Id warnings are also displayed from preferences dialog, so they are handled
    // by CStudioApp.
    case EditPresentationId:
        if (warningType == EmptyWarning)
            g_StudioApp.showPresentationIdEmptyWarning();
        else
            g_StudioApp.showPresentationIdUniqueWarning();
        return;
    case EditQmlStreamId:
        if (warningType == EmptyWarning)
            warning = tr("Qml stream Id must not be empty.");
        else
            warning = tr("Qml stream Id must be unique.");
        break;
    case EditPresentationName:
        if (warningType == EmptyWarning)
            warning = tr("Presentation name must not be empty.");
        else
            warning = tr("Renaming presentation failed.\n") + uniqueFileNote;
        break;
    case EditQmlStreamName:
        if (warningType == EmptyWarning)
            warning = tr("Qml stream name must not be empty.");
        else
            warning = tr("Renaming Qml stream failed.\n") + uniqueFileNote;
        break;
    case DuplicatePresentation:
        if (warningType == EmptyWarning)
            warning = tr("Presentation name must not be empty.");
        else
            warning = tr("Duplicating presentation failed.\n") + uniqueFileNote;
        break;
    case DuplicateQmlStream:
        if (warningType == EmptyWarning)
            warning = tr("Qml stream name must not be empty.");
        else
            warning = tr("Duplicating Qml stream failed.\n") + uniqueFileNote;
        break;
    default:
        break;
    }

    g_StudioApp.GetDialogs()->DisplayMessageBox(tr("Warning"), warning,
                                                Qt3DSMessageBox::ICON_WARNING, false);
}

EditPresentationIdDlg::~EditPresentationIdDlg()
{
    delete m_ui;
}

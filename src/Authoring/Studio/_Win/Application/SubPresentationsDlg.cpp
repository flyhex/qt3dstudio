/****************************************************************************
**
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
//  Prefix
//==============================================================================
#include "stdafx.h"
#include "StudioDefs.h"
#include "Strings.h"
#include "StringLoader.h"

//==============================================================================
//  Includes
//==============================================================================
#include "SubPresentationsDlg.h"
#include "ui_SubPresentationsDlg.h"
#include "ProductInfo.h"
#include "HotKeys.h"
#include "Preferences.h"
#include "StudioPreferences.h"

#include <QtGui/qevent.h>
#include <QtGui/qpainter.h>
#include <QtCore/qtimer.h>
#include <QtWidgets/qfiledialog.h>

CSubPresentationsDlg::CSubPresentationsDlg(
        const QString &directory,
        const QVector<SubPresentationRecord> &subpresentations,
        QWidget *parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
    , m_directory(directory)
    , m_records(subpresentations)
    , m_ui(new Ui::SubPresentationsDlg)
{
    m_ui->setupUi(this);

    connect(m_ui->deleteButton, &QPushButton::clicked, this,
            &CSubPresentationsDlg::OnDeletePresentation);
    connect(m_ui->pushButtonNew, &QPushButton::clicked, this,
            &CSubPresentationsDlg::OnNewPresentation);
    connect(m_ui->pushButtonBrowse, &QPushButton::clicked, this,
            &CSubPresentationsDlg::OnBrowsePresentation);
    connect(m_ui->comboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &CSubPresentationsDlg::OnSelectPresentation);
    connect(m_ui->lineEditId, &QLineEdit::textEdited, this, &CSubPresentationsDlg::OnIdChanged);

    OnInitDialog();
}

CSubPresentationsDlg::~CSubPresentationsDlg()
{
    delete m_ui;
}

QVector<SubPresentationRecord> CSubPresentationsDlg::subpresentations() const
{
    return m_records;
}

void CSubPresentationsDlg::OnInitDialog()
{
    current = -1;
    if (m_records.size() > 0) {
        for (SubPresentationRecord rec : m_records)
            m_ui->comboBox->addItem(rec.m_id);
        current = 0;
        m_ui->lineEditId->setText(m_records[0].m_id);
        m_ui->lineEditPreview->setText(m_records[0].m_argsOrSrc);
    }
    m_ui->lineEditId->setEnabled(current != -1);
    m_ui->lineEditPreview->setReadOnly(true);
    m_ui->buttonBox->setFocusPolicy(Qt::NoFocus);
}

void CSubPresentationsDlg::OnSelectPresentation(int selectionIndex)
{
    if (selectionIndex >= 0 && selectionIndex < m_records.size()) {
        current = selectionIndex;
        m_ui->lineEditId->setEnabled(true);
        m_ui->lineEditId->setText(m_records[current].m_id);
        m_ui->lineEditPreview->setText(m_records[current].m_argsOrSrc);
    } else {
        m_ui->lineEditId->setEnabled(false);
        m_ui->lineEditId->setText(QString());
        m_ui->lineEditPreview->setText(QString());
    }
}

void CSubPresentationsDlg::OnDeletePresentation()
{
    if (current >= 0 && current < m_records.size()) {
        m_records.remove(current);
        m_ui->comboBox->removeItem(current);
        current = qMax(current - 1, 0);
        if (m_records.size() == 0)
            current = -1;
        OnSelectPresentation(current);
    }
}

void CSubPresentationsDlg::OnNewPresentation()
{
    int index = m_records.size();
    m_records.push_back(
                SubPresentationRecord(QStringLiteral("presentation-qml"),
                                      QStringLiteral("presentation-id"), QString()));
    m_ui->comboBox->addItem(QStringLiteral("presentation-id"));
    m_ui->comboBox->setCurrentIndex(index);
    OnSelectPresentation(index);
}

void CSubPresentationsDlg::OnBrowsePresentation()
{
    const QString file = QDir::toNativeSeparators(
                QFileDialog::getOpenFileName(nullptr, nullptr, m_directory,
                                             QStringLiteral("*.qml; *.uip")));
    QString shortFile = file;
    int subdir = file.indexOf(m_directory);
    if (subdir >= 0)
        shortFile.remove(subdir, m_directory.size() + 1);

    QFileInfo fileInfo(file);
    if (fileInfo.exists()) {
        if (fileInfo.suffix() == QStringLiteral("qml"))
            m_records[current].m_type = QStringLiteral("presentation-qml");
        else
        if (fileInfo.suffix() == QStringLiteral("uip"))
            m_records[current].m_type = QStringLiteral("presentation");

        m_records[current].m_argsOrSrc = shortFile;
        m_ui->lineEditPreview->setText(shortFile);
    }
}

void CSubPresentationsDlg::OnIdChanged(const QString &text)
{
    if (current != -1) {
        m_records[current].m_id = text;
        m_ui->comboBox->setItemText(current, text);
    }
}

void CSubPresentationsDlg::on_buttonBox_accepted()
{
    QDialog::accept();
}

void CSubPresentationsDlg::on_buttonBox_rejected()
{
    QDialog::reject();
}

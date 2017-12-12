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

#include "SubPresentationDlg.h"
#include "ui_SubPresentationDlg.h"

#include <QtWidgets/qabstractbutton.h>
#include <QtWidgets/qfiledialog.h>
#include <QtCore/qdir.h>
#include <QtWidgets/qabstractbutton.h>

CSubPresentationDlg::CSubPresentationDlg(const QString &directory,
                                         const SubPresentationRecord &subpresentation,
                                         QWidget *parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
    , m_ui(new Ui::SubPresentationDlg)
    , m_subPresentation(subpresentation)
    , m_directory(directory)
{
    m_ui->setupUi(this);

    initDialog();

    connect(m_ui->lineEditId, &QLineEdit::textChanged, this,
            &CSubPresentationDlg::onIdChanged);
    connect(m_ui->comboBoxTypeList,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &CSubPresentationDlg::onTypeChanged);
    connect(m_ui->comboBoxFileList,
            static_cast<void(QComboBox::*)(int)>(&QComboBox::activated),
            this, &CSubPresentationDlg::onFileChanged);
}

CSubPresentationDlg::~CSubPresentationDlg()
{
    delete m_ui;
}

SubPresentationRecord CSubPresentationDlg::subpresentation()
{
    return m_subPresentation;
}

void CSubPresentationDlg::initDialog()
{
    QStringList types;
    types << tr("QML Stream") << tr("Presentation");
    m_ui->comboBoxTypeList->addItems(types);
    m_ui->comboBoxTypeList->setCurrentIndex(m_subPresentation.m_type
                                            == QStringLiteral("presentation-qml") ? 0 : 1);
    m_ui->lineEditId->setText(m_subPresentation.m_id);
    updateUI();
}

void CSubPresentationDlg::on_buttonBox_accepted()
{
    if (m_ui->comboBoxFileList->currentText() != tr("Browse..."))
        m_subPresentation.m_argsOrSrc = m_ui->comboBoxFileList->currentText();
    QDialog::accept();
}

void CSubPresentationDlg::on_buttonBox_rejected()
{
    QDialog::reject();
}

void CSubPresentationDlg::onTypeChanged(int index)
{
    m_subPresentation.m_type = index ? QStringLiteral("presentation")
                                     : QStringLiteral("presentation-qml");

    // Remove possible illegal file from argsOrSrc
    if ((m_subPresentation.m_type == QStringLiteral("presentation")
            && m_subPresentation.m_argsOrSrc.endsWith(QStringLiteral(".qml")))
            || (m_subPresentation.m_type == QStringLiteral("presentation-qml")
            && m_subPresentation.m_argsOrSrc.endsWith(QStringLiteral(".uip")))) {
        m_subPresentation.m_argsOrSrc.clear();
    }

    updateUI();
    // Disable "Ok" if type is presentation and no file has been selected
    m_ui->buttonBox->buttons()[0]->setEnabled(
                !(m_subPresentation.m_type == QStringLiteral("presentation")
                && m_ui->comboBoxFileList->currentText() == tr("Browse...")));
}

void CSubPresentationDlg::onFileChanged(int index)
{
    if (index != m_ui->comboBoxFileList->count() - 1)
        m_subPresentation.m_argsOrSrc = m_ui->comboBoxFileList->currentText();
    else
        browseFile();
    // Disable "Ok" if type is presentation and no file has been selected
    m_ui->buttonBox->buttons()[0]->setEnabled(
                !(m_subPresentation.m_type == QStringLiteral("presentation")
                && m_ui->comboBoxFileList->currentText() == tr("Browse...")));
}

void CSubPresentationDlg::onIdChanged(const QString &id)
{
    m_subPresentation.m_id = id;
}

void CSubPresentationDlg::updateUI() {
    m_ui->comboBoxFileList->clear();

    // Populate file combobox with current uip/qml + folder's uips/qmls + "Browse..."
    // and select the current uip/qml (or browse if none)
    QString filter = QStringLiteral("*.uip");
    if (m_subPresentation.m_type == QStringLiteral("presentation-qml"))
        filter = QStringLiteral("*.qml");

    QDir dir(m_directory, filter, QDir::Name, QDir::Files);
    m_ui->comboBoxFileList->addItems(dir.entryList());

    if (!m_subPresentation.m_argsOrSrc.isEmpty()) {
        // Do not add the current file to the combobox if it is already there
        if (m_ui->comboBoxFileList->findText(m_subPresentation.m_argsOrSrc) == -1)
            m_ui->comboBoxFileList->addItem(m_subPresentation.m_argsOrSrc);
        m_ui->comboBoxFileList->setCurrentText(m_subPresentation.m_argsOrSrc);
    }

    m_ui->comboBoxFileList->addItem(tr("Browse..."));
}

void CSubPresentationDlg::browseFile()
{
    QString filter = QStringLiteral("*.uip");
    if (m_subPresentation.m_type == QStringLiteral("presentation-qml"))
        filter = QStringLiteral("*.qml");
    const QString file = QDir::toNativeSeparators(
                QFileDialog::getOpenFileName(nullptr, nullptr, m_directory, filter));
    QString shortFile = file;
    int subdir = file.indexOf(m_directory);
    if (subdir >= 0)
        shortFile.remove(subdir, m_directory.size() + 1);

    QFileInfo fileInfo(file);
    if (fileInfo.exists()) {
        m_subPresentation.m_argsOrSrc = shortFile;
        m_ui->comboBoxFileList->insertItem(-1, shortFile);
        m_ui->comboBoxFileList->setCurrentText(shortFile);
    }
}

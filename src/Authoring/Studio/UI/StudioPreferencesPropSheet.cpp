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

#include "ui_StudioPreferencesPropSheet.h"

#include "StudioPreferences.h"
#include "StudioPreferencesPropSheet.h"
#include "StudioProjectSettingsPage.h"

#include <QtWidgets/qpushbutton.h>

CStudioPreferencesPropPage::CStudioPreferencesPropPage(QWidget *parent)
    : QWidget(parent)
{
}

void CStudioPreferencesPropPage::setModified(bool modified)
{
    setProperty("modified", modified);

    auto s = sheet();
    if (s) {
        auto buttons = s->findChild<QDialogButtonBox *>();
        bool anyModified = false;
        for (auto page : s->findChildren<CStudioPreferencesPropPage *>())
            anyModified |= page->property("modified").toBool();
        buttons->button(QDialogButtonBox::Apply)->setEnabled(anyModified);
    }
}

CStudioPreferencesPropSheet* CStudioPreferencesPropPage::sheet()
{
    QWidget *parent = parentWidget();
    while (parent != nullptr) {
        if (auto sheet = qobject_cast<CStudioPreferencesPropSheet *>(parent))
            return sheet;
        parent = parent->parentWidget();
    }
    return nullptr;
}


void CStudioPreferencesPropPage::endDialog(int returnCode)
{
    auto s = sheet();
    if (s)
        s->done(returnCode);
}

CStudioPreferencesPropSheet::CStudioPreferencesPropSheet(const QString &pszCaption,
                                                         QWidget *pParentWnd,
                                                         int iSelectPage)
    : QDialog(pParentWnd)
    , m_ui(new Ui::StudioPreferencesPropSheet)
{
    setWindowTitle(pszCaption);
    onInitDialog();
    m_ui->m_TabCtrl->setCurrentIndex(iSelectPage);
}

CStudioPreferencesPropSheet::~CStudioPreferencesPropSheet()
{
}

void CStudioPreferencesPropSheet::onInitDialog()
{
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

    m_Font = QFont(CStudioPreferences::GetFontFaceName(), 8);
    setFont(m_Font);

    connect(m_ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &CStudioPreferencesPropSheet::apply);
}

bool CStudioPreferencesPropSheet::apply()
{
    for (auto page : findChildren<CStudioPreferencesPropPage *>()) {
        if (!page->onApply())
            return false;
    }
    return true;
}

void CStudioPreferencesPropSheet::accept()
{
    if (apply())
        QDialog::accept();
}

void CStudioPreferencesPropSheet::reject()
{
    for (auto page : findChildren<CStudioPreferencesPropPage *>())
        page->onCancel();
    QDialog::reject();
}

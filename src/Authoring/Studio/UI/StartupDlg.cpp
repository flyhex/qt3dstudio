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

#include "Qt3DSCommonPrecompile.h"
#include "StudioDefs.h"
#include "StartupDlg.h"
#include "StudioPreferences.h"
#include "ui_StartupDlg.h"

#include <QtCore/qfileinfo.h>
#include <QtGui/qpalette.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qdir.h>
#include <QtWidgets/qdesktopwidget.h>

// CStartupDlg dialog

CStartupDlg::CStartupDlg(QWidget *pParent)
    : QDialog(pParent, Qt::MSWindowsFixedSizeDialogHint)
    , m_Choice(EStartupChoice_Invalid)
    , m_ui(new Ui::StartupDlg)
    , m_palette(nullptr)
{
    m_ui->setupUi(this);
}

CStartupDlg::~CStartupDlg()
{
    delete m_palette;
}

static QString GetFileTimeReadable(const Qt3DSFile &inFile)
{
    QFileInfo finfo(inFile.GetAbsolutePath().toQString());
    if (!finfo.exists())
        return {};

    return finfo.lastModified().toString("MM/dd/yyyy");
}

void CStartupDlg::showEvent(QShowEvent *ev)
{
    OnInitDialog();
    QDialog::showEvent(ev);
}

void CStartupDlg::reject()
{
    m_Choice = EStartupChoice_Exit;
    QDialog::reject();
}

void CStartupDlg::OnInitDialog()
{
    QWidget *p = qobject_cast<QWidget *>(parent());
    if (p) {
        QRect pRect;
        if (p->isMaximized()) {
            pRect = QApplication::desktop()->availableGeometry(
                        QApplication::desktop()->screenNumber(p));
        } else {
            pRect = p->frameGeometry();
        }

        move(pRect.x() + pRect.width() / 2 - width() / 2,
             pRect.y() + pRect.height() / 2 - height() / 2);
    }

    connect(m_ui->newDocument, &ClickableLabel::clicked, this, &CStartupDlg::OnNewDocClicked);
    connect(m_ui->openDocument, &ClickableLabel::clicked, this, &CStartupDlg::OnOpenDocClicked);

    // Load the product version
    m_ProductVersionStr = QStringLiteral("Qt 3D Studio v") + CStudioPreferences::GetVersionString();
    m_ui->versionStr->setText(m_ProductVersionStr);

    // Populate the recent document list
    for (uint theIndex = 0; theIndex < RECENT_COUNT; ++theIndex) {
        ClickableLabel *recent
                = findChild<ClickableLabel *>(QStringLiteral("recent%1").arg(theIndex));
        connect(recent, &ClickableLabel::clicked, this, &CStartupDlg::OnStnClickedStartupRecent);

        recent->setProperty("recentIndex", theIndex);

        if (m_RecentDocs.size() > theIndex) {
            // Set the name
            recent->setText(QFileInfo(m_RecentDocs[theIndex]).fileName());
            // Set path and date to tooltip
            QString toolTip = m_RecentDocs[theIndex];
            toolTip.append(QStringLiteral("\n"));
            toolTip.append(GetFileTimeReadable(m_RecentDocs[theIndex]));
            recent->setToolTip(toolTip);
        } else {
            recent->setEnabled(false);
            recent->setText(QString());
        }
    }
}

void CStartupDlg::AddRecentItem(const QString &inRecentItem)
{
    m_RecentDocs.push_back(inRecentItem);
}

CStartupDlg::EStartupChoice CStartupDlg::GetChoice()
{
    return m_Choice;
}

QString CStartupDlg::GetRecentDoc() const
{
    return m_RecentDocSelected;
}

void CStartupDlg::OnNewDocClicked()
{
    m_Choice = EStartupChoice_NewDoc;
    QDialog::accept();
}

void CStartupDlg::OnOpenDocClicked()
{
    m_Choice = EStartupChoice_OpenDoc;
    QDialog::accept();
}

void CStartupDlg::OnStnClickedStartupRecent()
{
    const int index = sender()->property("recentIndex").toInt();
    OpenRecent(index);
}

void CStartupDlg::OpenRecent(size_t inIndex)
{
    if (inIndex < m_RecentDocs.size()) {
        m_RecentDocSelected = m_RecentDocs[inIndex];
        m_Choice = EStartupChoice_OpenRecent;
        QDialog::accept();
    }
}

void CStartupDlg::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (m_palette)
        return;

    m_palette = new QPalette;
    QPixmap pic = QPixmap(":/startup/open_dialog.png");
    m_palette->setBrush(QPalette::Window, pic);
    setPalette(*m_palette);
    resize(pic.size());
    setFixedSize(size());
}

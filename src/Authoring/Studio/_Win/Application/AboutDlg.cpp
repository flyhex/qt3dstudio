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
#include "StudioDefs.h"
#include "Strings.h"
#include "StringLoader.h"
#include "qtAuthoring-config.h"

//==============================================================================
//	Includes
//==============================================================================
#include "AboutDlg.h"
#include "ui_AboutDlg.h"
#include "StudioPreferences.h"

#include <QtGui/qpalette.h>

//==============================================================================
/**
 * Constructor: Initializes the object.
 */
CAboutDlg::CAboutDlg(QWidget* parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint | Qt::FramelessWindowHint)
    , m_ui(new Ui::AboutDlg)
    , m_palette(nullptr)
{
    m_ui->setupUi(this);
    OnInitDialog();
}

//==============================================================================
/**
 * Destructor: Releases the object.
 */
CAboutDlg::~CAboutDlg()
{
    delete m_palette;
}

void CAboutDlg::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    if (m_palette)
        return;

    m_palette = new QPalette;
    QPixmap pic = QPixmap(":/startup/open_dialog.png");
    pic.setDevicePixelRatio(devicePixelRatio());
    m_palette->setBrush(QPalette::Window, pic);
    setPalette(*m_palette);
    resize(pic.size());
    setFixedSize(size());
}

void CAboutDlg::OnInitDialog()
{
    // Set the Studio version
    m_ProductVersionStr.Format(
        ::LoadResourceString(IDS_UIC_STUDIO_VERSION),
        static_cast<const wchar_t *>(CStudioPreferences::GetVersionString()));

    // Set the copyright string
    m_CopyrightStr.Format(::LoadResourceString(IDS_ABOUT_COPYRIGHT),
                          static_cast<const wchar_t *>(Q3DStudio::CString(STUDIO_COPYRIGHT_YEAR)));

    // Set the credit strings
#ifdef QT_3DSTUDIO_FBX
    m_Credit1Str.Format(::LoadResourceString(IDS_ABOUT_FBX_CREDIT));
#endif

#ifdef STUDIOSTORYNUM
    m_ProductVersionStr += " (Story #";
    m_ProductVersionStr += STUDIOSTORYNUM;
    m_ProductVersionStr += ")";
#endif

#ifdef BETA
    // Add "beta" to the Studio version if necessary
    m_ProductVersionStr += " BETA";
#endif

    // Add link to Web site
    Q3DStudio::CString theURL(::LoadResourceString(IDS_HELP_VISIT_QT));

    m_ui->m_WebSite->setText(QString("<a href=\"%1\"><font color=\"#%2\">%3</font></a>").arg(
                                 theURL.toQString(),
                                 CStudioPreferences::GetMasterColor().GetString().toQString(),
                                 theURL.toQString()));
    m_ui->m_WebSite->setToolTip(::LoadResourceString(IDS_WEBSITELINK).toQString());
    m_ui->m_WebSite->setOpenExternalLinks(true);

    // Add link to support address
    Q3DStudio::CString theSupport;

    theSupport = ::LoadResourceString(IDS_SUPPORTEMAIL);

    m_ui->m_Email->setText(QString("<a href=\"%1\"><font color=\"#%2\">%3</font></a>").arg(
                               theSupport.toQString(),
                               CStudioPreferences::GetMasterColor().GetString().toQString(),
                               theSupport.toQString()));
    m_ui->m_Email->setToolTip(::LoadResourceString(IDS_SUPPORTEMAIL_TEXT).toQString());
    m_ui->m_Email->setOpenExternalLinks(true);

    // Make the font bold for version number
    m_ui->m_ProductVersion->setStyleSheet("font-weight: bold;");

    m_ui->m_ProductVersion->setText(m_ProductVersionStr.toQString());
    m_ui->m_Copyright->setText(m_CopyrightStr.toQString());
    m_ui->m_Credit1->setText(m_Credit1Str.toQString());

    // We don't currently have secondary credit, so clear that
    m_ui->m_Credit2->setText(QString());
}

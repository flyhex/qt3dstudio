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
#include "qtAuthoring-config.h"

//==============================================================================
//	Includes
//==============================================================================
#include "AboutDlg.h"
#include "ui_AboutDlg.h"
#include "StudioPreferences.h"

#include <QtGui/qpalette.h>

#ifdef QT3DSTUDIO_REVISION
#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)
const char *const QT3DSTUDIO_REVISION_STR = STRINGIFY(QT3DSTUDIO_REVISION);
#endif

//==============================================================================
/**
 * Constructor: Initializes the object.
 */
CAboutDlg::CAboutDlg(QWidget* parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
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
    m_palette->setBrush(QPalette::Window, pic);
    setPalette(*m_palette);
    resize(pic.size());
    setFixedSize(size());
}

static QString compilerString()
{
#if defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
    QString isAppleString;
#if defined(__apple_build_version__) // Apple clang has other version numbers
    isAppleString = QStringLiteral(" (Apple)");
#endif
    return QStringLiteral("Clang " ) + QString::number(__clang_major__) + QStringLiteral(".")
            + QString::number(__clang_minor__) + isAppleString;
#elif defined(Q_CC_GNU)
    return QStringLiteral("GCC " ) + QStringLiteral(__VERSION__);
#elif defined(Q_CC_MSVC)
    if (_MSC_VER > 1999)
        return QStringLiteral("MSVC <unknown>");
    if (_MSC_VER >= 1910)
        return QStringLiteral("MSVC 2017");
    if (_MSC_VER >= 1900)
        return QStringLiteral("MSVC 2015");
#endif
    return QStringLiteral("<unknown compiler>");
}

void CAboutDlg::OnInitDialog()
{
    // Set the Studio version
    m_ProductVersionStr = QStringLiteral("Qt 3D Studio v") + CStudioPreferences::GetVersionString();

    // Set the copyright string
    m_CopyrightStr = QObject::tr("Copyright (C) %1 The Qt Company. All rights reserved.").arg(
                QString(STUDIO_COPYRIGHT_YEAR));

    // Set the credit strings
#ifdef QT_3DSTUDIO_FBX
    m_Credit1Str = QObject::tr("This software contains Autodesk(R) FBX(R) code developed by "
                               "Autodesk, Inc. Copyright 2014 Autodesk, Inc. All rights, reserved. "
                               "Such code is provided 'as is' and Autodesk, Inc. disclaims any "
                               "and all warranties, whether express or implied, including without "
                               "limitation the implied warranties of merchantability, fitness for "
                               "a particular purpose or non-infringement of third party rights. "
                               "In no event shall Autodesk, Inc. be liable for any direct, "
                               "indirect, incidental, special, exemplary, or consequential "
                               "damages (including, but not limited to, procurement of "
                               "substitute goods or services; loss of use, data, or profits; or "
                               "business interruption) however caused and on any theory of "
                               "liability, whether in contract, strict liability, or tort "
                               "(including negligence or otherwise) arising in any way out of "
                               "such code.");
#endif

    // Add link to Web site
    QString theURL(QStringLiteral("https://www.qt.io/3d-studio"));

    m_ui->m_WebSite->setText(QString("<a href=\"%1\"><font color=\"#%2\">%3</font></a>").arg(
                                 theURL,
                                 CStudioPreferences::GetMasterColor().name(),
                                 theURL));
    m_ui->m_WebSite->setToolTip(tr("Click to visit Qt web site"));
    m_ui->m_WebSite->setOpenExternalLinks(true);

    // Add link to support address
    const QString theSupport = QStringLiteral("https://account.qt.io/support");

    m_ui->m_Email->setText(QString("<a href=\"%1\"><font color=\"#%2\">%3</font></a>").arg(
                               theSupport,
                               CStudioPreferences::GetMasterColor().name(),
                               theSupport));
    m_ui->m_Email->setToolTip(tr("Send a Studio support request to the Qt Company"));
    m_ui->m_Email->setOpenExternalLinks(true);

    // Make the font bold for version number
    m_ui->m_ProductVersion->setStyleSheet("font-weight: bold;");

    m_ui->m_ProductVersion->setText(m_ProductVersionStr);
    m_ui->m_Copyright->setText(m_CopyrightStr);
    m_ui->m_Credit1->setText(m_Credit1Str);

    // Information about build
    m_ui->m_buildTimestamp->setText(
                tr("Built on %1 %2").arg(QStringLiteral(__DATE__), QStringLiteral(__TIME__)));
    m_ui->m_qtVersion->setText(
                tr("Based on Qt %1 (%2, %3 bit)").arg(
                    QString::fromLatin1(qVersion()),
                    compilerString(),
                    QString::number(QSysInfo::WordSize)));
#ifdef QT3DSTUDIO_REVISION
    m_ui->m_revisionSHA->setText(
                tr("From revision %1").arg(
                    QString::fromLatin1(QT3DSTUDIO_REVISION_STR).left(10)));
#else
    m_ui->m_revisionSHA->setText(QString());
    m_ui->m_revisionSHA->setMaximumHeight(0);
#endif
}

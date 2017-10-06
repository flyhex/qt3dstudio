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
#include "stdafx.h"
#include "qtAuthoring-config.h"
#include "StudioDefs.h"
#include "Strings.h"
#include "StringLoader.h"

//==============================================================================
//	Includes
//==============================================================================
#include "AboutDlg.h"
#include "ui_AboutDlg.h"
#include "ProductInfo.h"
#include "HotKeys.h"
#include "Preferences.h"
#include "StudioPreferences.h"

#include <QMouseEvent>
#include <QPainter>
#include <QTimer>

//==============================================================================
//	Constants
//==============================================================================
const int TIMER_RESIZEABOUT = 6;
const int TIMER_FADEOUT = 7;

//==============================================================================
/**
 * Constructor: Initializes the object.
 */
CAboutDlg::CAboutDlg(QWidget* parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint)
    , m_ScrollingCreditsFlag(false)
    , m_ui(new Ui::AboutDlg)
{
    m_ui->setupUi(this);
    m_IconRect = m_ui->icon->pixmap()->rect();
    setFixedSize(size());

    m_Font = QFont(CStudioPreferences::GetFontFaceName());
    m_Font.setPointSizeF(7.8);
    m_Font = QFont(CStudioPreferences::GetFontFaceName());
    m_Font.setPointSizeF(8.8);

    m_Color_Background = CStudioPreferences::GetDarkBaseColor();
    m_Color_Text = CStudioPreferences::GetNormalColor();

    OnInitDialog();
}

//==============================================================================
/**
 * Destructor: Releases the object.
 */
CAboutDlg::~CAboutDlg()
{
}

//==============================================================================
/**
 * Handles the painting of the icon in the About Box.
 * All additional painting (text, etc) occurs by default by MFC.
 */
//==============================================================================
void CAboutDlg::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (!m_ScrollingCreditsFlag) {
        DrawFadeRect();
    }
}

//==============================================================================
/**
 * Completes the construction of the About Box.
 * Stores the product version in the member variable associated with the static
 * text box in the dialog template. This version information comes from the
 * VersionNumber.h file in $/Studio/Build.
 * @return Returns TRUE always.
 */
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
    m_Credit1Str.Format(::LoadResourceString(IDS_ABOUT_PAINTLIB_CREDIT));
#ifdef QT_3DSTUDIO_FBX
    m_Credit2Str.Format(::LoadResourceString(IDS_ABOUT_FBX_CREDIT));
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

    m_ui->m_WebSite->setText(QString("<a href=\"%1\">%2</a>").arg(theURL.toQString(), theURL.toQString()));
    m_ui->m_WebSite->setToolTip(::LoadResourceString(IDS_WEBSITELINK).toQString());
    m_ui->m_WebSite->setOpenExternalLinks(true);

    // Add link to support email address
    Q3DStudio::CString theEmail;
    Q3DStudio::CString theEmailSansProtocol;

    theEmail = ::LoadResourceString(IDS_SUPPORTEMAIL);
    theEmailSansProtocol = "support@qt.io";

    m_ui->m_Email->setText(QString("<a href=\"%1\">%2</a>").arg(theEmail.toQString(), theEmailSansProtocol.toQString()));
    m_ui->m_Email->setToolTip(::LoadResourceString(IDS_SUPPORTEMAIL_TEXT).toQString());
    m_ui->m_Email->setOpenExternalLinks(true);

    // Set the fonts
    m_ui->m_Copyright->setFont(m_Font);
    m_ui->m_Credit1->setFont(m_Font);
    m_ui->m_Credit2->setFont(m_Font);
    m_ui->m_WebSiteLabel->setFont(m_Font);
    m_ui->m_WebSite->setFont(m_Font);
    m_ui->m_EmailLabel->setFont(m_Font);
    m_ui->m_Email->setFont(m_Font);

    // Make the font bold for version numbers
    m_ui->m_ProductVersion->setFont(m_BoldFont);

    // Create brush for background color
    m_Brush = QBrush(m_Color_Background);

    m_ui->m_ProductVersion->setText(m_ProductVersionStr.toQString());
    m_ui->m_Copyright->setText(m_CopyrightStr.toQString());
    m_ui->m_Credit1->setText(m_Credit1Str.toQString());
    m_ui->m_Credit2->setText(m_Credit2Str.toQString());

    // Hide the email link until we have a valid one.
    m_ui->m_EmailLabel->setVisible(false);
    m_ui->m_Email->setVisible(false);
}

//==============================================================================
/**
 * Handles the WM_LBUTTONDBLCLK message
 * Check to see if the mouse is within the Qt logo and the control key is pressed.
 * If so, begin the transition to show the detailed credits.
 */
void CAboutDlg::mouseDoubleClickEvent(QMouseEvent* event)
{
    // If the mouse's left button is double-clicked in the Qt logo while the control key is
    // pressed...
    if (m_IconRect.contains(event->pos())) {
        // if ( CHotKeys::IsKeyDown( VK_CONTROL ) )
        //{
        // Start the transition to display the scrolling credits
        // Crashing...			this->BeginCreditsTransition();
        //}
    }

    QDialog::mouseDoubleClickEvent(event);
}

//==============================================================================
/**
 * BeginCreditsTransition: Begin the transition from the About box to the scrolling credits.
 */
void CAboutDlg::BeginCreditsTransition()
{
    m_ScrollingCreditsFlag = TRUE;

    // Hide all controls
    for (QWidget* w : findChildren<QWidget*>())
        w->setVisible(false);

    // Invalidate the window
    update();

    // Set the resizing timer
    QTimer::singleShot(50, this, &CAboutDlg::ResizeAbout);
}

//==============================================================================
/**
 * Resize the about box height until the Client area is square.
 */
void CAboutDlg::ResizeAbout()
{
    bool retriggerResizeAbout = true;

    QRect theClientRect = rect();
    bool theRightSize = false;

    // this is just a sizing stuff to show some transition on size
    if (theClientRect.height() < theClientRect.width()) {
        theClientRect.setBottom(theClientRect.bottom() + 5);
        if (theClientRect.bottom() >= theClientRect.width()) {
            theRightSize = true;
            theClientRect.setBottom(theClientRect.width());
            retriggerResizeAbout = false;
        }
    } else {
        theClientRect.setRight(theClientRect.right() + 5);
        if (theClientRect.right() >= theClientRect.height()) {
            theRightSize = true;
            theClientRect.setRight(theClientRect.height());
            retriggerResizeAbout = false;
        }
    }

    if (parentWidget())
        theClientRect.moveCenter(parentWidget()->geometry().center());
    else
        theClientRect.moveTopLeft(geometry().topLeft());

    setGeometry(theClientRect);

    if (theRightSize) {
        QColor the3DColor;
        short theColorValue;

        m_CreditsRect = rect().adjusted(3, 3, -3, -3);

        the3DColor = palette().color(QPalette::Base);
        theColorValue =
            (short)((the3DColor.red() + the3DColor.green() + the3DColor.blue()) / 3);

        m_ColorFade = RGB(theColorValue, theColorValue, theColorValue);

        QTimer::singleShot(50, this, &CAboutDlg::FadeOut);
    }

    if (retriggerResizeAbout) {
        QTimer::singleShot(50, this, &CAboutDlg::ResizeAbout);
    }
}

//==============================================================================
/**
 * Fade out a rectangle in the client area, from the background 3D color to black.
 */
void CAboutDlg::FadeOut()
{
    update();

    // Continue fading to black
    if (m_ColorFade != Qt::black) {
        short theColorValue;

        theColorValue = (short)(m_ColorFade.red() - 10);

        if (theColorValue < 0)
            theColorValue = 0;

        m_ColorFade = qRgb(theColorValue, theColorValue, theColorValue);
        QTimer::singleShot(50, this, &CAboutDlg::FadeOut);
    }
}

//==============================================================================
/**
 *	DrawFadeRect: Draw the faded rectangle as we transition to the scrolling credits.
 */
void CAboutDlg::DrawFadeRect()
{
    QPainter painter(this);
    painter.fillRect(m_CreditsRect, m_ColorFade);
}

void CAboutDlg::OnStnClickedAboutboxProdver()
{
    // TODO: Add your control notification handler code here
}

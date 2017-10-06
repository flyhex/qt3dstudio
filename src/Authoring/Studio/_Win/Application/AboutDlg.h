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

#ifndef INCLUDED_ABOUT_DLG_H
#define INCLUDED_ABOUT_DLG_H 1

#pragma once

#include <QDialog>

#include "UICString.h"

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
    class AboutDlg;
}
QT_END_NAMESPACE

class CAboutDlg : public QDialog
{
    Q_OBJECT
public:
    CAboutDlg(QWidget* parent = nullptr);
    ~CAboutDlg();

    // Implementation
protected:
    bool m_ScrollingCreditsFlag = false; // TRUE if credits are being scrolled
    QRect m_IconRect; // bounding rectangle to display the about image
    QRect m_CreditsRect; // bounding rectangle for the scrolling credits
    QColor m_ColorFade; // fade-out color to fade to the credits window
    QFont m_Font; // Font for text
    QFont m_BoldFont; // Font for Studio/Client version number

    QColor m_Color_Background;
    QColor m_Color_Text;
    QBrush m_Brush;

    void BeginCreditsTransition();

    void DrawFadeRect();

    void FadeOut();

    void ResizeAbout();

    //{{AFX_MSG(CAboutDlg)
    void paintEvent(QPaintEvent* event) override;
    void OnInitDialog();

    void mouseDoubleClickEvent(QMouseEvent* event) override;

    Q3DStudio::CString m_ProductVersionStr;
    Q3DStudio::CString m_CopyrightStr;
    Q3DStudio::CString m_Credit1Str;
    Q3DStudio::CString m_Credit2Str;

public:
    void OnStnClickedAboutboxProdver();

private:
    QScopedPointer<Ui::AboutDlg> m_ui;
};

#endif // INCLUDED_ABOUT_DLG_H

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
// Prefix
//==============================================================================
#include "stdafx.h"

//==============================================================================
// Includes
//==============================================================================
#include "SplashView.h"
#include "WidgetControl.h"
#include "StudioApp.h"
#include "SplashControl.h"

//=============================================================================
/**
 * Constructor: Protected because the view is always created dynamically.
 * You must call Initialize() before trying to use this class.
 */
CSplashView::CSplashView(QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , m_WndControl(nullptr)
    , m_SplashControl(nullptr)
{
    OnInitializePalettes();
}

//=============================================================================
/**
 * Destructor
 */
CSplashView::~CSplashView()
{
    delete m_WndControl;
    m_WndControl = nullptr;

    delete m_SplashControl;
}

//==============================================================================
/**
 *	Handles the WM_INITIALUPDATE message.  Responsible for preparing the view
 *	before it is displayed for the first time.
 */
void CSplashView::OnInitializePalettes()
{
    if (!m_WndControl) {
        m_SplashControl = new CSplashControl;
        m_WndControl = new WidgetControl(m_SplashControl, this);
        m_SplashControl->SetName("Splash Control");

        setFixedSize(m_WndControl->sizeHint());
    }
}

//=============================================================================
/**
 * Resizes the wnd control to fill the whole view.
 */
void CSplashView::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    m_WndControl->setGeometry(rect());
}

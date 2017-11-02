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

#if !defined(AFX_STUDIOPROJECTSETTINGSPAGE_H__E3317E44_810D_4478_A7DD_CF8570B7C17C__INCLUDED_)
#define AFX_STUDIOPROJECTSETTINGSPAGE_H__E3317E44_810D_4478_A7DD_CF8570B7C17C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//==============================================================================
//	Includes
//==============================================================================

//#include "ParameterColor.h"

#include "StudioPreferencesPropSheet.h"

#ifdef _USENEWCOLORPICKER_
#include "StudioColorPicker.h"
#endif

//==============================================================================
//	Forwards
//==============================================================================
class CStudioApp;

QT_BEGIN_NAMESPACE
namespace Ui {
    class StudioProjectSettingsPage;
}
QT_END_NAMESPACE

//==============================================================================
//	Base class for the settings for different modes
//==============================================================================
class CStudioProjectSettingsPage : public CStudioPreferencesPropPage
{
    Q_OBJECT
    // Construction
public:
    explicit CStudioProjectSettingsPage(QWidget *parent = nullptr);
    ~CStudioProjectSettingsPage();

    // Overrides
    // ClassWizard generate virtual function overrides
public:
    bool OnApply() override;
    void OnOK() override;

    // Implementation
protected:
    double m_AspectRatio; ///< Stores the presentation width divided by the presentation height
    QFont m_Font; ///< Font for text
    QFont m_BoldFont; ///< Bold font for drawing the group boxes

    void LoadSettings();
    void SaveSettings();

protected:
    // Generated message map functions
    virtual void OnInitDialog();
    void OnChangeEditPresWidth();
    void OnChangeEditPresHeight();
    void OnCheckMaintainRatio();
    void OnChangeAuthor();
    void OnChangeCompany();
    void OnSettingsModified();
    void OnSelChangePreviewApp();
    void OnCustomPreviewMore();
    void OnChangeSet1();
    void OnChangeSet2();
    void OnChangeSet3();
    void OnChangeSet5();

    QScopedPointer<QT_PREPEND_NAMESPACE(Ui::StudioProjectSettingsPage)> m_ui;
};

#endif // !defined(AFX_STUDIOPROJECTSETTINGSPAGE_H__E3317E44_810D_4478_A7DD_CF8570B7C17C__INCLUDED_)

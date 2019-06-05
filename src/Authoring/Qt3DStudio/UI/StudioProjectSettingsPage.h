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

#ifndef STUDIOPROJECTSETTINGSPAGE_H
#define STUDIOPROJECTSETTINGSPAGE_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "StudioPreferencesPropSheet.h"

#ifdef _USENEWCOLORPICKER_
#include "StudioColorPicker.h"
#endif
class CStudioApp;

QT_BEGIN_NAMESPACE
namespace Ui {
class StudioProjectSettingsPage;
}
QT_END_NAMESPACE

class CStudioProjectSettingsPage : public CStudioPreferencesPropPage
{
    Q_OBJECT
    // Construction
public:
    explicit CStudioProjectSettingsPage(QWidget *parent = nullptr);
    ~CStudioProjectSettingsPage();

    // Overrides
public:
    bool onApply() override;

    // Implementation
protected:
    double m_aspectRatio; ///< Stores the presentation width divided by the presentation height
    QFont m_font; ///< Font for text
    QFont m_boldFont; ///< Bold font for drawing the group boxes

    void loadSettings();
    void saveSettings();

protected:
    // Generated message map functions
    virtual void onInitDialog();
    void onChangeEditPresWidth();
    void onChangeEditPresHeight();
    void onCheckMaintainRatio();

    QScopedPointer<QT_PREPEND_NAMESPACE(Ui::StudioProjectSettingsPage)> m_ui;
};

#endif

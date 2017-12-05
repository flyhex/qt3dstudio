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

#ifndef STUDIOAPPPREFSPAGE_H_
#define STUDIOAPPPREFSPAGE_H_

//==============================================================================
//	Includes
//==============================================================================
#include "BuildConfigParser.h"

#include "StudioPreferencesPropSheet.h"

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLabel)

class CStudioApp;
/////////////////////////////////////////////////////////////////////////////
// CStudioAppPrefsPage dialog
QT_BEGIN_NAMESPACE
namespace Ui
{
    class StudioAppPrefsPage;
}
QT_END_NAMESPACE

class CStudioAppPrefsPage : public CStudioPreferencesPropPage
{
    Q_OBJECT
protected:
    typedef std::pair<QLabel *, QComboBox *> TBuildLabelDropdownPair;
    typedef std::pair<Q3DStudio::CBuildConfiguration::SConfigProperty *, TBuildLabelDropdownPair>
        TBuildNameControlPair;

    // Construction
public:
    explicit CStudioAppPrefsPage(QWidget *parent = nullptr);
    ~CStudioAppPrefsPage();

    // Dialog Data
    QColor m_bgColor;

    double m_nudgeValue;

public:
    bool OnApply() override;
    void OnOK() override;
    void onBackgroundColorChanged(const QColor &color);

    // Implementation
protected:
    BOOL m_TimebarShowTime; ///< TRUE if timebars are to display their time value
    BOOL m_InterpolationIsSmooth; ///< TRUE if default interpolation is smooth
    QFont m_Font; ///< Font for text
    QFont m_BoldFont; ///< Bold font for drawing the group boxes
    void EnableOptions();
    void LoadSettings();
    void SaveSettings();

    // Generated message map functions
    virtual void OnInitDialog();
    void OnButtonRestoreDefaults();
    void OnSelChangeInterpolationDefault();
    void OnSelChangeSnapRange();
    void OnCheckTimelineAbsoluteSnapping();
    void OnChangeEditNudgeAmount();
    void OnSelChangeStartupView();
#if 0 // Removed until we have some other Preview configurations that just Viewer
    void OnChangePreviewConfiguration();
#endif
    void OnBgColorButtonClicked();

protected: // helper functions
    void InitEditStartViewCombo();

protected:
    std::list<TBuildNameControlPair>
        m_BuildProperties; ///< List of build properties, either ComboBox or Static

#if 0 // Removed until we have some other Preview configurations that just Viewer
    void LoadPreviewSelections();
    void LoadBuildProperties();
    void SavePreviewSettings();
    void RemovePreviewPropertyControls();
#endif
    QScopedPointer<QT_PREPEND_NAMESPACE(Ui::StudioAppPrefsPage)> m_ui;

private:
    void updateColorButton();
};

#endif

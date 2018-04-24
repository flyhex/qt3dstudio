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

#include "BuildConfigParser.h"

#include "StudioPreferencesPropSheet.h"

QT_FORWARD_DECLARE_CLASS(QComboBox)
QT_FORWARD_DECLARE_CLASS(QLabel)

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

public:
    explicit CStudioAppPrefsPage(QWidget *parent = nullptr);
    ~CStudioAppPrefsPage();

    // Dialog Data
    QColor m_bgColor;

public:
    bool onApply() override;
    void onOK() override;
    void onBackgroundColorChanged(const QColor &color);

protected:
    bool m_timebarShowTime; // TRUE if timebars are to display their time value
    bool m_interpolationIsSmooth; // TRUE if default interpolation is smooth
    QFont m_font; // Font for text
    QFont m_boldFont; // Bold font for drawing the group boxes
    bool m_restartNeeded;
    bool m_autosaveChanged;

    void enableOptions();
    void loadSettings();
    void saveSettings();

    virtual void onInitDialog();
    void onButtonRestoreDefaults();
#if 0 // Removed until we have some other Preview configurations that just Viewer
    void onChangePreviewConfiguration();
#endif
    void onBgColorButtonClicked();

    void enableAutosave(bool enabled);
    void setAutosaveInterval(int interval);
    void onClearAutosaveFiles();
    void onitEditStartViewCombo();

protected:
    std::list<TBuildNameControlPair> m_buildProperties; // List of build properties, either
                                                        // ComboBox or Static

#if 0 // Removed until we have some other Preview configurations that just Viewer
    void loadPreviewSelections();
    void loadBuildProperties();
    void savePreviewSettings();
    void removePreviewPropertyControls();
#endif
    QScopedPointer<QT_PREPEND_NAMESPACE(Ui::StudioAppPrefsPage)> m_ui;

private:
    void updateColorButton();
};

#endif

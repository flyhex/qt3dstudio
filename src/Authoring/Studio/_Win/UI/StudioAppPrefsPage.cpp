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

#include "ui_StudioAppPrefsPage.h"

//==============================================================================
//	Includes
//==============================================================================
#include "Doc.h"
#include "StudioAppPrefsPage.h"
#include "StudioConst.h"
#include "StudioProjectSettings.h"
#include "StudioPreferences.h"
#include "StudioApp.h"
#include "StudioPreferences.h"
#include "CommonConstants.h"
#include "Views.h"
#include "Qt3DSDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "Core.h"
#include "IStudioRenderer.h"

#include <QtWidgets/qcolordialog.h>
#include <QtWidgets/qmessagebox.h>
#include <QtGui/qstandarditemmodel.h>

/////////////////////////////////////////////////////////////////////////////
// CStudioAppPrefsPage property page

//==============================================================================
/**
 *	Constructor: Initializes the object.
 */
//==============================================================================
CStudioAppPrefsPage::CStudioAppPrefsPage(QWidget *parent)
    : CStudioPreferencesPropPage(parent)
    , m_nudgeValue(0.0)
    , m_TimebarShowTime(FALSE)
    , m_InterpolationIsSmooth(FALSE)
    , m_ui(new Ui::StudioAppPrefsPage)
{
    m_Font = QFont(CStudioPreferences::GetFontFaceName());
    m_Font.setPixelSize(CStudioPreferences::fontSize());

    // Create a bold font for the group box text
    m_BoldFont = m_Font;
    m_BoldFont.setBold(true);

    OnInitDialog();
}

//==============================================================================
/**
 *	Destructor: Releases the object.
 */
//==============================================================================
CStudioAppPrefsPage::~CStudioAppPrefsPage()
{
}

/////////////////////////////////////////////////////////////////////////////
// CStudioAppPrefsPage message handlers

void CStudioAppPrefsPage::OnInitDialog()
{
    m_ui->setupUi(this);
    m_ui->m_editNudgeAmount->setValidator(new QDoubleValidator(this));

    // Add tool tips for controls
    m_ui->m_editNudgeAmount->setToolTip(tr("Set the object nudge amount"));
    m_ui->m_DefaultInterpolation->setToolTip(tr("Set default keyframe interpolation type"));
    m_ui->m_checkTimelineAbsoluteSnapping->setToolTip(tr("Enable timeline snapping grid"));
    m_ui->m_SnapRangeCombo->setToolTip(tr("Set resolution of timeline snapping grid"));
    m_ui->m_buttonRestoreDefaults->setToolTip(tr("Click to restore default Studio settings"));
    m_ui->m_EditViewBGColor->setAutoFillBackground(true);

    // Set fonts for child windows.
    for (auto w : findChildren<QWidget *>())
        w->setFont(m_Font);

    // Make the group text bold
    for (auto w : findChildren<QGroupBox *>())
        w->setFont(m_BoldFont);

    // Hidden until we have some other Preview configurations than just Viewer
    m_ui->groupBoxPreview->setVisible(false);

    // Load the settings for the controls
    LoadSettings();

    auto activated = static_cast<void(QComboBox::*)(int)>(&QComboBox::activated);
    connect(m_ui->m_buttonRestoreDefaults, &QPushButton::clicked,
            this, &CStudioAppPrefsPage::OnButtonRestoreDefaults);
    connect(m_ui->m_DefaultInterpolation, activated,
            this, &CStudioAppPrefsPage::OnSelChangeInterpolationDefault);
    connect(m_ui->m_SnapRangeCombo, activated, this, &CStudioAppPrefsPage::OnSelChangeSnapRange);
    connect(m_ui->m_checkTimelineAbsoluteSnapping, &QCheckBox::clicked,
            this, &CStudioAppPrefsPage::OnCheckTimelineAbsoluteSnapping);
    connect(m_ui->m_EditViewBGColor, &QPushButton::clicked,
            this, &CStudioAppPrefsPage::OnBgColorButtonClicked);
    connect(m_ui->m_editNudgeAmount, &QLineEdit::textEdited,
            this, &CStudioAppPrefsPage::OnChangeEditNudgeAmount);
    connect(m_ui->m_EditViewStartupView, activated,
            this, &CStudioAppPrefsPage::OnSelChangeStartupView);
#if 0 // Removed until we have some other Preview configurations than just Viewer
    connect(m_ui->m_PreviewSelector, activated,
            this, &CStudioAppPrefsPage::OnChangePreviewConfiguration);
#endif
}

//==============================================================================
/**
 *	LoadSettings: Load the settings from the CDoc and set the control values.
 *
 *	@param	None
 */
//==============================================================================
void CStudioAppPrefsPage::LoadSettings()
{
    m_nudgeValue = CStudioPreferences::GetNudgeAmount();

    // Get the Interpolation Preference
    m_ui->m_DefaultInterpolation->addItem(tr("Smooth"));
    m_ui->m_DefaultInterpolation->addItem(tr("Linear"));

    long theInterpolationPref = 0;
    if (CStudioPreferences::GetInterpolation())
        theInterpolationPref = 0;
    else
        theInterpolationPref = 1;
    m_ui->m_DefaultInterpolation->setCurrentIndex(theInterpolationPref);

    // Timeline snapping grid
    m_ui->m_checkTimelineAbsoluteSnapping->setChecked(
                CStudioPreferences::IsTimelineSnappingGridActive());

    // The scale mode
    m_ui->m_SnapRangeCombo->addItem(tr("Low Resolution"));
    m_ui->m_SnapRangeCombo->addItem(tr("Medium Resolution"));
    m_ui->m_SnapRangeCombo->addItem(tr("High Resolution"));
    long theResolution = (long)CStudioPreferences::GetTimelineSnappingGridResolution();
    m_ui->m_SnapRangeCombo->setCurrentIndex(theResolution);

    InitEditStartViewCombo();

    EnableOptions();

#if 0 // Removed until we have some other Preview configurations than just Viewer
    LoadPreviewSelections();
#endif

    m_ui->m_editNudgeAmount->setText(QString::number(m_nudgeValue));
    m_bgColor = CStudioPreferences::GetEditViewBackgroundColor();
    updateColorButton();
}

void CStudioAppPrefsPage::updateColorButton()
{
    QString bgColorStyle = QStringLiteral("background-color: ") + m_bgColor.name();
    m_ui->m_EditViewBGColor->setStyleSheet(bgColorStyle);
}

//==============================================================================
/**
 *	SaveSettings: Save the settings from the controls to the CDoc
 *
 *	@param	None
 */
//==============================================================================
void CStudioAppPrefsPage::SaveSettings()
{
    // Nudge amount
    CStudioPreferences::SetNudgeAmount(m_nudgeValue);

    // Default interpolation
    g_StudioApp.GetCore()->GetDoc()->SetDefaultKeyframeInterpolation(
                m_ui->m_DefaultInterpolation->currentIndex() == 0);

    // Timeline snapping grid
    CStudioPreferences::SetTimelineSnappingGridActive(
                m_ui->m_checkTimelineAbsoluteSnapping->isChecked());
    long theCurrentSelection = m_ui->m_SnapRangeCombo->currentIndex();
    CStudioPreferences::SetTimelineSnappingGridResolution((ESnapGridResolution)theCurrentSelection);

    // Edit View Background Color
    CStudioPreferences::SetEditViewBackgroundColor(m_bgColor);

    // Preferred Startup View
    long theSel = m_ui->m_EditViewStartupView->currentIndex();
    long theNumItems = m_ui->m_EditViewStartupView->count();
    CStudioPreferences::SetPreferredStartupView(
                (theSel == theNumItems - 1) ? -1 : theSel); // -1 for deployment view

#if 0 // Removed until we have some other Preview configurations than just Viewer
    SavePreviewSettings();
#endif
}

//==============================================================================
/**
 *	OnApply: Handler for the Apply button
 *
 *	@param	None
 */
//==============================================================================
bool CStudioAppPrefsPage::OnApply()
{
    // Apply was clicked - save settings and disabled the Apply button
    this->SaveSettings();

    this->SetModified(FALSE);

    // Request that the renderer refreshes as settings may have changed
    g_StudioApp.GetRenderer().RequestRender();

    return CStudioPreferencesPropPage::OnApply();
}

//==============================================================================
/**
 *	OnOK: Handler for the OK button
 *
 *	@param	None
 */
//==============================================================================
void CStudioAppPrefsPage::OnOK()
{
    CStudioPreferencesPropPage::OnOK();
}

//==============================================================================
/**
 *	OnButtonRestoreDefaults: Restore the defaults and exit the preferences.
 *
 *	@param	None
 */
//==============================================================================
void CStudioAppPrefsPage::OnButtonRestoreDefaults()
{
    int theChoice = 0;

    // Ask the user if she really wants to do this
    theChoice = QMessageBox::question(this,
                                      tr("Restore Defaults"),
                                      tr("Are you sure that you want to restore all program "
                                         "\ndefaults? Your current settings will be lost."));

    // If the "yes" button was selected
    if (theChoice == QMessageBox::Yes) {
        // Restore default preferences by passing PREFS_RESET_DEFAULTS back
        // to the CStudioDocPreferences (that called this preferences sheet)
        CStudioPreferencesPropPage::EndDialog(PREFS_RESET_DEFAULTS);
    }
}

//==============================================================================
/**
 *	OnSelChangeInterpolationDefault: CBN_SELCHANGE handler for the
 *									 IDC_INTERPOLATION_DEFAULT combo
 *box.
 *
 *	@param	None
 */
//==============================================================================
void CStudioAppPrefsPage::OnSelChangeInterpolationDefault()
{
    this->SetModified(TRUE);
}

//==============================================================================
/**
 *	OnChangeEditNudgeAmount: EN_UPDATE handler for the IDC_EDIT_NUDGE
 *
 *	@param	None
 */
//==============================================================================
void CStudioAppPrefsPage::OnChangeEditNudgeAmount()
{
    QString editStr = m_ui->m_editNudgeAmount->text();
    m_nudgeValue = editStr.toDouble();
    this->SetModified(true);
}

//==============================================================================
/**
 *	OnSelChangeSnapRange: CBN_SELCHANGE handler for the IDC_SNAPRANGE combo box.
 *
 *	@param	None
 */
//==============================================================================
void CStudioAppPrefsPage::OnSelChangeSnapRange()
{
    this->SetModified(true);
}

//==============================================================================
/**
 *	OnCheckTimelineAbsoluteSnapping: Handler for the IDC_CHECK_TIMELINEABSOLUTESNAPPING
 *checkbox.
 *
 *	@param	None
 */
//==============================================================================
void CStudioAppPrefsPage::OnCheckTimelineAbsoluteSnapping()
{
    this->SetModified(true);
    this->EnableOptions();
}

//==============================================================================
/**
 *	EnableOptions: Enable/disable options.
 *
 *	@param	None
 */
//==============================================================================
void CStudioAppPrefsPage::EnableOptions()
{
    m_ui->m_SnapRangeCombo->setEnabled(m_ui->m_checkTimelineAbsoluteSnapping->isChecked());
}

//==============================================================================
/**
 *	OnSelChangeStartupView: CBN_SELCHANGE handler for the IDC_COMBO_EDIT_STARTUP_VIEW
 *	Combo box
 */
void CStudioAppPrefsPage::OnSelChangeStartupView()
{
    this->SetModified(true);
}

//==============================================================================
/**
 *	Initialise the combo box that displays the preferred startup view.
 *	Set the initial selection to that saved to the preferences
 */
//==============================================================================
void CStudioAppPrefsPage::InitEditStartViewCombo()
{
    Q3DStudio::IStudioRenderer &theRenderer = g_StudioApp.GetRenderer();
    QStringList theCameraNames;
    theRenderer.GetEditCameraList(theCameraNames);
    for (size_t idx = 0, end = theCameraNames.size(); idx < end; ++idx) {
        m_ui->m_EditViewStartupView->addItem(
                    theCameraNames.at(idx));
        m_ui->m_EditViewStartupView->setItemData(m_ui->m_EditViewStartupView->count() - 1,
                                                 QVariant((int)idx + 1));
    }

    m_ui->m_EditViewStartupView->addItem("--------------------------");
    m_ui->m_EditViewStartupView->setItemData(m_ui->m_EditViewStartupView->count() - 1, -1); // set to an invalid pointer
    // make item non-selectable
    QStandardItemModel *model =
            qobject_cast<QStandardItemModel *>(m_ui->m_EditViewStartupView->model());
    QStandardItem *item = model->item(theCameraNames.size());
    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);

    // add the deployment view as the last selection
    m_ui->m_EditViewStartupView->addItem(tr("Scene Camera View"));
    m_ui->m_EditViewStartupView->setItemData(m_ui->m_EditViewStartupView->count() - 1, 0);

    long thePreferredView = CStudioPreferences::GetPreferredStartupView();
    long theNumItems = m_ui->m_EditViewStartupView->count();
    if (thePreferredView == -1) // deployment view
        m_ui->m_EditViewStartupView->setCurrentIndex(theNumItems - 1); // set to the last one
    else if (thePreferredView < theNumItems - 1)
        m_ui->m_EditViewStartupView->setCurrentIndex(thePreferredView);
    else // possibly from old content where cameras are removed
        m_ui->m_EditViewStartupView->setCurrentIndex(0);
}

#if 0 // Removed until we have some other Preview configurations than just Viewer
void CStudioAppPrefsPage::LoadPreviewSelections()
{
    // Load the configurations from all the .build files
    Q3DStudio::CBuildConfigurations &theConfig = g_StudioApp.GetCore()->GetBuildConfigurations();
    Q3DStudio::CBuildConfigurations::TBuildConfigurations theConfigurations =
            theConfig.GetConfigurations();
    Q3DStudio::CBuildConfigurations::TBuildConfigurations::iterator theIter;
    for (theIter = theConfigurations.begin(); theIter != theConfigurations.end(); ++theIter) {
        const Q3DStudio::CString &theConfig = theIter->first;
        m_ui->m_PreviewSelector->addItem(theConfig.toQString());
        m_ui->m_PreviewSelector->setItemData(m_ui->m_PreviewSelector->count() - 1,
                                             QVariant::fromValue(theIter->second));
    }

    int thePreviewSelected = m_ui->m_PreviewSelector->findText(
                CStudioPreferences::GetPreviewConfig().toQString());
    m_ui->m_PreviewSelector->setCurrentIndex(thePreviewSelected);
    if (thePreviewSelected == -1) {
        // select the first build configuration, or if no conriguration, the first application, i.e.
        // AMPlayer
        m_ui->m_PreviewSelector->setCurrentIndex(0);
        long thePreviewCount = m_ui->m_PreviewSelector->count();
        for (long theIndex = 0; theIndex < thePreviewCount; ++theIndex) {
            if (m_ui->m_PreviewSelector->itemData(
                        theIndex).value<Q3DStudio::CBuildConfiguration *>() != nullptr) {
                m_ui->m_PreviewSelector->setCurrentIndex(theIndex);
                break;
            }
        }
    }

    LoadBuildProperties();
}

//==============================================================================
/**
 *	When the build configuration is changed, all the properties have to be updated.
 */
//==============================================================================
void CStudioAppPrefsPage::OnChangePreviewConfiguration()
{
    LoadBuildProperties();
}
#endif

void CStudioAppPrefsPage::OnBgColorButtonClicked() 
{
    QColor previousColor = m_bgColor;
    QColorDialog *theColorDlg = new QColorDialog(previousColor, this);
    theColorDlg->setOption(QColorDialog::DontUseNativeDialog, true);
    connect(theColorDlg, &QColorDialog::currentColorChanged,
            this, &CStudioAppPrefsPage::onBackgroundColorChanged);
    if (theColorDlg->exec() == QDialog::Accepted)
        m_bgColor = theColorDlg->selectedColor();
    else
        m_bgColor = previousColor;
    updateColorButton();
    this->SetModified(true);
    OnApply();
}

void CStudioAppPrefsPage::onBackgroundColorChanged(const QColor &color)
{
    m_bgColor = color;
    updateColorButton();
    this->SetModified(true);
    OnApply();
}

//==============================================================================
/**
 *	Load the build properties for the current preview application selected
 */
//==============================================================================
#if 0 // Removed until we have some other Preview configurations than just Viewer
void CStudioAppPrefsPage::LoadBuildProperties()
{
    // Remove those dynamic controls
    RemovePreviewPropertyControls();

    if (m_ui->m_PreviewSelector->count() > 0) {
        Q3DStudio::CBuildConfiguration *theConfig =
                m_ui->m_PreviewSelector->itemData(
                    m_ui->m_PreviewSelector->currentIndex())
                .value<Q3DStudio::CBuildConfiguration *>();
        if (theConfig) {
            // Only configuration read from .build files will have the ItemDataPtr set.

            Q3DStudio::CBuildConfiguration::TConfigProperties &theProperties =
                    theConfig->GetBuildProperties();

            auto layout = qobject_cast<QFormLayout *>(m_ui->groupBoxPreview->layout());
            auto activated = static_cast<void(QComboBox::*)(int)>(&QComboBox::activated);

            if (theProperties.empty() == false) {
                Q3DStudio::CBuildConfiguration::TConfigProperties::iterator theIter;

                for (theIter = theProperties.begin(); theIter != theProperties.end(); ++theIter) {
                    Q3DStudio::CBuildConfiguration::TConfigPropertyValues &theValues =
                            theIter->GetAcceptableValues();
                    // Only create the combo if there is more than 1 choices
                    if (theValues.size() > 1) {
                        Q3DStudio::CBuildConfiguration::TConfigPropertyValues::iterator
                                theValueIter;
                        long theMaxLength = 0;
                        for (theValueIter = theValues.begin(); theValueIter != theValues.end();
                             ++theValueIter) {
                            long theLabelLength = theValueIter->GetLabel().Length();
                            if (theLabelLength > theMaxLength)
                                theMaxLength = theLabelLength;
                        }

                        QLabel *theStaticText = new QLabel(theIter->GetLabel().toQString());
                        theStaticText->setFont(m_Font);
                        QComboBox *thePropertyDropdown = new QComboBox();
                        connect(thePropertyDropdown, activated, [&]() {SetModified(true);});
                        thePropertyDropdown->setFont(m_Font);
                        layout->addRow(theStaticText, thePropertyDropdown);

                        m_BuildProperties.push_back(std::make_pair(
                                                        &*theIter, std::make_pair(theStaticText, thePropertyDropdown)));

                        Q3DStudio::CString thePropertyValue =
                                CStudioPreferences::GetPreviewProperty(theIter->GetName());
                        for (theValueIter = theValues.begin(); theValueIter != theValues.end();
                             ++theValueIter) {
                            thePropertyDropdown->addItem(theValueIter->GetLabel().toQString());
                            thePropertyDropdown->setItemData(thePropertyDropdown->count() - 1, QVariant::fromValue(&*theValueIter));
                            if (theValueIter->GetName() == thePropertyValue)
                                thePropertyDropdown->setCurrentIndex(thePropertyDropdown->count() - 1);
                        }

                        // Select the first entry
                        if (thePropertyDropdown->currentIndex() == -1)
                            thePropertyDropdown->setCurrentIndex(0);
                    }
                }
            }
        }
    }
}

void CStudioAppPrefsPage::SavePreviewSettings()
{
    QString thePreviewApp = m_ui->m_PreviewSelector->currentText();
    CStudioPreferences::SetPreviewConfig(Q3DStudio::CString::fromQString(thePreviewApp));

    std::list<TBuildNameControlPair>::iterator theIter;
    for (theIter = m_BuildProperties.begin(); theIter != m_BuildProperties.end(); ++theIter) {
        QComboBox *theCombo = theIter->second.second;
        Q3DStudio::CString theName = theIter->first->GetName();
        Q3DStudio::CBuildConfiguration::SConfigPropertyValue *thePropertyValue =
                theCombo->itemData(theCombo->currentIndex())
                .value<Q3DStudio::CBuildConfiguration::SConfigPropertyValue *>();
        CStudioPreferences::SetPreviewProperty(theName, thePropertyValue->GetName());
    }
}

//==============================================================================
/**
 *	Remove all the dynamically added controls that was read in from the build file
 */
//==============================================================================
void CStudioAppPrefsPage::RemovePreviewPropertyControls()
{
    // Remove the created control
    std::list<TBuildNameControlPair>::iterator theIter;
    for (theIter = m_BuildProperties.begin(); theIter != m_BuildProperties.end(); ++theIter) {
        delete theIter->second.first;
        delete theIter->second.second;
    }
    m_BuildProperties.clear();
}
#endif

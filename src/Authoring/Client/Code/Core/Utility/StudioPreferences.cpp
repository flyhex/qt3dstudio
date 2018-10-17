/****************************************************************************
**
** Copyright (C) 2000 NVIDIA Corporation.
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

#include "Qt3DSCommonPrecompile.h"

#include "StudioPreferences.h"
#include "Preferences.h"
#include "CColor.h"
#include "MasterP.h"
#include "CommonConstants.h"

#include <QtGui/qpalette.h>
#include <QtQml/qqmlcontext.h>

static ::CColor s_BaseColor;
static ::CColor s_DarkBaseColor;
static ::CColor s_NormalColor;
static ::CColor s_MasterColor;
static ::CColor s_MouseOverHighlightColor;
static ::CColor s_ButtonDownColor;
static ::CColor s_DisabledTextColor;

static QColor s_studioColor1;
static QColor s_studioColor2;
static QColor s_studioColor3;
static QColor s_backgroundColor;
static QColor s_guideColor;
static QColor s_selectionColor;
static QColor s_textColor;
static QColor s_masterColor;
static QColor s_disabledColor;
static QColor s_dataInputColor;
static QLinearGradient s_welcomeBackgroundGradient;

static QColor s_timelineRowColorNormal;
static QColor s_timelineRowColorNormalProp;
static QColor s_timelineRowColorOver;
static QColor s_timelineRowColorSelected;
static QColor s_timelineRowColorDurationOff1; // duration off ancestors' bounds color1
static QColor s_timelineRowColorDurationOff2; // duration off ancestors' bounds color2
static QColor s_timelineRowColorDurationEdge; // duration left and right edge lines
static QColor s_timelineRowColorDurationSelected;
static QColor s_timelineRowColorDndSource;
static QColor s_timelineRowColorDndTarget;
static QColor s_timelineRowColorDndTargetSP;
static QColor s_timelineRowMoverColor;
static QColor s_timelineRowSubpColor;
static QColor s_timelineRowSubpDescendantColor;
static QColor s_timelineRulerColor;
static QColor s_timelineRulerColorDisabled;
static QColor s_timelineWidgetBgColor;
static QColor s_timelinePlayheadLineColor;
static QColor s_timelineFilterButtonSelectedColor;
static QColor s_timelineFilterButtonHoveredColor;

static int s_fontSize;
static int s_controlBaseHeight;
static int s_idWidth;
static int s_valueWidth;
static QSize s_browserPopupSize;

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

// Statics
const ::CColor CStudioPreferences::EDITVIEW_DEFAULTBGCOLOR = ::CColor("#262829");
std::unique_ptr<CPreferences> CStudioPreferences::m_preferences = nullptr;

CStudioPreferences::CStudioPreferences()
{
}

CStudioPreferences::~CStudioPreferences()
{
}

//==============================================================================
/**
 * Loads the default preferences from the registry.  Must be called after the
 * registry root has been set up, and before calling any of the Get functions.
 */
void CStudioPreferences::loadPreferences(const QString &filePath)
{
    if (!m_preferences)
        m_preferences = std::unique_ptr<CPreferences>(new CPreferences);

    m_preferences->SetPreferencesFile(filePath);

    s_BaseColor = m_preferences->GetColorValue(QStringLiteral("BaseColor"), ::CColor("#262829"),
                                               QStringLiteral("Preferences"));

    s_NormalColor = m_preferences->GetColorValue(QStringLiteral("NormalColor"), ::CColor("#ffffff"),
                                                 QStringLiteral("Preferences"));
    s_MasterColor = m_preferences->GetColorValue(QStringLiteral("MasterColor"), ::CColor("#5caa15"),
                                                 QStringLiteral("Preferences"));

    s_DarkBaseColor = s_BaseColor;
    s_DarkBaseColor.SetLuminance(s_DarkBaseColor.GetLuminance() - 0.10f);

    s_MouseOverHighlightColor = s_BaseColor;
    s_MouseOverHighlightColor.SetLuminance(s_MouseOverHighlightColor.GetLuminance() - 0.05f);

    s_ButtonDownColor = s_DarkBaseColor; // CPreferences::GetUserPreferences( "Preferences"
                                         // ).GetColorValue( "ButtonDownColor", ::CColor( 118, 202,
                                         // 8 ) );

    s_DisabledTextColor = ::CColor(128, 128, 128);

    s_studioColor1 = QColor("#262829");
    s_studioColor2 = QColor("#404244");
    s_studioColor3 = QColor("#727476");
    s_backgroundColor = QColor("#2e2f30");
    s_guideColor = QColor("#f4be04");
    s_selectionColor = QColor("#23516D");
    s_textColor = QColor("#ffffff");
    s_masterColor = QColor("#5caa15");
    s_disabledColor = QColor("#727476");
    s_dataInputColor = QColor("#ff5102");

    s_welcomeBackgroundGradient = QLinearGradient(0.0, 0.0, 1.0, 0.0);
    s_welcomeBackgroundGradient.setColorAt(0.0, QColor("#343E55"));
    s_welcomeBackgroundGradient.setColorAt(1.0, QColor("#000727"));

    s_timelineRowColorNormal = QColor("#404040");
    s_timelineRowColorNormalProp = QColor("#373737");
    s_timelineRowColorOver = QColor("#4d4d4d");
    s_timelineRowColorSelected = QColor("#336699");
    s_timelineRowColorDurationOff1 = QColor("#3388B3");
    s_timelineRowColorDurationOff2 = QColor("#222222");
    s_timelineRowColorDurationEdge = QColor("#000000");
    s_timelineRowColorDurationSelected = QColor("#80000000");
    s_timelineRowColorDndSource = QColor("#464600");
    s_timelineRowColorDndTarget = QColor("#292929");
    s_timelineRowColorDndTargetSP = QColor("#222000");
    s_timelineRowMoverColor = QColor("#ffff00");
    s_timelineRulerColor = QColor("#888888");
    s_timelineRulerColorDisabled = QColor("#444444");
    s_timelineWidgetBgColor = QColor("#222222");
    s_timelinePlayheadLineColor = QColor("#b20808");
    s_timelineFilterButtonSelectedColor = QColor("#000000");
    s_timelineFilterButtonHoveredColor = QColor("#40000000");
    s_timelineRowSubpColor = QColor("#e2ceff");
    s_timelineRowSubpDescendantColor = QColor("#a263ff");

    s_fontSize = 12;
    s_controlBaseHeight = 22;
    s_idWidth = 130;
    s_valueWidth = 250;
    s_browserPopupSize = QSize(400, 400);
}

void CStudioPreferences::savePreferences()
{
    m_preferences->save();
}

//==============================================================================
/**
 *  Returns the state of the timeline snapping grid
 *  @return true if the snapping grid is active
 */
bool CStudioPreferences::IsTimelineSnappingGridActive()
{
    return m_preferences->GetValue(QStringLiteral("SnappingGridActive"), true);
}

//==============================================================================
/**
 *	Sets the state of the timeline snapping grid
 *	@param inActiveFlag true if the snapping grid is active
 */
void CStudioPreferences::SetTimelineSnappingGridActive(bool inActive)
{
    m_preferences->SetValue(QStringLiteral("SnappingGridActive"), inActive);
}

//==============================================================================
/**
 *	Gets the timeline snapping grid resolution
 *	@return Index value for the snapping resolution:
 * (See StudioConst.h for these values)
 */
ESnapGridResolution CStudioPreferences::GetTimelineSnappingGridResolution()
{
    return (ESnapGridResolution)m_preferences->GetLongValue(
            QStringLiteral("SnappingGridResolution"), (long)SNAPGRID_SECONDS);
}

//==============================================================================
/**
 *	Sets the timeline snapping grid resolution
 *	@param inSnappingResolution Index value for the snapping resolution:
 * (See StudioConst.h for these values)
 */
void CStudioPreferences::SetTimelineSnappingGridResolution(ESnapGridResolution inResolution)
{
    m_preferences->SetLongValue(QStringLiteral("SnappingGridResolution"), (long)inResolution);
}

/**
 *	Get the fill mode to render the geometries when in editing view
 *	@return true to render as solid, else as wireframe
 */
bool CStudioPreferences::GetEditViewFillMode()
{
    return m_preferences->GetValue(QStringLiteral("EditViewFillMode"), true);
}

//==============================================================================
/**
 *	Set the fill mode to render the geometries when in editing view
 *	@param inRenderAsSolid true to render as solid, else as wireframe
 */
void CStudioPreferences::SetEditViewFillMode(bool inRenderAsSolid)
{
    m_preferences->SetValue(QStringLiteral("EditViewFillMode"), inRenderAsSolid);
}

//==============================================================================
/**
 *	Get the preferred startup view. -1 means the scene view and positive value
 *	means the index of the editing camera, thus the view
 */
long CStudioPreferences::GetPreferredStartupView()
{
    return m_preferences->GetLongValue(QStringLiteral("PreferredStartupView"),
                                       PREFERREDSTARTUP_DEFAULTINDEX);
}

//==============================================================================
/**
 *	Set the preferred startup view. -1 means the scene view and positive value
 *	means the index of the editing camera, thus the view
 *	@param inStartupView	the preferred startup view of new presentation.
 */
void CStudioPreferences::SetPreferredStartupView(long inStartupView)
{
    m_preferences->SetLongValue(QStringLiteral("PreferredStartupView"), inStartupView);
}

//==============================================================================
/**
 *	Indicates whether or not auto keyframing is turned on.
 *	Fetches the autoset keyframes preference from the registry.
 *	@return true if autoset keyframes is turned on, otherwise false
 */
bool CStudioPreferences::IsAutosetKeyframesOn()
{
    return m_preferences->GetValue(QStringLiteral("AutosetKeyframes"), true);
}

//==============================================================================
/**
 *	Sets the autoset keyframe preference.
 *	Saves the autoset keyframes preference in the registry.
 *	@param inEnable TRUE to enable autosetting of keyframes
 */
void CStudioPreferences::SetAutosetKeyframesOn(bool inEnable)
{
    m_preferences->SetValue(QStringLiteral("AutosetKeyframes"), inEnable);
}

//==============================================================================
/**
 *	Indicates whether or not displaying bounding box is turned on.
 *	Fetches the displaying bounding box preference from the registry.
 *	@return true if displaying bounding box is turned on, otherwise false
 */
bool CStudioPreferences::IsBoundingBoxesOn()
{
    return m_preferences->GetValue(QStringLiteral("BoundingBoxes"), true);
}

//==============================================================================
/**
 *	Sets the preference to whether display the bounding box
 *	Saves the bounding box displaying preference in the registry.
 *	@param inEnable TRUE to enable display the bounding box
 */
void CStudioPreferences::SetBoundingBoxesOn(bool inEnable)
{
    m_preferences->SetValue(QStringLiteral("BoundingBoxes"), inEnable);
}

//==============================================================================
/**
 *	Indicates whether or not displaying pivot point is turned on.
 *	Fetches the displaying pivot point preference from the registry.
 *	@return true if displaying pivot point is turned on, otherwise false
 */
bool CStudioPreferences::ShouldDisplayPivotPoint()
{
    return m_preferences->GetValue(QStringLiteral("Display Pivot Point"), true);
}

//==============================================================================
/**
 *	Sets the preference to whether display the pivot point
 *	Saves the pivot point displaying preference in the registry.
 *	@param inEnable TRUE to enable display the pivot point
 */
void CStudioPreferences::SetDisplayPivotPoint(bool inEnable)
{
    return m_preferences->SetValue(QStringLiteral("Display Pivot Point"), inEnable);
}

//==============================================================================
/**
 *	Indicates whether or not displaying geometry as wireframe.
 *	Fetches the displaying wireframe mode preference from the registry.
 *	@return true if displaying wireframe mode is turned on, otherwise false
 */
bool CStudioPreferences::IsWireframeModeOn()
{
    return m_preferences->GetValue(QStringLiteral("WireframeMode"), true);
}

//==============================================================================
/**
 *	Sets the preference to whether display geometry in wireframe mode
 *	Saves thewireframe mode displaying preference in the registry.
 *	@param inEnable TRUE to enable display the bounding box
 */
void CStudioPreferences::SetWireframeModeOn(bool inEnable)
{
    m_preferences->SetValue(QStringLiteral("WireframeMode"), inEnable);
}

//==============================================================================
/**
 *	Indicates whether or not to show tooltips on inspector palette.
 *	Retrieves tooltip visibility preference from the registry.
 *	@return true if tooltips should be shown, false if tooltips should not be shown.
 */
bool CStudioPreferences::ShouldShowTooltips()
{
    return m_preferences->GetValue(QStringLiteral("ShowTooltips"), true);
}

//==============================================================================
/**
 *	Indicates whether or not to show tooltips on inspector palette.
 *	Saves tooltip visibility preference in the registry.
 *	@param inEnable true if tooltips should be shown, false if tooltips should not be shown.
 */
void CStudioPreferences::SetShowTooltips(bool inShowTooltips)
{
    m_preferences->SetValue(QStringLiteral("ShowTooltips"), inShowTooltips);
}

//==============================================================================
/**
 *	Returns the preferred location of the timeline's splitter bar.
 */
long CStudioPreferences::GetTimelineSplitterLocation()
{
    return m_preferences->GetLongValue(QStringLiteral("TimelineSplitterLoc"), 250,
                                       QStringLiteral("Timeline"));
}

//==============================================================================
/**
 *	Stores the location of the splitter bar in the timeline.
 *	@param inLocation location of the splitter
 */
void CStudioPreferences::SetTimelineSplitterLocation(long inLocation)
{
    m_preferences->SetLongValue(QStringLiteral("TimelineSplitterLoc"), inLocation,
                                QStringLiteral("Timeline"));
}

//==============================================================================
/**
 *	Gets the preferred method of interpolation.
 *	Indicates whether the user prefers smooth or linear interpolation by default.
 *	@return true indicates that smooth interpolation is preferred, false indicates linear
 *interpolation.
 */
bool CStudioPreferences::GetInterpolation()
{
    return m_preferences->GetValue(QStringLiteral("InterpolationPreference"), true);
}

//==============================================================================
/**
 *	Saves the preferred interpolation method in the registry.
 *	@param inSmooth	true indicates smooth interpolation, false indicates linear
 */
void CStudioPreferences::SetInterpolation(bool inSmooth)
{
    m_preferences->SetValue(QStringLiteral("InterpolationPreference"), inSmooth);
}

//==============================================================================
/**
 *	Gets the snapping range of the timebars from the registry.
 *	@return	New range in milliseconds.
 */
long CStudioPreferences::GetSnapRange()
{
    return m_preferences->GetLongValue(QStringLiteral("SnapRange"), DEFAULT_SNAPRANGE);
}

//==============================================================================
/**
 *	Sets the snapping range of the timebars by saving value in registry.
 *	@param inSnapRange New range in milliseconds.
 */
void CStudioPreferences::SetSnapRange(long inSnapRange)
{
    m_preferences->SetLongValue(QStringLiteral("SnapRange"), inSnapRange);
}

//==============================================================================
/**
 *	Gets the delay between auto saves from the registry.
 *	@return	autosave delay in minutes.
 */
long CStudioPreferences::GetAutoSaveDelay()
{
    // default delay is 10 minutes (600 seconds)
    return m_preferences->GetLongValue(QStringLiteral("Delay"), DEFAULT_AUTOSAVE_DELAY,
                                       QStringLiteral("AutoSave"));
}

//==============================================================================
/**
 *	Sets the delay between auto saves by saving value in registry.
 *	@param inDelay New autosave delay in minutes.
 */
void CStudioPreferences::SetAutoSaveDelay(long inAutoSaveDelay)
{
    m_preferences->SetLongValue(QStringLiteral("Delay"), inAutoSaveDelay,
                                QStringLiteral("AutoSave"));
}

//==============================================================================
/**
 *	Returns true if Auto Saving is turned on.
 *  AutoSaving is turned ON by default.
 *
 *	@return	true if autosaving is on.
 */
bool CStudioPreferences::GetAutoSavePreference()
{
    return m_preferences->GetValue(QStringLiteral("Preference"), true, QStringLiteral("AutoSave"));
}

//==============================================================================
/**
 *	Sets user preference of whether to autosave.
 *	@param inActive user autosave preference.
 */
void CStudioPreferences::SetAutoSavePreference(bool inActive)
{
    m_preferences->SetValue(QStringLiteral("Preference"), inActive, QStringLiteral("AutoSave"));
}

//==============================================================================
/**
 *	Gets the default object lifetime.
 *	The Preferences dialog can change the default object lifetime.  When an
 *	object is created, this is how long it should be alive for by default.
 *	The value is stored in the registry.
 *	@return New lifetime in milliseconds.
 */
long CStudioPreferences::GetDefaultObjectLifetime()
{
    return m_preferences->GetLongValue(QStringLiteral("DefaultObjectLifetime"), DEFAULT_LIFETIME);
}

//==============================================================================
/**
 *	Sets the default object lifetime.
 *	The Preferences dialog can change the default object lifetime.  When an
 *	object is created, this is how long it should be alive for by default.
 *	The value is stored in the registry.
 *	@param inAmount	New lifetime in milliseconds.
 */
void CStudioPreferences::SetDefaultObjectLifetime(long inLifetime)
{
    m_preferences->SetLongValue(QStringLiteral("DefaultObjectLifetime"), inLifetime);
}

//==============================================================================
/**
 *	Gets the current timebar setting.
 *	Timebars will be able to display different information such as user-defined
 *	comments or the start/end time of the timebar.  This setting toggles between
 *	the two modes. The values are stored in the registry.
 *	@return true if time is being displayed
 */
bool CStudioPreferences::GetTimebarDisplayTime()
{
    return m_preferences->GetValue(QStringLiteral("TimebarSetting"), false);
}

//==============================================================================
/**
 *	Sets the timebar setting.
 *	The Preferences dialog can change the default timebar setting.  A timebar
 *	can show either time information, or comments entered by the user.  The value
 *	is stored in the registry and preserved between sessions.
 *	@param inShowTime True if the time should be shown
 */
void CStudioPreferences::SetTimebarDisplayTime(bool inDisplayTime)
{
    m_preferences->SetValue(QStringLiteral("TimebarSetting"), inDisplayTime);
}

//==============================================================================
/**
 *	Gets the advance property expanded flag
 *	When an object is selected, this universal flag is used for either toggle
 *  the advance properties on or off but will not be visually available in
 *  the application preference dialog and default to false.
 *	@return The expanded flag
 */
bool CStudioPreferences::GetAdvancePropertyExpandedFlag()
{
    return m_preferences->GetValue(QStringLiteral("AdvancePropertyFlag"), false);
}

//==============================================================================
/**
 *	Sets the expanded flag for object when toggled
 *	@param inAdvancePropertyFlag Save the last toggle flag
 */
void CStudioPreferences::SetAdvancePropertyExpandedFlag(bool inAdvancePropertyFlag)
{
    m_preferences->SetValue(QStringLiteral("AdvancePropertyFlag"), inAdvancePropertyFlag);
}

QString CStudioPreferences::GetPreviewConfig()
{
    return m_preferences->GetStringValue(QStringLiteral("Preview.Config"));
}

void CStudioPreferences::SetPreviewConfig(const QString &inValue)
{
    m_preferences->SetStringValue(QStringLiteral("Preview.Config"), inValue);
}

// Preview Properties at the registry are prepend with Preview.
QString CStudioPreferences::GetPreviewProperty(const QString &inName)
{
    QString theName = QStringLiteral("Preview.") + inName;
    return m_preferences->GetStringValue(theName);
}

void CStudioPreferences::SetPreviewProperty(const QString &inName, const QString &inValue)
{
    QString theName = QStringLiteral("Preview.") + inName;
    m_preferences->SetStringValue(theName, inValue);
}

//=============================================================================
/**
 *	Get whether we should show OpenGL Version Warning or Error Dialog
 *	This is used to disable / enable OpenGL Version checking
 *	@return true if user don't want to see the GL Version warning dialog
 */
bool CStudioPreferences::GetDontShowGLVersionDialog()
{
    return m_preferences->GetValue(QStringLiteral("DontShowGLVersionDialog"), false);
}

//=============================================================================
/**
 *	Set whether we should show OpenGL Version Warning or Error Dialog
 *	This is used to disable / enable OpenGL Version checking
 */
void CStudioPreferences::SetDontShowGLVersionDialog(bool inValue)
{
    m_preferences->SetValue(QStringLiteral("DontShowGLVersionDialog"), inValue);
}

CPt CStudioPreferences::GetDefaultClientSize()
{
    CPt theSize;
    theSize.x = m_preferences->GetLongValue(QStringLiteral("DefaultClientWidth"),
                                            DEFAULT_CLIENT_WIDTH);
    theSize.y = m_preferences->GetLongValue(QStringLiteral("DefaultClientHeight"),
                                            DEFAULT_CLIENT_HEIGHT);
    return theSize;
}

void CStudioPreferences::SetDefaultClientSize(int width, int height)
{
    m_preferences->SetLongValue(QStringLiteral("DefaultClientWidth"), (long)width);
    m_preferences->SetLongValue(QStringLiteral("DefaultClientHeight"), (long)height);
}

int CStudioPreferences::getNumRecentItems()
{
    return m_preferences->GetLongValue(QStringLiteral("RecentValid"));
}

void CStudioPreferences::setNumRecentItems(int n)
{
    m_preferences->SetLongValue(QStringLiteral("RecentValid"), n);
}

QString CStudioPreferences::getRecentItem(int index)
{
    return m_preferences->GetStringValue(QStringLiteral("RecentItem") + QString::number(index));
}

void CStudioPreferences::setRecentItem(int index, const QString &path)
{
    m_preferences->SetStringValue(QStringLiteral("RecentItem") + QString::number(index), path);
    savePreferences();
}

//==============================================================================
/**
 * Get the amount of time that the playhead should be advanced/reduced when the
 * '.' and ',' keys are used.
 */
long CStudioPreferences::GetTimeAdvanceAmount()
{
    return m_preferences->GetLongValue(QStringLiteral("TimeAdvance"), DEFAULT_TIME_ADVANCE);
}

//==============================================================================
/**
 * Sets the amount of time that the playhead should be advanced/reduced when the
 * '.' and ',' keys are used.
 */
void CStudioPreferences::SetTimeAdvanceAmount(long inTime)
{
    m_preferences->SetLongValue(QStringLiteral("TimeAdvance"), inTime);
}

//==============================================================================
/**
 * Get the amount of time that the playhead should be advanced/reduced when the
 * '<' and '>' keys are used.
 */
long CStudioPreferences::GetBigTimeAdvanceAmount()
{
    return m_preferences->GetLongValue(QStringLiteral("BigTimeAdvance"), DEFAULT_BIG_TIME_ADVANCE);
}

//==============================================================================
/**
 * Set the amount of time that the playhead should be advanced/reduced when the
 * '<' and '>' keys are used.
 */
void CStudioPreferences::SetBigTimeAdvanceAmount(long inTime)
{
    m_preferences->SetLongValue(QStringLiteral("BigTimeAdvance"), inTime);
}

/**
 * Retrieves the color that should be used when the mouse goes over a row, such
 * as in the timeline or inspector palettes.
 */
::CColor CStudioPreferences::GetMouseOverHighlightColor()
{
    return s_MouseOverHighlightColor;
}

/**
 * Returns the normal color used for non-master items and text throughout the UI
 */
::CColor CStudioPreferences::GetNormalColor()
{
    return s_NormalColor;
}

//=============================================================================
/**
 * Returns the color for master items and text throughout the UI
 */
::CColor CStudioPreferences::GetMasterColor()
{
    return s_MasterColor;
}

//=============================================================================
/**
 * Returns the color for inactive items and text throughout the UI
 */
::CColor CStudioPreferences::GetInactiveColor()
{
    return s_disabledColor;
}

/**
 * @return default color for object timebars in the timeline (if not specified by one of these other
 * functions)
 */
::CColor CStudioPreferences::GetObjectTimebarColor()
{
    return ::CColor("#788ac5");
}

//=============================================================================
/**
 * @return default colors for specific timebars in the timeline
 */
::CColor CStudioPreferences::GetLayerTimebarColor()
{
    return ::CColor("#e7e0cd");
}

/**
 *	Color when text is disabled
 */
::CColor CStudioPreferences::GetDisabledTextColor()
{
    return s_DisabledTextColor;
}

//==============================================================================
/**
 *  Colors for bounding boxes
 */
::CColor CStudioPreferences::GetSingleBoundingBoxColor()
{
    return ::CColor("#ff0000");
}

::CColor CStudioPreferences::GetGroupBoundingBoxColor()
{
    return ::CColor("#ff0000");
}

/**
 *  Colors for rulers and guides
 */
::CColor CStudioPreferences::GetRulerBackgroundColor()
{
    return s_studioColor1;
}

::CColor CStudioPreferences::GetRulerTickColor()
{
    return s_studioColor3;
}

::CColor CStudioPreferences::GetGuideColor()
{
    return ::CColor("#7a5f02"); // #f4be04 plus faked alpha 50%
}

::CColor CStudioPreferences::GetGuideSelectedColor()
{
    return s_guideColor;
}

::CColor CStudioPreferences::GetGuideFillColor()
{
    return ::CColor("#140F00"); // #f4be04 plus faked alpha 8%
}

::CColor CStudioPreferences::GetGuideFillSelectedColor()
{
    return ::CColor("#7a5f02"); // #f4be04 plus faked alpha 50%
}

//==============================================================================
/**
 *	Font face name for dialog
 */
QString CStudioPreferences::GetFontFaceName()
{
    return QStringLiteral("Segoe UI");
}

float CStudioPreferences::getSelectorLineWidth()
{
    return m_preferences->GetLongValue("SelectorLineWidth", DEFAULT_SELECTOR_WIDTH) / 10.0f;
}

void CStudioPreferences::setSelectorLineWidth(float width)
{
    m_preferences->SetLongValue("SelectorLineWidth", int(width * 10.0f));
}

float CStudioPreferences::getSelectorLineLength()
{
    return float(m_preferences->GetLongValue("SelectorLineLength", DEFAULT_SELECTOR_LENGTH));
}

void CStudioPreferences::setSelectorLineLength(float length)
{
    m_preferences->SetLongValue("SelectorLineLength", int(length));
}

void CStudioPreferences::setQmlContextProperties(QQmlContext *qml)
{
    qml->setContextProperty(QStringLiteral("_studioColor1"), s_studioColor1);
    qml->setContextProperty(QStringLiteral("_studioColor2"), s_studioColor2);
    qml->setContextProperty(QStringLiteral("_studioColor3"), s_studioColor3);
    qml->setContextProperty(QStringLiteral("_backgroundColor"), s_backgroundColor);
    qml->setContextProperty(QStringLiteral("_buttonDownColor"), s_ButtonDownColor.getQColor());
    qml->setContextProperty(QStringLiteral("_guideColor"), s_guideColor);
    qml->setContextProperty(QStringLiteral("_selectionColor"), s_selectionColor);
    qml->setContextProperty(QStringLiteral("_textColor"), s_textColor);
    qml->setContextProperty(QStringLiteral("_masterColor"), s_masterColor);
    qml->setContextProperty(QStringLiteral("_disabledColor"), s_disabledColor);
    qml->setContextProperty(QStringLiteral("_dataInputColor"), s_dataInputColor);
    qml->setContextProperty(QStringLiteral("_fontSize"), s_fontSize);
    qml->setContextProperty(QStringLiteral("_controlBaseHeight"), s_controlBaseHeight);
    qml->setContextProperty(QStringLiteral("_idWidth"), s_idWidth);
    qml->setContextProperty(QStringLiteral("_valueWidth"), s_valueWidth);
}

QColor CStudioPreferences::studioColor1()
{
    return s_studioColor1;
}

QColor CStudioPreferences::studioColor2()
{
    return s_studioColor2;
}

QColor CStudioPreferences::studioColor3()
{
    return s_studioColor3;
}

QColor CStudioPreferences::backgroundColor()
{
    return s_backgroundColor;
}

QColor CStudioPreferences::guideColor()
{
    return s_guideColor;
}

QColor CStudioPreferences::selectionColor()
{
    return s_selectionColor;
}

QColor CStudioPreferences::textColor()
{
    return s_textColor;
}

QColor CStudioPreferences::masterColor()
{
    return s_masterColor;
}

QColor CStudioPreferences::disabledColor()
{
    return s_disabledColor;
}

QColor CStudioPreferences::dataInputColor()
{
    return s_dataInputColor;
}

QLinearGradient CStudioPreferences::welcomeBackgroundGradient()
{
    return s_welcomeBackgroundGradient;
}

QColor CStudioPreferences::timelineRowColorNormal()
{
    return s_timelineRowColorNormal;
}

QColor CStudioPreferences::timelineRowColorNormalProp()
{
    return s_timelineRowColorNormalProp;
}

QColor CStudioPreferences::timelineRowColorOver()
{
    return s_timelineRowColorOver;
}

QColor CStudioPreferences::timelineRowColorSelected()
{
    return s_timelineRowColorSelected;
}

QColor CStudioPreferences::timelineRowColorDurationOff1()
{
    return s_timelineRowColorDurationOff1;
}

QColor CStudioPreferences::timelineRowColorDurationOff2()
{
    return s_timelineRowColorDurationOff2;
}

QColor CStudioPreferences::timelineRowColorDurationEdge()
{
    return s_timelineRowColorDurationEdge;
}

QColor CStudioPreferences::timelineRowColorDurationSelected()
{
    return s_timelineRowColorDurationSelected;
}

QColor CStudioPreferences::timelineRowColorDndSource()
{
    return s_timelineRowColorDndSource;
}

// get subpresentation target color
QColor CStudioPreferences::timelineRowColorDndTargetSP()
{
    return s_timelineRowColorDndTargetSP;
}

QColor CStudioPreferences::timelineRowColorDndTarget()
{
    return s_timelineRowColorDndTarget;
}

QColor CStudioPreferences::timelineRowMoverColor()
{
    return s_timelineRowMoverColor;
}

QColor CStudioPreferences::timelineRulerColor()
{
    return s_timelineRulerColor;
}

QColor CStudioPreferences::timelineRulerColorDisabled()
{
    return s_timelineRulerColorDisabled;
}

QColor CStudioPreferences::timelineWidgetBgColor()
{
    return s_timelineWidgetBgColor;
}

QColor CStudioPreferences::timelinePlayheadLineColor()
{
    return s_timelinePlayheadLineColor;
}

QColor CStudioPreferences::timelineFilterButtonSelectedColor()
{
    return s_timelineFilterButtonSelectedColor;
}

QColor CStudioPreferences::timelineFilterButtonHoveredColor()
{
    return s_timelineFilterButtonHoveredColor;
}

QColor CStudioPreferences::timelineRowSubpColor()
{
    return s_timelineRowSubpColor;
}

QColor CStudioPreferences::timelineRowSubpDescendantColor()
{
    return s_timelineRowSubpDescendantColor;
}

int CStudioPreferences::fontSize()
{
    return s_fontSize;
}

int CStudioPreferences::controlBaseHeight()
{
    return s_controlBaseHeight;
}

int CStudioPreferences::idWidth()
{
    return s_idWidth;
}

int CStudioPreferences::valueWidth()
{
    return s_valueWidth;
}

QSize CStudioPreferences::browserPopupSize()
{
    return s_browserPopupSize;
}

QString CStudioPreferences::GetVersionString()
{
    QString theVersionNumber = STRINGIFY(STUDIO_VERSION);
    theVersionNumber.replace(QLatin1String(","), QLatin1String("."));

    return theVersionNumber;
}

bool CStudioPreferences::showEditModePreview()
{
    return m_preferences->GetValue("showEditModePreview", true);
}

void CStudioPreferences::setShowEditModePreview(bool show)
{
    m_preferences->SetValue("showEditModePreview", show);
}

bool CStudioPreferences::editModeLightingEnabled()
{
    return m_preferences->GetValue("editModeLightingEnabled", true);
}

void CStudioPreferences::setEditModeLightingEnabled(bool enabled)
{
    m_preferences->SetValue("editModeLightingEnabled", enabled);
}

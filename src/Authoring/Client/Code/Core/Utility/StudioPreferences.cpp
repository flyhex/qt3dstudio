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

//==============================================================================
//	Prefix
//==============================================================================
#include "stdafx.h"

#include "StudioPreferences.h"
#include "Preferences.h"
#include "CColor.h"
#include "MasterP.h"
#include "CommonConstants.h"

// using namespace Q3DStudio;  <-- Do not do this here because it will conflict with CList and make
// the template generator go blah

#include <QtGui/qpalette.h>
#include <QtQml/qqmlcontext.h>

::CColor s_BaseColor;
::CColor s_DarkBaseColor;
::CColor s_NormalColor;
::CColor s_MasterColor;
::CColor s_MouseOverHighlightColor;
::CColor s_SelectColor;
::CColor s_ButtonDownColor;
::CColor s_TopRowColor;
::CColor s_LayerBackgroundColor;
::CColor s_LayerHighlightBGColor;
::CColor s_ExtendedLockedLightColor;
::CColor s_LockedBorderColor;
::CColor s_ScrollBGColor;
::CColor s_TextBoxBGColorWithFocus;
::CColor s_ControlRectBottomLineDarkColor;
::CColor s_ControlRectBottomLineColor;
::CColor s_ControlRectSideLineColor;
::CColor s_ControlRectTopLineColor;
::CColor s_ExtendedObjectLightColor;
::CColor s_ComboEditBoxGradientStartColor;
::CColor s_TimebarBorderColor;
::CColor s_RowTopColor;
::CColor s_ButtonHighlightColor;
::CColor s_PropertyFloorColor;
::CColor s_ButtonShadowColor;
::CColor s_TreeFloorColor;
::CColor s_TabButtonDownColor;
::CColor s_DisabledTextColor;

QColor s_studioColor1;
QColor s_studioColor2;
QColor s_studioColor3;
QColor s_backgroundColor;
QColor s_guideColor;
QColor s_selectionColor;
QColor s_textColor;
QColor s_masterColor;
QColor s_disabledColor;
QColor s_dataInputColor;
int s_fontSize;
int s_controlBaseHeight;
int s_idWidth;
int s_valueWidth;
QSize s_browserPopupSize;

bool CStudioPreferences::m_SudoMode = false;

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

// Static Consts
const ::CColor CStudioPreferences::EDITVIEW_DEFAULTBGCOLOR = ::CColor("#262829");

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
void CStudioPreferences::LoadPreferences()
{
    s_BaseColor = CPreferences::GetUserPreferences("Preferences")
                      .GetColorValue("BaseColor", ::CColor("#262829"));

    s_NormalColor = CPreferences::GetUserPreferences("Preferences")
                        .GetColorValue("NormalColor", ::CColor("#ffffff"));
    s_MasterColor = CPreferences::GetUserPreferences("Preferences")
                        .GetColorValue("MasterColor", ::CColor("#5caa15"));

    s_DarkBaseColor = s_BaseColor;
    s_DarkBaseColor.SetLuminance(s_DarkBaseColor.GetLuminance() - 0.10f);

    s_MouseOverHighlightColor = s_BaseColor;
    s_MouseOverHighlightColor.SetLuminance(s_MouseOverHighlightColor.GetLuminance() - 0.05f);

    s_SelectColor = s_BaseColor;
    s_SelectColor.SetLuminance(s_SelectColor.GetLuminance() - 0.10f);

    s_ButtonDownColor = s_DarkBaseColor; // CPreferences::GetUserPreferences( "Preferences"
                                         // ).GetColorValue( "ButtonDownColor", ::CColor( 118, 202,
                                         // 8 ) );

    m_SudoMode = CPreferences::GetUserPreferences().GetValue("sudo", false);

    s_TopRowColor = s_BaseColor;
    s_TopRowColor.SetLuminance(s_TopRowColor.GetLuminance() - 0.10f);

    s_ExtendedLockedLightColor = s_BaseColor;
    s_ExtendedLockedLightColor.SetLuminance(s_ExtendedLockedLightColor.GetLuminance() + 0.07f);

    s_LockedBorderColor = s_BaseColor;
    s_LockedBorderColor.SetLuminance(s_LockedBorderColor.GetLuminance() - 0.07f);

    s_ScrollBGColor = s_BaseColor;

    s_TextBoxBGColorWithFocus = s_BaseColor;
    s_TextBoxBGColorWithFocus.SetLuminance(s_TextBoxBGColorWithFocus.GetLuminance() + 0.20f);

    s_ControlRectBottomLineDarkColor = s_BaseColor;
    s_ControlRectBottomLineDarkColor.SetLuminance(s_ControlRectBottomLineDarkColor.GetLuminance()
                                                  - 0.50f);

    s_ControlRectBottomLineColor = s_BaseColor;
    s_ControlRectBottomLineColor.SetLuminance(s_ControlRectBottomLineColor.GetLuminance() - 0.20f);

    s_ControlRectSideLineColor = s_BaseColor;
    s_ControlRectSideLineColor.SetLuminance(s_ControlRectSideLineColor.GetLuminance() - 0.20f);

    s_ControlRectTopLineColor = s_BaseColor;
    s_ControlRectTopLineColor.SetLuminance(s_ControlRectTopLineColor.GetLuminance() - 0.30f);

    s_ExtendedObjectLightColor = s_BaseColor;
    s_ExtendedObjectLightColor.SetLuminance(s_ExtendedObjectLightColor.GetLuminance() + 0.13f);

    s_ComboEditBoxGradientStartColor = s_BaseColor;
    s_ComboEditBoxGradientStartColor.SetLuminance(s_ComboEditBoxGradientStartColor.GetLuminance()
                                                  + 0.14f);

    s_TimebarBorderColor = s_BaseColor;
    s_TimebarBorderColor.SetLuminance(s_TimebarBorderColor.GetLuminance() - 0.45f);

    s_RowTopColor = s_BaseColor;
    s_RowTopColor.SetLuminance(s_RowTopColor.GetLuminance() - 0.45f);

    s_ButtonHighlightColor = s_BaseColor;
    s_ButtonHighlightColor.SetLuminance(s_ButtonHighlightColor.GetLuminance() + 0.14f);

    s_PropertyFloorColor = s_BaseColor;
    s_PropertyFloorColor.SetLuminance(s_PropertyFloorColor.GetLuminance() - 0.07f);

    s_ButtonShadowColor = s_BaseColor;
    s_ButtonShadowColor.SetLuminance(s_ButtonShadowColor.GetLuminance() - 0.45f);

    s_TreeFloorColor = s_BaseColor;
    s_TreeFloorColor.SetLuminance(s_TreeFloorColor.GetLuminance() - 0.07f);

    s_TabButtonDownColor = ::CColor(175, 175, 175); // rgb value is 249, 255, 195
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
    s_fontSize = 12;
    s_controlBaseHeight = 22;
    s_idWidth = 130;
    s_valueWidth = 250;
    s_browserPopupSize = QSize(400, 400);
}

//==============================================================================
/**
 *	Returns the state of the timeline snapping grid
 *	@return true if the snapping grid is active
 */
bool CStudioPreferences::IsTimelineSnappingGridActive()
{
    return CPreferences::GetUserPreferences().GetValue("SnappingGridActive", true);
}

//==============================================================================
/**
 *	Sets the state of the timeline snapping grid
 *	@param inActiveFlag true if the snapping grid is active
 */
void CStudioPreferences::SetTimelineSnappingGridActive(bool inActive)
{
    CPreferences::GetUserPreferences().SetValue("SnappingGridActive", inActive);
}

//==============================================================================
/**
 *	Gets the timeline snapping grid resolution
 *	@return Index value for the snapping resolution:
 * (See StudioConst.h for these values)
 */
ESnapGridResolution CStudioPreferences::GetTimelineSnappingGridResolution()
{
    return (ESnapGridResolution)CPreferences::GetUserPreferences().GetLongValue(
        "SnappingGridResolution", (long)SNAPGRID_SECONDS);
}

//==============================================================================
/**
 *	Sets the timeline snapping grid resolution
 *	@param inSnappingResolution Index value for the snapping resolution:
 * (See StudioConst.h for these values)
 */
void CStudioPreferences::SetTimelineSnappingGridResolution(ESnapGridResolution inResolution)
{
    CPreferences::GetUserPreferences().SetLongValue("SnappingGridResolution", (long)inResolution);
}

//==============================================================================
/**
 *	Get the background color of the editing view
 *	@return the color of the editing view
 */
::CColor CStudioPreferences::GetEditViewBackgroundColor()
{
    return CPreferences::GetUserPreferences().GetColorValue("EditViewBGColor",
                                                            EDITVIEW_DEFAULTBGCOLOR);
}

//==============================================================================
/**
 *	Set the background color of the editing view
 *	@param inColor the color of the editing view
 */
void CStudioPreferences::SetEditViewBackgroundColor(::CColor inColor)
{
    CPreferences::GetUserPreferences().SetColorValue("EditViewBGColor", inColor);
}

//==============================================================================
/**
 *	Get the fill mode to render the geometries when in editing view
 *	@return true to render as solid, else as wireframe
 */
bool CStudioPreferences::GetEditViewFillMode()
{
    return CPreferences::GetUserPreferences().GetValue("EditViewFillMode", true);
}

//==============================================================================
/**
 *	Set the fill mode to render the geometries when in editing view
 *	@param inRenderAsSolid true to render as solid, else as wireframe
 */
void CStudioPreferences::SetEditViewFillMode(bool inRenderAsSolid)
{
    CPreferences::GetUserPreferences().SetValue("EditViewFillMode", inRenderAsSolid);
}

//==============================================================================
/**
 *	Get the preferred startup view. -1 means the scene view and positive value
 *	means the index of the editing camera, thus the view
 */
long CStudioPreferences::GetPreferredStartupView()
{
    return CPreferences::GetUserPreferences().GetLongValue("PreferredStartupView",
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
    CPreferences::GetUserPreferences().SetLongValue("PreferredStartupView", inStartupView);
}

//==============================================================================
/**
 *	Indicates whether or not auto keyframing is turned on.
 *	Fetches the autoset keyframes preference from the registry.
 *	@return true if autoset keyframes is turned on, otherwise false
 */
bool CStudioPreferences::IsAutosetKeyframesOn()
{
    return CPreferences::GetUserPreferences().GetValue("AutosetKeyframes", true);
}

//==============================================================================
/**
 *	Sets the autoset keyframe preference.
 *	Saves the autoset keyframes preference in the registry.
 *	@param inEnable TRUE to enable autosetting of keyframes
 */
void CStudioPreferences::SetAutosetKeyframesOn(bool inEnable)
{
    CPreferences::GetUserPreferences().SetValue("AutosetKeyframes", inEnable);
}

//==============================================================================
/**
 *	Indicates whether or not displaying bounding box is turned on.
 *	Fetches the displaying bounding box preference from the registry.
 *	@return true if displaying bounding box is turned on, otherwise false
 */
bool CStudioPreferences::IsBoundingBoxesOn()
{
    return CPreferences::GetUserPreferences().GetValue("BoundingBoxes", true);
}

//==============================================================================
/**
 *	Sets the preference to whether display the bounding box
 *	Saves the bounding box displaying preference in the registry.
 *	@param inEnable TRUE to enable display the bounding box
 */
void CStudioPreferences::SetBoundingBoxesOn(bool inEnable)
{
    CPreferences::GetUserPreferences().SetValue("BoundingBoxes", inEnable);
}

//==============================================================================
/**
 *	Indicates whether or not displaying pivot point is turned on.
 *	Fetches the displaying pivot point preference from the registry.
 *	@return true if displaying pivot point is turned on, otherwise false
 */
bool CStudioPreferences::ShouldDisplayPivotPoint()
{
    return CPreferences::GetUserPreferences().GetValue("Display Pivot Point", true);
}

//==============================================================================
/**
 *	Sets the preference to whether display the pivot point
 *	Saves the pivot point displaying preference in the registry.
 *	@param inEnable TRUE to enable display the pivot point
 */
void CStudioPreferences::SetDisplayPivotPoint(bool inEnable)
{
    return CPreferences::GetUserPreferences().SetValue("Display Pivot Point", inEnable);
}

//==============================================================================
/**
 *	Indicates whether or not displaying geometry as wireframe.
 *	Fetches the displaying wireframe mode preference from the registry.
 *	@return true if displaying wireframe mode is turned on, otherwise false
 */
bool CStudioPreferences::IsWireframeModeOn()
{
    return CPreferences::GetUserPreferences().GetValue("WireframeMode", true);
}

//==============================================================================
/**
 *	Sets the preference to whether display geometry in wireframe mode
 *	Saves thewireframe mode displaying preference in the registry.
 *	@param inEnable TRUE to enable display the bounding box
 */
void CStudioPreferences::SetWireframeModeOn(bool inEnable)
{
    CPreferences::GetUserPreferences().SetValue("WireframeMode", inEnable);
}

//==============================================================================
/**
 *	Indicates whether or not to show tooltips on inspector palette.
 *	Retrieves tooltip visibility preference from the registry.
 *	@return true if tooltips should be shown, false if tooltips should not be shown.
 */
bool CStudioPreferences::ShouldShowTooltips()
{
    return CPreferences::GetUserPreferences().GetValue("ShowTooltips", true);
}

//==============================================================================
/**
 *	Indicates whether or not to show tooltips on inspector palette.
 *	Saves tooltip visibility preference in the registry.
 *	@param inEnable true if tooltips should be shown, false if tooltips should not be shown.
 */
void CStudioPreferences::SetShowTooltips(bool inShowTooltips)
{
    CPreferences::GetUserPreferences().SetValue("ShowTooltips", inShowTooltips);
}

//==============================================================================
/**
 *	@return the color to use for the background of a tooltip
 */
::CColor CStudioPreferences::GetTooltipBackgroundColor()
{
    ::CColor theTooltipBGColor(255, 255, 225);
    return theTooltipBGColor;
}

//==============================================================================
/**
 *	Returns the preferred location of the timeline's splitter bar.
 */
long CStudioPreferences::GetTimelineSplitterLocation()
{
    return CPreferences::GetUserPreferences("Timeline").GetLongValue("TimelineSplitterLoc", 250);
}

//==============================================================================
/**
 *	Stores the location of the splitter bar in the timeline.
 *	@param inLocation location of the splitter
 */
void CStudioPreferences::SetTimelineSplitterLocation(long inLocation)
{
    CPreferences::GetUserPreferences("Timeline").SetLongValue("TimelineSplitterLoc", inLocation);
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
    return CPreferences::GetUserPreferences().GetValue("InterpolationPreference", true);
}

//==============================================================================
/**
 *	Saves the preferred interpolation method in the registry.
 *	@param inSmooth	true indicates smooth interpolation, false indicates linear
 */
void CStudioPreferences::SetInterpolation(bool inSmooth)
{
    CPreferences::GetUserPreferences().SetValue("InterpolationPreference", inSmooth);
}

//==============================================================================
/**
 *	Gets the snapping range of the timebars from the registry.
 *	@return	New range in milliseconds.
 */
long CStudioPreferences::GetSnapRange()
{
    return CPreferences::GetUserPreferences().GetLongValue("SnapRange", DEFAULT_SNAPRANGE);
}

//==============================================================================
/**
 *	Sets the snapping range of the timebars by saving value in registry.
 *	@param inSnapRange New range in milliseconds.
 */
void CStudioPreferences::SetSnapRange(long inSnapRange)
{
    CPreferences::GetUserPreferences().SetLongValue("SnapRange", inSnapRange);
}

//==============================================================================
/**
 *	Gets the delay between auto saves from the registry.
 *	@return	autosave delay in minutes.
 */
long CStudioPreferences::GetAutoSaveDelay()
{
    // default delay is 10 minutes (600 seconds)
    return CPreferences::GetUserPreferences("AutoSave").GetLongValue("Delay",
                                                                     DEFAULT_AUTOSAVE_DELAY);
}

//==============================================================================
/**
 *	Sets the delay between auto saves by saving value in registry.
 *	@param inDelay New autosave delay in minutes.
 */
void CStudioPreferences::SetAutoSaveDelay(long inAutoSaveDelay)
{
    CPreferences::GetUserPreferences("AutoSave").SetLongValue("Delay", inAutoSaveDelay);
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
    return CPreferences::GetUserPreferences("AutoSave").GetValue("Preference", true);
}

//==============================================================================
/**
 *	Sets user preference of whether to autosave.
 *	@param inActive user autosave preference.
 */
void CStudioPreferences::SetAutoSavePreference(bool inActive)
{
    CPreferences::GetUserPreferences("AutoSave").SetValue("Preference", inActive);
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
    return CPreferences::GetUserPreferences().GetLongValue("DefaultObjectLifetime",
                                                           DEFAULT_LIFETIME);
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
    CPreferences::GetUserPreferences().SetLongValue("DefaultObjectLifetime", inLifetime);
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
    return CPreferences::GetUserPreferences().GetValue("TimebarSetting", false);
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
    CPreferences::GetUserPreferences().SetValue("TimebarSetting", inDisplayTime);
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
    return CPreferences::GetUserPreferences().GetValue("AdvancePropertyFlag", false);
}

//==============================================================================
/**
 *	Sets the expanded flag for object when toggled
 *	@param inAdvancePropertyFlag Save the last toggle flag
 */
void CStudioPreferences::SetAdvancePropertyExpandedFlag(bool inAdvancePropertyFlag)
{
    CPreferences::GetUserPreferences().SetValue("AdvancePropertyFlag", inAdvancePropertyFlag);
}

Q3DStudio::CString CStudioPreferences::GetPreviewConfig()
{
    return CPreferences::GetUserPreferences().GetStringValue("Preview.Config", "");
}

void CStudioPreferences::SetPreviewConfig(const Q3DStudio::CString &inValue)
{
    CPreferences::GetUserPreferences().SetStringValue("Preview.Config", inValue);
}

// Preview Properties at the registry are prepend with Preview.
Q3DStudio::CString CStudioPreferences::GetPreviewProperty(const Q3DStudio::CString &inName)
{
    Q3DStudio::CString theName = "Preview." + inName;
    return CPreferences::GetUserPreferences().GetStringValue(theName, "");
}

void CStudioPreferences::SetPreviewProperty(const Q3DStudio::CString &inName,
                                            const Q3DStudio::CString &inValue)
{
    Q3DStudio::CString theName = "Preview." + inName;
    CPreferences::GetUserPreferences().SetStringValue(theName, inValue);
}

//=============================================================================
/**
 *	Get whether we should show OpenGL Version Warning or Error Dialog
 *	This is used to disable / enable OpenGL Version checking
 *	@return true if user don't want to see the GL Version warning dialog
 */
bool CStudioPreferences::GetDontShowGLVersionDialog()
{
    return CPreferences::GetUserPreferences().GetValue("DontShowGLVersionDialog", false);
}

//=============================================================================
/**
 *	Set whether we should show OpenGL Version Warning or Error Dialog
 *	This is used to disable / enable OpenGL Version checking
 */
void CStudioPreferences::SetDontShowGLVersionDialog(bool inValue)
{
    CPreferences::GetUserPreferences().SetValue("DontShowGLVersionDialog", inValue);
}

CPt CStudioPreferences::GetDefaultClientSize()
{
    CPt theSize;
    theSize.x = CPreferences::GetUserPreferences().GetLongValue("DefaultClientWidth",
                                                                DEFAULT_CLIENT_WIDTH);
    theSize.y = CPreferences::GetUserPreferences().GetLongValue("DefaultClientHeight",
                                                                DEFAULT_CLIENT_HEIGHT);
    return theSize;
}

void CStudioPreferences::SetDefaultClientSize(int width, int height)
{
    CPreferences::GetUserPreferences().SetLongValue("DefaultClientWidth", (long)width);
    CPreferences::GetUserPreferences().SetLongValue("DefaultClientHeight", (long)height);
}

//==============================================================================
/**
 * Get the amount of time that the playhead should be advanced/reduced when the
 * '.' and ',' keys are used.
 */
long CStudioPreferences::GetTimeAdvanceAmount()
{
    return CPreferences::GetUserPreferences().GetLongValue("TimeAdvance", DEFAULT_TIME_ADVANCE);
}

//==============================================================================
/**
 * Sets the amount of time that the playhead should be advanced/reduced when the
 * '.' and ',' keys are used.
 */
void CStudioPreferences::SetTimeAdvanceAmount(long inTime)
{
    CPreferences::GetUserPreferences().SetLongValue("TimeAdvance", inTime);
}

//==============================================================================
/**
 * Get the amount of time that the playhead should be advanced/reduced when the
 * '<' and '>' keys are used.
 */
long CStudioPreferences::GetBigTimeAdvanceAmount()
{
    return CPreferences::GetUserPreferences().GetLongValue("BigTimeAdvance",
                                                           DEFAULT_BIG_TIME_ADVANCE);
}

//==============================================================================
/**
 * Set the amount of time that the playhead should be advanced/reduced when the
 * '<' and '>' keys are used.
 */
void CStudioPreferences::SetBigTimeAdvanceAmount(long inTime)
{
    CPreferences::GetUserPreferences().SetLongValue("BigTimeAdvance", inTime);
}

//==============================================================================
/**
 * Color used in between most tree items in the timeline.
 */
::CColor CStudioPreferences::GetTreeFloorColor()
{
    return s_BaseColor;
}

//==============================================================================
/**
 * @return darker shadow color for use around the edges of most controls
 */
::CColor CStudioPreferences::GetButtonShadowColor()
{
    return s_BaseColor;
}

//==============================================================================
/**
 * Color to use between property rows and as the darker color behind the highlights
 * within the timeline.
 */
::CColor CStudioPreferences::GetPropertyFloorColor()
{
    return s_BaseColor;
}

::CColor CStudioPreferences::GetButtonHighlightColor()
{
    return s_BaseColor;
}

//==============================================================================
/**
 * Retrieves the color that should be used when the mouse goes over a row, such
 * as in the timeline or inspector palettes.
 */
::CColor CStudioPreferences::GetSelectColor()
{
    return ::CColor("#46a2da");
}

::CColor CStudioPreferences::GetTimelineSelectColor()
{
    return s_selectionColor; // The normal select color (#46a2da) does not work with master color
}

//==============================================================================
/**
 * Retrieves the color that should be used when the mouse goes over a row, such
 * as in the timeline or inspector palettes.
 */
::CColor CStudioPreferences::GetMouseOverHighlightColor()
{
    return s_MouseOverHighlightColor;
}
//==============================================================================
/**
 * @return the fill color of the top-most row of a palette (for instance, the fill color behind the
 * time measure)
 */
::CColor CStudioPreferences::GetRowTopColor()
{
    return s_RowTopColor;
}

::CColor CStudioPreferences::GetTimeBarBorderColor()
{
    return s_TimebarBorderColor;
}

//=============================================================================
/**
 * @return color to use for the background of a property row in the timeline
 */
::CColor CStudioPreferences::GetPropertyBackgroundColor()
{
    return s_BaseColor;
}

//=============================================================================
/**
 * @return color to use for the background of a property row in the timeline
 * when the mouse is over the row
 */
::CColor CStudioPreferences::GetPropertyMouseOverColor()
{
    return CStudioPreferences::GetMouseOverHighlightColor();
}

//=============================================================================
/**
 * @return color to use for the background of a property row in the timeline
 */
::CColor CStudioPreferences::GetComboEditBoxGradientStartColor()
{
    return s_ComboEditBoxGradientStartColor;
}

::CColor CStudioPreferences::GetPropertyTimbarLeft()
{
    return s_BaseColor;
}

//=============================================================================
/**
 * Returns the base color which is what most of the other colors derive from.
 * The base color is also used to fill in empty areas of some controls and
 * palettes.
 * @return Base color from which most of the other colors are derived
 */
::CColor CStudioPreferences::GetBaseColor()
{
    return s_BaseColor;
}

//=============================================================================
/**
 * @return a darker version of the base color
 */
::CColor CStudioPreferences::GetDarkBaseColor()
{
    return s_DarkBaseColor;
}

//=============================================================================
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

//=============================================================================
/**
 * Returns the background color of the toolbars. This is now just the base color.
 */
::CColor CStudioPreferences::GetToolBarBaseColor()
{
    return GetBaseColor();
}

//=============================================================================
/**
 * Returns the background color of the menu bar
 * sk - Ideally this would be the BaseColor (what the palettes are using) but there is a border with
 * the windows color that
 * I have not figured where it can be customized, so for the time being, that this fallback on the
 * old window system color (jason prefers that anyways)
 * This should be equivalent to if the associated code that uses this color, do not exist, but this
 * will allow us to easily change the color when we want to.
 */
::CColor CStudioPreferences::GetMenuBarBaseColor()
{
    QColor theWinColor = QPalette().color(QPalette::Background);
    return ::CColor(theWinColor.red(), theWinColor.green(), theWinColor.blue());
}

//=============================================================================
/**
 * Returns the "button down color" used by some procedurally drawn CButtonControls.
 */
::CColor CStudioPreferences::GetButtonDownColor()
{
    return s_ButtonDownColor;
}

::CColor CStudioPreferences::GetExtendedObjectLightColor()
{
    return s_ExtendedObjectLightColor;
}

::CColor CStudioPreferences::GetExtendedObjectDarkColor()
{
    return GetBaseColor();
}

//=============================================================================
/**
 * Controls, such as CTextEdit draw rects with different color sides around the
 * control.  This function retrieves the color for the top line of the rect.
 */
::CColor CStudioPreferences::GetControlRectTopLineColor()
{
    return s_ControlRectTopLineColor;
}

//=============================================================================
/**
 * Controls, such as CTextEdit draw rects with different color sides around the
 * control.  This function retrieves the color for the left and right lines of
 * the rect.
 */
::CColor CStudioPreferences::GetControlRectSideLineColor()
{
    return s_ControlRectSideLineColor;
}

//=============================================================================
/**
 * Controls, such as CTextEdit draw rects with different color sides around the
 * control.  This function retrieves the color for the bottom line of the rect.
 */
::CColor CStudioPreferences::GetControlRectBottomLineColor()
{
    return s_ControlRectBottomLineColor;
}

//=============================================================================
/**
 * Controls, such as CTextEdit draw rects with different color sides around the
 * control.  This function retrieves the color for the bottom line of the rect,
 * for certain controls that have a darker bottom line (see CComboTextBox).
 */
::CColor CStudioPreferences::GetControlRectBottomLineDarkColor()
{
    return s_ControlRectBottomLineDarkColor;
}

//=============================================================================
/**
 * Controls sometimes have a drop-shadow on the top of the control (an indented
 * control) or on the bottom (an outdented control).  This function returns the
 * appropriate color.
 */
::CColor CStudioPreferences::GetControlShadowColor()
{
    ::CColor theColor = GetBaseColor();
    return theColor;
}

//=============================================================================
/**
 * Fill color for text boxes that do not have focus.
 */
::CColor CStudioPreferences::GetTextBoxBGColorNoFocus()
{
    return s_BaseColor;
}

//=============================================================================
/**
 * Fill color for text boxes that do have focus.
 */
::CColor CStudioPreferences::GetTextBoxBGColorWithFocus()
{
    return s_TextBoxBGColorWithFocus;
}

//=============================================================================
/**
 * @return background color for scroll bars (behind thumb)
 */
::CColor CStudioPreferences::GetScrollBGColor()
{
    return s_ScrollBGColor;
}

//=============================================================================
/**
 * @return color of the scroll bar thumb
 */
::CColor CStudioPreferences::GetScrollThumbBGColor()
{
    return CColor("#999A9B");
}

//=============================================================================
/**
 * @return color of the scalable handle of the thumb
 */
::CColor CStudioPreferences::GetScrollThumbHighlightColor()
{
    return s_studioColor2;
}

//=============================================================================
/**
 * @return color for the outline of the scroll bar thumb (opposite side of the highlight)
 */
::CColor CStudioPreferences::GetLockedTimebarColor()
{
    return GetBaseColor();
}

::CColor CStudioPreferences::GetLockedBorderColor()
{
    return s_LockedBorderColor;
}

::CColor CStudioPreferences::GetExtendedLockedLightColor()
{
    return s_ExtendedLockedLightColor;
}

::CColor CStudioPreferences::GetExtendedLockedDarkColor()
{
    return GetLockedBorderColor();
}

//=============================================================================
/**
 * @return color for disabled (locked) text
 */
::CColor CStudioPreferences::GetLockedTextColor()
{
    return s_disabledColor;
}

//=============================================================================
/**
 * @return color for background of layer rows in the timeline
 */
::CColor CStudioPreferences::GetLayerBackgroundColor()
{
    return s_backgroundColor;
}

//=============================================================================
/**
 * @return color of timeline row background for layers when mouse is over
 */
::CColor CStudioPreferences::GetGroupBackgroundColor()
{
    return s_studioColor2;
}

//=============================================================================
/**
 * @return default color for rows in the timeline (if not specified by one of these other color
 * functions)
 */
::CColor CStudioPreferences::GetObjectBackgroundColor()
{
    return s_studioColor2;
}

//=============================================================================
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
::CColor CStudioPreferences::GetGroupTimebarColor()
{
    return ::CColor("#cbb0de");
}

::CColor CStudioPreferences::GetLayerTimebarColor()
{
    return ::CColor("#e7e0cd");
}

::CColor CStudioPreferences::GetBehaviorTimebarColor()
{
    return ::CColor("#91ba60");
}

::CColor CStudioPreferences::GetCameraTimebarColor()
{
    return ::CColor("#a0a1a2");
}

::CColor CStudioPreferences::GetLightTimebarColor()
{
    return ::CColor("#a0a1a2");
}

::CColor CStudioPreferences::GetModelTimebarColor()
{
    return ::CColor("#788ac5");
}

::CColor CStudioPreferences::GetComponentTimebarColor()
{
    return ::CColor("#bb7333");
}

::CColor CStudioPreferences::GetEffectTimebarColor()
{
    return ::CColor("#cb927f");
}

//==============================================================================
/**
 * Color for the top row of the inspector and timeline palettes (title row color).
 */
::CColor CStudioPreferences::GetTopRowColor()
{
    return s_TopRowColor;
}

//==============================================================================
/**
 *	Color when the button is down on a tab control
 */
::CColor CStudioPreferences::GetTabButtonDownColor()
{
    return s_TabButtonDownColor;
}

//==============================================================================
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

::CColor CStudioPreferences::GetLightBoundingBoxColor()
{
    return ::CColor("#ffff00");
}

//==============================================================================
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
    return CPreferences::GetUserPreferences().GetLongValue("SelectorLineWidth",
                                                           DEFAULT_SELECTOR_WIDTH) / 10.0f;
}

void CStudioPreferences::setSelectorLineWidth(float width)
{
    CPreferences::GetUserPreferences().SetLongValue("SelectorLineWidth", int(width * 10.0f));
}

float CStudioPreferences::getSelectorLineLength()
{
    return float(CPreferences::GetUserPreferences().GetLongValue("SelectorLineLength",
                                                                 DEFAULT_SELECTOR_LENGTH));
}

void CStudioPreferences::setSelectorLineLength(float length)
{
    CPreferences::GetUserPreferences().SetLongValue("SelectorLineLength", int(length));
}

//==============================================================================
/**
 * Return the standard timeline row height
 */
long CStudioPreferences::GetRowSize()
{
    return 20;
}

//==============================================================================
/**
 * Return the timebar tip size for the tip that lies outside the timebar.
 */
long CStudioPreferences::GetTimebarTipSize()
{
    return 6;
}

//==============================================================================
/**
 * Return the timebar tip size for the tip that lies inside the timebar.
 */
long CStudioPreferences::GetTimebarInnerTipSize()
{
    return 3;
}

//==============================================================================
/**
 * Return the timeline name text size
 */
long CStudioPreferences::GetTimelineNameSize()
{
    return 400;
}

//==============================================================================
/**
 * Return the default comment size
 */
long CStudioPreferences::GetDefaultCommentSize()
{
    return 1500;
}

//==============================================================================
/**
 * @return the default height of a text edit box
 */
long CStudioPreferences::GetDefaultTextEditSize()
{
    long theReturnValue = 15;
    return theReturnValue;
}

long CStudioPreferences::GetDefaultButtonWidth()
{
    return 16;
}

//==============================================================================
/**
 * @return The gutter size on the right and left edges of the inspector palette.
 */
long CStudioPreferences::GetInspectorGutterSize()
{
    return 5;
}

//==============================================================================
/**
 * @return The height of the top row of the timeline and inspector palettes.
 */
long CStudioPreferences::GetHeaderHeight()
{
    return 21;
}

bool CStudioPreferences::IsSudoMode()
{
    return m_SudoMode;
}

void CStudioPreferences::setQmlContextProperties(QQmlContext *qml)
{
    qml->setContextProperty(QStringLiteral("_studioColor1"), s_studioColor1);
    qml->setContextProperty(QStringLiteral("_studioColor2"), s_studioColor2);
    qml->setContextProperty(QStringLiteral("_studioColor3"), s_studioColor3);
    qml->setContextProperty(QStringLiteral("_backgroundColor"), s_backgroundColor);
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

Q3DStudio::CString CStudioPreferences::GetVersionString()
{
    Q3DStudio::CString theVersionNumber = STRINGIFY(STUDIO_VERSION);
    theVersionNumber.Replace(",", ".");

    return theVersionNumber;
}

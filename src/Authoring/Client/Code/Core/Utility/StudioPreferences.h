/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifndef INCLUDED_STUDIO_PREFERENCES_H
#define INCLUDED_STUDIO_PREFERENCES_H 1

#pragma once

#include <qglobal.h>

#include "CoreConst.h"
#include "Qt3DSFile.h"
#include "Preferences.h"
#include "Rct.h"

QT_FORWARD_DECLARE_CLASS(QQmlContext)

class CStudioPreferences
{
    CStudioPreferences();
    virtual ~CStudioPreferences();

public:
    static void LoadPreferences();

    static bool IsTimelineSnappingGridActive();
    static void SetTimelineSnappingGridActive(bool inActive);

    static ESnapGridResolution GetTimelineSnappingGridResolution();
    static void SetTimelineSnappingGridResolution(ESnapGridResolution inResolution);

    static CPreferences GetTrialSettings(const Q3DStudio::CString &inKeyName);

    static bool ShowHelpOnLaunch();
    static void SetShowHelpOnLaunch(bool inShowHelpOnLaunch);

    static ::CColor GetEditViewBackgroundColor();
    static void SetEditViewBackgroundColor(::CColor inColor);

    static bool GetEditViewFillMode();
    static void SetEditViewFillMode(bool inRenderAsSolid);

    static long GetPreferredStartupView();
    static void SetPreferredStartupView(long inStartupView);

    static bool IsAutosetKeyframesOn();
    static void SetAutosetKeyframesOn(bool inEnable);

    static bool IsBoundingBoxesOn();
    static void SetBoundingBoxesOn(bool inEnable);

    static bool ShouldDisplayPivotPoint();
    static void SetDisplayPivotPoint(bool inEnable);

    static bool IsWireframeModeOn();
    static void SetWireframeModeOn(bool inEnable);

    static bool ShouldShowTooltips();
    static void SetShowTooltips(bool inShowTooltips);

    static ::CColor GetTooltipBackgroundColor();

    static long GetTimelineSplitterLocation();
    static void SetTimelineSplitterLocation(long inLocation);

    static long GetInspectorSplitterLocation();
    static void SetInspectorSplitterLocation(long inLocation);

    static bool GetInterpolation();
    static void SetInterpolation(bool inSmooth);

    static long GetSnapRange();
    static void SetSnapRange(long inSnapRange);

    static long GetAutoSaveDelay();
    static void SetAutoSaveDelay(long inAutoSaveDelay);

    static bool GetAutoSavePreference();
    static void SetAutoSavePreference(bool inActive);

    static long GetDefaultObjectLifetime();
    static void SetDefaultObjectLifetime(long inLifetime);

    static bool GetTimebarDisplayTime();
    static void SetTimebarDisplayTime(bool inDisplayTime);

    static bool ShowVersionDialog();
    static void SetShowVersionDialog(bool inShowVersionDialog);

    static Q3DStudio::CString GetDefaultApplication();
    static void SetDefaultApplication(const Q3DStudio::CString &inApplication);

    static Q3DStudio::CString GetVersionString();

    static bool GetDontShowGLVersionDialog();
    static void SetDontShowGLVersionDialog(bool inValue);

    static CPt GetObjectReferenceGadgetSize();
    static void SetObjectReferenceGadgetSize(const CPt &inSize);

    static CPt GetObjectReferenceGadgetPosition(const CPt &inDefault);
    static void SetObjectReferenceGadgetPosition(const CPt &inPosition);

    static CPt GetPropertyReferenceGadgetSize();
    static void SetPropertyReferenceGadgetSize(const CPt &inSize);

    static CPt GetPropertyReferenceGadgetPosition(const CPt &inDefault);
    static void SetPropertyReferenceGadgetPosition(const CPt &inSize);

    static CPt GetHandlerReferenceGadgetSize();
    static void SetHandlerReferenceGadgetSize(const CPt &inSize);

    static CPt GetEventReferenceGadgetSize();
    static void SetEventReferenceGadgetSize(const CPt &inSize);

    static CRct GetMultilineTextLocation();
    static void SetMultilineTextLocation(const CRct &inSize);

    static long GetTimeAdvanceAmount();
    static void SetTimeAdvanceAmount(const long &inTime);
    static long GetBigTimeAdvanceAmount();
    static void SetBigTimeAdvanceAmount(const long &inTime);

    static long GetActionSplitterSize();
    static void SetActionSplitterSize(long inSize);

    static long GetCustomPropertySplitterSize();
    static void SetCustomPropertySplitterSize(long inSize);

    static long GetAssetSplitterSize();
    static void SetAssetSplitterSize(long inSize);

    static CPt GetDefaultClientSize();
    static bool GetCanImportComponent();

    static bool GetAdvancePropertyExpandedFlag();
    static void SetAdvancePropertyExpandedFlag(bool inAdvancePropertyFlag);

    static Q3DStudio::CString GetPreviewConfig();
    static void SetPreviewConfig(const Q3DStudio::CString &inValue);
    static Q3DStudio::CString GetPreviewProperty(const Q3DStudio::CString &inName);
    static void SetPreviewProperty(const Q3DStudio::CString &inName,
                                   const Q3DStudio::CString &inValue);

    static bool IsSlideIconVisible();
    static void SetSlideIconVisible(bool inVisible);

    static bool ShouldWriteMapFile();

    static ::CColor GetDefaultSceneColor();
    static ::CColor GetDefaultPrimitiveColor();

    static bool IsDebugTimes();

    static ::CColor GetBaseColor();
    static ::CColor GetDarkBaseColor();

    static ::CColor GetNormalColor();
    static ::CColor GetMasterColor();
    static ::CColor GetInactiveColor();

    static ::CColor GetMouseOverHighlightColor();
    static ::CColor GetSelectColor();
    static ::CColor GetTimelineSelectColor();

    static ::CColor GetToolBarBaseColor();
    static ::CColor GetMenuBarBaseColor();

    static ::CColor GetButtonDownColor();
    static ::CColor GetTreeFloorColor();
    static ::CColor GetButtonShadowColor();
    static ::CColor GetPropertyFloorColor();
    static ::CColor GetButtonHighlightColor();
    static ::CColor GetRowSelectedColor();
    static ::CColor GetRowTopColor();
    static ::CColor GetTimeBarBorderColor();
    static ::CColor GetPropertyBackgroundColor();
    static ::CColor GetPropertyMouseOverColor();
    static ::CColor GetPropertyTimbarLeft();
    static ::CColor GetExtendedObjectLightColor();
    static ::CColor GetExtendedObjectDarkColor();
    static ::CColor GetControlRectTopLineColor();
    static ::CColor GetControlRectSideLineColor();
    static ::CColor GetControlRectBottomLineColor();
    static ::CColor GetControlRectBottomLineDarkColor();
    static ::CColor GetControlShadowColor();
    static ::CColor GetTextBoxBGColorNoFocus();
    static ::CColor GetTextBoxBGColorWithFocus();
    static ::CColor GetScrollBGColor();
    static ::CColor GetScrollThumbBGColor();
    static ::CColor GetScrollThumbHighlightColor();
    static ::CColor GetComboEditBoxGradientStartColor();

    static ::CColor GetLockedTimebarColor();
    static ::CColor GetLockedBorderColor();
    static ::CColor GetExtendedLockedLightColor();
    static ::CColor GetExtendedLockedDarkColor();
    static ::CColor GetLockedTextColor();
    static ::CColor GetLayerBackgroundColor();
    static ::CColor GetGroupBackgroundColor();
    static ::CColor GetObjectBackgroundColor();
    static ::CColor GetObjectTimebarColor();
    static ::CColor GetGroupTimebarColor();
    static ::CColor GetLayerTimebarColor();
    static ::CColor GetBehaviorTimebarColor();
    static ::CColor GetEffectTimebarColor();
    static ::CColor GetCameraTimebarColor();
    static ::CColor GetLightTimebarColor();
    static ::CColor GetModelTimebarColor();
    static ::CColor GetComponentTimebarColor();
    static ::CColor GetTopRowColor();
    static ::CColor GetTabButtonDownColor();
    static ::CColor GetDisabledTextColor();

    static ::CColor GetSingleBoundingBoxColor();
    static ::CColor GetGroupBoundingBoxColor();
    static ::CColor GetLightBoundingBoxColor();

    static ::CColor GetRulerBackgroundColor();
    static ::CColor GetRulerTickColor();
    static ::CColor GetGuideColor();
    static ::CColor GetGuideSelectedColor();
    static ::CColor GetGuideFillColor();
    static ::CColor GetGuideFillSelectedColor();

    static QString GetFontFaceName();

    static long GetRowSize();
    static long GetTimebarTipSize();
    static long GetTimebarInnerTipSize();
    static long GetTimelineNameSize();
    static long GetDefaultCommentSize();
    static long GetDefaultTextEditSize();
    static long GetDefaultButtonWidth();
    static long GetInspectorGutterSize();
    static long GetHeaderHeight();

    static bool IsSudoMode();

    static void setQmlContextProperties(QQmlContext *qml);
    static QColor studioColor1();
    static QColor studioColor2();
    static QColor studioColor3();
    static QColor backgroundColor();
    static QColor guideColor();
    static QColor selectionColor();
    static QColor textColor();
    static QColor masterColor();
    static QColor disabledColor();
    static QColor dataInputColor();
    static int fontSize();
    static int controlBaseHeight();
    static int idWidth();
    static int valueWidth();
    static QSize browserPopupSize();

    // Default values that Studio will start out with or to restore
    static const long GUTTER_SIZE = 10;
    static const ::CColor EDITVIEW_DEFAULTBGCOLOR;
    static const long PREFERREDSTARTUP_DEFAULTINDEX = -1;

protected:
    static bool m_SudoMode;
    static bool m_DebugTimes;
};
#endif // INCLUDED_STUDIO_PREFERENCES_H

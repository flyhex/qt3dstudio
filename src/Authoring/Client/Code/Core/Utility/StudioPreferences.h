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

    static bool IsLegacyViewerActive();
    static void SetLegacyViewerActive(bool inActive);

    static ESnapGridResolution GetTimelineSnappingGridResolution();
    static void SetTimelineSnappingGridResolution(ESnapGridResolution inResolution);

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

    static long GetTimelineSplitterLocation();
    static void SetTimelineSplitterLocation(long inLocation);

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

    static Q3DStudio::CString GetVersionString();

    static bool GetDontShowGLVersionDialog();
    static void SetDontShowGLVersionDialog(bool inValue);

    static long GetTimeAdvanceAmount();
    static void SetTimeAdvanceAmount(long inTime);
    static long GetBigTimeAdvanceAmount();
    static void SetBigTimeAdvanceAmount(long inTime);

    static CPt GetDefaultClientSize();
    static void SetDefaultClientSize(int width, int height);

    static bool GetAdvancePropertyExpandedFlag();
    static void SetAdvancePropertyExpandedFlag(bool inAdvancePropertyFlag);

    static Q3DStudio::CString GetPreviewConfig();
    static void SetPreviewConfig(const Q3DStudio::CString &inValue);
    static Q3DStudio::CString GetPreviewProperty(const Q3DStudio::CString &inName);
    static void SetPreviewProperty(const Q3DStudio::CString &inName,
                                   const Q3DStudio::CString &inValue);

    static ::CColor GetNormalColor();
    static ::CColor GetMasterColor();
    static ::CColor GetInactiveColor();

    static ::CColor GetMouseOverHighlightColor();

    static ::CColor GetObjectTimebarColor();
    static ::CColor GetLayerTimebarColor();
    static ::CColor GetDisabledTextColor();

    static ::CColor GetSingleBoundingBoxColor();
    static ::CColor GetGroupBoundingBoxColor();

    static ::CColor GetRulerBackgroundColor();
    static ::CColor GetRulerTickColor();
    static ::CColor GetGuideColor();
    static ::CColor GetGuideSelectedColor();
    static ::CColor GetGuideFillColor();
    static ::CColor GetGuideFillSelectedColor();

    static QString GetFontFaceName();

    static float getSelectorLineWidth();
    static void setSelectorLineWidth(float width);
    static float getSelectorLineLength();
    static void setSelectorLineLength(float length);

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

    static QColor timelineRowColorNormal();
    static QColor timelineRowColorNormalProp();
    static QColor timelineRowColorOver();
    static QColor timelineRowColorSelected();
    static QColor timelineRowColorDurationOff1();
    static QColor timelineRowColorDurationOff2();
    static QColor timelineRowColorDurationEdge();
    static QColor timelineRowColorDurationSelected();
    static QColor timelineRowColorDndSource();
    static QColor timelineRowColorDndTarget();
    static QColor timelineRowMoverColor();
    static QColor timelineRulerColor();
    static QColor timelineRulerColorDisabled();
    static QColor timelineWidgetBgColor();
    static QColor timelinePlayheadLineColor();
    static QColor timelineFilterButtonSelectedColor();
    static QColor timelineFilterButtonHoveredColor();

    static int fontSize();
    static int controlBaseHeight();
    static int idWidth();
    static int valueWidth();
    static QSize browserPopupSize();

    // Default values that Studio will start out with or to restore
    static const long GUTTER_SIZE = 10;
    static const ::CColor EDITVIEW_DEFAULTBGCOLOR;
    static const long PREFERREDSTARTUP_DEFAULTINDEX = -1;
    static const long DEFAULT_SNAPRANGE = 10;
    static const long DEFAULT_LIFETIME = 10000;
    static const long DEFAULT_CLIENT_WIDTH = 1920;
    static const long DEFAULT_CLIENT_HEIGHT = 1080;
    static const long DEFAULT_TIME_ADVANCE = 100;
    static const long DEFAULT_BIG_TIME_ADVANCE = 200;
    static const long DEFAULT_SELECTOR_WIDTH = 30;
    static const long DEFAULT_SELECTOR_LENGTH = 50;
    static const long DEFAULT_AUTOSAVE_DELAY = 600;
};
#endif // INCLUDED_STUDIO_PREFERENCES_H

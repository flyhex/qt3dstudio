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
#include <QtGui/qbrush.h>

#include <memory>

#include "CoreConst.h"
#include "Preferences.h"
#include "Rct.h"

QT_FORWARD_DECLARE_CLASS(QQmlContext)

class CStudioPreferences
{
    CStudioPreferences();
    virtual ~CStudioPreferences();

public:
    static void loadPreferences(const QString &filePath);
    static void savePreferences();

    static bool IsTimelineSnappingGridActive();
    static void SetTimelineSnappingGridActive(bool inActive);

    static ESnapGridResolution GetTimelineSnappingGridResolution();
    static void SetTimelineSnappingGridResolution(ESnapGridResolution inResolution);

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

    static bool editModeLightingEnabled();
    static void setEditModeLightingEnabled(bool enabled);

    static bool GetTimebarDisplayTime();
    static void SetTimebarDisplayTime(bool inDisplayTime);

    static QString GetVersionString();

    static bool GetDontShowGLVersionDialog();
    static void SetDontShowGLVersionDialog(bool inValue);

    static long GetTimeAdvanceAmount();
    static void SetTimeAdvanceAmount(long inTime);
    static long GetBigTimeAdvanceAmount();
    static void SetBigTimeAdvanceAmount(long inTime);

    static QSize GetDefaultClientSize();
    static void SetDefaultClientSize(int width, int height);

    static int getNumRecentItems();
    static void setNumRecentItems(int n);

    static QString getRecentItem(int index);
    static void setRecentItem(int index, const QString &path);

    static bool GetAdvancePropertyExpandedFlag();
    static void SetAdvancePropertyExpandedFlag(bool inAdvancePropertyFlag);

    static QString GetPreviewConfig();
    static void SetPreviewConfig(const QString &inValue);
    static QString GetPreviewProperty(const QString &inName);
    static void SetPreviewProperty(const QString &inName, const QString &inValue);

    static QColor GetNormalColor();
    static QColor GetMasterColor();
    static QColor GetInactiveColor();

    static QColor GetMouseOverHighlightColor();

    static QColor GetObjectTimebarColor();
    static QColor GetLayerTimebarColor();
    static QColor GetDisabledTextColor();

    static QColor GetSingleBoundingBoxColor();
    static QColor GetGroupBoundingBoxColor();
    static QColor GetXAxisColor();
    static QColor GetYAxisColor();
    static QColor GetZAxisColor();

    static QColor GetRulerBackgroundColor();
    static QColor GetRulerTickColor();
    static QColor GetGuideColor();
    static QColor GetGuideSelectedColor();
    static QColor GetGuideFillColor();
    static QColor GetGuideFillSelectedColor();

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
    static QColor matteColor();
    static QColor projectReferencedColor();

    static QLinearGradient welcomeBackgroundGradient();

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
    static QColor timelineRowColorDndTargetSP();
    static QColor timelineRowMoverColor();
    static QColor timelineRulerColor();
    static QColor timelineRulerColorDisabled();
    static QColor timelineWidgetBgColor();
    static QColor timelinePlayheadLineColor();
    static QColor timelineFilterButtonSelectedColor();
    static QColor timelineFilterButtonHoveredColor();
    static QColor timelineRowSubpColor();
    static QColor timelineRowSubpDescendantColor();
    static QColor timelineRowCommentBgColor();
    static QColor timelinePressedKeyframeColor();

    static QColor invalidDataInputIndicatorColor();

    static int fontSize();
    static int controlBaseHeight();
    static int idWidth();
    static int valueWidth();
    static QSize browserPopupSize();
    static int rulerSize();

    // Default values that Studio will start out with or to restore
    static const long GUTTER_SIZE = 10;
    static const QColor EDITVIEW_DEFAULTBGCOLOR;
    static const long PREFERREDSTARTUP_DEFAULTINDEX = -1;
    static const long DEFAULT_SNAPRANGE = 10;
    static const long DEFAULT_LIFETIME = 10000;
    static const long DEFAULT_CLIENT_WIDTH = 1920;
    static const long DEFAULT_CLIENT_HEIGHT = 1080;
    static const long DEFAULT_TIME_ADVANCE = 100;
    static const long DEFAULT_BIG_TIME_ADVANCE = 500;
    static const long DEFAULT_SELECTOR_WIDTH = 30;
    static const long DEFAULT_SELECTOR_LENGTH = 50;
    static const long DEFAULT_AUTOSAVE_DELAY = 600;

private:
    static std::unique_ptr<CPreferences> m_preferences;
};
#endif // INCLUDED_STUDIO_PREFERENCES_H

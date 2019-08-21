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

#include "CoreConst.h"
#include "Rct.h"
#include "CColor.h"

QT_FORWARD_DECLARE_CLASS(QQmlContext)

class CStudioPreferences
{
    CStudioPreferences();
    virtual ~CStudioPreferences();

public:
    static void loadPreferences();
    static void savePreferences();

    static bool isTimelineSnappingGridActive();
    static void setTimelineSnappingGridActive(bool inActive);

    static bool isLegacyViewerActive();
    static void setLegacyViewerActive(bool inActive);

    static ESnapGridResolution timelineSnappingGridResolution();
    static void setTimelineSnappingGridResolution(ESnapGridResolution inResolution);

    static bool isEditViewFillMode();
    static void setEditViewFillMode(bool inRenderAsSolid);

    static int preferredStartupView();
    static void setPreferredStartupView(int inStartupView);

    static bool isAutosetKeyframesOn();
    static void setAutosetKeyframesOn(bool inEnable);

    static bool isBoundingBoxesOn();
    static void setBoundingBoxesOn(bool inEnable);

    static bool isPivotPointOn();
    static void setPivotPointOn(bool inEnable);

    static bool isWireframeModeOn();
    static void setWireframeModeOn(bool inEnable);

    static bool isTooltipsOn();
    static void setTooltipsOn(bool inShowTooltips);

    static bool isHelperGridOn();
    static void setHelperGridOn(bool showGrid);

    static int helperGridLines();
    static void setHelperGridLines(int lines);

    static int helperGridSpacing();
    static void setHelperGridSpacing(int spacing);

    static double timelineSplitterLocation();
    static void setTimelineSplitterLocation(double inLocation);

    static bool isInterpolation();
    static void setInterpolation(bool inSmooth);

    static double snapRange();
    static void setSnapRange(double inSnapRange);

    static int autoSaveDelay();
    static void setAutoSaveDelay(int inAutoSaveDelay);

    static bool isAutoSavePreference();
    static void setAutoSavePreference(bool inActive);

    static bool isEditModeLightingEnabled();
    static void setEditModeLightingEnabled(bool enabled);

    static bool isTimebarDisplayTime();
    static void setTimebarDisplayTime(bool inDisplayTime);

    static bool isDontShowGLVersionDialog();
    static void setDontShowGLVersionDialog(bool inValue);

    static double timeAdvanceAmount();
    static void setTimeAdvanceAmount(double inTime);
    static double bigTimeAdvanceAmount();
    static void setBigTimeAdvanceAmount(double inTime);

    static QSize defaultClientSize();
    static void setDefaultClientSize(int width, int height);

    static int numRecentItems();
    static void setNumRecentItems(int numberOfItems);

    static QString recentItem(int index);
    static void setRecentItem(int index, const QString &path);

    static QString previewConfig();
    static void setPreviewConfig(const QString &inValue);
    static QString previewProperty(const QString &inName);
    static void setPreviewProperty(const QString &inName, const QString &inValue);

    static float selectorLineWidth();
    static void setSelectorLineWidth(float width);
    static float selectorLineLength();
    static void setSelectorLineLength(float length);

    static QString versionString();

    static ::CColor normalColor();
    static ::CColor inactiveColor();

    static ::CColor mouseOverHighlightColor();

    static ::CColor objectTimebarColor();
    static ::CColor layerTimebarColor();
    static ::CColor disabledTextColor();

    static ::CColor singleBoundingBoxColor();
    static ::CColor groupBoundingBoxColor();
    static QColor xAxisColor();
    static QColor yAxisColor();
    static QColor zAxisColor();
    static QColor wAxisColor();
    static QColor helperGridColor();
    static QColor bezierControlColor();

    static ::CColor rulerBackgroundColor();
    static ::CColor rulerTickColor();
    static ::CColor guideNormalColor();
    static ::CColor guideSelectedColor();
    static ::CColor guideFillColor();
    static ::CColor guideFillSelectedColor();

    static QString fontFaceName();

    static void setQmlContextProperties(QQmlContext *qml);
    static QColor studioColor1();
    static QColor studioColor2();
    static QColor studioColor3();
    static QColor studioColor4();
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

    // Default values that Studio will start out with or to restore
    static const int PREFERREDSTARTUP_DEFAULTINDEX = -1;
    static constexpr double DEFAULT_SNAPRANGE = 10.;
    static const int DEFAULT_CLIENT_WIDTH = 1920;
    static const int DEFAULT_CLIENT_HEIGHT = 1080;
    static constexpr double DEFAULT_TIME_ADVANCE = 100.;
    static constexpr double DEFAULT_BIG_TIME_ADVANCE = 500.;
    static constexpr float DEFAULT_SELECTOR_WIDTH = 30.f;
    static constexpr float DEFAULT_SELECTOR_LENGTH = 50.f;
    static const int DEFAULT_AUTOSAVE_DELAY = 600;
};
#endif // INCLUDED_STUDIO_PREFERENCES_H

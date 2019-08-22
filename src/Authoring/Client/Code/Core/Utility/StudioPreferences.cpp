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
#include "CColor.h"
#include "MasterP.h"

#include <QtGui/qpalette.h>
#include <QtQml/qqmlcontext.h>
#include <QtCore/qsettings.h>

static QColor s_studioColor1;
static QColor s_studioColor2;
static QColor s_studioColor3;
static QColor s_studioColor4;
static QColor s_buttonDownColor;
static QColor s_mouseOverHighlightColor;
static QColor s_backgroundColor;
static QColor s_guideColor;
static QColor s_selectionColor;
static QColor s_textColor;
static QColor s_disabledTextColor;
static QColor s_masterColor;
static QColor s_disabledColor;
static QColor s_dataInputColor;
static QColor s_matteColor;
static QColor s_projectReferencedColor;
static QColor s_xAxisColor;
static QColor s_yAxisColor;
static QColor s_zAxisColor;
static QColor s_wAxisColor;
static QColor s_helperGridColor;
static QColor s_inspectorGroupHeaderColor;
static QColor s_variantsSlideViewBGColor;
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
static QColor s_timelineRowCommentBgColor;
static QColor s_timelinePressedKeyframeColor; // pressed keyframe from multiple selection
static QColor s_invalidDataInputIndicatorColor;

static int s_fontSize;
static int s_controlBaseHeight;
static int s_idWidth;
static int s_valueWidth;
static QSize s_browserPopupSize;

static QSettings s_preferences(QStringLiteral("The Qt Company"),
                               QStringLiteral("Qt 3D Studio"));

#define STRINGIFY(x) STRINGIFY2(x)
#define STRINGIFY2(x) #x

CStudioPreferences::CStudioPreferences()
{
}

CStudioPreferences::~CStudioPreferences()
{
}

void CStudioPreferences::loadPreferences()
{
    s_studioColor1 = QColor("#262829");
    s_studioColor2 = QColor("#404244");
    s_studioColor3 = QColor("#727476");
    s_studioColor4 = QColor("#959596");
    s_buttonDownColor = s_studioColor1.lighter(90);
    s_mouseOverHighlightColor = s_studioColor1.lighter(95);
    s_backgroundColor = QColor("#2e2f30");
    s_guideColor = QColor("#f4be04");
    s_selectionColor = QColor("#23516D");
    s_textColor = QColor("#ffffff");
    s_disabledTextColor = QColor("#808080");
    s_masterColor = QColor("#5caa15");
    s_disabledColor = s_studioColor3;
    s_dataInputColor = QColor("#ff5102");
    s_matteColor = QColor("#222222");
    s_projectReferencedColor = QColor("#aaaa00");
    s_xAxisColor = QColor("#ca2f2e");
    s_yAxisColor = QColor("#64cd35");
    s_zAxisColor = QColor("#068ac2");
    s_wAxisColor = QColor("#fc0388");
    s_helperGridColor = QColor("#515151");
    s_inspectorGroupHeaderColor = QColor("#111111");
    s_variantsSlideViewBGColor = QColor("#111111");

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
    s_timelineRowCommentBgColor = QColor("#d0000000");
    s_timelinePressedKeyframeColor = QColor("#ffff00");

    s_invalidDataInputIndicatorColor = QColor("#ff2121");

    s_fontSize = 12;
    s_controlBaseHeight = 22;
    s_idWidth = 130;
    s_valueWidth = 250;
    s_browserPopupSize = QSize(400, 400);
}

void CStudioPreferences::savePreferences()
{
    s_preferences.sync();
}

//
// Settings to save in QSettings
//

// MainWindow settings
//
QByteArray CStudioPreferences::windowGeometry(int version)
{
    QString geoKey = QStringLiteral("MainWindow/MainWindowGeometry")
            + QString::number(version);
    return s_preferences.value(geoKey).toByteArray();
}

void CStudioPreferences::setWindowGeometry(const QByteArray &geometry, int version)
{
    QString geoKey = QStringLiteral("MainWindow/MainWindowGeometry")
            + QString::number(version);
    s_preferences.setValue(geoKey, geometry);
}

void CStudioPreferences::resetWindowGeometry(int version)
{
    QString geoKey = QStringLiteral("MainWindow/MainWindowGeometry")
            + QString::number(version);
    s_preferences.remove(geoKey);
}

bool CStudioPreferences::containsWindowState(int version)
{
    QString stateKey = QStringLiteral("MainWindow/MainWindowState")
            + QString::number(version);
    return s_preferences.contains(stateKey);
}

QByteArray CStudioPreferences::windowState(int version)
{
    QString stateKey = QStringLiteral("MainWindow/MainWindowState")
            + QString::number(version);
    return s_preferences.value(stateKey).toByteArray();
}

void CStudioPreferences::setWindowState(const QByteArray &state, int version)
{
    QString stateKey = QStringLiteral("MainWindow/MainWindowState")
            + QString::number(version);
    s_preferences.setValue(stateKey, state);
}

void CStudioPreferences::resetWindowState(int version)
{
    QString stateKey = QStringLiteral("MainWindow/MainWindowState")
            + QString::number(version);
    s_preferences.remove(stateKey);
}

// Viewing settings
//
bool CStudioPreferences::isLegacyViewerActive()
{
    return s_preferences.value(QStringLiteral("Viewing/LegacyViewerActive"), false).toBool();
}

void CStudioPreferences::setLegacyViewerActive(bool inActive)
{
    s_preferences.setValue(QStringLiteral("Viewing/LegacyViewerActive"), inActive);
}

bool CStudioPreferences::isEditViewFillMode()
{
    return s_preferences.value(QStringLiteral("Viewing/EditViewFillMode"), true).toBool();
}

void CStudioPreferences::setEditViewFillMode(bool inRenderAsSolid)
{
    s_preferences.setValue(QStringLiteral("Viewing/EditViewFillMode"), inRenderAsSolid);
}

int CStudioPreferences::preferredStartupView()
{
    return s_preferences.value(QStringLiteral("Viewing/PreferredStartupView"),
                               PREFERREDSTARTUP_DEFAULTINDEX).toInt();
}

void CStudioPreferences::setPreferredStartupView(int inStartupView)
{
    s_preferences.setValue(QStringLiteral("Viewing/PreferredStartupView"), int(inStartupView));
}

bool CStudioPreferences::isDontShowGLVersionDialog()
{
    return s_preferences.value(QStringLiteral("Viewing/DontShowGLVersionDialog"), false).toBool();
}

void CStudioPreferences::setDontShowGLVersionDialog(bool inValue)
{
    s_preferences.setValue(QStringLiteral("Viewing/DontShowGLVersionDialog"), inValue);
}

QSize CStudioPreferences::defaultClientSize()
{
    QSize theSize;
    theSize.setWidth(s_preferences.value(QStringLiteral("Viewing/DefaultClientWidth"),
                                         DEFAULT_CLIENT_WIDTH).toInt());
    theSize.setHeight(s_preferences.value(QStringLiteral("Viewing/DefaultClientHeight"),
                                          DEFAULT_CLIENT_HEIGHT).toInt());
    return theSize;
}

void CStudioPreferences::setDefaultClientSize(int width, int height)
{
    s_preferences.setValue(QStringLiteral("Viewing/DefaultClientWidth"), width);
    s_preferences.setValue(QStringLiteral("Viewing/DefaultClientHeight"), height);
}

bool CStudioPreferences::isEditModeLightingEnabled()
{
    return s_preferences.value(QStringLiteral("Viewing/EditModeLightingEnabled"), true).toBool();
}

void CStudioPreferences::setEditModeLightingEnabled(bool enabled)
{
    s_preferences.setValue(QStringLiteral("Viewing/EditModeLightingEnabled"), enabled);
}

bool CStudioPreferences::containsShowWelcomeScreen()
{
    return s_preferences.contains(QStringLiteral("Viewing/ShowWelcomeScreen"));
}

bool CStudioPreferences::isShowWelcomeScreen()
{
    return s_preferences.value(QStringLiteral("Viewing/ShowWelcomeScreen")).toBool();
}

void CStudioPreferences::setShowWelcomeScreen(bool show)
{
    s_preferences.setValue(QStringLiteral("Viewing/ShowWelcomeScreen"), show);
}

// Timeline settings
//
bool CStudioPreferences::isTimelineSnappingGridActive()
{
    return s_preferences.value(QStringLiteral("Timeline/SnappingGridActive"), true).toBool();
}

void CStudioPreferences::setTimelineSnappingGridActive(bool inActive)
{
    s_preferences.setValue(QStringLiteral("Timeline/SnappingGridActive"), inActive);
}

ESnapGridResolution CStudioPreferences::timelineSnappingGridResolution()
{
    return ESnapGridResolution(s_preferences.value(
                                   QStringLiteral("Timeline/SnappingGridResolution"),
                                   SNAPGRID_SECONDS).toInt());
}

void CStudioPreferences::setTimelineSnappingGridResolution(ESnapGridResolution inResolution)
{
    s_preferences.setValue(QStringLiteral("Timeline/SnappingGridResolution"), int(inResolution));
}

bool CStudioPreferences::isAutosetKeyframesOn()
{
    return s_preferences.value(QStringLiteral("Timeline/AutosetKeyframes"), true).toBool();
}

void CStudioPreferences::setAutosetKeyframesOn(bool inEnable)
{
    s_preferences.setValue(QStringLiteral("Timeline/AutosetKeyframes"), inEnable);
}

double CStudioPreferences::timelineSplitterLocation()
{
    return s_preferences.value(QStringLiteral("Timeline/SplitterLocation"), 250).toDouble();
}

void CStudioPreferences::setTimelineSplitterLocation(double inLocation)
{
    s_preferences.setValue(QStringLiteral("Timeline/SplitterLocation"), inLocation);
}

bool CStudioPreferences::isInterpolation()
{
    return s_preferences.value(QStringLiteral("Timeline/InterpolationPreference"), true).toBool();
}

void CStudioPreferences::setInterpolation(bool inSmooth)
{
    s_preferences.setValue(QStringLiteral("Timeline/InterpolationPreference"), inSmooth);
}

double CStudioPreferences::snapRange()
{
    return s_preferences.value(QStringLiteral("Timeline/SnapRange"), DEFAULT_SNAPRANGE).toDouble();
}

void CStudioPreferences::setSnapRange(double inSnapRange)
{
    s_preferences.setValue(QStringLiteral("Timeline/SnapRange"), inSnapRange);
}

bool CStudioPreferences::isTimebarDisplayTime()
{
    return s_preferences.value(QStringLiteral("Timeline/DisplayTime"), false).toBool();
}

void CStudioPreferences::setTimebarDisplayTime(bool inDisplayTime)
{
    s_preferences.setValue(QStringLiteral("Timeline/DisplayTime"), inDisplayTime);
}

double CStudioPreferences::timeAdvanceAmount()
{
    return s_preferences.value(QStringLiteral("Timeline/TimeAdvance"),
                               DEFAULT_TIME_ADVANCE).toDouble();
}

void CStudioPreferences::setTimeAdvanceAmount(double inTime)
{
    s_preferences.setValue(QStringLiteral("Timeline/TimeAdvance"), inTime);
}

double CStudioPreferences::bigTimeAdvanceAmount()
{
    return s_preferences.value(QStringLiteral("Timeline/BigTimeAdvance"),
                               DEFAULT_BIG_TIME_ADVANCE).toDouble();
}

void CStudioPreferences::setBigTimeAdvanceAmount(double inTime)
{
    s_preferences.setValue(QStringLiteral("Timeline/BigTimeAdvance"), inTime);
}

// VisualAids settings
//
bool CStudioPreferences::isBoundingBoxesOn()
{
    return s_preferences.value(QStringLiteral("VisualAids/BoundingBoxes"), true).toBool();
}

void CStudioPreferences::setBoundingBoxesOn(bool inEnable)
{
    s_preferences.setValue(QStringLiteral("VisualAids/BoundingBoxes"), inEnable);
}

bool CStudioPreferences::isPivotPointOn()
{
    return s_preferences.value(QStringLiteral("VisualAids/DisplayPivotPoint"), true).toBool();
}

void CStudioPreferences::setPivotPointOn(bool inEnable)
{
    s_preferences.setValue(QStringLiteral("VisualAids/DisplayPivotPoint"), inEnable);
}

bool CStudioPreferences::isWireframeModeOn()
{
    return s_preferences.value(QStringLiteral("VisualAids/WireframeMode"), true).toBool();
}

void CStudioPreferences::setWireframeModeOn(bool inEnable)
{
    s_preferences.setValue(QStringLiteral("VisualAids/WireframeMode"), inEnable);
}

bool CStudioPreferences::isTooltipsOn()
{
    return s_preferences.value(QStringLiteral("VisualAids/ShowTooltips"), true).toBool();
}

void CStudioPreferences::setTooltipsOn(bool inShowTooltips)
{
    s_preferences.setValue(QStringLiteral("VisualAids/ShowTooltips"), inShowTooltips);
}

bool CStudioPreferences::isHelperGridOn()
{
    return s_preferences.value(QStringLiteral("VisualAids/ShowHelperGrid"), true).toBool();
}

void CStudioPreferences::setHelperGridOn(bool showGrid)
{
    s_preferences.setValue(QStringLiteral("VisualAids/ShowHelperGrid"), showGrid);
}

int CStudioPreferences::helperGridLines()
{
    return s_preferences.value(QStringLiteral("VisualAids/HelperGridLines"), 10).toInt();
}

void CStudioPreferences::setHelperGridLines(int lines)
{
    s_preferences.setValue(QStringLiteral("VisualAids/HelperGridLines"), lines);
}

int CStudioPreferences::helperGridSpacing()
{
    return s_preferences.value(QStringLiteral("VisualAids/HelperGridSpacing"), 100).toInt();
}

void CStudioPreferences::setHelperGridSpacing(int spacing)
{
    s_preferences.setValue(QStringLiteral("VisualAids/HelperGridSpacing"), spacing);
}

float CStudioPreferences::selectorLineWidth()
{
    return s_preferences.value(QStringLiteral("VisualAids/SelectorLineWidth"),
                               DEFAULT_SELECTOR_WIDTH).toFloat() / 10.0f;
}

void CStudioPreferences::setSelectorLineWidth(float width)
{
    s_preferences.setValue(QStringLiteral("VisualAids/SelectorLineWidth"), int(width * 10.0f));
}

float CStudioPreferences::selectorLineLength()
{
    return s_preferences.value(QStringLiteral("VisualAids/SelectorLineLength"),
                               DEFAULT_SELECTOR_LENGTH).toFloat();
}

void CStudioPreferences::setSelectorLineLength(float length)
{
    s_preferences.setValue(QStringLiteral("VisualAids/SelectorLineLength"), length);
}

// Autosave settings
//
int CStudioPreferences::autoSaveDelay()
{
    // default delay is 10 minutes (600 seconds)
    return s_preferences.value(QStringLiteral("Autosave/Delay"), DEFAULT_AUTOSAVE_DELAY).toInt();
}

void CStudioPreferences::setAutoSaveDelay(int inAutoSaveDelay)
{
    s_preferences.setValue(QStringLiteral("Autosave/Delay"), inAutoSaveDelay);
}

bool CStudioPreferences::isAutoSavePreference()
{
    return s_preferences.value(QStringLiteral("Autosave/Preference"), true).toBool();
}

void CStudioPreferences::setAutoSavePreference(bool inActive)
{
    s_preferences.setValue(QStringLiteral("Autosave/Preference"), inActive);
}

// Preview settings
//
QString CStudioPreferences::previewConfig()
{
    return s_preferences.value(QStringLiteral("Preview/Config")).toString();
}

void CStudioPreferences::setPreviewConfig(const QString &inValue)
{
    s_preferences.setValue(QStringLiteral("Preview/Config"), inValue);
}

QString CStudioPreferences::previewProperty(const QString &inName)
{
    QString theName = QStringLiteral("Preview/") + inName;
    return s_preferences.value(theName).toString();
}

void CStudioPreferences::setPreviewProperty(const QString &inName, const QString &inValue)
{
    QString theName = QStringLiteral("Preview/") + inName;
    s_preferences.setValue(theName, inValue);
}

QString CStudioPreferences::remoteDeploymentIP()
{
    return s_preferences.value(QStringLiteral("Preview/LastRemoteDeploymentIP"),
                               QStringLiteral("127.0.0.1")).toString();
}

void CStudioPreferences::setRemoteDeploymentIP(const QString &ip)
{
    s_preferences.setValue(QStringLiteral("Preview/LastRemoteDeploymentIP"), ip);
}

QString CStudioPreferences::remoteDeploymentPort()
{
    return s_preferences.value(QStringLiteral("Preview/LastRemoteDeploymentPort"),
                               QStringLiteral("36000")).toString();
}

void CStudioPreferences::setRemoteDeploymentPort(const QString &port)
{
    s_preferences.setValue(QStringLiteral("Preview/LastRemoteDeploymentPort"), port);
}

// Recent settings
//
int CStudioPreferences::numRecentItems()
{
    return s_preferences.value(QStringLiteral("Recent/Valid"), 0).toInt();
}

void CStudioPreferences::setNumRecentItems(int numberOfItems)
{
    s_preferences.setValue(QStringLiteral("Recent/Valid"), numberOfItems);
}

QString CStudioPreferences::recentItem(int index)
{
    return s_preferences.value(QStringLiteral("Recent/Item") + QString::number(index)).toString();
}

void CStudioPreferences::setRecentItem(int index, const QString &path)
{
    s_preferences.setValue(QStringLiteral("Recent/Item") + QString::number(index), path);
}

//
// Other settings
//
void CStudioPreferences::setQmlContextProperties(QQmlContext *qml)
{
    qml->setContextProperty(QStringLiteral("_studioColor1"), s_studioColor1);
    qml->setContextProperty(QStringLiteral("_studioColor2"), s_studioColor2);
    qml->setContextProperty(QStringLiteral("_studioColor3"), s_studioColor3);
    qml->setContextProperty(QStringLiteral("_studioColor4"), s_studioColor4);
    qml->setContextProperty(QStringLiteral("_backgroundColor"), s_backgroundColor);
    qml->setContextProperty(QStringLiteral("_buttonDownColor"), s_buttonDownColor);
    qml->setContextProperty(QStringLiteral("_guideColor"), s_guideColor);
    qml->setContextProperty(QStringLiteral("_selectionColor"), s_selectionColor);
    qml->setContextProperty(QStringLiteral("_textColor"), s_textColor);
    qml->setContextProperty(QStringLiteral("_masterColor"), s_masterColor);
    qml->setContextProperty(QStringLiteral("_disabledColor"), s_disabledColor);
    qml->setContextProperty(QStringLiteral("_dataInputColor"), s_dataInputColor);
    qml->setContextProperty(QStringLiteral("_projectReferencedColor"), s_projectReferencedColor);
    qml->setContextProperty(QStringLiteral("_xAxisColor"), s_xAxisColor);
    qml->setContextProperty(QStringLiteral("_yAxisColor"), s_yAxisColor);
    qml->setContextProperty(QStringLiteral("_zAxisColor"), s_zAxisColor);
    qml->setContextProperty(QStringLiteral("_wAxisColor"), s_wAxisColor);
    qml->setContextProperty(QStringLiteral("_fontSize"), s_fontSize);
    qml->setContextProperty(QStringLiteral("_controlBaseHeight"), s_controlBaseHeight);
    qml->setContextProperty(QStringLiteral("_idWidth"), s_idWidth);
    qml->setContextProperty(QStringLiteral("_valueWidth"), s_valueWidth);
    qml->setContextProperty(QStringLiteral("_inspectorGroupHeaderColor"),
                            s_inspectorGroupHeaderColor);
    qml->setContextProperty(QStringLiteral("_variantsSlideViewBGColor"),
                            s_variantsSlideViewBGColor);
}

QString CStudioPreferences::fontFaceName()
{
    return QStringLiteral("Segoe UI");
}

::CColor CStudioPreferences::mouseOverHighlightColor()
{
    return s_mouseOverHighlightColor;
}

::CColor CStudioPreferences::normalColor()
{
    return s_textColor;
}

::CColor CStudioPreferences::inactiveColor()
{
    return s_disabledColor;
}

::CColor CStudioPreferences::objectTimebarColor()
{
    return ::CColor("#788ac5");
}

::CColor CStudioPreferences::layerTimebarColor()
{
    return ::CColor("#e7e0cd");
}

::CColor CStudioPreferences::disabledTextColor()
{
    return s_disabledTextColor;
}

::CColor CStudioPreferences::singleBoundingBoxColor()
{
    return ::CColor("#ff0000");
}

::CColor CStudioPreferences::groupBoundingBoxColor()
{
    return ::CColor("#ff0000");
}

QColor CStudioPreferences::xAxisColor()
{
    return s_xAxisColor;
}

QColor CStudioPreferences::yAxisColor()
{
    return s_yAxisColor;
}

QColor CStudioPreferences::zAxisColor()
{
    return s_zAxisColor;
}

QColor CStudioPreferences::wAxisColor()
{
    return s_wAxisColor;
}

QColor CStudioPreferences::helperGridColor()
{
    return s_helperGridColor;
}

QColor CStudioPreferences::bezierControlColor()
{
    return QColor("#f4bd04");
}

::CColor CStudioPreferences::rulerBackgroundColor()
{
    return s_studioColor1;
}

::CColor CStudioPreferences::rulerTickColor()
{
    return s_studioColor3;
}

::CColor CStudioPreferences::guideNormalColor()
{
    return ::CColor("#7a5f02"); // #f4be04 plus faked alpha 50%
}

::CColor CStudioPreferences::guideSelectedColor()
{
    return s_guideColor;
}

::CColor CStudioPreferences::guideFillColor()
{
    return ::CColor("#140F00"); // #f4be04 plus faked alpha 8%
}

::CColor CStudioPreferences::guideFillSelectedColor()
{
    return ::CColor("#7a5f02"); // #f4be04 plus faked alpha 50%
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

QColor CStudioPreferences::studioColor4()
{
    return s_studioColor4;
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

QColor CStudioPreferences::matteColor()
{
    return s_matteColor;
}

QColor CStudioPreferences::projectReferencedColor()
{
    return s_projectReferencedColor;
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

QColor CStudioPreferences::timelineRowCommentBgColor()
{
    return s_timelineRowCommentBgColor;
}

QColor CStudioPreferences::timelinePressedKeyframeColor()
{
    return s_timelinePressedKeyframeColor;
}

QColor CStudioPreferences::invalidDataInputIndicatorColor()
{
    return s_invalidDataInputIndicatorColor;
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

QString CStudioPreferences::versionString()
{
    QString theVersionNumber = STRINGIFY(STUDIO_VERSION);
    theVersionNumber.replace(QLatin1String(","), QLatin1String("."));

    return theVersionNumber;
}

/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "RowTree.h"
#include "RowTimeline.h"
#include "RowTimelinePropertyGraph.h"
#include "RowManager.h"
#include "TimelineConstants.h"
#include "StudioObjectTypes.h"
#include "TimelineGraphicsScene.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/Qt3DSDMTimelineItemBinding.h"
#include "Qt3DSString.h"
#include "TreeHeader.h"
#include "StudioPreferences.h"
#include "KeyframeManager.h"
#include "StudioApp.h"
#include "MainFrm.h"
#include "Core.h"
#include "Doc.h"
#include "ClientDataModelBridge.h"
#include "Qt3DSDMStudioSystem.h"
#include "Qt3DSDMSlides.h"
#include "StudioUtils.h"
#include "TimelineToolbar.h"
#include "HotKeys.h"

#include <QtGui/qpainter.h>
#include "QtGui/qtextcursor.h"
#include <QtWidgets/qgraphicslinearlayout.h>
#include <QtWidgets/qgraphicssceneevent.h>

// object row constructor
RowTree::RowTree(TimelineGraphicsScene *timelineScene, EStudioObjectType objType,
                 const QString &label)
    : m_rowTimeline(new RowTimeline())
    , m_scene(timelineScene)
    , m_objectType(objType)
    , m_label(label)
{
    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    m_onMasterSlide = doc->GetStudioSystem()->GetSlideSystem()
                         ->IsMasterSlide(doc->GetActiveSlide());

    initialize();
}

// property row constructor
RowTree::RowTree(TimelineGraphicsScene *timelineScene, const QString &propType)
    : InteractiveTimelineItem()
    , m_rowTimeline(new RowTimeline())
    , m_isProperty(true)
    , m_scene(timelineScene)
    , m_label(propType)
{
    m_rowTimeline->m_isProperty = true;

    initialize();
}

RowTree::~RowTree()
{
    delete m_rowTimeline; // this will also delete the keyframes
    m_rowTimeline = nullptr;
}

ITimelineItemBinding *RowTree::getBinding() const
{
    return m_binding;
}

// object instance handle
 qt3dsdm::Qt3DSDMInstanceHandle RowTree::instance() const
{
    if (m_isProperty || !m_binding)
        return 0;

    return static_cast<Qt3DSDMTimelineItemBinding *>(m_binding)->GetInstance();
}

void RowTree::initialize()
{
    setTimelineRow(m_rowTimeline);
    m_rowTimeline->setRowTree(this);

    setMinimumWidth(TimelineConstants::TREE_BOUND_W);

    initializeAnimations();

    m_labelItem.setParentItem(this);
    m_labelItem.setRowTree(this);
    m_labelItem.setLabel(m_label);
    updateLabelPosition();

    // Default all rows to collapsed
    setRowVisible(false);
    m_expandState = ExpandState::HiddenCollapsed;

    connect(&m_labelItem, &RowTreeLabel::labelChanged, this,
            [this](const QString &label) {
        // Update label on timeline and on model
        m_label = label;
        // TODO: Get rid of CString APIs
        auto clabel = Q3DStudio::CString::fromQString(m_label);
        m_binding->GetTimelineItem()->SetName(clabel);
    });
}

void RowTree::initializeAnimations()
{
    // Init left side expand animations
    m_expandHeightAnimation = new QPropertyAnimation(this, "maximumSize");
    m_expandHeightAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION);
    m_expandAnimation.addAnimation(m_expandHeightAnimation);
    m_expandOpacityAnimation = new QPropertyAnimation(this, "opacity");
    m_expandOpacityAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION / 3);
    m_expandAnimation.addAnimation(m_expandOpacityAnimation);

    // Init right side expand animations
    m_expandTimelineHeightAnimation = new QPropertyAnimation(m_rowTimeline, "maximumSize");
    m_expandTimelineHeightAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION);
    m_expandAnimation.addAnimation(m_expandTimelineHeightAnimation);
    m_expandTimelineOpacityAnimation = new QPropertyAnimation(m_rowTimeline, "opacity");
    m_expandTimelineOpacityAnimation->setDuration(TimelineConstants::EXPAND_ANIMATION_DURATION / 3);
    m_expandAnimation.addAnimation(m_expandTimelineOpacityAnimation);

    connect(&m_expandAnimation, &QAbstractAnimation::stateChanged,
            [this](const QAbstractAnimation::State newState) {
        if (m_rowTimeline) {
            if (newState == QAbstractAnimation::Running) {
                setVisible(true);
                m_rowTimeline->setVisible(true);
            } else if (newState == QAbstractAnimation::Stopped) {
                if (this->maximumHeight() == 0) {
                    setVisible(false);
                    m_rowTimeline->setVisible(false);
                }
            }
        }
    });
}

void RowTree::animateExpand(ExpandState state)
{
    int endHeight = 0; // hidden states
    float endOpacity = 0;
    if (state == ExpandState::Expanded) {
        endHeight = m_propGraphExpanded ? m_propGraphHeight : TimelineConstants::ROW_H;
        endOpacity = 1;
    } else if (state == ExpandState::Collapsed) {
        endHeight = TimelineConstants::ROW_H;
        endOpacity = 1;
    }
    // Changing end values while animation is running does not affect currently running animation,
    // so let's make sure the animation is stopped first.
    m_expandAnimation.stop();
    m_expandHeightAnimation->setEndValue(QSizeF(size().width(), endHeight));
    m_expandTimelineHeightAnimation->setEndValue(QSizeF(m_rowTimeline->size().width(), endHeight));
    m_expandOpacityAnimation->setEndValue(endOpacity);
    m_expandTimelineOpacityAnimation->setEndValue(endOpacity);

    m_expandAnimation.start();
}

void RowTree::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    bool hiResIcons = StudioUtils::devicePixelRatio(widget->window()->windowHandle()) > 1.0;

    if (!y()) // prevents flickering when the row is just inserted to the layout
        return;

    static const int ICON_SIZE = 16;
    static const int LEFT_DIVIDER = 18;
    static const int ICON_Y = (TimelineConstants::ROW_H - ICON_SIZE) / 2;
    const int offset = 5 + m_depth * TimelineConstants::ROW_DEPTH_STEP;

    // update button bounds rects
    m_rectArrow  .setRect(offset, ICON_Y, ICON_SIZE, ICON_SIZE);
    m_rectType   .setRect(offset + ICON_SIZE, ICON_Y, ICON_SIZE, ICON_SIZE);
    m_rectShy    .setRect(treeWidth() - 16 * 3.3, ICON_Y, ICON_SIZE, ICON_SIZE);
    m_rectVisible.setRect(treeWidth() - 16 * 2.2, ICON_Y, ICON_SIZE, ICON_SIZE);
    m_rectLocked .setRect(treeWidth() - 16 * 1.1, ICON_Y, ICON_SIZE, ICON_SIZE);

    // Background
    QColor bgColor;
    if (m_dndState == DnDState::Source)
        bgColor = CStudioPreferences::timelineRowColorDndSource();
    else if (m_dndState == DnDState::SP_TARGET)
        bgColor = CStudioPreferences::timelineRowColorDndTargetSP();
    else if (m_isProperty)
        bgColor = CStudioPreferences::timelineRowColorNormalProp();
    else if (m_dndHover)
        bgColor = CStudioPreferences::timelineRowColorDndTarget();
    else if (m_state == Selected)
        bgColor = CStudioPreferences::timelineRowColorSelected();
    else if (m_state == Hovered && !m_locked)
        bgColor = CStudioPreferences::timelineRowColorOver();
    else
        bgColor = CStudioPreferences::timelineRowColorNormal();

    painter->fillRect(QRect(0, 0, size().width(), size().height() - 1), bgColor);

    // left divider
    painter->setPen(CStudioPreferences::timelineWidgetBgColor());
    painter->drawLine(LEFT_DIVIDER, 0, LEFT_DIVIDER, size().height() - 1);

    // Shy, eye, lock separator
    painter->fillRect(QRect(rightDividerX(), 0, 1, size().height()),
                      CStudioPreferences::timelineWidgetBgColor());

    // Shy, eye, lock
    static const QPixmap pixEmpty(":/images/Toggle-Empty.png");
    static const QPixmap pixShy(":/images/Toggle-Shy.png");
    static const QPixmap pixHide(":/images/Toggle-HideShow.png");
    static const QPixmap pixHideDisabled(":/images/Toggle-HideShow-disabled.png");
    static const QPixmap pixHideCtrld(":/images/Toggle-HideShowControlled.png");
    static const QPixmap pixLock(":/images/Toggle-Lock.png");
    static const QPixmap pixEmpty2x(":/images/Toggle-Empty@2x.png");
    static const QPixmap pixShy2x(":/images/Toggle-Shy@2x.png");
    static const QPixmap pixHide2x(":/images/Toggle-HideShow@2x.png");
    static const QPixmap pixHideDisabled2x(":/images/Toggle-HideShow-disabled@2x.png");
    static const QPixmap pixHideCtrld2x(":/images/Toggle-HideShowControlled@2x.png");
    static const QPixmap pixLock2x(":/images/Toggle-Lock@2x.png");
    if (hasActionButtons()) {
        painter->drawPixmap(m_rectShy, hiResIcons ? (m_shy ? pixShy2x : pixEmpty2x)
                                                  : (m_shy ? pixShy : pixEmpty));
        // Eyeball visibility follows the visibility setting for the object even if it has
        // datainput controller
        // Disable eyeball from master slide
        if (m_onMasterSlide) {
            painter->drawPixmap(m_rectVisible, hiResIcons ? pixHideDisabled2x
                                                          : pixHideDisabled);
        } else if (m_visibilityCtrld) {
            painter->drawPixmap(m_rectVisible, hiResIcons
                                ? (m_visible ? pixHideCtrld2x : pixEmpty2x)
                                : (m_visible ? pixHideCtrld : pixEmpty));
        } else {
            painter->drawPixmap(m_rectVisible, hiResIcons
                                ? (m_visible ? pixHide2x : pixEmpty2x)
                                : (m_visible ? pixHide : pixEmpty));
        }
        painter->drawPixmap(m_rectLocked, hiResIcons ? (m_locked ? pixLock2x : pixEmpty2x)
                                                     : (m_locked ? pixLock : pixEmpty));
    }

    static const QPixmap pixInsertLeft(":/images/Insert-Left.png");
    static const QPixmap pixInsertRight(":/images/Insert-Right.png");
    static const QPixmap pixInsertLeft2x(":/images/Insert-Left@2x.png");
    static const QPixmap pixInsertRight2x(":/images/Insert-Right@2x.png");
    if (m_dndState == DnDState::SP_TARGET) { // Candidate target of a subpresentation drop
        painter->drawPixmap(19, 2, hiResIcons ? pixInsertLeft2x : pixInsertLeft);
        painter->drawPixmap(rightDividerX() - 8, 2, hiResIcons ? pixInsertRight2x : pixInsertRight);
    } else if (m_dndState == DnDState::Parent) { // Candidate parent of a dragged row
        painter->setPen(QPen(CStudioPreferences::timelineRowMoverColor(), 1));
        painter->drawRect(QRect(1, 1, treeWidth() - 2, size().height() - 3));
    }

    // Action indicators
    static const QPixmap pixMasterAction(":/images/Action-MasterAction.png");
    static const QPixmap pixAction(":/images/Action-Action.png");
    static const QPixmap pixChildMasterAction(":/images/Action-ChildMasterAction.png");
    static const QPixmap pixChildAction(":/images/Action-ChildAction.png");
    static const QPixmap pixCompMasterAction(":/images/Action-ComponentMasterAction.png");
    static const QPixmap pixCompAction(":/images/Action-ComponentAction.png");
    static const QPixmap pixMasterAction2x(":/images/Action-MasterAction@2x.png");
    static const QPixmap pixAction2x(":/images/Action-Action@2x.png");
    static const QPixmap pixChildMasterAction2x(":/images/Action-ChildMasterAction@2x.png");
    static const QPixmap pixChildAction2x(":/images/Action-ChildAction@2x.png");
    static const QPixmap pixCompMasterAction2x(":/images/Action-ComponentMasterAction@2x.png");
    static const QPixmap pixCompAction2x(":/images/Action-ComponentAction@2x.png");

    if (!isProperty()) {
        // subpresentation indicators
        if (m_hasSubpresentation) {
            painter->fillRect(QRect(0, 0, LEFT_DIVIDER, size().height() - 1),
                              CStudioPreferences::timelineRowSubpColor());
        } else if (!expanded() && m_numDescendantSubpresentations > 0) {
            painter->fillRect(QRect(0, 0, LEFT_DIVIDER, size().height() - 1),
                              CStudioPreferences::timelineRowSubpDescendantColor());
        }

        if (m_actionStates & ActionState::MasterAction) // has master action
            painter->drawPixmap(0, 0, hiResIcons ? pixMasterAction2x : pixMasterAction);
        else if (m_actionStates & ActionState::Action) // has action
            painter->drawPixmap(0, 0, hiResIcons ? pixAction2x : pixAction);

        if (!expanded()) {
            if (m_actionStates & ActionState::MasterChildAction) {
                // children have master action
                painter->drawPixmap(0, 0, hiResIcons ? pixChildMasterAction2x
                                                     : pixChildMasterAction);
            } else if (m_actionStates & ActionState::ChildAction)  {
                // children have action
                painter->drawPixmap(0, 0, hiResIcons ? pixChildAction2x : pixChildAction);
            }
        }

        if (m_actionStates & ActionState::MasterComponentAction) // component has master action
            painter->drawPixmap(0, 0, hiResIcons ? pixCompMasterAction2x : pixCompMasterAction);
        else if (m_actionStates & ActionState::ComponentAction) // component has action
            painter->drawPixmap(0, 0, hiResIcons ? pixCompAction2x : pixCompAction);
    } else { // property row
        if (m_propGraphExpanded) {
            if (m_hoveredRect)
                painter->fillRect(*m_hoveredRect, CStudioPreferences::studioColor1());

            // draw maximize, fit graph buttons
            static const QPixmap pixMaximize(":/images/maximize.png");
            static const QPixmap pixFit(":/images/editcamera_tools_hi-00.png");
            static const QPixmap pixGradient(":/images/gradient.png");
            static const QPixmap pixMaximizeDisabled(":/images/maximize_disabled.png");
            static const QPixmap pixFitDisabled(":/images/editcamera_tools_hi-00_disabled.png");
            static const QPixmap pixGradientDisabled(":/images/gradient_disabled.png");

            m_rectMaximizePropGraph.setRect(rightDividerX() - 16 * 1.2, TimelineConstants::ROW_H,
                                            ICON_SIZE, ICON_SIZE);
            painter->setPen(m_propGraphHeight != TimelineConstants::ROW_GRAPH_H && !m_locked
                            ? CStudioPreferences::bezierControlColor()
                            : CStudioPreferences::studioColor3());
            painter->drawRect(m_rectMaximizePropGraph);

            m_rectFitPropGraph.setRect(rightDividerX() - 16 * 2.4, TimelineConstants::ROW_H,
                                       ICON_SIZE, ICON_SIZE);
            painter->setPen(CStudioPreferences::studioColor3());
            painter->drawRect(m_rectFitPropGraph);
            painter->drawPixmap(m_rectMaximizePropGraph, m_locked ? pixMaximizeDisabled
                                                                  : pixMaximize);
            painter->drawPixmap(m_rectFitPropGraph, m_locked ? pixFitDisabled : pixFit);

            if (m_rowTimeline->isColorProperty()) {
                m_rectColorGradient.setRect(rightDividerX() - 16 * 3.6,
                                            TimelineConstants::ROW_H, ICON_SIZE, ICON_SIZE);
                painter->setPen(m_rowTimeline->m_drawColorGradient && !m_locked
                                ? CStudioPreferences::bezierControlColor()
                                : CStudioPreferences::studioColor3());
                painter->drawRect(m_rectColorGradient);
                painter->drawPixmap(m_rectColorGradient, m_locked ? pixGradientDisabled
                                                                  : pixGradient);
            }

            // draw channel selection buttons
            const QString channelNames = m_rowTimeline->isColorProperty() ? QStringLiteral("rgba")
                                                                          : QStringLiteral("xyzw");
            for (int i = 0; i < m_rectChannels.size(); ++i) {
                if (m_activeChannels[i])
                    painter->fillRect(m_rectChannels[i], CStudioPreferences::studioColor1());

                painter->setPen(m_activeChannels[i] && !m_locked
                                ? CStudioPreferences::bezierControlColor()
                                : CStudioPreferences::studioColor3());
                painter->drawRect(m_rectChannels[i]);

                painter->setPen(m_locked ? CStudioPreferences::studioColor3()
                                         : CStudioPreferences::textColor());
                painter->drawText(m_rectChannels[i].topLeft() + QPointF(5, 12), channelNames.at(i));
            }

            if (m_hoveredRect) {
                painter->setPen(CStudioPreferences::textColor());
                painter->drawRect(*m_hoveredRect);
            }
        }
    }

    // variants indicator
    if (m_variantsGroups.size() > 0) {
        const auto variantsDef = g_StudioApp.GetCore()->getProjectFile().variantsDef();
        for (int i = 0; i < m_variantsGroups.size(); ++i) {
            painter->fillRect(QRect(clipX() + 2 + i * 8, 6, 6, 6),
                              variantsDef[m_variantsGroups[i]].m_color);
            painter->setPen(CStudioPreferences::timelineWidgetBgColor());
            painter->drawRect(QRect(clipX() + 2 + i * 8, 6, 6, 6));
        }
    }

    // The following items need to be clipped so that they do not draw overlapping shy etc. buttons

    painter->setClipRect(0, 0, clipX(), TimelineConstants::ROW_H);

    // expand/collapse arrow
    static const QPixmap pixArrow(":/images/arrow.png");
    static const QPixmap pixArrowDown(":/images/arrow_down.png");
    static const QPixmap pixArrow2x(":/images/arrow@2x.png");
    static const QPixmap pixArrowDown2x(":/images/arrow_down@2x.png");
    if (m_arrowVisible) {
        painter->drawPixmap(m_rectArrow, hiResIcons ? (expanded() ? pixArrowDown2x : pixArrow2x)
                                                    : (expanded() ? pixArrowDown : pixArrow));
    }

    // Row type icon
    static const QPixmap pixSceneNormal(":/images/Objects-Scene-Normal.png");
    static const QPixmap pixLayerNormal(":/images/Objects-Layer-Normal.png");
    static const QPixmap pixObjectNormal(":/images/Objects-Model-Normal.png");
    static const QPixmap pixLightNormal(":/images/Objects-Light-Normal.png");
    static const QPixmap pixCameraNormal(":/images/Objects-Camera-Normal.png");
    static const QPixmap pixTextNormal(":/images/Objects-Text-Normal.png");
    static const QPixmap pixAliasNormal(":/images/Objects-Alias-Normal.png");
    static const QPixmap pixGroupNormal(":/images/Objects-Group-Normal.png");
    static const QPixmap pixComponentNormal(":/images/Objects-Component-Normal.png");
    static const QPixmap pixMaterialNormal(":/images/Objects-Material-Normal.png");
    static const QPixmap pixPropertyNormal(":/images/Objects-Property-Normal.png");
    static const QPixmap pixImageNormal(":/images/Objects-Image-Normal.png");
    static const QPixmap pixBehaviorNormal(":/images/Objects-Behavior-Normal.png");
    static const QPixmap pixEffectNormal(":/images/Objects-Effect-Normal.png");
    static const QPixmap pixSceneNormal2x(":/images/Objects-Scene-Normal@2x.png");
    static const QPixmap pixLayerNormal2x(":/images/Objects-Layer-Normal@2x.png");
    static const QPixmap pixObjectNormal2x(":/images/Objects-Model-Normal@2x.png");
    static const QPixmap pixLightNormal2x(":/images/Objects-Light-Normal@2x.png");
    static const QPixmap pixCameraNormal2x(":/images/Objects-Camera-Normal@2x.png");
    static const QPixmap pixTextNormal2x(":/images/Objects-Text-Normal@2x.png");
    static const QPixmap pixAliasNormal2x(":/images/Objects-Alias-Normal@2x.png");
    static const QPixmap pixGroupNormal2x(":/images/Objects-Group-Normal@2x.png");
    static const QPixmap pixComponentNormal2x(":/images/Objects-Component-Normal@2x.png");
    static const QPixmap pixMaterialNormal2x(":/images/Objects-Material-Normal@2x.png");
    static const QPixmap pixPropertyNormal2x(":/images/Objects-Property-Normal@2x.png");
    static const QPixmap pixImageNormal2x(":/images/Objects-Image-Normal@2x.png");
    static const QPixmap pixBehaviorNormal2x(":/images/Objects-Behavior-Normal@2x.png");
    static const QPixmap pixEffectNormal2x(":/images/Objects-Effect-Normal@2x.png");

    static const QPixmap pixSceneDisabled(":/images/Objects-Scene-Disabled.png");
    static const QPixmap pixLayerDisabled(":/images/Objects-Layer-Disabled.png");
    static const QPixmap pixObjectDisabled(":/images/Objects-Model-Disabled.png");
    static const QPixmap pixLightDisabled(":/images/Objects-Light-Disabled.png");
    static const QPixmap pixCameraDisabled(":/images/Objects-Camera-Disabled.png");
    static const QPixmap pixTextDisabled(":/images/Objects-Text-Disabled.png");
    static const QPixmap pixAliasDisabled(":/images/Objects-Alias-Disabled.png");
    static const QPixmap pixGroupDisabled(":/images/Objects-Group-Disabled.png");
    static const QPixmap pixComponentDisabled(":/images/Objects-Component-Disabled.png");
    static const QPixmap pixMaterialDisabled(":/images/Objects-Material-Disabled.png");
    static const QPixmap pixPropertyDisabled(":/images/Objects-Property-Disabled.png");
    static const QPixmap pixImageDisabled(":/images/Objects-Image-Disabled.png");
    static const QPixmap pixBehaviorDisabled(":/images/Objects-Behavior-Disabled.png");
    static const QPixmap pixEffectDisabled(":/images/Objects-Effect-Disabled.png");
    static const QPixmap pixSceneDisabled2x(":/images/Objects-Scene-Disabled@2x.png");
    static const QPixmap pixLayerDisabled2x(":/images/Objects-Layer-Disabled@2x.png");
    static const QPixmap pixObjectDisabled2x(":/images/Objects-Model-Disabled@2x.png");
    static const QPixmap pixLightDisabled2x(":/images/Objects-Light-Disabled@2x.png");
    static const QPixmap pixCameraDisabled2x(":/images/Objects-Camera-Disabled@2x.png");
    static const QPixmap pixTextDisabled2x(":/images/Objects-Text-Disabled@2x.png");
    static const QPixmap pixAliasDisabled2x(":/images/Objects-Alias-Disabled@2x.png");
    static const QPixmap pixGroupDisabled2x(":/images/Objects-Group-Disabled@2x.png");
    static const QPixmap pixComponentDisabled2x(":/images/Objects-Component-Disabled@2x.png");
    static const QPixmap pixMaterialDisabled2x(":/images/Objects-Material-Disabled@2x.png");
    static const QPixmap pixPropertyDisabled2x(":/images/Objects-Property-Disabled@2x.png");
    static const QPixmap pixImageDisabled2x(":/images/Objects-Image-Disabled@2x.png");
    static const QPixmap pixBehaviorDisabled2x(":/images/Objects-Behavior-Disabled@2x.png");
    static const QPixmap pixEffectDisabled2x(":/images/Objects-Effect-Disabled@2x.png");

    QPixmap pixRowType;
    if (m_isProperty) {
        pixRowType = hiResIcons ? (m_locked ? pixPropertyDisabled2x : pixPropertyNormal2x)
                                : (m_locked ? pixPropertyDisabled : pixPropertyNormal);
    } else {
        switch (m_objectType) {
        case OBJTYPE_SCENE:
            pixRowType = hiResIcons ? (m_locked ? pixSceneDisabled2x : pixSceneNormal2x)
                                    : (m_locked ? pixSceneDisabled : pixSceneNormal);
            break;
        case OBJTYPE_LAYER:
            pixRowType = hiResIcons ? (m_locked ? pixLayerDisabled2x : pixLayerNormal2x)
                                    : (m_locked ? pixLayerDisabled : pixLayerNormal);
            break;
        case OBJTYPE_MODEL:
            pixRowType = hiResIcons ? (m_locked ? pixObjectDisabled2x : pixObjectNormal2x)
                                    : (m_locked ? pixObjectDisabled : pixObjectNormal);
            break;
        case OBJTYPE_LIGHT:
            pixRowType = hiResIcons ? (m_locked ? pixLightDisabled2x : pixLightNormal2x)
                                    : (m_locked ? pixLightDisabled : pixLightNormal);
            break;
        case OBJTYPE_CAMERA:
            pixRowType = hiResIcons ? (m_locked ? pixCameraDisabled2x : pixCameraNormal2x)
                                    : (m_locked ? pixCameraDisabled : pixCameraNormal);
            break;
        case OBJTYPE_TEXT:
            pixRowType = hiResIcons ? (m_locked ? pixTextDisabled2x : pixTextNormal2x)
                                    : (m_locked ? pixTextDisabled : pixTextNormal);
            break;
        case OBJTYPE_ALIAS:
            pixRowType = hiResIcons ? (m_locked ? pixAliasDisabled2x : pixAliasNormal2x)
                                    : (m_locked ? pixAliasDisabled : pixAliasNormal);
            break;
        case OBJTYPE_GROUP:
            pixRowType = hiResIcons ? (m_locked ? pixGroupDisabled2x : pixGroupNormal2x)
                                    : (m_locked ? pixGroupDisabled : pixGroupNormal);
            break;
        case OBJTYPE_COMPONENT:
            pixRowType = hiResIcons ? (m_locked ? pixComponentDisabled2x : pixComponentNormal2x)
                                    : (m_locked ? pixComponentDisabled : pixComponentNormal);
            break;
        case OBJTYPE_MATERIAL:
        case OBJTYPE_CUSTOMMATERIAL:
        case OBJTYPE_REFERENCEDMATERIAL:
            pixRowType = hiResIcons ? (m_locked ? pixMaterialDisabled2x : pixMaterialNormal2x)
                                    : (m_locked ? pixMaterialDisabled : pixMaterialNormal);
            break;
        case OBJTYPE_IMAGE:
            pixRowType = hiResIcons ? (m_locked ? pixImageDisabled2x : pixImageNormal2x)
                                    : (m_locked ? pixImageDisabled : pixImageNormal);
            break;
        case OBJTYPE_BEHAVIOR:
            pixRowType = hiResIcons ? (m_locked ? pixBehaviorDisabled2x : pixBehaviorNormal2x)
                                    : (m_locked ? pixBehaviorDisabled : pixBehaviorNormal);
            break;
        case OBJTYPE_EFFECT:
            pixRowType = hiResIcons ? (m_locked ? pixEffectDisabled2x : pixEffectNormal2x)
                                    : (m_locked ? pixEffectDisabled : pixEffectNormal);
            break;
        default:
            break;
        }
    }

    painter->drawPixmap(m_rectType, pixRowType);
}

void RowTree::updateVariants(const QStringList &groups)
{
    m_variantsGroups = groups;
    update();
}

int RowTree::treeWidth() const
{
    return m_scene->treeWidth() - m_scene->getScrollbarOffsets().x();
}

void RowTree::setBinding(ITimelineItemBinding *binding)
{
    m_binding = binding;

    // Restore the expansion state of rows
    m_expandState = m_scene->expandMap().value(instance(), ExpandState::Unknown);

    if (m_expandState == ExpandState::Unknown) {
        // Everything but scene/component is initially collapsed and hidden
        if (m_objectType == OBJTYPE_SCENE || m_objectType == OBJTYPE_COMPONENT)
            m_expandState = ExpandState::Expanded;
        else
            m_expandState = ExpandState::HiddenCollapsed;
    }

    // Make sure all children of visible expanded parents are shown, and vice versa
    if (parentRow()) {
        if (parentRow()->expanded()) {
            if (m_expandState == ExpandState::HiddenCollapsed)
                m_expandState = ExpandState::Collapsed;
            else if (m_expandState == ExpandState::HiddenExpanded)
                m_expandState = ExpandState::Expanded;
        } else {
            if (m_expandState == ExpandState::Collapsed)
                m_expandState = ExpandState::HiddenCollapsed;
            else if (m_expandState == ExpandState::Expanded)
                m_expandState = ExpandState::HiddenExpanded;
        }
    }

    setRowVisible(m_expandState == ExpandState::Collapsed
            || m_expandState == ExpandState::Expanded);

    updateFromBinding();
}

// x value where label should clip
int RowTree::clipX() const
{
    return rightDividerX() - m_variantsGroups.size() * 8 - 2;
}

int RowTree::rightDividerX() const
{
    return treeWidth() - TimelineConstants::TREE_ICONS_W;
}

ITimelineItemProperty *RowTree::propBinding()
{
    return m_PropBinding;
}

void RowTree::refreshPropBinding(bool forceSync)
{
    // Mahmoud_TODO: update only the changed property binding
    m_scene->widgetTimeline()->runFullReconstruct(forceSync);
}

void RowTree::setPropBinding(ITimelineItemProperty *binding)
{
    m_PropBinding = binding;
    if (!m_PropBinding)
        return;

    // update timeline isColorProperty
    qt3dsdm::TDataTypePair propType = m_PropBinding->GetType();
    if (propType.first == qt3dsdm::DataModelDataType::Float4
        && propType.second == qt3dsdm::AdditionalMetaDataType::Color) {
        m_rowTimeline->m_isColorProperty = true;
    }

    int chCount = int(m_PropBinding->GetChannelCount());

    // for color properties only show alpha channel for custom materials
    if (m_rowTimeline->m_isColorProperty && m_parentRow->m_objectType != OBJTYPE_CUSTOMMATERIAL)
        chCount = 3;

    m_activeChannels.resize(chCount);

    if (chCount == 1) // if property has only 1 channel (ex: alpha), don't show channel buttons
        chCount = 0;

    m_rectChannels.resize(chCount);

    // For bezier animation select first channel (ie x) only by default, else select all channels
    if (m_PropBinding->animationType() == qt3dsdm::EAnimationTypeBezier)
        m_activeChannels[0] = true;
    else
        std::fill(m_activeChannels.begin(), m_activeChannels.end(), true);

    for (int i = 0; i < m_rectChannels.size(); ++i)
        m_rectChannels[i].setRect(22, TimelineConstants::ROW_H * (i+1), 16, 16);

    if (m_parentRow->expanded()) {
        setRowVisible(true);

        // restore property graph expand state if saved
        if (m_scene->propGraphHeightMap().contains(m_parentRow->instance())) {
            int propGraphH = m_scene->propGraphHeightMap().value(m_parentRow->instance())
                    .value(m_PropBinding->getPropertyHandle(), 0);
            m_propGraphExpanded = propGraphH > 0;
            if (m_propGraphExpanded) {
                m_propGraphHeight = propGraphH;
                setMaximumSize(QSizeF(size().width(), m_propGraphHeight));
                m_rowTimeline->setMaximumSize(QSizeF(size().width(), m_propGraphHeight));
                m_rowTimeline->propertyGraph()->setExpandHeight(m_propGraphHeight);
                m_rowTimeline->propertyGraph()->updateChannelFiltering(m_activeChannels);
            }
        }
    }

    // Update label color
    m_labelItem.setMaster(m_PropBinding->IsMaster());
}

void RowTree::setState(State state)
{
    m_state = state;
    m_rowTimeline->m_state = state;

    update();
    m_rowTimeline->update();
}

void RowTree::setTimelineRow(RowTimeline *rowTimeline)
{
    m_rowTimeline = rowTimeline;
}

void RowTree::setParentRow(RowTree *parent)
{
    m_parentRow = parent;
}

void RowTree::selectLabel()
{
    m_labelItem.setEnabled(true);
    m_labelItem.setFocus();
    // Select all text
    QTextCursor cursor = m_labelItem.textCursor();
    cursor.select(QTextCursor::Document);
    m_labelItem.setTextCursor(cursor);
}

RowTree *RowTree::parentRow() const
{
    return m_parentRow;
}

int RowTree::depth() const
{
    return m_depth;
}

EStudioObjectType RowTree::objectType() const
{
   return m_objectType;
}

int RowTree::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeRowTree;
}

int RowTree::index() const
{
    // first child in a parent has index 0
    return m_index;
}

int RowTree::indexInLayout() const
{
    // first child (scene) at index 1, tree header at index 0 (invisible rows are also counted)
    return m_indexInLayout;
}

void RowTree::addChild(RowTree *child)
{
    int index = getLastChildIndex(child->isProperty()) + 1;
    addChildAt(child, index);
}

int RowTree::getLastChildIndex(bool isProperty) const
{
    int index = -1;
    if (isProperty && !m_childProps.empty())
        index = m_childProps.last()->index();
    else if (!isProperty && !m_childRows.empty())
        index = m_childRows.last()->index();

    return index;
}

void RowTree::updateArrowVisibility()
{
    bool oldVisibility = m_arrowVisible;
    if (m_childRows.empty() && m_childProps.empty()) {
        m_arrowVisible = false;
    } else {
        if (m_childProps.empty()) {
            m_arrowVisible = false;
            for (RowTree *row : qAsConst(m_childRows)) {
                if (!row->m_filtered) {
                    m_arrowVisible = true;
                    break;
                }
            }
        } else {
            m_arrowVisible = true;
        }
    }
    if (oldVisibility != m_arrowVisible)
        update();
}

bool RowTree::isInVariantsFilter() const
{
    const QString filterStr = g_StudioApp.m_pMainWnd->getVariantsFilterStr();

    if (m_objectType & ~OBJTYPE_IS_VARIANT || filterStr.isEmpty()
        || !m_scene->widgetTimeline()->toolbar()->isVariantsFilterOn()) {
        return true;
    }

    CDoc *doc = g_StudioApp.GetCore()->GetDoc();
    const auto propertySystem = doc->GetStudioSystem()->GetPropertySystem();
    const auto bridge = doc->GetStudioSystem()->GetClientDataModelBridge();
    auto property = bridge->getVariantsProperty(instance());

    using namespace qt3dsdm;
    SValue sValue;
    if (propertySystem->GetInstancePropertyValue(instance(), property, sValue)) {
        QString propVal = get<TDataStrPtr>(sValue)->toQString();
        const QStringList filterPairs = filterStr.split(QLatin1Char(','));
        QHash<QString, bool> matches;
        for (auto &filterPair : filterPairs) {
            QString group = filterPair.left(filterPair.indexOf(QLatin1Char(':')) + 1);
            if (propVal.contains(group)) { // the layer has 1 or more tags from this filter group
                if (propVal.contains(filterPair))
                    matches[group] = true; // filter tag exists in the property variant group
                else if (!matches.contains(group))
                    matches[group] = false;
            }
        }

        for (auto m : qAsConst(matches)) {
            if (!m)
                return false;
        }
    }

    return true;
}

void RowTree::updateFilter()
{
    auto bridge = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
    if (bridge->isMaterialContainer(instance()))
        return;

    bool parentOk = !m_parentRow || m_parentRow->isVisible();
    bool shyOk     = !m_shy      || !m_scene->treeHeader()->filterShy();
    bool visibleOk = m_visible   || !m_scene->treeHeader()->filterHidden();
    bool lockOk    = !m_locked   || !m_scene->treeHeader()->filterLocked();
    bool expandOk  = !expandHidden();
    bool variantsOk = isInVariantsFilter();

    m_filtered = !(shyOk && visibleOk && lockOk && variantsOk);
    const bool visible = parentOk && expandOk && !m_filtered;
    setVisible(visible);
    m_rowTimeline->setVisible(visible);
    for (auto propRow : qAsConst(m_childProps)) {
        propRow->setVisible(visible);
        propRow->m_rowTimeline->setVisible(visible);
    }
}

int RowTree::getCountDecendentsRecursive() const
{
    int num = m_childProps.count();

    for (auto child : qAsConst(m_childRows)) {
       num++;
       num += child->getCountDecendentsRecursive();
    }

    return num;
}

void RowTree::addChildAt(RowTree *child, int index)
{
    // Mahmoud_TODO: improvement: implement moving the child (instead of remove/add) if it is added
    //               under the same parent.

    int maxIndex = getLastChildIndex(child->isProperty()) + 1;

    if (index > maxIndex)
        index = maxIndex;

    if (child->parentRow() == this && index == child->m_index) // same place
        return;

    if (child->parentRow())
        child->parentRow()->removeChild(child);

    child->m_index = index;

    QList<RowTree *> &childRows = child->isProperty() ? m_childProps : m_childRows;
    int updateIndexInLayout = child->m_indexInLayout;
    child->m_indexInLayout = m_indexInLayout + index + 1;

    if (!child->isProperty()) {
        child->m_indexInLayout += m_childProps.count();

        if (m_childRows.size() >= index) {
            for (int i = 0; i < index; ++i)
                child->m_indexInLayout += m_childRows.at(i)->getCountDecendentsRecursive();
        }
    }

    if (!childRows.contains(child))
        childRows.insert(index, child);

    child->m_parentRow = this;
    child->updateDepthRecursive();
    if (!child->isProperty()) {
        m_rowTimeline->updateChildrenMinStartXRecursive(this);
        m_rowTimeline->updateChildrenMaxEndXRecursive(this);
    }

    // update the layout
    child->addToLayout(child->m_indexInLayout);

    // update indices
    updateIndexInLayout = std::min(updateIndexInLayout, child->m_indexInLayout);
    updateIndices(true, child->m_index + 1, updateIndexInLayout, child->isProperty());
    updateArrowVisibility();
}

int RowTree::addToLayout(int indexInLayout)
{
    m_scene->layoutTree()->insertItem(indexInLayout, this);
    m_scene->layoutTimeline()->insertItem(indexInLayout, rowTimeline());

    indexInLayout++;

    for (auto p : qAsConst(m_childProps))
        indexInLayout = p->addToLayout(indexInLayout);

    for (auto c : qAsConst(m_childRows))
        indexInLayout = c->addToLayout(indexInLayout);

    return indexInLayout;
}

RowTree *RowTree::getChildAt(int index) const
{
    if (index < 0 || index > m_childRows.count() - 1)
        return nullptr;

    return m_childRows.at(index);
}

// this does not destroy the row, just remove it from the layout and parenting hierarchy
void RowTree::removeChild(RowTree *child)
{
    if (m_childProps.contains(child) || m_childRows.contains(child)) { // child exists
        removeChildFromLayout(child);

        // detach from parent
        if (child->isProperty())
            m_childProps.removeAll(child);
        else
            m_childRows.removeAll(child);

        child->m_depth = -1;
        child->m_parentRow = nullptr;

        updateIndices(false, child->m_index, child->m_indexInLayout, child->isProperty());
        updateArrowVisibility();
    }
}

int RowTree::removeChildFromLayout(RowTree *child) const
{
    int numRemoved = 0;
    int deleteIndex = child->m_indexInLayout;
    for (;;) {
       RowTree *row_i = static_cast<RowTree *>(m_scene->layoutTree()->itemAt(deleteIndex)
                                               ->graphicsItem());
       if (row_i->depth() <= child->depth() && numRemoved > 0)
           break;

       m_scene->layoutTree()->removeItem(row_i);
       m_scene->layoutTimeline()->removeItem(row_i->rowTimeline());
       numRemoved++;

       if (m_scene->layoutTree()->count() == deleteIndex) // reached end of the list
           break;
    }

    return numRemoved;
}

bool RowTree::draggable() const
{
    return !m_locked && !isProperty()
           && m_objectType & ~(OBJTYPE_IMAGE | OBJTYPE_SCENE | OBJTYPE_IS_MATERIAL);
}

void RowTree::updateDepthRecursive()
{
    if (m_parentRow) {
        m_depth = m_parentRow->m_depth + 1;
        updateLabelPosition();

        for (auto p : qAsConst(m_childProps))
            p->updateDepthRecursive();

        for (auto r : qAsConst(m_childRows))
            r->updateDepthRecursive();
    }
}

// update this parent's children indices after a child row is inserted or removed
void RowTree::updateIndices(bool isInsertion, int index, int indexInLayout, bool isProperty)
{
    // update index
    if (isProperty && index < m_childProps.count()) {
        for (int i = index; i < m_childProps.count(); i++)
            m_childProps.at(i)->m_index += isInsertion ? 1 : -1;
    } else if (!isProperty && index < m_childRows.count()) {
        for (int i = index; i < m_childRows.count(); i++)
            m_childRows.at(i)->m_index += isInsertion ? 1 : -1;
    }

    // update indexInLayout
    for (int i = indexInLayout; i < m_scene->layoutTree()->count(); ++i) {
        RowTree *row_i = static_cast<RowTree *>(m_scene->layoutTree()->itemAt(i)->graphicsItem());
        row_i->m_indexInLayout = i;
    }
}

void RowTree::updateFromBinding()
{
    // update view (shy, visible, locked)
    m_shy = m_binding->GetTimelineItem()->IsShy();
    m_visible = m_binding->GetTimelineItem()->IsVisible();
    updateLock(m_binding->GetTimelineItem()->IsLocked());
    m_visibilityCtrld = m_binding->GetTimelineItem()->IsVisibilityControlled();

    // Update label color
    Qt3DSDMTimelineItemBinding *itemBinding =
            static_cast<Qt3DSDMTimelineItemBinding *>(m_binding);
    m_master = itemBinding->IsMaster();
    m_labelItem.setMaster(m_master);
    // Update timeline comments
    m_rowTimeline->updateCommentItem();
}

void RowTree::updateLabel()
{
    if (m_binding)
        m_labelItem.setLabel(m_binding->GetTimelineItem()->GetName().toQString());
}

void RowTree::setRowVisible(bool visible)
{
    auto bridge = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();
    if (bridge->isMaterialContainer(instance()))
        return;

    if (visible) {
        setMaximumHeight(TimelineConstants::ROW_H);
        setOpacity(1.0);
        setVisible(true);
        m_rowTimeline->setMaximumHeight(TimelineConstants::ROW_H);
        m_rowTimeline->setOpacity(1.0);
        m_rowTimeline->setVisible(true);
    } else {
        setMaximumHeight(0.0);
        setOpacity(0.0);
        setVisible(false);
        m_rowTimeline->setMaximumHeight(0.0);
        m_rowTimeline->setOpacity(0.0);
        m_rowTimeline->setVisible(false);
    }
}

bool RowTree::hasPropertyChildren() const
{
    return !m_childProps.empty();
}

void RowTree::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_rectType.contains(event->pos().toPoint()) && !m_locked)
        if (m_binding)
            m_binding->OpenAssociatedEditor();
}

void RowTree::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (m_locked)
        return;

    QPoint p = mapFromScene(event->scenePos()).toPoint();
    QRect *hoveredRect = nullptr;
    if (m_rectMaximizePropGraph.contains(p)) {
        setToolTip(tr("Toggle property graph size"));
        hoveredRect = &m_rectMaximizePropGraph;
    } else if (m_rectFitPropGraph.contains(p)) {
        setToolTip(tr("Fit curves"));
        hoveredRect = &m_rectFitPropGraph;
    } else if (m_rectColorGradient.contains(p)) {
        setToolTip(tr("Toggle color gradient"));
        hoveredRect = &m_rectColorGradient;
    } else {
        setToolTip({});

        // check hovering a channel button
        auto it = std::find_if(m_rectChannels.begin(), m_rectChannels.end(),
                               [&p](const QRect &r){ return r.contains(p); });
        if (it != m_rectChannels.end())
            hoveredRect = it;
    }

    if (m_hoveredRect != hoveredRect) {
        // Update hover status only if it has changed
        m_hoveredRect = hoveredRect;
        update();
    }
}

void RowTree::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    InteractiveTimelineItem::hoverLeaveEvent(event);

    m_hoveredRect = nullptr;
    update();
}

// handle clicked control and return its type
TreeControlType RowTree::getClickedControl(const QPointF &scenePos)
{
    QPoint p = mapFromScene(scenePos).toPoint();
    if (m_arrowVisible && m_rectArrow.contains(p)) {
        updateExpandStatus(m_expandState == ExpandState::Expanded ? ExpandState::Collapsed
                                                                  : ExpandState::Expanded, false);
        update();
        return TreeControlType::Arrow;
    }

    if (hasActionButtons()) {
        if (m_rectShy.contains(p)) {
            toggleShy();
            return TreeControlType::Shy;
        } else if (!m_onMasterSlide && m_rectVisible.contains(p)) {
            // Prevent toggling hide on master slide
            toggleVisible();
            return TreeControlType::Hide;
        } else if (m_rectLocked.contains(p)) {
            toggleLocked();
            return TreeControlType::Lock;
        }
    }

    if (isProperty() && !m_locked) {
        if (m_rectFitPropGraph.contains(p)) { // toggle fit graph
            m_rowTimeline->propertyGraph()->fitGraph();
        } else if (m_rectMaximizePropGraph.contains(p)) { // toggle maximize graph
            m_propGraphHeight = m_propGraphHeight == TimelineConstants::ROW_GRAPH_H
                    ? TimelineConstants::ROW_GRAPH_H_MAX : TimelineConstants::ROW_GRAPH_H;
            m_rowTimeline->propertyGraph()->setExpandHeight(m_propGraphHeight);
            animateExpand(ExpandState::Expanded);
            m_scene->propGraphHeightMap()[m_parentRow->instance()]
                    .insert(m_PropBinding->getPropertyHandle(), m_propGraphHeight);
        } else if (m_rectColorGradient.contains(p)) { // toggle color gradient
            m_rowTimeline->toggleColorGradient();
        } else { // toggle channels
            auto it = std::find_if(m_rectChannels.begin(), m_rectChannels.end(),
                                   [&p](const QRect &r){ return r.contains(p); });

            if (it != m_rectChannels.end()) { // clicked a channel button
                bool ctrlDown = CHotKeys::isCtrlDown();
                int chIdx = int(it->y() / TimelineConstants::ROW_H) - 1;
                int numSelectedChannel = std::count(m_activeChannels.begin(),
                                                    m_activeChannels.end(), true);
                bool isSingleSelected = numSelectedChannel == 1 && m_activeChannels[chIdx];
                bool isMultiSelected = numSelectedChannel > 1 && m_activeChannels[chIdx];
                if (!isSingleSelected && !(isMultiSelected && !ctrlDown))
                    m_activeChannels[chIdx] = !m_activeChannels[chIdx];

                if (!ctrlDown) {
                    for (int i = 0; i < m_activeChannels.size(); ++i) {
                        if (i != chIdx)
                            m_activeChannels[i] = false;
                    }
                }
                m_rowTimeline->propertyGraph()->updateChannelFiltering(m_activeChannels);
                update();
            }
        }
    }

    return TreeControlType::None;
}

void RowTree::updateExpandStatus(ExpandState state, bool animate, bool forceChildUpdate)
{
    const bool changed = m_expandState != state;
    if (!forceChildUpdate && !changed)
        return;

    m_expandState = state;

    if (m_scene->widgetTimeline()->isFullReconstructPending())
        return;

    // Store the expanded state of items so we can restore it on slide change
    if (changed && m_binding)
        m_scene->expandMap().insert(instance(), m_expandState);

    if (animate)
        animateExpand(m_expandState);

    // updateFilter updates the row visibility. It must be called before children are handled
    // to ensure parent visibility is up to date.
    if (changed)
        updateFilter();

    if (!m_childRows.empty()) {
        for (auto child : qAsConst(m_childRows)) {
            if (state == ExpandState::Expanded) {
                if (child->m_expandState == ExpandState::HiddenExpanded)
                    child->updateExpandStatus(ExpandState::Expanded);
                else if (child->m_expandState == ExpandState::HiddenCollapsed)
                    child->updateExpandStatus(ExpandState::Collapsed);
            } else {
                if (child->m_expandState == ExpandState::Expanded)
                    child->updateExpandStatus(ExpandState::HiddenExpanded);
                else if (child->m_expandState == ExpandState::Collapsed)
                    child->updateExpandStatus(ExpandState::HiddenCollapsed);
            }
        }
    }

    if (!m_childProps.empty()) {
        for (auto child : qAsConst(m_childProps)) {
            // Properties can never be collapsed
            if (state == ExpandState::Expanded)
                child->updateExpandStatus(ExpandState::Expanded);
            else
                child->updateExpandStatus(ExpandState::HiddenExpanded);
        }
    }
}

void RowTree::updateLockRecursive(bool state)
{
    updateLock(state);
    if (!m_childRows.empty()) {
        for (auto child : qAsConst(m_childRows))
            child->updateLockRecursive(m_locked);
    }
}

void RowTree::updateLock(bool state)
{
    m_locked = state;

    m_labelItem.setLocked(m_locked);
    update();
    if (!m_childProps.empty()) {
        for (auto child : qAsConst(m_childProps))
            child->updateLock(m_locked);
    }
    if (m_locked)
        m_scene->keyframeManager()->deselectRowKeyframes(this);
}

void RowTree::updateSubpresentations(int updateParentsOnlyVal)
{
    if (updateParentsOnlyVal != 0) {
        int n = m_numDescendantSubpresentations;
        if (m_hasSubpresentation)
            n++;
        if (n > 0) {
            RowTree *parentRow = m_parentRow;
            while (parentRow) {
                parentRow->m_numDescendantSubpresentations += n * updateParentsOnlyVal;
                parentRow->update();
                parentRow = parentRow->m_parentRow;
            }
        }
    } else {
        auto binding = static_cast<Qt3DSDMTimelineItemBinding *>(m_binding);
        bool hasSubp = binding->hasSubpresentation();

        if (m_hasSubpresentation != hasSubp) {
            m_hasSubpresentation = hasSubp;
            int n = hasSubp ? 1 : -1;
            RowTree *parentRow = m_parentRow;
            while (parentRow) {
                parentRow->m_numDescendantSubpresentations += n;
                parentRow->update();
                parentRow = parentRow->m_parentRow;
            }
        }
    }
    update();
}

void RowTree::updateLabelPosition()
{
    int offset = 5 + m_depth * TimelineConstants::ROW_DEPTH_STEP + 30;
    m_labelItem.setPos(offset, -1);
}

bool RowTree::expanded() const
{
    if (m_isProperty)
        return false;
    else
        return m_expandState == ExpandState::Expanded;
}

bool RowTree::expandHidden() const
{
    return m_expandState == ExpandState::HiddenExpanded
            || m_expandState == ExpandState::HiddenCollapsed;
}

bool RowTree::isDecendentOf(RowTree *row) const
{
    RowTree *parentRow = m_parentRow;

    while (parentRow) {
        if (parentRow == row)
            return true;

        parentRow = parentRow->parentRow();
    }

    return false;
}

void RowTree::setDnDHover(bool val)
{
    m_dndHover = val;
    update();
}

void RowTree::setDnDState(DnDState state, DnDState onlyIfState, bool recursive)
{
    if (m_dndState == onlyIfState || onlyIfState == DnDState::Any) {
        m_dndState = state;
        update();

        if (recursive) { // used by source rows to highlights all of their descendants
            for (auto child : qAsConst(m_childProps))
                child->setDnDState(state, onlyIfState, true);

            for (auto child : qAsConst(m_childRows))
                child->setDnDState(state, onlyIfState, true);
        }
    }
}

RowTree::DnDState RowTree::getDnDState() const
{
    return m_dndState;
}

void RowTree::setActionStates(ActionStates states)
{
    if (states != m_actionStates) {
        m_actionStates = states;
        update();
    }
}

bool RowTree::isContainer() const
{
    return !m_isProperty && m_objectType & OBJTYPE_IS_CONTAINER;
}

bool RowTree::isProperty() const
{
    return m_isProperty;
}

RowTree *RowTree::getPropertyRow(const QString &type) const
{
    for (RowTree *prop : qAsConst(m_childProps)) {
        if (prop->label() == type)
            return prop;
    }

    return nullptr;
}


bool RowTree::isPropertyOrMaterial() const
{
    return m_isProperty || m_objectType & (OBJTYPE_IS_MATERIAL | OBJTYPE_IMAGE);
}

bool RowTree::isComponent() const
{
    return m_objectType == OBJTYPE_COMPONENT;
}

bool RowTree::isComponentRoot() const
{
    if (m_objectType == OBJTYPE_COMPONENT && m_binding)
        return static_cast<Qt3DSDMTimelineItemBinding *>(m_binding)->isRootComponent();

    return false;
}

bool RowTree::isMaster() const
{
    return m_master;
}

bool RowTree::isDefaultMaterial() const
{
    if (m_binding)
        return static_cast<Qt3DSDMTimelineItemBinding *>(m_binding)->isDefaultMaterial();

    return false;
}

bool RowTree::empty() const
{
    return m_childRows.empty() && m_childProps.empty();
}

bool RowTree::selected() const
{
    return m_state == Selected;
}

QList<RowTree *> RowTree::childRows() const
{
    return m_childRows;
}

QList<RowTree *> RowTree::childProps() const
{
    return m_childProps;
}

RowTimeline *RowTree::rowTimeline() const
{
    return m_rowTimeline;
}

QString RowTree::label() const
{
    return m_label;
}

void RowTree::toggleShy()
{
    if (hasActionButtons()) {
        m_shy = !m_shy;
        update();
        m_binding->GetTimelineItem()->SetShy(m_shy);
    }
}

void RowTree::toggleVisible()
{
    if (hasActionButtons()) {
        m_visible = !m_visible;
        update();
        m_binding->GetTimelineItem()->SetVisible(m_visible);
    }
}

void RowTree::toggleLocked()
{
    if (hasActionButtons()) {
        updateLockRecursive(!m_locked);
        m_binding->GetTimelineItem()->SetLocked(m_locked);
        if (m_locked && selected())
            m_scene->rowManager()->clearSelection();
    }
}

bool RowTree::shy() const
{
    return m_shy;
}

bool RowTree::visible() const
{
    return m_visible;
}

bool RowTree::locked() const
{
    return m_locked;
}

// Returns true for items with shy/visible/lock buttons
bool RowTree::hasActionButtons() const
{
    return !m_isProperty && m_indexInLayout != 1
           && m_objectType & ~(OBJTYPE_SCENE | OBJTYPE_IS_MATERIAL | OBJTYPE_IMAGE);
}

bool RowTree::hasComponentAncestor() const
{
    RowTree *parentRow = m_parentRow;
    while (parentRow) {
        if (parentRow->objectType() == OBJTYPE_COMPONENT)
            return true;
        parentRow = parentRow->parentRow();
    }
    return false;
}

// Returns true for items with duration bar
bool RowTree::hasDurationBar() const
{
    return hasActionButtons(); // Same at least now
}

bool RowTree::propertyExpanded() const
{
    return m_propGraphExpanded;
}

/**
 * toggle property graph if the mouse isn't over other buttons
 *
 * @param scenePos mouse position in graphics scene coordinates
 */
void RowTree::togglePropertyExpanded(const QPointF &scenePos)
{
    QPoint p = mapFromScene(scenePos).toPoint();

    // check mouse over a channel button
    for (int i = 0; i < m_rectChannels.size(); ++i) {
        if (m_rectChannels[i].contains(p))
            return;
    }

    // check mouse over fit, maximize, or toggle color gradient buttons
    if (m_rectFitPropGraph.contains(p) || m_rectMaximizePropGraph.contains(p)
        || m_rectColorGradient.contains(p)) {
        return;
    }

    // toggle property graph expand
    m_propGraphExpanded = !m_propGraphExpanded;

    if (m_propGraphExpanded) {
        // start graph in normal (not maximized) size
        m_propGraphHeight = TimelineConstants::ROW_GRAPH_H;
        m_rowTimeline->propertyGraph()->setExpandHeight(m_propGraphHeight);
        m_rowTimeline->propertyGraph()->updateChannelFiltering(m_activeChannels);
        animateExpand(ExpandState::Expanded);

        m_scene->propGraphHeightMap()[m_parentRow->instance()]
                .insert(m_PropBinding->getPropertyHandle(), m_propGraphHeight);
    } else {
        animateExpand(ExpandState::Collapsed);

        m_scene->propGraphHeightMap()[m_parentRow->instance()]
                .remove(m_PropBinding->getPropertyHandle());
    }
}

void RowTree::showDataInputSelector(const QString &propertyname, const QPoint &pos)
{
    auto bridge = g_StudioApp.GetCore()->GetDoc()->GetStudioSystem()->GetClientDataModelBridge();

    // Set the datainput to control property in referenced object if this
    // is a referenced material.
    auto refInstance = bridge->getMaterialReference(instance());

    m_scene->handleShowDISelector(propertyname, refInstance.Valid() ? refInstance : instance(),
                                  pos);
}

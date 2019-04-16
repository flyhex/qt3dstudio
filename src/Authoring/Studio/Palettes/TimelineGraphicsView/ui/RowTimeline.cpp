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

#include "RowTimeline.h"
#include "RowTimelinePropertyGraph.h"
#include "RowTree.h"
#include "RowManager.h"
#include "Ruler.h"
#include "TimelineConstants.h"
#include "Keyframe.h"
#include "KeyframeManager.h"
#include "TimelineGraphicsScene.h"
#include "Bindings/ITimelineItemBinding.h"
#include "Bindings/ITimelineTimebar.h"
#include "Bindings/Qt3DSDMTimelineItemProperty.h"
#include "AppFonts.h"
#include "StudioPreferences.h"
#include "TimelineToolbar.h"
#include "StudioUtils.h"

#include <QtGui/qpainter.h>
#include <QtGui/qbrush.h>
#include <QtWidgets/qdesktopwidget.h>
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qgraphicssceneevent.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qlabel.h>
#include <QtCore/qdatetime.h>

RowTimeline::RowTimeline()
    : InteractiveTimelineItem()
{
    // 999999: theoretically big enough row width (~ 4.6 hrs of presentation length)
    setMinimumWidth(999999);
    setMaximumWidth(999999);
}

RowTimeline::~RowTimeline()
{
    // remove keyframes
    if (!m_keyframes.empty()) {
        if (m_isProperty) // non-property rows use the same keyframes from property rows.
            qDeleteAll(m_keyframes);

        m_keyframes.clear();
    }
}

void RowTimeline::initialize()
{
    // Called once m_rowTree exists

    m_commentItem = new RowTimelineCommentItem(this);
    m_commentItem->setParentRow(m_rowTree);
    updateCommentItemPos();

    TimelineToolbar *toolbar = m_rowTree->m_scene->widgetTimeline()->toolbar();
    connect(toolbar, &TimelineToolbar::showRowTextsToggled, this, [this]() {
        updateCommentItem();
    });

    connect(m_commentItem, &RowTimelineCommentItem::labelChanged, this,
            [this](const QString &label) {
        // Update label on timeline and on model
        ITimelineTimebar *timebar = m_rowTree->m_binding->GetTimelineItem()->GetTimebar();
        timebar->SetTimebarComment(label);
    });

    connect(m_rowTree->m_scene->ruler(), &Ruler::viewportXChanged, this,
            &RowTimeline::updateCommentItemPos);
}

void RowTimeline::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)

    bool hiResIcons = StudioUtils::devicePixelRatio(widget->window()->windowHandle()) > 1.0;

    if (!y()) // prevents flickering when the row is just inserted to the layout
        return;

    const int currentHeight = size().height() - 1;

    if (isColorProperty() && !m_keyframes.empty()) {
        drawColorPropertyGradient(painter, widget->width());
    } else {
        // Background
        QColor bgColor;
        if (m_rowTree->isProperty())
            bgColor = CStudioPreferences::timelineRowColorNormalProp();
        else if (m_state == Selected)
            bgColor = CStudioPreferences::timelineRowColorSelected();
        else if (m_state == Hovered && !m_rowTree->m_locked)
            bgColor = CStudioPreferences::timelineRowColorOver();
        else
            bgColor = CStudioPreferences::timelineRowColorNormal();
        painter->fillRect(0, 0, size().width(), currentHeight, bgColor);
    }

    const double edgeOffset = TimelineConstants::RULER_EDGE_OFFSET;

    // Duration. Draw duration bar (for scene/component root) also if it has
    // datainput controller
    if (m_rowTree->hasDurationBar() || m_controllerDataInput.size()) {
        painter->save();

        // fully outside ancestors' limits, draw fully hashed
        if (m_minStartX > m_endX || m_maxEndX < m_startX) {
            painter->setBrush(QBrush(CStudioPreferences::timelineRowColorDurationOff1(),
                                     Qt::BDiagPattern));
            painter->setPen(Qt::NoPen);
            painter->fillRect(QRect(edgeOffset + m_startX, 0, m_endX - m_startX, currentHeight),
                              CStudioPreferences::timelineRowColorDurationOff2());
            painter->drawRect(QRect(edgeOffset + m_startX, 0, m_endX - m_startX, currentHeight));

            painter->setPen(QPen(CStudioPreferences::timelineRowColorDurationEdge(), 2));
            painter->drawLine(edgeOffset + m_startX, 0, edgeOffset + m_startX, currentHeight);
            painter->drawLine(edgeOffset + m_endX, 0, edgeOffset + m_endX, currentHeight);
        } else {
            // draw main duration part
            double x = edgeOffset + qMax(m_startX, m_minStartX);
            double w = edgeOffset + qMin(m_endX, m_maxEndX) - x;
            static const int marginY = 3;

            painter->setPen(Qt::NoPen);

            if (m_controllerDataInput.size()) {
                painter->fillRect(QRect(x, 0, w, currentHeight),
                                  CStudioPreferences::dataInputColor());
            } else if (m_rowTree->indexInLayout() != 1) {
                painter->fillRect(QRect(x, 0, w, currentHeight), m_barColor);
            }

            if (m_state == Selected) {
                // draw selection overlay on bar
                painter->fillRect(QRect(x, marginY, w, currentHeight - marginY * 2),
                                  CStudioPreferences::timelineRowColorDurationSelected());
            }

            if (m_controllerDataInput.size()) {
                static const QPixmap pixDataInput = QPixmap(":/images/Objects-DataInput-White.png");
                static const QPixmap pixDataInput2x
                        = QPixmap(":/images/Objects-DataInput-White@2x.png");
                static const QFontMetrics fm(painter->font());

                // need clip region to limit datainput icon visibility to the same rect as we use
                // for text
                painter->setClipRect(x, 0, w, currentHeight);
                painter->setClipping(true);
                painter->setPen(QPen(CStudioPreferences::textColor(), 2));
                // +5 added to text location to make margin comparable to other datainput controls
                painter->drawText(QRect(x + pixDataInput.width() + 5, 0, w, currentHeight),
                                  m_controllerDataInput, QTextOption(Qt::AlignCenter));
                // place the icon in front of the text
                int textwidth = fm.width(m_controllerDataInput);
                int iconx = x + (w - textwidth) / 2;
                if (iconx < x)
                    iconx = x;
                painter->drawPixmap(iconx, marginY, hiResIcons ? pixDataInput2x : pixDataInput);
                painter->setPen(Qt::NoPen);
                painter->setClipping(false);
            }

            // draw hashed part before
            painter->setBrush(QBrush(CStudioPreferences::timelineRowColorDurationOff1(),
                                     Qt::BDiagPattern));
            if (m_startX < m_minStartX) {
                painter->setPen(Qt::NoPen);
                painter->fillRect(QRect(edgeOffset + m_startX, 0, m_minStartX - m_startX,
                                        currentHeight),
                                  CStudioPreferences::timelineRowColorDurationOff2());
                painter->drawRect(QRect(edgeOffset + m_startX, 0, m_minStartX - m_startX,
                                        currentHeight));
                painter->setPen(CStudioPreferences::timelineRowColorDurationEdge());
                painter->drawLine(edgeOffset + m_minStartX, 0, edgeOffset + m_minStartX,
                                  currentHeight);
            }

            // draw hashed part after
            if (m_endX > m_maxEndX) {
                painter->setPen(Qt::NoPen);
                painter->fillRect(QRect(edgeOffset + m_maxEndX, 0, m_endX - m_maxEndX,
                                        currentHeight),
                                  CStudioPreferences::timelineRowColorDurationOff2());
                painter->drawRect(QRect(edgeOffset + m_maxEndX, 0, m_endX - m_maxEndX,
                                        currentHeight));
                painter->setPen(CStudioPreferences::timelineRowColorDurationEdge());
                painter->drawLine(edgeOffset + m_maxEndX, 0, edgeOffset + m_maxEndX, currentHeight);
            }

            if (m_rowTree->indexInLayout() != 1) {
                painter->setPen(QPen(CStudioPreferences::timelineRowColorDurationEdge(), 2));
                painter->drawLine(edgeOffset + m_startX, 0, edgeOffset + m_startX, currentHeight);
                painter->drawLine(edgeOffset + m_endX, 0, edgeOffset + m_endX, currentHeight);
            }
        }

        painter->restore();
    }

    if (m_propertyGraph) { // Property graph
        QRectF graphRect(edgeOffset, 0, widget->width(), currentHeight);
        m_propertyGraph->paintGraphs(painter, graphRect);
    }

    // Keyframes
    const qreal keyFrameH = 16.0;
    const qreal keyFrameHalfH = keyFrameH / 2.0;
    const qreal keyFrameY = (qMin(currentHeight, TimelineConstants::ROW_H) / 2.0) - keyFrameHalfH;
    const qreal hiddenKeyFrameY = keyFrameY + (keyFrameH * 2.0 / 3.0) + 2.0;
    const qreal keyFrameOffset = hiResIcons ? 8 : 7.5;

    // Hidden descendant keyframe indicators
    if (!m_rowTree->expanded()) {
        static const QPixmap pixKeyframeHidden = QPixmap(":/images/keyframe-hidden-normal.png");
        static const QPixmap pixKeyframeHidden2x
                = QPixmap(":/images/keyframe-hidden-normal@2x.png");
        QVector<long> childKeyframeTimes;
        collectChildKeyframeTimes(childKeyframeTimes);

        const qreal oldOpacity = painter->opacity();
        painter->setOpacity(0.75);
        for (const auto time : qAsConst(childKeyframeTimes)) {
            const qreal xCoord = edgeOffset + m_rowTree->m_scene->ruler()->timeToDistance(time)
                                 - 2.5;
            painter->drawPixmap(QPointF(xCoord, hiddenKeyFrameY), hiResIcons ? pixKeyframeHidden2x
                                                                             : pixKeyframeHidden);
        }
        painter->setOpacity(oldOpacity);
    }

    if (m_rowTree->hasPropertyChildren()) { // object row keyframes
        static const QPixmap pixKeyframeMasterDisabled
                = QPixmap(":/images/Keyframe-Master-Disabled.png");
        static const QPixmap pixKeyframeMasterNormal
                = QPixmap(":/images/Keyframe-Master-Normal.png");
        static const QPixmap pixKeyframeMasterSelected
                = QPixmap(":/images/Keyframe-Master-Selected.png");
        static const QPixmap pixKeyframeMasterDynamicDisabled
                = QPixmap(":/images/Keyframe-MasterDynamic-Disabled.png");
        static const QPixmap pixKeyframeMasterDynamicNormal
                = QPixmap(":/images/Keyframe-MasterDynamic-Normal.png");
        static const QPixmap pixKeyframeMasterDynamicSelected
                = QPixmap(":/images/Keyframe-MasterDynamic-Selected.png");
        static const QPixmap pixKeyframeMasterDisabled2x
                = QPixmap(":/images/Keyframe-Master-Disabled@2x.png");
        static const QPixmap pixKeyframeMasterNormal2x
                = QPixmap(":/images/Keyframe-Master-Normal@2x.png");
        static const QPixmap pixKeyframeMasterSelected2x
                = QPixmap(":/images/Keyframe-Master-Selected@2x.png");
        static const QPixmap pixKeyframeMasterDynamicDisabled2x
                = QPixmap(":/images/Keyframe-MasterDynamic-Disabled@2x.png");
        static const QPixmap pixKeyframeMasterDynamicNormal2x
                = QPixmap(":/images/Keyframe-MasterDynamic-Normal@2x.png");
        static const QPixmap pixKeyframeMasterDynamicSelected2x
                = QPixmap(":/images/Keyframe-MasterDynamic-Selected@2x.png");
        for (auto keyframe : qAsConst(m_keyframes)) {
            QPixmap pixmap;
            if (m_rowTree->locked()) {
                if (keyframe->dynamic) {
                    pixmap = hiResIcons ? pixKeyframeMasterDynamicDisabled2x
                                        : pixKeyframeMasterDynamicDisabled;
                } else {
                    pixmap = hiResIcons ? pixKeyframeMasterDisabled2x
                                       : pixKeyframeMasterDisabled;
                }
            } else if (keyframe->selected()) {
                if (keyframe->dynamic) {
                    pixmap = hiResIcons ? pixKeyframeMasterDynamicSelected2x
                                        : pixKeyframeMasterDynamicSelected;
                } else {
                    pixmap = hiResIcons ? pixKeyframeMasterSelected2x
                                        : pixKeyframeMasterSelected;
                }
            } else {
                if (keyframe->dynamic) {
                    pixmap = hiResIcons ? pixKeyframeMasterDynamicNormal2x
                                        : pixKeyframeMasterDynamicNormal;
                } else {
                    pixmap = hiResIcons ? pixKeyframeMasterNormal2x
                                        : pixKeyframeMasterNormal;
                }
            }
            painter->drawPixmap(QPointF(edgeOffset + m_rowTree->m_scene->ruler()
                                        ->timeToDistance(keyframe->time) - keyFrameOffset,
                                        keyFrameY), pixmap);

            // highlight the pressed keyframe in a multi-selection (the keyframe that is affected
            // by snapping, and setting time dialog)
            if (m_rowTree->m_scene->keyframeManager()->selectedKeyframes().size() > 1
                && m_rowTree->m_scene->pressedKeyframe() == keyframe) {
                painter->setPen(QPen(CStudioPreferences::timelinePressedKeyframeColor(), 1));
                painter->drawArc(edgeOffset + m_rowTree->m_scene->ruler()
                                 ->timeToDistance(keyframe->time) - 4, keyFrameY + 4, 9, 9, 0,
                                 5760);
            }
        }
    } else if (m_rowTree->isProperty()) { // property row keyframes
        static const QPixmap pixKeyframePropertyDisabled
                = QPixmap(":/images/Keyframe-Property-Disabled.png");
        static const QPixmap pixKeyframePropertyNormal
                = QPixmap(":/images/Keyframe-Property-Normal.png");
        static const QPixmap pixKeyframePropertySelected
                = QPixmap(":/images/Keyframe-Property-Selected.png");
        static const QPixmap pixKeyframePropertyDynamicDisabled
                = QPixmap(":/images/Keyframe-PropertyDynamic-Disabled.png");
        static const QPixmap pixKeyframePropertyDynamicNormal
                = QPixmap(":/images/Keyframe-PropertyDynamic-Normal.png");
        static const QPixmap pixKeyframePropertyDynamicSelected
                = QPixmap(":/images/Keyframe-PropertyDynamic-Selected.png");
        static const QPixmap pixKeyframePropertyDisabled2x
                = QPixmap(":/images/Keyframe-Property-Disabled@2x.png");
        static const QPixmap pixKeyframePropertyNormal2x
                = QPixmap(":/images/Keyframe-Property-Normal@2x.png");
        static const QPixmap pixKeyframePropertySelected2x
                = QPixmap(":/images/Keyframe-Property-Selected@2x.png");
        static const QPixmap pixKeyframePropertyDynamicDisabled2x
                = QPixmap(":/images/Keyframe-PropertyDynamic-Disabled@2x.png");
        static const QPixmap pixKeyframePropertyDynamicNormal2x
                = QPixmap(":/images/Keyframe-PropertyDynamic-Normal@2x.png");
        static const QPixmap pixKeyframePropertyDynamicSelected2x
                = QPixmap(":/images/Keyframe-PropertyDynamic-Selected@2x.png");
        for (auto keyframe : qAsConst(m_keyframes)) {
            QPixmap pixmap;
            if (m_rowTree->locked()) {
                if (keyframe->dynamic) {
                    pixmap = hiResIcons ? pixKeyframePropertyDynamicDisabled2x
                                        : pixKeyframePropertyDynamicDisabled;

                } else {
                    pixmap = hiResIcons ? pixKeyframePropertyDisabled2x
                                        : pixKeyframePropertyDisabled;
                }
            } else if (keyframe->selected()) {
                if (keyframe->dynamic) {
                    pixmap = hiResIcons ? pixKeyframePropertyDynamicSelected2x
                                        : pixKeyframePropertyDynamicSelected;

                } else {
                    pixmap = hiResIcons ? pixKeyframePropertySelected2x
                                        : pixKeyframePropertySelected;
                }
            } else {
                if (keyframe->dynamic) {
                    pixmap = hiResIcons ? pixKeyframePropertyDynamicNormal2x
                                        : pixKeyframePropertyDynamicNormal;

                } else {
                    pixmap = hiResIcons ? pixKeyframePropertyNormal2x
                                        : pixKeyframePropertyNormal;
                }
            }
            painter->drawPixmap(QPointF(edgeOffset + m_rowTree->m_scene->ruler()
                                        ->timeToDistance(keyframe->time) - keyFrameOffset,
                                        keyFrameY), pixmap);
        }
    }
}

bool RowTimeline::isColorProperty() const
{
    ITimelineItemProperty *propBinding = m_rowTree->propBinding();
    if (propBinding) {
        qt3dsdm::TDataTypePair type = propBinding->GetType();
        if (m_rowTree->isProperty()
                && type.first == qt3dsdm::DataModelDataType::Float4
                && type.second == qt3dsdm::AdditionalMetaDataType::Color) {
            return true;
        }
    }
    return false;
}

void RowTimeline::drawColorPropertyGradient(QPainter *painter, int width)
{
    // Gradient scaled width, or at least widget width
    double minWidth = width;
    double timelineScale = m_rowTree->m_scene->ruler()->timelineScale();
    double scaledWidth = width * (timelineScale / 2);
    width = qMax(minWidth, scaledWidth);

    ITimelineItemProperty *propBinding = m_rowTree->propBinding();
    QLinearGradient bgGradient(0, 0, width, 0);

    for (auto keyframe : qAsConst(m_keyframes)) {
        double xPos = m_rowTree->m_scene->ruler()->timeToDistance(keyframe->time);
        double gradPos = xPos / width;
        gradPos = qBound(0.0, gradPos, 1.0);
        QColor currentColor;
        // Get the color at the specified time.
        currentColor.setRed(propBinding->GetChannelValueAtTime(0, keyframe->time));
        currentColor.setGreen(propBinding->GetChannelValueAtTime(1, keyframe->time));
        currentColor.setBlue(propBinding->GetChannelValueAtTime(2, keyframe->time));
        bgGradient.setColorAt(gradPos, currentColor);
    }
    painter->fillRect(TimelineConstants::RULER_EDGE_OFFSET, 0,
                      width, size().height() - 1, bgGradient);
}

Keyframe *RowTimeline::getClickedKeyframe(const QPointF &scenePos)
{
    if (rowTree()->locked())
        return nullptr;

    QPointF p = mapFromScene(scenePos.x(), scenePos.y());
    double x;

    QList<Keyframe *> keyframes;
    if (m_rowTree->hasPropertyChildren()) {
        const auto childProps = m_rowTree->childProps();
        for (auto child : childProps)
            keyframes.append(child->rowTimeline()->m_keyframes);
    } else {
        keyframes = m_keyframes;
    }

    for (const auto keyframe : qAsConst(keyframes)) {
        x = TimelineConstants::RULER_EDGE_OFFSET
            + m_rowTree->m_scene->ruler()->timeToDistance(keyframe->time);

        if (p.x() > x - 5 && p.x() < x + 5 && p.y() > 3 && p.y() < 16)
            return keyframe;
    }

    return nullptr;
}

QList<Keyframe *> RowTimeline::getKeyframesInRange(const QRectF &rect) const
{
    double x;
    QRectF localRect = mapFromScene(rect).boundingRect();

    QList<Keyframe *> result;

    static const int KF_CENTER_Y = 10;
    for (auto keyframe : qAsConst(m_keyframes)) {
        x = TimelineConstants::RULER_EDGE_OFFSET
            + m_rowTree->m_scene->ruler()->timeToDistance(keyframe->time);

        if (localRect.left() < x && localRect.right() > x
                && localRect.top() < KF_CENTER_Y && localRect.bottom() > KF_CENTER_Y) {
            result.append(keyframe);
        }
    }

    return result;
}

void RowTimeline::updateDurationFromBinding()
{
    if (m_rowTree->isProperty()) // this method works for main rows only
        return;

    ITimelineTimebar *timebar = m_rowTree->m_binding->GetTimelineItem()->GetTimebar();
    clearBoundChildren();
    setStartTime(timebar->GetStartTime());
    setEndTime(timebar->GetEndTime());
}

void RowTimeline::updateKeyframesFromBinding(const QList<int> &properties)
{
    if (m_rowTree->isProperty()) // this method works for main rows only
        return;

    const auto childProps = m_rowTree->childProps();
    for (auto child : childProps) {
        qt3dsdm::Qt3DSDMPropertyHandle propertyHandle =
            static_cast<Qt3DSDMTimelineItemProperty *>(child->m_PropBinding)
            ->getPropertyHandle();
        if (properties.contains(propertyHandle)) {
            m_rowTree->m_scene->keyframeManager()->deleteKeyframes(child->rowTimeline(), false);

            for (int i = 0; i < child->m_PropBinding->GetKeyframeCount(); i++) {
                Qt3DSDMTimelineKeyframe *kf = static_cast<Qt3DSDMTimelineKeyframe *>
                        (child->m_PropBinding->GetKeyframeByIndex(i));

                Keyframe *kfUI = new Keyframe(kf->GetTime(), child->rowTimeline());
                kfUI->binding = kf;
                kfUI->dynamic = kf->IsDynamic();
                kf->setUI(kfUI);
                child->rowTimeline()->insertKeyframe(kfUI);
                child->parentRow()->rowTimeline()->insertKeyframe(kfUI);
                if (kf->IsSelected())
                    m_rowTree->m_scene->keyframeManager()->selectKeyframe(kfUI);
            }

            if (isVisible()) {
                child->rowTimeline()->update();
            } else {
                // Find the first visible parent and update that to show hidden keyframes
                RowTree *updateRow = m_rowTree->parentRow();
                while (updateRow && !updateRow->isVisible())
                    updateRow = updateRow->parentRow();
                if (updateRow)
                    updateRow->rowTimeline()->update();
            }
        }
    }
    update();
}

void RowTimeline::insertKeyframe(Keyframe *keyframe)
{
    if (!m_keyframes.contains(keyframe))
        m_keyframes.append(keyframe);
}

void RowTimeline::removeKeyframe(Keyframe *keyframe)
{
    m_keyframes.removeAll(keyframe);
}

void RowTimeline::putSelectedKeyframesOnTop()
{
    if (!m_keyframes.empty()) {
        std::partition(m_keyframes.begin(), m_keyframes.end(), [](Keyframe *kf) {
            return !kf->selected();
        });
    }

    if (m_rowTree->hasPropertyChildren()) { // has property rows
        const auto childProps = m_rowTree->childProps();
        for (auto child : childProps) {
            std::partition(child->rowTimeline()->m_keyframes.begin(),
                           child->rowTimeline()->m_keyframes.end(), [](Keyframe *kf) {
                return !kf->selected();
            });
        }
    }
}

void RowTimeline::updateKeyframes()
{
    update();

    if (m_rowTree->hasPropertyChildren()) { // master keyframes
        const auto childProps = m_rowTree->childProps();
        for (const auto child : childProps)
            child->rowTimeline()->update();
    }
}

TimelineControlType RowTimeline::getClickedControl(const QPointF &scenePos) const
{
    if (!m_rowTree->hasDurationBar())
        return TimelineControlType::None;

    if (!m_rowTree->locked()) {
        QPointF p = mapFromScene(scenePos.x(), scenePos.y());
        p.setX(p.x() - TimelineConstants::RULER_EDGE_OFFSET);

        const int halfHandle = TimelineConstants::DURATION_HANDLE_W * .5;
        // Never choose start handle if end time is zero, as you cannot adjust it in that case
        bool startHandle = p.x() > m_startX - halfHandle && p.x() < m_startX + halfHandle
                && m_endTime > 0;
        bool endHandle = p.x() > m_endX - halfHandle && p.x() < m_endX + halfHandle;
        if (startHandle && endHandle) {
            // If handles overlap, choose the handle based on the side of the click relative to start
            startHandle = p.x() < m_startX;
            endHandle = !startHandle;
        }
        if (startHandle)
            return TimelineControlType::StartHandle;
        else if (endHandle)
            return TimelineControlType::EndHandle;
        else if (p.x() > m_startX && p.x() < m_endX && !rowTree()->locked())
            return TimelineControlType::Duration;
    }

    return TimelineControlType::None;
}

void RowTimeline::startDurationMove(double clickX)
{
    // clickX is in ruler coordinate space
    m_startDurationMoveStartTime = m_startTime;
    m_startDurationMoveOffsetX = clickX - m_startX;
}

void RowTimeline::updateBoundChildren(bool start)
{
    // Collect all bound children
    // Children are considered bound if the start/end time matches the parent time
    if (start)
        m_boundChildrenStart.clear();
    else
        m_boundChildrenEnd.clear();
    if (m_rowTree->hasDurationBar()) {
        const auto childRows = m_rowTree->childRows();
        for (auto child : childRows) {
            if (child->hasDurationBar() && !child->locked()) {
                RowTimeline *rowTimeline = child->rowTimeline();
                if (start && rowTimeline->m_startX == m_startX) {
                    m_boundChildrenStart.append(rowTimeline);
                    rowTimeline->updateBoundChildren(start);
                } else if (!start && rowTimeline->m_endX == m_endX) {
                    m_boundChildrenEnd.append(rowTimeline);
                    rowTimeline->updateBoundChildren(start);
                }
            }
        }
    }
}

void RowTimeline::clearBoundChildren()
{
    m_boundChildrenStart.clear();
    m_boundChildrenEnd.clear();
}

// move the duration area (start/end x)
void RowTimeline::moveDurationBy(double dx)
{
    if (m_startX + dx < 0)
        dx = -m_startX;

    m_startX += dx;
    m_endX += dx;

    if (!m_rowTree->parentRow() || m_rowTree->rowType() == OBJTYPE_LAYER
            || m_rowTree->hasComponentAncestor()) {
        m_minStartX = m_startX;
        m_maxEndX = m_endX;
    }

    Ruler *ruler = m_rowTree->m_scene->ruler();
    m_startTime = ruler->distanceToTime(m_startX);
    m_endTime = ruler->distanceToTime(m_endX);

    // move keyframes with the row
    if (!m_rowTree->isProperty()) { // make sure we don't move the keyframes twice
        for (Keyframe *keyframe : qAsConst(m_keyframes))
            keyframe->time += rowTree()->m_scene->ruler()->distanceToTime(dx);
    }

    update();

    if (!m_rowTree->empty()) {
        updateChildrenMinStartXRecursive(m_rowTree);
        updateChildrenMaxEndXRecursive(m_rowTree);

        for (RowTree *child : qAsConst(m_rowTree->m_childRows)) {
            if (!child->locked())
                child->m_rowTimeline->moveDurationBy(dx);
        }
    }
}

void RowTimeline::moveDurationTo(double newX)
{
    if (newX < 0)
        newX = 0;

    double dx = newX - m_startX;
    double durationX = m_endX - m_startX;

    m_startX = newX;
    m_endX = m_startX + durationX;

    if (!m_rowTree->parentRow() || m_rowTree->rowType() == OBJTYPE_LAYER
            || m_rowTree->hasComponentAncestor()) {
        m_minStartX = m_startX;
        m_maxEndX = m_endX;
    }

    Ruler *ruler = m_rowTree->m_scene->ruler();
    m_startTime = ruler->distanceToTime(m_startX);
    m_endTime = ruler->distanceToTime(m_endX);

    // move keyframes with the row
    if (!m_rowTree->isProperty()) { // make sure we don't move the keyframes twice
        for (Keyframe *keyframe : qAsConst(m_keyframes))
            keyframe->time += ruler->distanceToTime(dx);
    }

    update();

    if (!m_rowTree->empty()) {
        updateChildrenMinStartXRecursive(m_rowTree);
        updateChildrenMaxEndXRecursive(m_rowTree);

        for (RowTree *child : qAsConst(m_rowTree->m_childRows)) {
            if (!child->locked())
                child->m_rowTimeline->moveDurationBy(dx);
        }
    }
}

long RowTimeline::getDurationMoveTime() const
{
    return m_startTime - m_startDurationMoveStartTime;
}

double RowTimeline::getDurationMoveOffsetX() const
{
    return m_startDurationMoveOffsetX;
}

long RowTimeline::getDuration() const
{
    return m_endTime - m_startTime;
}

void RowTimeline::collectChildKeyframeTimes(QVector<long> &childKeyframeTimes)
{
    const auto childRows = m_rowTree->childRows();
    for (const auto row : childRows) {
        row->rowTimeline()->collectChildKeyframeTimes(childKeyframeTimes);
        const auto keyframes = row->rowTimeline()->keyframes();
        for (const auto kf : keyframes)
            childKeyframeTimes.append(kf->time);
    }
}

// called after timeline scale is changed to update duration star/end positions
void RowTimeline::updatePosition()
{
    clearBoundChildren();
    setStartTime(m_startTime);
    setEndTime(m_endTime);
}

// Set the position of the start of the row duration
void RowTimeline::setStartX(double startX)
{
    if (startX < 0)
        startX = 0;
    else if (startX > m_endX)
        startX = m_endX;

    m_startX = startX;
    m_startTime = m_rowTree->m_scene->ruler()->distanceToTime(startX);

    if (!m_rowTree->parentRow() || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE
            || m_rowTree->hasComponentAncestor()) {
        m_minStartX = 0;
    }

    updateChildrenStartRecursive();
    updateChildrenMinStartXRecursive(m_rowTree);
    update();
}

// Set the position of the end of the row duration
void RowTimeline::setEndX(double endX)
{
    if (endX < m_startX)
        endX = m_startX;

    m_endX = endX;
    m_endTime = m_rowTree->m_scene->ruler()->distanceToTime(endX);

    if (!m_rowTree->parentRow() || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE
        || m_rowTree->hasComponentAncestor()) {
        m_maxEndX = 999999;
    }

    updateChildrenEndRecursive();
    updateChildrenMaxEndXRecursive(m_rowTree);
    update();
}

QColor RowTimeline::barColor() const
{
    return m_barColor;
}

void RowTimeline::setBarColor(const QColor &color)
{
    m_barColor = color;
    update();
}

void RowTimeline::setControllerText(const QString &controller)
{
    m_controllerDataInput = controller;
    update();
}

void RowTimeline::updateChildrenStartRecursive()
{
    for (auto child : qAsConst(m_boundChildrenStart)) {
        if (!child.isNull()) {
            child->m_startX = m_startX;
            child->m_startTime = m_startTime;
            child->updateChildrenStartRecursive();
            child->update();
        }
    }
}

void RowTimeline::updateChildrenEndRecursive()
{
    for (auto child : qAsConst(m_boundChildrenEnd)) {
        if (!child.isNull()) {
            child->m_endX = m_endX;
            child->m_endTime = m_endTime;
            child->updateChildrenEndRecursive();
            child->update();
        }
    }
}

void RowTimeline::updateChildrenMinStartXRecursive(RowTree *rowTree)
{
    if (m_rowTree->rowType() != OBJTYPE_SCENE && !rowTree->empty()) {
        const auto childRows = rowTree->childRows();
        bool isComponentChild = m_rowTree->rowType() == OBJTYPE_COMPONENT
                || m_rowTree->hasComponentAncestor();
        for (auto child : childRows) {
            if (isComponentChild) {
                child->rowTimeline()->m_minStartX = 0;
            } else {
                child->rowTimeline()->m_minStartX = qMax(rowTree->rowTimeline()->m_startX,
                                                         rowTree->rowTimeline()->m_minStartX);
            }
            child->rowTimeline()->update();

            updateChildrenMinStartXRecursive(child);
        }
    }
}

void RowTimeline::updateChildrenMaxEndXRecursive(RowTree *rowTree)
{
    if (m_rowTree->rowType() != OBJTYPE_SCENE && !rowTree->empty()) {
        const auto childRows = rowTree->childRows();
        bool isComponentChild = m_rowTree->rowType() == OBJTYPE_COMPONENT
                || m_rowTree->hasComponentAncestor();
        for (auto child : childRows) {
            if (isComponentChild) {
                child->rowTimeline()->m_maxEndX = 999999;
            } else {
                child->rowTimeline()->m_maxEndX = qMin(rowTree->rowTimeline()->m_endX,
                                                       rowTree->rowTimeline()->m_maxEndX);
            }
            child->rowTimeline()->update();

            updateChildrenMaxEndXRecursive(child);
        }
    }
}

void RowTimeline::updateCommentItem()
{
    if (!m_commentItem)
        return;
    TimelineToolbar *toolbar = m_rowTree->m_scene->widgetTimeline()->toolbar();
    // Backend allows storing comments for rows with duration bar
    bool canHaveComment = m_rowTree->hasDurationBar();
    bool showComments = canHaveComment && toolbar->actionShowRowTexts()->isChecked();
    m_commentItem->setVisible(showComments);
    if (showComments && m_rowTree->m_binding) {
        ITimelineTimebar *timebar = m_rowTree->m_binding->GetTimelineItem()->GetTimebar();
        m_commentItem->setLabel(timebar->GetTimebarComment());
    }
}

void RowTimeline::updateCommentItemPos()
{
    if (!m_commentItem)
        return;

    Ruler *ruler = m_rowTree->m_scene->ruler();
    m_commentItem->setPos(TimelineConstants::RULER_EDGE_OFFSET + ruler->viewportX(),
                         -TimelineConstants::ROW_TEXT_OFFSET_Y);
}

void RowTimeline::setStartTime(long startTime)
{
    m_startTime = startTime;
    m_startX = m_rowTree->m_scene->ruler()->timeToDistance(startTime);

    if (!m_rowTree->parentRow() || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE
            || m_rowTree->hasComponentAncestor()) {
        m_minStartX = 0;
    }

    updateChildrenStartRecursive();
    updateChildrenMinStartXRecursive(m_rowTree);
    update();
}

void RowTimeline::setEndTime(long endTime)
{
    m_endTime = endTime;
    m_endX = m_rowTree->m_scene->ruler()->timeToDistance(endTime);

    if (!m_rowTree->parentRow() || m_rowTree->parentRow()->rowType() == OBJTYPE_SCENE
            || m_rowTree->hasComponentAncestor()) {
        m_maxEndX = 999999;
    }

    updateChildrenEndRecursive();
    updateChildrenMaxEndXRecursive(m_rowTree);
    update();
}

// duration start x in local space (x=0 at time=0)
double RowTimeline::getStartX() const
{
    return m_startX;
}

// duration end x in local space
double RowTimeline::getEndX() const
{
    return m_endX;
}

long RowTimeline::getStartTime() const
{
    return m_startTime;
}

long RowTimeline::getEndTime() const
{
    return m_endTime;
}

void RowTimeline::setState(State state)
{
    m_state = state;
    m_rowTree->m_state = state;

    update();
    m_rowTree->update();
}

void RowTimeline::setRowTree(RowTree *rowTree)
{
    m_rowTree = rowTree;
    if (m_rowTree->isProperty()) {
        if (m_propertyGraph)
            delete m_propertyGraph;
        m_propertyGraph = new RowTimelinePropertyGraph(this);
    }
    initialize();
}

RowTree *RowTimeline::rowTree() const
{
    return m_rowTree;
}

QList<Keyframe *> RowTimeline::keyframes() const
{
    return m_keyframes;
}

QString RowTimeline::formatTime(long millis) const
{
    static const QString timeTemplate = tr("%1:%2.%3");
    static const QChar fillChar = tr("0").at(0);

    long mins = millis % 3600000 / 60000;
    long secs = millis % 60000 / 1000;
    long mils = millis % 1000;

    return timeTemplate.arg(mins).arg(secs, 2, 10, fillChar).arg(mils, 3, 10, fillChar);
}

void RowTimeline::showToolTip(const QPointF &pos)
{
    QLabel *tooltip = m_rowTree->m_scene->timebarTooltip();

    tooltip->setText(formatTime(m_startTime) + " - " + formatTime(m_endTime)
                     + " (" + formatTime(m_endTime - m_startTime) + ")");

    tooltip->adjustSize();

    QPoint newPos = pos.toPoint() + QPoint(-tooltip->width() / 2,
                     -tooltip->height() - TimelineConstants::TIMEBAR_TOOLTIP_OFFSET_V);

    // Confine the tooltip to the current screen area to avoid artifacts from different pixel ratios
    static const int MARGIN = 5;
    const QRect screenGeometry = QApplication::desktop()->screenGeometry(
                m_rowTree->m_scene->widgetTimeline());
    int xMin = screenGeometry.x() + MARGIN;
    int xMax = screenGeometry.x() + screenGeometry.width() - tooltip->width() - MARGIN;
    if (newPos.x() < xMin)
        newPos.setX(xMin);
    else if (newPos.x() > xMax)
        newPos.setX(xMax);

    tooltip->move(newPos);
    tooltip->raise();
    tooltip->show();
}

RowTimeline *RowTimeline::parentRow() const
{
    if (!m_rowTree->m_parentRow)
        return nullptr;

    return m_rowTree->m_parentRow->rowTimeline();
}

void RowTimeline::hoverLeaveEvent(QGraphicsSceneHoverEvent  *event)
{
    InteractiveTimelineItem::hoverLeaveEvent(event);
    // Make sure mouse cursor is reseted when moving away from timeline row
    m_rowTree->m_scene->resetMouseCursor();
}

int RowTimeline::type() const
{
    // Enable the use of qgraphicsitem_cast with this item.
    return TypeRowTimeline;
}

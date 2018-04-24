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

#ifndef KEYFRAMEMANAGER_H
#define KEYFRAMEMANAGER_H

#include "Bindings/ITimelineKeyframesManager.h"
#include <QtCore/qlist.h>
#include <StudioObjectTypes.h>
#include "Qt3DSDMAnimation.h"

class RowTimeline;
class TimelineGraphicsScene;
class CPasteKeyframeCommandHelper;
struct Keyframe;

QT_FORWARD_DECLARE_CLASS(QGraphicsSceneContextMenuEvent)
QT_FORWARD_DECLARE_CLASS(QRectF)

class KeyframeManager : public ITimelineKeyframesManager
{
public:
    KeyframeManager(TimelineGraphicsScene *m_scene);
    virtual ~KeyframeManager();

    QList<Keyframe *> insertKeyframe(RowTimeline *row, double time,
                                     bool selectInsertedKeyframes = true);
    void selectKeyframe(Keyframe *keyframe);
    void selectConnectedKeyframes(Keyframe *keyframe);
    void selectKeyframesInRect(const QRectF &rect);
    void selectKeyframes(const QList<Keyframe *> &keyframes);
    void deselectKeyframe(Keyframe *keyframe);
    void deselectConnectedKeyframes(Keyframe *keyframe);
    void deselectAllKeyframes();
    void deleteKeyframes(RowTimeline *row, bool repaint = true);
    void copySelectedKeyframes();
    void pasteKeyframes();
    void moveSelectedKeyframes(double dx);
    void commitMoveSelectedKeyframes();
    bool deleteSelectedKeyframes();
    bool oneMasterRowSelected() const;
    bool hasSelectedKeyframes() const;
    bool hasCopiedKeyframes() const;

    // IKeyframesManager interface
    // Mahmoud_TODO: rewrite a better interface for the new timeline
    // ITimelineKeyframesManager interface
    void SetKeyframeTime(long inTime) override;
    void SetKeyframesDynamic(bool inDynamic) override;
    long OffsetSelectedKeyframes(long inOffset) override;
    bool CanMakeSelectedKeyframesDynamic() override;
    void CommitChangedKeyframes() override;
    void RollbackChangedKeyframes() override;
    // IKeyframesManager interface
    bool HasSelectedKeyframes(bool inOnlyDynamic) override;
    bool HasDynamicKeyframes() override;
    bool CanPerformKeyframeCopy() override;
    bool CanPerformKeyframePaste() override;
    void CopyKeyframes() override;
    bool RemoveKeyframes(bool inPerformCopy) override;
    void PasteKeyframes() override;
    void SetKeyframeInterpolation() override;
    void SelectAllKeyframes() override;
    void DeselectAllKeyframes() override;
    void SetChangedKeyframes() override;

private:
    static const QHash<int, QList<QString>> SUPPORTED_ROW_PROPS;

    QList<Keyframe *> filterKeyframesForRow(RowTimeline *row, const QList<Keyframe *> &keyframes);
    qt3dsdm::SGetOrSetKeyframeInfo setKeyframeInfo(qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe,
                                                   qt3dsdm::IAnimationCore &inCore);
    CPasteKeyframeCommandHelper *m_pasteKeyframeCommandHelper;
    TimelineGraphicsScene *m_scene;
    QList<Keyframe *> m_selectedKeyframes;
    QList<RowTimeline *> m_selectedKeyframesMasterRows;
};

#endif // KEYFRAMEMANAGER_H

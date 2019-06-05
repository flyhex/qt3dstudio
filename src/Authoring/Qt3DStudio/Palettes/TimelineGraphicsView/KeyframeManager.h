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

#include "IKeyframesManager.h"
#include "Qt3DSDMAnimation.h"
#include <QtCore/qlist.h>
#include <StudioObjectTypes.h>

class RowTimeline;
class RowTree;
class TimelineGraphicsScene;
class CPasteKeyframeCommandHelper;
struct Keyframe;

QT_FORWARD_DECLARE_CLASS(QGraphicsSceneContextMenuEvent)
QT_FORWARD_DECLARE_CLASS(QRectF)

class KeyframeManager : public IKeyframesManager
{
public:
    KeyframeManager(TimelineGraphicsScene *m_scene);
    virtual ~KeyframeManager() override;

    QList<Keyframe *> insertKeyframe(RowTimeline *row, long time,
                                     bool selectInsertedKeyframes = true);
    void selectKeyframe(Keyframe *keyframe);
    void selectConnectedKeyframes(Keyframe *keyframe);
    void selectKeyframesInRect(const QRectF &rect);
    void selectKeyframes(const QList<Keyframe *> &keyframes);
    QList<Keyframe *> selectedKeyframes() const;
    void deselectKeyframe(Keyframe *keyframe);
    void deselectConnectedKeyframes(Keyframe *keyframe);
    void deselectAllKeyframes();
    void deselectRowKeyframes(RowTree *row);
    void deleteKeyframes(RowTimeline *row, bool repaint = true);
    void copySelectedKeyframes();
    void pasteKeyframes();
    void moveSelectedKeyframes(long newTime);
    void commitMoveSelectedKeyframes();
    bool deleteSelectedKeyframes();
    bool oneMasterRowSelected() const;
    bool hasSelectedKeyframes() const;
    bool hasCopiedKeyframes() const;
    bool hasDynamicKeyframes(RowTree *row) const;

    // IKeyframesManager interface
    void SetKeyframeTime(long inTime) override;
    void SetKeyframesDynamic(bool inDynamic) override;
    void CommitChangedKeyframes() override;
    void RollbackChangedKeyframes() override;
    bool HasSelectedKeyframes() override;
    bool HasDynamicKeyframes() override;
    bool CanPerformKeyframeCopy() override;
    bool CanPerformKeyframePaste() override;
    void CopyKeyframes() override;
    bool RemoveKeyframes(bool inPerformCopy) override;
    void PasteKeyframes() override;
    void SetKeyframeInterpolation() override;
    void DeselectAllKeyframes() override;
    void SetChangedKeyframes() override;
    long getPressedKeyframeOffset() const;

private:
    qt3dsdm::SGetOrSetKeyframeInfo setKeyframeInfo(qt3dsdm::Qt3DSDMKeyframeHandle inKeyframe,
                                                   qt3dsdm::IAnimationCore &inCore);
    long getMinSelectedKeyframesTime() const;

    CPasteKeyframeCommandHelper *m_pasteKeyframeCommandHelper = nullptr;
    TimelineGraphicsScene *m_scene;
    QList<Keyframe *> m_selectedKeyframes;
    QList<RowTimeline *> m_selectedKeyframesMasterRows;
};

#endif // KEYFRAMEMANAGER_H

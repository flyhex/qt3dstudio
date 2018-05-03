/****************************************************************************
**
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
#ifndef TIMELINEOBJECTMODEL_H
#define TIMELINEOBJECTMODEL_H

#include "ObjectListModel.h"

#include "TimelineRow.h"

#include <QSharedPointer>

class CSlideRow;
class CPropertyRow;
class IKeyframe;
class ITimelineItemBinding;

struct TimebarTimeInfo {
    Q_PROPERTY(double startPosition MEMBER m_startPosition FINAL)
    Q_PROPERTY(double endPosition MEMBER m_endPosition FINAL)
    Q_PROPERTY(double lifeStart MEMBER m_lifeStart FINAL)
    Q_PROPERTY(double lifeEnd MEMBER m_lifeEnd FINAL)

    double m_startPosition = 0;
    double m_endPosition = 0;
    double m_lifeStart = 0;
    double m_lifeEnd = 0;

    Q_GADGET
};

struct KeyframeInfo {
    Q_PROPERTY(double position MEMBER m_position FINAL)
    Q_PROPERTY(long time MEMBER m_time FINAL)
    Q_PROPERTY(bool selected MEMBER m_selected FINAL)
    Q_PROPERTY(bool dynamic MEMBER m_dynamic FINAL)

    long m_time;
    bool m_selected;
    bool m_dynamic;
    double m_position;

    Q_GADGET
};

Q_DECLARE_METATYPE(TimebarTimeInfo)
Q_DECLARE_METATYPE(KeyframeInfo)

class TimelineObjectModel : public ObjectListModel
{
    Q_OBJECT

public:
    using ObjectListModel::ObjectListModel;
    ~TimelineObjectModel() override;

    enum Roles {
        TimelineRowRole = ObjectListModel::LastRole + 100,
        ItemColorRole,
        SelectedColorRole,
        SelectedRole,
        TimeInfoRole,
        KeyframesRole,
        ShyRowRole,
        VisibleRowRole,
        LockedRowRole,
        IsPropertyRole,
        PropertyExpandedRole,
        VisibleRole,
        HasActionRole,
        HasMasterActionRole,
        HasChildActionRole,
        HasMasterChildActionRole,
        HasComponentActionRole,
        HasMasterComponentActionRole
    };

    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant dataForProperty(CPropertyRow *propertyRow, const QModelIndex &index,
                             int role = Qt::DisplayRole) const;
    QModelIndex parent(const QModelIndex &index) const override;

    void setTimelineItemBinding(ITimelineItemBinding *inTimelineItem);

    CTimelineRow* timelineRowForIndex(const QModelIndex &index);

    void addProperty(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                     qt3dsdm::Qt3DSDMPropertyHandle property);
    void removeProperty(qt3dsdm::Qt3DSDMInstanceHandle parentInstance,
                        qt3dsdm::Qt3DSDMPropertyHandle property);

    void setHideShy(bool enabled);
    bool hideShy() const { return m_hideShy; }
    void setHideHidden(bool enabled);
    bool hideHidden() const { return m_hideHidden; }
    void setHideLocked(bool enabled);
    bool hideLocked() const { return m_hideLocked; }

    void updateActions();

protected:
    qt3dsdm::TInstanceHandleList childrenList(const qt3dsdm::Qt3DSDMSlideHandle &slideHandle,
                                              const qt3dsdm::Qt3DSDMInstanceHandle &handle) const override;



private:
   void appendKey(QVariantList &keyframes, IKeyframe *key, double timeRatio) const;
   bool isVisible(const QModelIndex &index) const;

   QSharedPointer<CSlideRow> m_slideRow;
   ITimelineItemBinding *m_timelineItemBinding = nullptr;
   QHash<int, CTimelineRow* > m_rows;
   mutable QHash<int, QVector<qt3dsdm::Qt3DSDMInstanceHandle> > m_properties;

   bool m_hideShy = false;
   bool m_hideHidden = false;
   bool m_hideLocked = true;
};

Q_DECLARE_METATYPE(CTimelineRow *);

#endif // TIMELINEOBJECTMODEL_H

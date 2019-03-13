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

#ifndef SLIDEMODEL_H
#define SLIDEMODEL_H

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qhash.h>

#include "Qt3DSDMHandles.h"

class CClientDataModelBridge;
class CDoc;

class SlideModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::DisplayRole,
        HandleRole = Qt::UserRole + 1,
        SelectedRole,
        VariantsRole
    };

    SlideModel(int slideCount, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::DisplayRole) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool insertRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int row, int count,
                    const QModelIndex &parent = QModelIndex()) override;
    void duplicateRow(int row);
    void startRearrange(int row);
    void move(int fromRow, int toRow);
    void finishRearrange(bool commit);

    void clear();
    void addNewSlide(int row);
    void removeSlide(int row);

    void onNewSlide(const qt3dsdm::Qt3DSDMSlideHandle &inSlide);
    void onDeleteSlide(const qt3dsdm::Qt3DSDMSlideHandle &inSlide);
    void onSlideRearranged(const qt3dsdm::Qt3DSDMSlideHandle &inMaster, int inOldIndex,
                           int toRow);
    void refreshSlideLabel(qt3dsdm::Qt3DSDMInstanceHandle instanceHandle,
                           qt3dsdm::Qt3DSDMPropertyHandle propertyHandle);
    void setSelectedSlideIndex(const QModelIndex &index);
    void refreshVariants(const QVector<QHash<QString, QStringList>> &vModel = {},
                         const QVector<QStringList> &vModelKeys = {});
    int rowToSlideIndex(int row) const;
    QVector<QHash<QString, QStringList> > variantsModel() const { return m_variantsModel; }
    QVector<QStringList> variantsModelKeys() const { return m_variantsModelKeys; }

private:
    bool hasSlideWithName(const QString &name) const;
    QString slideName(const qt3dsdm::Qt3DSDMSlideHandle &handle) const;
    void setSlideName(const qt3dsdm::Qt3DSDMSlideHandle &handle, const QString &name);
    inline CDoc *GetDoc() const;
    inline long slideIndex(const qt3dsdm::Qt3DSDMSlideHandle &handle) const;
    inline CClientDataModelBridge *GetBridge() const;

    QVector<qt3dsdm::Qt3DSDMSlideHandle> m_slides;
    int m_selectedRow = -1;
    int m_rearrangeStartRow = -1;
    int m_rearrangeEndRow = -1;
    QVector<QHash<QString, QStringList> > m_variantsModel;
    QVector<QStringList> m_variantsModelKeys;
    QHash<qt3dsdm::Qt3DSDMInstanceHandle, qt3dsdm::Qt3DSDMSlideHandle> m_slideLookupHash;
};


#endif // SLIDEMODEL_H

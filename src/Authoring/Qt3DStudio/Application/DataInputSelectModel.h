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
#ifndef DATAINPUTSELECTMODEL_H
#define DATAINPUTSELECTMODEL_H

#include <QtCore/qstringlistmodel.h>

class DataInputSelectModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int fixedItemCount MEMBER m_fixCount NOTIFY fixCountChanged)
public:
    Q_INVOKABLE QString getActiveIconPath() const;
    Q_INVOKABLE QString getInactiveIconPath() const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return m_diTypePairList.size();
    }
    void setData(const QVector<QPair<QString, QString>> &data);
    void clear();
    QHash<int, QByteArray> roleNames() const;
    explicit DataInputSelectModel(QObject *parent = nullptr);
    virtual ~DataInputSelectModel();
    void setFixedItemCount(int count) { m_fixCount = count; }
    int getFixedItemCount() const { return m_fixCount; }
    void showFixedItems(bool show) { m_showFixedItems = show; }
    bool getShowFixedItems() const { return m_showFixedItems; }
Q_SIGNALS:
    void fixCountChanged();

private:
    int m_fixCount = 0;
    bool m_showFixedItems = true;
    QVector<QPair<QString, QString>> m_diTypePairList;
    static const int TypeRole;

};
#endif // DATAINPUTSELECTMODEL_H

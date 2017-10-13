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
#ifndef OBJECTBROWSERVIEW_H
#define OBJECTBROWSERVIEW_H

#include <QQuickWidget>

#include "RelativePathTools.h"
#include "UICDMHandles.h"

#include <QColor>

class ObjectListModel;
class FlatObjectListModel;

class QAbstractItemModel;

class ObjectBrowserView : public QQuickWidget
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *model READ model NOTIFY modelChanged FINAL)
    Q_PROPERTY(int selection READ selection WRITE setSelection NOTIFY selectionChanged FINAL)
    Q_PROPERTY(PathType pathType READ pathType WRITE setPathType NOTIFY pathTypeChanged FINAL)

public:
    ObjectBrowserView(QWidget *parent = nullptr);


    enum PathType {
        Name = CRelativePathTools::EPATHTYPE_GUID,
        Relative = CRelativePathTools::EPATHTYPE_RELATIVE,
    };
    Q_ENUM(PathType)

    QAbstractItemModel *model() const;
    void setModel(ObjectListModel *model);
    QSize sizeHint() const override;

    Q_INVOKABLE QString name(int index) const;
    Q_INVOKABLE QString path(int index) const;

    int selection() const { return m_selection; }
    void setSelection(int index);

    PathType pathType() const {return m_pathType;}
    void setPathType(PathType type);

    qt3dsdm::CUICDMInstanceHandle selectedHandle() const;

Q_SIGNALS:
    void modelChanged();
    void pathTypeChanged();
    void selectionChanged();

protected:
    void focusOutEvent(QFocusEvent *event) override;

private:
    void initialize();

    FlatObjectListModel *m_model = nullptr;
    QHash<int, ObjectListModel *> m_subModels;
    QColor m_baseColor = QColor::fromRgb(75, 75, 75);
    QColor m_selectColor;
    int m_selection = -1;
    PathType m_pathType = Name;
};

#endif // OBJECTBROWSERVIEW_H

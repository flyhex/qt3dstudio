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

#ifndef ROWTREELABELITEM_H
#define ROWTREELABELITEM_H

#include "StudioObjectTypes.h"
#include <QtWidgets/qgraphicsitem.h>
#include <QtCore/qstring.h>
#include <QtWidgets/qgraphicssceneevent.h>
#include <QtGui/qevent.h>

class RowTree;

class RowTreeLabelItem : public QGraphicsTextItem
{
    Q_OBJECT
public:
    explicit RowTreeLabelItem(QGraphicsItem *parent = nullptr);

    QString label() const;
    void setLabel(const QString &label);
    void setLocked(bool isLocked);
    void setRowTypeLabel(EStudioObjectType rowType);
    RowTree *parentRow() const;
    void setParentRow(RowTree *row);
    int type() const;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    QRectF boundingRect() const override;

signals:
    void invalidLabel();
    void labelChanged(const QString label);

private:
    void validateLabel();

    RowTree *m_rowTree = nullptr;
    QString m_label;
    QString m_rowTypeLabel;
    bool m_locked;
    bool m_acceptOnFocusOut;

};

#endif // ROWTREELABELITEM_H

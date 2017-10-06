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
#ifndef EVENTSBROWSERVIEW_H
#define EVENTSBROWSERVIEW_H

#include <QQuickWidget>

#include "UICDMHandles.h"

class EventsModel;

class QAbstractItemModel;

class EventsBrowserView : public QQuickWidget
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *model READ model NOTIFY modelChanged FINAL)
    Q_PROPERTY(int selection READ selection WRITE setSelection NOTIFY selectionChanged FINAL)
public:
    explicit EventsBrowserView(QWidget *parent = nullptr);

    QAbstractItemModel *model() const;
    void setModel(EventsModel *model);
    QSize sizeHint() const override;
    UICDM::CDataModelHandle selectedHandle() const;

    int selection() const { return m_selection; }
    void setSelection(int index);

Q_SIGNALS:
    void modelChanged();
    void selectionChanged();

protected:
    void focusOutEvent(QFocusEvent *event) override;

private:
    void initialize();
    EventsModel *m_model = nullptr;
    QColor m_baseColor = QColor::fromRgb(75, 75, 75);
    QColor m_selectColor;
    int m_selection = -1;
};

#endif // EVENTSBROWSERVIEW_H

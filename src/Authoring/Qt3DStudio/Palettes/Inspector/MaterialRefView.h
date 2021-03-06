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

#ifndef MATERIALREFVIEW_H
#define MATERIALREFVIEW_H

#include <QtWidgets/qlistwidget.h>
#include "Qt3DSDMHandles.h"

class MaterialRefView : public QListWidget
{
    Q_OBJECT
    Q_PROPERTY(bool focused READ isFocused NOTIFY focusChanged)

public:
    explicit MaterialRefView(QWidget *parent = nullptr);

    QSize refreshMaterials(int instance, int handle);

    void updateSelection();
protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

Q_SIGNALS:
    void focusChanged();

private:
    bool isFocused() const;
    int getRefInstance() const;

    int m_instance = -1;
    int m_handle = -1;
};

#endif // MATERIALREFVIEW_H

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

#ifndef NAVIGATIONBAR_H
#define NAVIGATIONBAR_H

#include <QtCore/qpropertyanimation.h>
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qboxlayout.h>
#include "IBreadCrumbProvider.h"

class NavigationBar : public QWidget
{
    Q_OBJECT
public:
    explicit NavigationBar(QWidget *parent = nullptr);
    void updateNavigationItems(IBreadCrumbProvider *inBreadCrumbProvider);

public slots:
    void itemClicked(int index);

private:
    void setBarVisibility(bool visible);
    IBreadCrumbProvider *m_breadCrumbProvider = nullptr;
    QHBoxLayout *m_layout = nullptr;
    int m_itemAmount = 0;
    QPropertyAnimation m_expandAnimation;
};

#endif // NAVIGATIONBAR_H

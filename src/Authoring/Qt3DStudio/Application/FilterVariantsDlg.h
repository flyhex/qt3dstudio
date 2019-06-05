/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef FILTER_VARIANTS_DLG_H
#define FILTER_VARIANTS_DLG_H

#include "QmlUtils.h"

#include <QtQuickWidgets/qquickwidget.h>

class FilterVariantsModel;

class FilterVariantsDlg : public QQuickWidget
{
    Q_OBJECT

public:
    explicit FilterVariantsDlg(QWidget *parent, QAction *action, int actionSize,
                               QWidget *actionWidget);

    Q_INVOKABLE int actionSize() const;

    QString filterStr() const;
    void clearFilter();

protected:
    void focusOutEvent(QFocusEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    void showEvent(QShowEvent *event) override;

    void initialize();

    QmlUtils m_qmlUtils;
    QHash<QString, QSet<QString> > m_variantsFilter; // key: group, value: tags
    FilterVariantsModel *m_model = nullptr;
    QAction *m_action = nullptr;
    int m_actionSize = 0; // width/height of the action icon
    QWidget *m_actionWidget = nullptr;
};

#endif // FILTER_VARIANTS_DLG_H

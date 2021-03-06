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

#ifndef FILECHOOSERVIEW_H
#define FILECHOOSERVIEW_H

#include <QtQuickWidgets/qquickwidget.h>
#include <QtCore/qtimer.h>

class FileChooserModel;

class FileChooserView : public QQuickWidget
{
    Q_OBJECT
    Q_PROPERTY(int instance READ instance)
    Q_PROPERTY(int handle READ handle)

public:
    explicit FileChooserView(QWidget *parent = nullptr);

    void setHandle(int handle);
    int handle() const;

    void setInstance(int instance);
    int instance() const;

    void updateSelection();

Q_SIGNALS:
    void fileSelected(int handle, int instance, const QString &name);

protected:
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void showEvent(QShowEvent *event) override;
    void initialize();
    int m_handle = -1;
    int m_instance = -1;
    FileChooserModel *m_model = nullptr;
    QTimer *m_focusOutTimer = nullptr;
};

#endif // IMAGECHOOSERVIEW_H

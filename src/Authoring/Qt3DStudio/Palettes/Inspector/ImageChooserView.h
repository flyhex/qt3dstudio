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

#ifndef IMAGECHOOSERVIEW_H
#define IMAGECHOOSERVIEW_H

#include <QQuickWidget>

class ImageChooserModel;

class ImageChooserView : public QQuickWidget
{
    Q_OBJECT

    Q_PROPERTY(bool focused READ isFocused NOTIFY focusChanged)
    Q_PROPERTY(int instance READ instance)
    Q_PROPERTY(int handle READ handle)

public:
    explicit ImageChooserView(QWidget *parent = nullptr);

    void setHandle(int handle);
    int handle() const;

    void setInstance(int instance);
    int instance() const;
    QString currentDataModelPath() const;

    void updateSelection();

Q_SIGNALS:
    void imageSelected(int handle, int instance, const QString &name);

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

Q_SIGNALS:
    void focusChanged();

private:
    void initialize();
    bool isFocused() const;

    int m_handle = 0;
    int m_instance = 0;
    ImageChooserModel *m_model = nullptr;
};

#endif // IMAGECHOOSERVIEW_H

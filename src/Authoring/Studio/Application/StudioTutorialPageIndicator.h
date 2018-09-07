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

#ifndef STUDIOTUTORIALPAGEINDICATOR_H
#define STUDIOTUTORIALPAGEINDICATOR_H

#include <QtWidgets/qwidget.h>
#include <QtGui/qevent.h>

class StudioTutorialPageIndicator : public QWidget
{
    Q_OBJECT
public:
    explicit StudioTutorialPageIndicator(QWidget *parent = nullptr);

    void setCount(int count);
    void setCurrentIndex(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

signals:
    void indexChanged(int index);

private:
    int m_pageCount = 0;
    int m_currentIndex = 0;
    int m_dotSize = 26;
    int m_dotMargin = 4;
    QPixmap m_pixmapActive;
    QPixmap m_pixmapInactive;
};

#endif // STUDIOTUTORIALPAGEINDICATOR_H

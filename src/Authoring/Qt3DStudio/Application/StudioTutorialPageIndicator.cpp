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

#include "StudioTutorialPageIndicator.h"
#include <QtGui/qpainter.h>
#include <QtCore/qdebug.h>

StudioTutorialPageIndicator::StudioTutorialPageIndicator(QWidget *parent)
    : QWidget(parent)
{
    m_pixmapActive = QPixmap(":/images/Tutorial/dot_active.png");
    m_pixmapInactive = QPixmap(":/images/Tutorial/dot_inactive.png");
}

void StudioTutorialPageIndicator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    for (int i = 0; i < m_pageCount; i++) {
        const QPixmap pixmap = (i == m_currentIndex) ? m_pixmapActive : m_pixmapInactive;
        painter.drawPixmap(i * (m_dotSize + m_dotMargin), height() - m_dotSize, pixmap);
    }
}

void StudioTutorialPageIndicator::mousePressEvent(QMouseEvent *event)
{
    int posX = event->pos().x();
    int index = posX / (m_dotSize + m_dotMargin);
    emit indexChanged(index);
}

void StudioTutorialPageIndicator::setCount(int count)
{
    m_pageCount = count;
    int dotSize = m_pixmapActive.width();
    int margin = 4;
    setFixedSize(m_pageCount * (dotSize + margin), 80);
}

void StudioTutorialPageIndicator::setCurrentIndex(int index)
{
    m_currentIndex = index;
    update();
}

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

#include "NavigationBarItem.h"
#include "StudioPreferences.h"
#include "ResourceCache.h"

#include <QtCore/qdebug.h>
#include <QtWidgets/qsizepolicy.h>

NavigationBarItem::NavigationBarItem(QWidget *parent)
    : QWidget(parent)
{
    setHighlight(false);
    m_layout.setMargin(0);
    m_layout.setSpacing(0);
    m_iconLabel.setFixedWidth(20);
    m_iconLabel.setStyleSheet("padding: 0 0 0 4;");
    m_textLabel.setStyleSheet("padding: 0 4 0 0;");
    m_textLabel.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    m_layout.addWidget(&m_iconLabel);
    m_layout.addWidget(&m_textLabel);
    setLayout(&m_layout);
}

void NavigationBarItem::setIndex(int index)
{
    m_index = index;
}

void NavigationBarItem::setIcon(const QPixmap &pixmap)
{
    m_iconLabel.setPixmap(pixmap);
}

void NavigationBarItem::setText(const QString &text)
{
    QColor textColor = isEnabled() ? CStudioPreferences::GetNormalColor()
                                   : CStudioPreferences::GetInactiveColor();
    const QString fonttemplate = tr("<font color='%1'>%2</font>");
    m_textLabel.setText(fonttemplate.arg(textColor.name(), text));
}

void NavigationBarItem::setHighlight(bool highlight)
{
    if (highlight) {
        QColor bgColor = CStudioPreferences::GetMouseOverHighlightColor();
        QString bgColorStyle = QStringLiteral("background-color: ") + bgColor.name();
        setStyleSheet(bgColorStyle);
    } else {
        setStyleSheet("background-color: transparent;");
    }
}

void NavigationBarItem::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    emit clicked(m_index);
}

void NavigationBarItem::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (isEnabled())
        setHighlight(true);
}

void NavigationBarItem::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (isEnabled())
        setHighlight(false);
}

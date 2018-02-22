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

#ifndef TIMELINETOOLBAR_H
#define TIMELINETOOLBAR_H

#include <QtWidgets/qtoolbar.h>

QT_FORWARD_DECLARE_CLASS(QAction)
QT_FORWARD_DECLARE_CLASS(QSlider)

class TimelineToolbar : public QToolBar
{
    Q_OBJECT

signals:
    void newLayerTriggered();
    void deleteLayerTriggered();
    void gotoTimeTriggered();
    void firstFrameTriggered();
    void stopTriggered();
    void playTriggered();
    void lastFrameTriggered();
    void timelineScaleChanged(int scale);
    void setDurationTriggered();

public:
    TimelineToolbar();

    void setTime(double millis);

private:
    void addSpacing(int width);

    QAction *m_actionTime;
    QAction *m_actionDuration;
    QSlider *m_scaleSlider;
};

#endif // TIMELINETOOLBAR_H

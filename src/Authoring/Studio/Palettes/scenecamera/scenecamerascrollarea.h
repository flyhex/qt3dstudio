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

#ifndef SCENE_CAMERA_SCROLL_AREA
#define SCENE_CAMERA_SCROLL_AREA

#include <QtWidgets/qabstractscrollarea.h>

class SceneCameraGlWidget;

class SceneCameraScrollArea : public QAbstractScrollArea
{
    Q_OBJECT

public:
    SceneCameraScrollArea(QWidget *parent = nullptr);
    virtual ~SceneCameraScrollArea();

    SceneCameraGlWidget *glWidget() const { return m_glWidget; }

    void setZoom(qreal zoom, const QPoint &zoomPoint);
    void setPresentationSize(const QSize &size);
    void recalculateScrollRanges();
    void recalculateOffsets();
    void setPresentationAvailable(bool available);

Q_SIGNALS:
    void needUpdate();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void scrollContentsBy(int, int) override;
    void showEvent(QShowEvent *event) override;

private:
    void resizeGlWidget();
    QSizeF zoomedPresentationSize();

protected:
    SceneCameraGlWidget *m_glWidget = nullptr;
    qreal m_zoom = 1.0;
    QSize m_presentationSize;
};

#endif

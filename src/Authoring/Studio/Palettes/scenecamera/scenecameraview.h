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

#ifndef SCENECAMERAVIEW_H
#define SCENECAMERAVIEW_H

#include "DispatchListeners.h"

#include <QtWidgets/qwidget.h>
#include <QtCore/qtimer.h>

#ifdef QT_NAMESPACE
using namespace QT_NAMESPACE;
#endif

QT_BEGIN_NAMESPACE
namespace Ui {
class SceneCameraView;
}
QT_END_NAMESPACE

class CMainFrame;

class SceneCameraView : public QWidget,
                        public CPresentationChangeListener
{
    Q_OBJECT

public:
    explicit SceneCameraView(CMainFrame *mainFrame, QWidget *parent = 0);
    ~SceneCameraView();

    // CPresentationChangeListener
    void OnNewPresentation() override;
    void OnClosingPresentation() override;

protected:
    void wheelEvent(QWheelEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    void handleSliderValueChange();
    void doUpdate();
    void requestUpdate();

    Ui::SceneCameraView *m_ui = nullptr;

    QTimer m_updateTimer;
    QPoint m_zoomPoint;
    QPoint m_mousePressPointLeft;
    QPoint m_mousePressPointRight;
    QPoint m_mousePressScrollValues;
    int m_mousePressZoomValue = 0;

    QCursor m_cursorPan;
    QCursor m_cursorZoom;
};

#endif // SCENECAMERAVIEW_H

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

#ifndef Q3DS_PLAYER_WND
#define Q3DS_PLAYER_WND

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "DropContainer.h"

#include <QtWidgets/qscrollarea.h>
#include <QtGui/qwindow.h>
#include <QtCore/qcoreapplication.h>

class CSceneView;
class CStudioApp;
class CMouseCursor;
class CHotkeys;

namespace Q3DStudio {

class RenderWindow : public QWindow
{
    Q_OBJECT
public:
    QWidget *m_container = nullptr;
    RenderWindow(QWindow *window = nullptr)
        : QWindow(window)
    {

    }
#ifdef Q_OS_WIN //QTBUG-50505
    bool event(QEvent *e) override
    {
        switch (e->type())
        {
            case QEvent::MouseButtonPress:
            case QEvent::MouseButtonRelease:
            case QEvent::MouseButtonDblClick:
            case QEvent::MouseMove:
            case QEvent::FocusIn:
            case QEvent::FocusOut:
            case QEvent::FocusAboutToChange:
            case QEvent::Enter:
            case QEvent::Leave:
            case QEvent::Wheel:
            case QEvent::TabletMove:
            case QEvent::TabletPress:
            case QEvent::TabletRelease:
            case QEvent::TabletEnterProximity:
            case QEvent::TabletLeaveProximity:
            case QEvent::TouchBegin:
            case QEvent::TouchUpdate:
            case QEvent::TouchEnd:
            case QEvent::InputMethodQuery:
            case QEvent::TouchCancel:
                return QCoreApplication::sendEvent(m_container, e);
            default:
                break;
        }
        return QWindow::event(e);
    }
#endif
};

class Q3DSPlayerWnd : public QScrollArea, public CWinDropContainer
{
    Q_OBJECT
public:

    typedef enum
    {
        VIEW_EDIT = 0,
        VIEW_SCENE,
    }  EViewMode;

    explicit Q3DSPlayerWnd(QWidget *parent = nullptr);
    ~Q3DSPlayerWnd() override;

    QSize sizeHint() const override;

    void onDragEnter() override;
    bool OnDragWithin(CDropSource &inSource) override;
    bool OnDragReceive(CDropSource &inSource) override;
    void OnDragLeave() override;
    void OnReflectMouse(CPt &, Qt::KeyboardModifiers) override {}

    qreal fixedDevicePixelRatio() const;
    void setToolMode(long toolMode) { m_previousToolMode = toolMode; }
    void setWindowPosition();

    void setScrollRanges();
    void recenterClient();
    void onRulerGuideToggled();

    void setViewMode(EViewMode inViewMode);
    EViewMode viewMode();
    bool isDeploymentView();

    QRect displayedClientRect() const { return m_ClientRect; }

    QSize effectivePresentationSize() const;

    void setSceneView(CSceneView *view) { m_SceneView = view; }

Q_SIGNALS:
    void dropReceived();
    void newFrame();
    void toolChanged();

private Q_SLOTS:
    void handleObjectPicked(int instance);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *) override;
    void scrollContentsBy(int, int) override;

    bool shouldHideScrollBars();

    class ObjectRequestData {
    public:
        int m_instance = 0;
        bool m_dropping = false;
        QString m_matFilePath;

        void clear() {
            m_instance = 0;
            m_dropping = false;
            m_matFilePath.clear();
        }
    };

    RenderWindow *m_renderWindow;
    QWidget *m_widget;
    bool m_mouseDown;
    bool m_resumePlayOnMouseRelease = false;
    long m_previousToolMode;
    CSceneView *m_SceneView;
    QRect m_ClientRect;
    EViewMode m_ViewMode;
    ObjectRequestData m_objectRequestData;
};

}

#endif // Q3DS_PLAYER_WND

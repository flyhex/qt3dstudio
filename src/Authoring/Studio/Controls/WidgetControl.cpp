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

#include "WidgetControl.h"

#include "Control.h"
#include "DropSource.h"
#include "IDragable.h"
#include "OffscreenRenderer.h"
#include "Pt.h"
#include "Rct.h"
#include "UICFile.h"

#include <QDrag>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QShowEvent>
#include <QWheelEvent>

WidgetControl::WidgetControl(CControl *control, QWidget *parent)
    : QWidget(parent)
    , m_control(control)
    , m_controlListener(this)
{
    Q_ASSERT(control);
    control->SetWindowListener(&m_controlListener);
    setControlSize(sizeHint());
}

WidgetControl::~WidgetControl()
{
    m_control->SetWindowListener(nullptr);
}

bool WidgetControl::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        if (ke->key() == Qt::Key_C && ke->modifiers() == Qt::ControlModifier)
            m_control->OnKeyDown(ke->key(), ke->modifiers());
    }
    return QWidget::event(event);
}

void WidgetControl::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
}

void WidgetControl::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    const auto boundRect = QRect(QPoint(0,0), size());
    CWinRenderer renderer(&painter, boundRect);
    CRct rect(event->rect());
    m_control->OnDraw(&renderer, rect, true);

    QWidget::paintEvent(event);
}

void WidgetControl::resizeEvent(QResizeEvent *event)
{
    setControlSize(event->size());
    QWidget::resizeEvent(event);
}

void WidgetControl::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
    m_control->OnKeyDown(event->key(), event->modifiers());
    m_control->OnChar(event->text(), event->modifiers());
}

void WidgetControl::keyReleaseEvent(QKeyEvent *event)
{
    m_control->OnKeyUp(event->key(), event->modifiers());
    QWidget::keyReleaseEvent(event);
}

void WidgetControl::mousePressEvent(QMouseEvent *event)
{
    const auto pos = CPt(event->pos());
    if (m_isLeftMouseDown)
        m_control->OnMouseUp(pos, event->modifiers());

    m_isLeftMouseDown = (event->button() == Qt::LeftButton);
    if (m_isLeftMouseDown)
        m_control->OnMouseDown(pos, event->modifiers());
    else
        m_control->OnMouseRDown(pos, event->modifiers());

    setFocus();
    QWidget::mousePressEvent(event);
}

void WidgetControl::mouseReleaseEvent(QMouseEvent *event)
{
    const auto pos = CPt(event->pos());
    if (event->button() == Qt::LeftButton) {
        m_isLeftMouseDown = false;
        m_control->OnMouseUp(pos, event->modifiers());
    } else {
        m_control->OnMouseRUp(pos, event->modifiers());
    }

    QWidget::mouseReleaseEvent(event);
}

void WidgetControl::mouseMoveEvent(QMouseEvent *event)
{
    m_control->OnMouseMove(event->pos(), event->modifiers());
    QWidget::mouseMoveEvent(event);
}

void WidgetControl::mouseDoubleClickEvent(QMouseEvent *event)
{
    // call QWidget handler first to not deliver OnMouseDown after OnMouseDoubleClick
    QWidget::mouseDoubleClickEvent(event);
    m_control->OnMouseDoubleClick(event->pos(), event->modifiers());
}

void WidgetControl::wheelEvent(QWheelEvent *event)
{
    m_control->OnMouseWheel(event->pos(), event->angleDelta().y(), event->modifiers());
    QWidget::wheelEvent(event);
}

void WidgetControl::enterEvent(QEvent *event)
{
    setMouseTracking(true);
    m_control->OnMouseHover(mapFromGlobal(QCursor::pos()), {});
    QWidget::enterEvent(event);
}

void WidgetControl::leaveEvent(QEvent *event)
{
    setMouseTracking(false);
    m_control->OnMouseOut(mapFromGlobal(QCursor::pos()), {});
    QWidget::leaveEvent(event);
}

void WidgetControl::focusInEvent(QFocusEvent *event)
{
    m_control->OnGainFocus();
    QWidget::focusInEvent(event);
}

void WidgetControl::focusOutEvent(QFocusEvent *event)
{
    if (!m_isContextMenuShown)
        m_control->OnLoseFocus();
    QWidget::focusOutEvent(event);
}

QSize WidgetControl::sizeHint() const
{
    const auto preferredSize = m_control->GetPreferredSize();
    return QSize(preferredSize.x, preferredSize.y);
}

/*
 * CPaletteManager::GetTimelineControl() needs a way of accessing
 * the CControl inside the widget
 */
CControl *WidgetControl::getControl() const
{
    return m_control;
}

void WidgetControl::setControlSize(const QSize &size)
{
    m_control->SetSize(size.width(), size.height());
}

void WidgetControl::DoStartDrag(IDragable *inDragable)
{
    if (m_isDragging || !m_isLeftMouseDown)
        return;

    QDrag drag(this);
    m_isDragging = true;

    drag.setMimeData(CDropSourceFactory::Create(inDragable->GetFlavor(), inDragable));
    drag.exec();
    m_isLeftMouseDown = false;

    m_isDragging = false;
}

void WidgetControl::DoStartDrag(std::vector<Q3DStudio::CString> &inDragFileNameList)
{
    if (m_isDragging || !m_isLeftMouseDown)
        return;

    QDrag drag(this);
    m_isDragging = true;

    try {
        auto thePos = inDragFileNameList.begin();
        auto theEndPos = inDragFileNameList.end();

        Q3DStudio::CAutoMemPtr<CUICFile> theDragFile;
        for (; thePos != theEndPos; ++thePos) {
            Q3DStudio::CString theDragFileName = *thePos;
            if (theDragFileName.Length() > 0) {
                theDragFile = new CUICFile(theDragFileName);
                CDropSource *theDropSource = CDropSourceFactory::Create(
                    QT3DS_FLAVOR_ASSET_UICFILE, (void *)theDragFile, sizeof(theDragFile));
                // Add the QT3DS_GESTURE_FLAVOR.  This will allow us to drag to StudioControls.
                drag.setMimeData(theDropSource);
                break;
            }
        }
        drag.exec();
        m_isLeftMouseDown = false;
    } catch (...) { // if there are any errors that throws an exception, there
        // there will be no more drag and drop, since the flag will not be reset.
    }

    m_isDragging = false;
}

bool WidgetControl::OnDragWithin(CDropSource &inSource)
{
    bool theReturn = false;
    CPt thePoint = inSource.GetCurrentPoint();
    Qt::KeyboardModifiers theFlags = inSource.GetCurrentFlags();
    CDropTarget *theDropTarget = m_control->FindDropCandidate(thePoint, theFlags);

    if (theDropTarget) {
        theReturn = theDropTarget->Accept(inSource);
        delete theDropTarget;
    }
    return theReturn;
}

bool WidgetControl::OnDragReceive(CDropSource &inSource)
{
    bool theReturn = false;
    CPt thePoint = inSource.GetCurrentPoint();
    Qt::KeyboardModifiers theFlags = inSource.GetCurrentFlags();

    CDropTarget *theDropTarget = m_control->FindDropCandidate(thePoint, theFlags);

    if (theDropTarget) {
        theReturn = theDropTarget->Drop(inSource);
        delete theDropTarget;
    }
    return theReturn;
}

void WidgetControl::OnDragLeave()
{
    m_control->OnMouseMove(CPt(-1, -1), 0);
}

void WidgetControl::OnReflectMouse(CPt &inPoint, Qt::KeyboardModifiers inFlags)
{
    // Notify the control that the mouse moved
    m_control->OnMouseMove(inPoint, inFlags /*CHotKeys::GetCurrentKeyModifiers( )*/);

    // If the control invalidated because of a mouse event then we want to do an immediate redraw.
    // this ensures consistent visible feedback.
    if (m_control->IsChildInvalidated())
        repaint();
}

//=============================================================================
/**
 * Creates the pass thru class for the wnd control.
 */
WidgetControlControlListener::WidgetControlControlListener(WidgetControl *inParent)
{
  m_Parent = inParent;
}

WidgetControlControlListener::~WidgetControlControlListener()
{
}

//=============================================================================
/**
 * Notification from the control that the window was invalidated.
 */
void WidgetControlControlListener::OnControlInvalidated()
{
    m_Parent->update();
}

//=============================================================================
/**
 * Notification from the control to do a popup at the specified location.
 */
long WidgetControlControlListener::DoPopup(QMenu *inMenu, CPt inPoint)
{
    long selectedIndex = -1;
    if (inMenu) {
        m_Parent->setContextMenuShown(true);
        auto action = inMenu->exec(m_Parent->mapToGlobal(inPoint));
        m_Parent->setContextMenuShown(false);
        if (action)
            selectedIndex = inMenu->actions().indexOf(action);
    }

    return selectedIndex;
}

//=============================================================================
/**
 * Get the location of the point in Screen coordinates.
 */
CPt WidgetControlControlListener::ClientToScreen(CPt inPoint)
{
    return m_Parent->mapToGlobal(inPoint);
}

//=============================================================================
/**
 * Get the location of the point into client coordinates.
 */
CPt WidgetControlControlListener::ScreenToClient(CPt inPoint)
{
    return m_Parent->mapFromGlobal(inPoint);
}

//=============================================================================
/**
 * Get the platform dependent view that this is embedding.
 * Used when platform dependent controls need to be embedded into the Controls.
 */
TPlatformView WidgetControlControlListener::GetPlatformView()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    return m_Parent->m_hWnd;
#else
    return nullptr;
#endif
}

//=============================================================================
/**
 * Pass-thru to the CWndControl's SetIsDragging function.
 * @param inIsDragging true to specify that a drag is occurring or false to cancel a drag
 */
void WidgetControlControlListener::SetIsDragging(bool inIsDragging)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    m_Parent->SetIsDragging(inIsDragging);
#endif
}

//=============================================================================
/**
 * Pass-thru to the CWndControl's ShowTooltips function.
 * @param inLocation mid-point of the tooltip in global coordinates
 * @param inText text to display as a tooltip
 */
void WidgetControlControlListener::ShowTooltips(CPt inLocation, const Q3DStudio::CString &inText)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    m_Parent->ShowTooltips(inLocation, inText);
#endif
}

//=============================================================================
/**
 * Pass-thru to the CWndControl's HideTooltips function.
 */
void WidgetControlControlListener::HideTooltips()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    m_Parent->HideTooltips();
#endif
}

//=============================================================================
/**
 * Pass-thru to the CWndControl's ShowMoveableTooltips function.
 * @param inLocation mid-point of the tooltip in global coordinates
 * @param inText text to display as a tooltip
 */
void WidgetControlControlListener::ShowMoveableWindow(CPt inLocation, const Q3DStudio::CString &inText,
                                                    CRct inBoundingRct)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    m_Parent->ShowMoveableWindow(inLocation, inText, inBoundingRct);
#endif
}

//=============================================================================
/**
 * Pass-thru to the CWndControl's HideMoveableTooltips function.
 */
void WidgetControlControlListener::HideMoveableWindow()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    m_Parent->HideMoveableWindow();
#endif
}

void WidgetControlControlListener::DoStartDrag(IDragable *inDragable)
{
    m_Parent->DoStartDrag(inDragable);
}

//===============================================================================
/**
* performs a drag operation on a file
*/
void WidgetControlControlListener::DoStartDrag(std::vector<Q3DStudio::CString> &inDragFileNameList)
{
    m_Parent->DoStartDrag(inDragFileNameList);
}




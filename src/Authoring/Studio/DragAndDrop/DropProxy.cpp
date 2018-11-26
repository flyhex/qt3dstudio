/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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

#include "Qt3DSCommonPrecompile.h"
#include "HotKeys.h"
#include "DropProxy.h"
#include "DropContainer.h"
#include "DropSource.h"
#include "Qt3DSFile.h"
#include "Qt3DSString.h"
#include "IDragable.h"

#include <QtGui/qevent.h>
#include <QtWidgets/qwidget.h>

//=============================================================================
/**
 * Constructor
 * @param inParent gesture listener that messages will be passed to
 */
CDropProxy::CDropProxy(CDropContainer *inParent)
{
    m_Parent = inParent;
}

//=============================================================================
/**
 * Destructor
 */
CDropProxy::~CDropProxy()
{
}

//=============================================================================
/**
 * Called when a drag begins over a window.
 */
void CDropProxy::dragEnterEvent(QDragEnterEvent *event)
{
    event->ignore();
    if (GetDragItemCount(event->mimeData())) {
        CDropContainer::TFlavorItr theItr = m_Parent->GetFlavorBegin();
        CDropContainer::TFlavorItr theEnd = m_Parent->GetFlavorEnd();

        for (; theItr != theEnd; ++theItr) {
            if (HasMainFlavor(event->mimeData(), *theItr)) {
                event->accept();
                event->setDropAction(Qt::CopyAction);
                break;
            }
        }
    }
}

//===============================================================================
/**
 * 	This will extract the Data from the OLEDropObject and convert it to a CDropSource.
 *	@param inDataObject the Win specific dropSource.
 *	@param inFlavor the Flavor we need.
 *	@param inItem the position of the data in the DataObject.
 *	@return Newly extracted or created DropSource.
 *	Note: if we created it we need to make sure we destroy this object.
 */
CDropSource *CDropProxy::GetDropSource(const QMimeData *inDataObject, long inFlavor, long inItem)
{
    const CDropSource *theDropSource = nullptr;
    switch (inFlavor) {
    case QT3DS_FLAVOR_FILE: {
        // Check if we have a Drop File on our hands.
        if (inDataObject->hasUrls()) {
            // Get the number of files being dragged
            short theFileCount = inDataObject->urls().count();

            // Only allow single files to be dragged
            if (theFileCount > inItem && !inDataObject->urls().isEmpty()) {
                // Get the filename of the object being dragged
                QString theFilename = inDataObject->urls().at(inItem).toLocalFile();

                Qt3DSFile theFile(theFilename);
                // Dragging a single file
                theDropSource =
                    CDropSourceFactory::Create(QT3DS_FLAVOR_FILE, &theFile, sizeof(Qt3DSFile *));
            }
        }
    } break;
    case QT3DS_FLAVOR_TEXT:
    // Don't do anything for this
    case QT3DS_FLAVOR_BASIC_OBJECTS:
    // Don't do anything for this
    case QT3DS_FLAVOR_ASSET_LIB:
    // make an asset out of this
    case QT3DS_FLAVOR_ASSET_TL:
    // make an asset out of this
    case QT3DS_FLAVOR_ASSET_UICFILE:
        // make an asset out of this
        // Get a pointer to the object
        theDropSource = dynamic_cast<const CDropSource *>(inDataObject);
        if (theDropSource != nullptr && theDropSource->GetFlavor() != inFlavor)
            theDropSource = nullptr;
        break;
    }
    return const_cast<CDropSource *>(theDropSource);
}

//===============================================================================
/**
 * 	This is to count the number of objects in the DataObject.
 *	@param inDataObject the WinSpecific dropobject
 *	@return  The number of items found.
 */
long CDropProxy::GetDragItemCount(const QMimeData *inDataObject)
{
    long theCount = 0;

    // Check if we have a Drop File on our hands.
    if (inDataObject->hasUrls()) {
        // Get the number of files being dragged
        theCount = inDataObject->urls().count();
    } else {
        auto source = dynamic_cast<const CDropSource *>(inDataObject);
        theCount = source != nullptr ? 1 : 0;
    }

    return theCount;
}
//===============================================================================
/**
 * 	This will check the DataObject for the Flavor.
 *	@param inDataObject the Win specific data object.
 *	@param inFlavor the requested Flavor.
 *	@param true if the DataObject contains the flavor.
 */
bool CDropProxy::HasMainFlavor(const QMimeData *inDataObject, long inFlavor)
{
    if (inFlavor == QT3DS_FLAVOR_FILE)
        return inDataObject->hasUrls();
    auto source = dynamic_cast<const CDropSource *>(inDataObject);
    return source != nullptr && source->GetFlavor() == inFlavor;
}

//=============================================================================
/**
 * Called when a drag event leaves the window.  May cause the drag to end if
 * the mouse button is no longer down.
 */
void CDropProxy::dragLeaveEvent(QDragLeaveEvent *event)
{
    if (m_Parent) {
        m_Parent->OnDragLeave();
        event->accept();
    }
}

//=============================================================================
/**
 * Called when an item is being dragged over this window.  Passes the message
 * on to the gesture listener.
 */
void CDropProxy::dragMoveEvent(QDragMoveEvent *event)
{
    bool theAcceptFlag = false;

    if (m_Parent) {
        long theCount = GetDragItemCount(event->mimeData());
        for (long theIndex = 0; theIndex < theCount; ++theIndex) {
            Qt::KeyboardModifiers theModifyerFlags = ReflectMouse(event->pos().x(), event->pos().y());

            CDropContainer::TFlavorItr theItr = m_Parent->GetFlavorBegin();
            CDropContainer::TFlavorItr theEnd = m_Parent->GetFlavorEnd();

            for (; theItr != theEnd; ++theItr) {
                // Find the Flavor of this Item.
                if (HasMainFlavor(event->mimeData(), *theItr)) {
                    CDropSource *theDropSource = GetDropSource(event->mimeData(), *theItr, theIndex);
                    if (theDropSource) {
                        CPt thePoint(event->pos());
                        theModifyerFlags = CHotKeys::GetCurrentKeyModifiers();
                        theDropSource->SetCurrentPoint(thePoint);
                        theDropSource->SetCurrentFlags(theModifyerFlags);

                        theDropSource->InterpretKeyFlags(theModifyerFlags);
                        // This will be implemented in the cross platform code.
                        theAcceptFlag = m_Parent->OnDragWithin(*theDropSource);
                        event->setAccepted(theAcceptFlag);
                        if (theAcceptFlag) {
                            if (theDropSource->CanCopy())
                                event->setDropAction(Qt::CopyAction);
                            else
                                event->setDropAction(Qt::MoveAction);
                            // Breakout of the outer loop.
                            theIndex = theCount;
                        }

                        // delete the drop source if it was a file
                        if (QT3DS_FLAVOR_FILE == theDropSource->GetFlavor()) {
                            delete theDropSource;
                        }
                        break;
                    }
                }
            }
        }
    }
}

//=============================================================================
/**
 * Called when an OLE item is dropped on this window.
 * @return TRUE if the drop was valid, otherwise FALSE
 */
void CDropProxy::dropEvent(QDropEvent *event)
{
    if (m_Parent) {
        long theCount = GetDragItemCount(event->mimeData());
        for (long theIndex = 0; theIndex < theCount; ++theIndex) {
            Qt::KeyboardModifiers theModifyerFlags = ReflectMouse(event->pos().x(), event->pos().y());

            theModifyerFlags = CHotKeys::GetCurrentKeyModifiers();

            CDropContainer::TFlavorItr theItr = m_Parent->GetFlavorBegin();
            CDropContainer::TFlavorItr theEnd = m_Parent->GetFlavorEnd();

            for (; theItr != theEnd; ++theItr) {
                // Find the Flavor of this Item.
                if (HasMainFlavor(event->mimeData(), *theItr)) {
                    // This will convert all stuff into a DropSource
                    CDropSource *theDropSource = GetDropSource(event->mimeData(), *theItr, theIndex);

                    // This will be implemented in the cross platform code.
                    if (theDropSource) {
                        CPt thePoint(event->pos());
                        theDropSource->SetCurrentPoint(thePoint);
                        theDropSource->SetCurrentFlags(theModifyerFlags);

                        // Don't call the recieve if we did not have a valid droptarget to begin
                        // with.
                        if (theDropSource->GetHasValidTarget()) {
                            theDropSource->InterpretKeyFlags(theModifyerFlags);
                            // This will be implemented in the cross platform code.
                            m_Parent->OnDragReceive(*theDropSource);
                        }
                    }

                    // Don't delete the drop source here, the creator will destroy it
                    // delete theDropSource;
                }
            }
        }
    }

    event->accept();
}
//===============================================================================
/**
 *	This function is used to report back to the container where the mouse is during a drag.
 *	The container can send this to children as MouseOvers so the children can do mouseover
 *things.
 *	@param inX the X position
 * 	@param inY the Y position.
 * @return the Key modifyers.
 */
Qt::KeyboardModifiers CDropProxy::ReflectMouse(long inX, long inY)
{
    Qt::KeyboardModifiers theModifierFlags = Qt::NoModifier;

    if (m_Parent) {
        // Get the mouse stuff
        CPt theMouseLoc(inX, inY);

        // Determine which modifier keys are down so that we can pass them along

        theModifierFlags = CHotKeys::GetCurrentKeyModifiers();

        // Pass the gesture into the studio control
        m_Parent->OnReflectMouse(theMouseLoc, theModifierFlags);
    }
    return theModifierFlags;
}

void CDropProxy::Register(QWidget *widget)
{
    widget->installEventFilter(this);
    widget->setAcceptDrops(true);
}

bool CDropProxy::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    switch (event->type()) {
    case QEvent::DragEnter:
        dragEnterEvent(static_cast<QDragEnterEvent *>(event));
        return event->isAccepted();
    case QEvent::DragLeave:
        dragLeaveEvent(static_cast<QDragLeaveEvent *>(event));
        return event->isAccepted();
    case QEvent::DragMove:
        dragMoveEvent(static_cast<QDragMoveEvent *>(event));
        return event->isAccepted();
    case QEvent::Drop:
        dropEvent(static_cast<QDropEvent *>(event));
        return event->isAccepted();
    default:
        return false;
    }
}

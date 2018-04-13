/****************************************************************************
**
** Copyright (C) 1999-2005 Anark Corporation.
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

//==============================================================================
//	Prefix
//==============================================================================
#ifndef INCLUDED_IDOC_H
#define INCLUDED_IDOC_H

#pragma once

//==============================================================================
//	Includes
//==============================================================================
#include "Qt3DSDMDataTypes.h"
#include "Qt3DSDMValue.h"
#include "SelectedValueImpl.h"

//==============================================================================
//	Forwards
//==============================================================================
class CAsset;
class IKeyframesManager;
class ISelectable;
class CCore;
class TimelineWidget;

namespace qt3ds {
namespace foundation {
    class IInStream;
}
}
namespace qt3dsdm {
class Qt3DSDMInstanceHandle;
class Qt3DSDMSlideHandle;
class CStudioSystem;
class IPropertySystem;
class IAnimationCore;
class IDOMWriter;
class IDOMReader;
class ISignalConnection;
}

namespace Q3DStudio {
class IDocumentEditor;
class IDocumentReader;
class IDocumentBufferCache;
class CString;
class IComposerSerializer;
}

//==============================================================================
/**
 *	Doc
 */
class IDoc
{
public:
    virtual long GetCurrentViewTime() const = 0;
    virtual void NotifyTimeChanged(long inNewTime) = 0;
    virtual void NotifyActiveSlideChanged(qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide) = 0;
    virtual void NotifyActiveSlideChanged(qt3dsdm::Qt3DSDMSlideHandle inNewActiveSlide,
                                          bool inForceRefresh,
                                          bool inIgnoreLastDisplayTime = false) = 0;
    virtual void NotifySelectionChanged(
        Q3DStudio::SSelectedValue inNewSelection = Q3DStudio::SSelectedValue()) = 0;

    virtual qt3dsdm::CStudioSystem *GetStudioSystem() = 0;

    virtual void SetKeyframeInterpolation() = 0;
    virtual void DeselectAllKeyframes() = 0;

    virtual void SetModifiedFlag(bool inIsModified = true) = 0;

    virtual void SetKeyframesManager(IKeyframesManager *inManager) = 0;
    virtual IKeyframesManager *GetKeyframesManager() = 0;

    virtual qt3dsdm::IPropertySystem *GetPropertySystem() = 0;
    virtual qt3dsdm::IAnimationCore *GetAnimationCore() = 0;
    virtual void SetInstancePropertyValue(qt3dsdm::Qt3DSDMInstanceHandle inInstance,
                                          const std::wstring &inPropertyName,
                                          const qt3dsdm::SValue &inValue) = 0;
    virtual void SetInstancePropertyControlled(qt3dsdm::Qt3DSDMInstanceHandle instance,
                                               Q3DStudio::CString instancepath,
                                               qt3dsdm::Qt3DSDMPropertyHandle propName,
                                               Q3DStudio::CString controller,
                                               bool controlled) = 0;
    // Return an editor to editing the scene graph of the document.
    // This editor takes care of putting objects into the property slide
    // as well as updating the world
    virtual Q3DStudio::IDocumentBufferCache &GetBufferCache() = 0;

    // Get an object responsible for reading the document
    virtual Q3DStudio::IDocumentReader &GetDocumentReader() = 0;
    // Open a transaction, change the model however you want to
    // then close the transaction and a command will be generated which will
    // automatically do most of the work of doing/undoing changes
    // to the data model (and the graphs, and the selection state).
    virtual Q3DStudio::IDocumentEditor &OpenTransaction(const Q3DStudio::CString &cmdName,
                                                        const char *inFile, int inLine) = 0;

    // Get the document editor if a transaction has already been opened.
    // Else open a new transaction.
    virtual Q3DStudio::IDocumentEditor &MaybeOpenTransaction(const Q3DStudio::CString &cmdName,
                                                             const char *inFile, int inLine) = 0;
    virtual bool IsTransactionOpened() const = 0;
    // Undo whatever has been done and clear the transaction's
    // internal data.  Leave the transaction open and on the stack, however.
    virtual void RollbackTransaction() = 0;
    virtual void CloseTransaction() = 0;

    // This should only be called from a global exception handler or something like that.
    // This will close the transaction regardless of the open count.
    // inAsynchronousCommit will in effect commit the command during a post-message callback.
    // This is usually what you want because it will protect studio against crashes due to the
    // command reconfiguring the UI.  There are cases, however, when you really want the command to
    // be
    // committed right at this very moment (like just before undo).
    virtual void IKnowWhatIAmDoingForceCloseTransaction() = 0;

    virtual std::shared_ptr<Q3DStudio::IComposerSerializer> CreateSerializer() = 0;
    // Create a DOM writer that is opened to the project element.  This is where the serializer
    // should write to.
    virtual std::shared_ptr<qt3dsdm::IDOMWriter> CreateDOMWriter() = 0;
    // Create a DOM reader and check that the top element's version is correct.  Opens the reader
    // to the project element.
    virtual std::shared_ptr<qt3dsdm::IDOMReader>
    CreateDOMReader(const Q3DStudio::CString &inFilePath, qt3ds::QT3DSI32 &outVersion) = 0;

    virtual CCore *GetCore() = 0;
};

#define OPEN_TRANSACTION(name) OpenTransaction(name, __FILE__, __LINE__)

#endif // INCLUDED_IDOC_H

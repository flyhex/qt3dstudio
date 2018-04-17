/****************************************************************************
**
** Copyright (C) 2013 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#pragma once
#ifndef QT3DS_STATE_VISUAL_BINDING_CONTEXT_H
#define QT3DS_STATE_VISUAL_BINDING_CONTEXT_H
#include "Qt3DSState.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/StringTable.h"
#include "Qt3DSStateVisualBindingContextCommands.h"

namespace qt3ds {
namespace foundation {
    class IDOMReader;
    class CStrTableOrDataRef;
}
}

namespace qt3ds {
namespace state {

    struct SVisualStateCommand;
    struct SSetAttribute;

    // Entity responsible for implementing the various visual state commands.
    class IVisualStateCommandHandler : public NVRefCounted
    {
    protected:
        virtual ~IVisualStateCommandHandler() {}
    public:
        virtual void Handle(const SVisualStateCommand &inCommand,
                            IScriptContext &inScriptContext) = 0;
    };

    class IVisualStateInterpreterFactory : public NVRefCounted
    {
    protected:
        virtual ~IVisualStateInterpreterFactory() {}
    };

    // It is important that the visual state context list the elements it expects to find in a uip
    // file
    // so that during parse of the uip file, we can generate an error if some element isn't found.
    struct SElementReference
    {
        CRegisteredString m_ElementPath;
        CRegisteredString m_Attribute;
        SElementReference(CRegisteredString elemPath = CRegisteredString(),
                          CRegisteredString att = CRegisteredString())
            : m_ElementPath(elemPath)
            , m_Attribute(att)
        {
        }
    };

    class IVisualStateContext : public NVRefCounted
    {
    protected:
        virtual ~IVisualStateContext() {}
    public:
        // All machines are loaded execute is called on the first update call.
        // made after LoadVisualStateMapping
        virtual void LoadStateMachine(const char8_t *id, const char8_t *inRelativePath,
                                      const char8_t *inDatamodelFunction) = 0;
        virtual void LoadVisualStateMapping(IDOMReader &inReader) = 0;
        // We have to pre-parse the xml so we can reference everything in the various presentations
        // We run this pass, then during uip file parsing we output errors if things don't match up.
        virtual NVConstDataRef<SElementReference> PreParseDocument(IDOMReader &inReader) = 0;
        virtual void SetCommandHandler(IVisualStateCommandHandler *inHandler) = 0;
        virtual void SetInterpreterFactory(IVisualStateInterpreterFactory *inHandler) = 0;

        // Initialize the state machines.  Machines are initialized in order of LoadStateMachine
        // calls.
        virtual void Initialize() = 0;
        virtual void Start() = 0;

        // Initialize the state machines.  Machines are updated in order of LoadStateMachine calls.
        virtual void Update() = 0;

        // Save out to a format that allows very rapid loading.
        virtual void BinarySave(IOutStream &stream) = 0;
        virtual void BinaryLoad(IInStream &stream, NVDataRef<QT3DSU8> inStringTableData) = 0;

        static IVisualStateContext &Create(NVFoundationBase &inFoundation,
                                           IStringTable &inStrTable);
    };
}
}

#endif

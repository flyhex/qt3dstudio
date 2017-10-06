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
#ifndef UIC_STATE_EDITOR_H
#define UIC_STATE_EDITOR_H
#pragma once
#include "UICState.h"
#include "UICStateSignalConnection.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/TaggedPointer.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "foundation/Qt3DSVec2.h"
#include "foundation/Qt3DSVec3.h"
#include "UICStateInterpreter.h"

namespace uic {
namespace state {
    namespace editor {

        class IEditorObject;
        typedef NVScopedRefCounted<IEditorObject> TObjPtr;
        typedef eastl::vector<TObjPtr> TObjList;

        class IEditorChangeListener
        {
        protected:
            virtual ~IEditorChangeListener() {}
        public:
            // The same object may be represented multiple times in this list.
            virtual void OnDataChange(const TObjList &inChangedObjects,
                                      const TObjList &inRemovedObjects) = 0;
        };

        struct EditorPropertyTypes
        {
            enum Enum {
                NoProperty = 0,
                Event, // string
                Id, // string
                Object, // IEditorObject
                ObjectList, // IEditorObject list
                // A string, but one from a set of strings.
                StringSet, // string
                String, // string, single line
                BigString, // string than may be multiple lines long
                Position, // vec2
                Dimension, // vec2
                PositionList, // vector<QT3DSVec2>
                Script, // string
                Color,
                Boolean,
                U32,
            };
        };

        struct SPropertyDeclaration
        {
            CRegisteredString m_Name;
            EditorPropertyTypes::Enum m_Type;
            // for enumerations, the list of enumerations
            SPropertyDeclaration() {}
            SPropertyDeclaration(CRegisteredString inName, EditorPropertyTypes::Enum inType)
                : m_Name(inName)
                , m_Type(inType)
            {
            }
        };

        typedef eastl::vector<SPropertyDeclaration> TPropertyDeclarationList;

        struct SPropertyDecNameFinder
        {
            const char8_t *m_NameStr;
            SPropertyDecNameFinder(const char8_t *nmStr)
                : m_NameStr(nmStr)
            {
            }
            // Implemented in UICStateEditor.cpp to include "foundation/Utils.h"
            bool operator()(const SPropertyDeclaration &dec) const;
        };

        typedef eastl::vector<QT3DSVec2> TVec2List;
        typedef eastl::string TEditorStr;
        typedef eastl::vector<TEditorStr> TEditorStrList;

        // Our generic discriminated union type for all property values.
        struct SValue;
        struct SValueOpt;

        class IEditor;

        class IEditorObject : public NVRefCounted
        {
        protected:
            virtual ~IEditorObject() {}
            const char8_t *m_TypeName;

        public:
            IEditorObject(const char8_t *tn)
                : m_TypeName(tn)
            {
            }
            const char8_t *TypeName() const { return m_TypeName; }
            // implemented in UICStateEditor.cpp using getpropertyvalue.
            virtual TEditorStr GetId() = 0;
            virtual TEditorStr GetDescription() = 0;
            virtual void SetId(const TEditorStr &inId) = 0;
            virtual void SetDescription(const TEditorStr &inName) = 0;
            // Things only have one canonical parent.
            virtual TObjPtr Parent() = 0;

            virtual void GetProperties(eastl::vector<SPropertyDeclaration> &outProperties) = 0;
            virtual Option<SPropertyDeclaration> FindProperty(const char8_t *propName) = 0;
            // The legal values for a property tend to be something that have to be calculated
            // dynamically.
            virtual eastl::vector<CRegisteredString> GetLegalValues(const char8_t *propName) = 0;

            virtual Option<SValue> GetPropertyValue(const char8_t *inPropName) = 0;
            virtual void SetPropertyValue(const char8_t *inPropName, const SValueOpt &inValue) = 0;
            virtual bool endEdit() { return false; } // should SetPropertyValue end property edit

            // Utility function implemented in UICStateEditor.cpp
            virtual void Append(const char8_t *inPropName, TObjPtr inObj);
            virtual void Remove(const char8_t *inPropName, TObjPtr inObj);

            // Remove this from any parents.  Remove any transitions pointing to this.
            // Only works for state-node derived types.
            // Executable content, onEntry onExit and such will not work with this.
            // Also removes this item id from the id map.  This is what is used to delete objects.
            // Do not reattach yourself; callers should release all references to this object after
            // calling
            // this.
            virtual void RemoveObjectFromGraph() = 0;

            // Internal calls, don't call externally
            virtual void RemoveIdFromContext() = 0;
            virtual void AddIdToContext() = 0;

            // Most things do not have any executable content.
            virtual NVConstDataRef<InterpreterEventTypes::Enum> GetExecutableContentTypes()
            {
                return NVConstDataRef<InterpreterEventTypes::Enum>();
            }

            // Be aware that there are more types of executable content than indicated in the store;
            // a lot more.
            // So just ignore the types the story ignores.
            virtual TObjList GetExecutableContent(InterpreterEventTypes::Enum /*inType*/)
            {
                return TObjList();
            }

            // inName can be an executable content:
            //'script', 'send', 'cancel'
            virtual TObjPtr CreateAndAppendExecutableContent(InterpreterEventTypes::Enum /*inType*/,
                                                             const char8_t * /*inName*/)
            {
                return TObjPtr();
            }

            virtual IEditor &GetEditor() = 0;
        };

        class ITransaction : public NVRefCounted
        {
        protected:
            virtual ~ITransaction() {}
            TEditorStr m_Name;

        public:
            ITransaction(const TEditorStr inName = TEditorStr())
                : m_Name(inName)
            {
            }
            TEditorStr GetName() const { return m_Name; }
            // Send the signals collected during the creation of this transaction
            virtual void SendDoSignals() = 0;
            virtual TObjList GetEditedObjects() = 0;
            virtual void Do() = 0;
            virtual void Undo() = 0;
            virtual bool Empty() = 0;
        };

        typedef NVScopedRefCounted<ITransaction> TTransactionPtr;

        class ITransactionManager : public NVRefCounted
        {
        protected:
            virtual ~ITransactionManager() {}
        public:
            virtual TSignalConnectionPtr AddChangeListener(IEditorChangeListener &inListener) = 0;
            // Undo/redo is supported via a transparent transaction system.
            // calls are reentrant but last call will close the transaction object.
            // Any changes to any editor objects will go through this transaction when they happen.
            virtual TTransactionPtr BeginTransaction(const TEditorStr &inName = TEditorStr()) = 0;
            virtual TTransactionPtr GetOpenTransaction() = 0;
            virtual void RollbackTransaction() = 0;
            virtual void EndTransaction() = 0;
        };

        // Editor interface to a UICState dataset
        class IEditor : public ITransactionManager
        {
        protected:
            virtual ~IEditor() {}
        public:
            // Traditional editor interface.
            virtual TObjPtr GetRoot() = 0;
            // You can force the id to be a particular ID in some cases, this is useful for testing.
            // ID is ignored for objects that don't have an id.
            virtual TObjPtr Create(const char8_t *inTypeName, const char8_t *inId = 0) = 0;
            virtual TObjPtr GetOrCreate(SSCXML &inData) = 0;
            virtual TObjPtr GetOrCreate(SState &inData) = 0;
            virtual TObjPtr GetOrCreate(STransition &inData) = 0;
            virtual TObjPtr GetOrCreate(SParallel &inData) = 0;
            virtual TObjPtr GetOrCreate(SHistory &inData) = 0;
            virtual TObjPtr GetOrCreate(SFinal &inData) = 0;
            virtual TObjPtr GetOrCreate(SDataModel &inData) = 0;
            virtual TObjPtr GetOrCreate(SData &inData) = 0;
            virtual TObjPtr ToEditor(SStateNode &inItem) = 0;

            // Get a particular object by id.  Useful in testing scenarios.
            virtual TObjPtr GetObjectById(const char8_t *inId) = 0;

            // Get an editor, if it already has been created, for this piece of graph data.
            // Note that if it has not been created, this function couldn't possibly create it
            // due to lack of type information.
            virtual TObjPtr GetEditor(void *inGraphData) = 0;

            // Copy save a subgraph to a string.  Converts all top level positions to be relative to
            // mouse pos.
            virtual TEditorStr Copy(TObjList inObjects, const QT3DSVec2 &inMousePos) = 0;
            virtual bool CanPaste(TObjPtr inTarget) = 0;
            // Paste in, adding relative pos to all top level positions.
            virtual void Paste(const TEditorStr &inCopiedObjects, TObjPtr inTarget,
                               const QT3DSVec2 &inRelativePos) = 0;

            virtual TObjPtr GetLeastCommonAncestor(NVConstDataRef<TObjPtr> inObjects) = 0;

            // Returns the list of send ids that have a delay on them.
            virtual TObjList GetCancelableSendIds() = 0;

            // Return the set of events in use in the state machine.
            virtual TEditorStrList GetStateMachineEvents() = 0;

            // Change content from one type to another.
            virtual TObjPtr ChangeExecutableContentType(TObjPtr inContent,
                                                        const char8_t *newType) = 0;

            virtual TEditorStr ToXML(TObjPtr inContent) = 0;
            // Returns the error from parsing the xml or if everything went correctly replaces
            // inContent
            // with the result of parsing the xml and returns the new content.
            virtual eastl::pair<TEditorStr, TObjPtr> FromXML(TObjPtr inContent,
                                                             const TEditorStr &ioEditedXML) = 0;

            virtual bool Save(const char8_t *inFname) = 0;
            virtual void Save(IOutStream &inStream) = 0;

            // This must be called on history creation once it has been added to the proper parent.
            virtual void SetValidHistoryDefault(TObjPtr inHistoryNode) = 0;

            static IEditor &CreateEditor();

            static IEditor *CreateEditor(const char8_t *inFname);
            static IEditor *CreateEditor(const char8_t *inFname, IInStream &inStream);
        };

        typedef NVScopedRefCounted<IEditor> TEditorPtr;

        struct SEditorImplTransactionScope
        {
            ITransactionManager &m_Editor;
            TTransactionPtr m_Transaction;
            SEditorImplTransactionScope(ITransactionManager &inEditor)
                : m_Editor(inEditor)
                , m_Transaction(inEditor.BeginTransaction())
            {
            }
            ~SEditorImplTransactionScope() { m_Editor.EndTransaction(); }

            TTransactionPtr operator->() { return m_Transaction; }
        };
    }
}
}

#endif
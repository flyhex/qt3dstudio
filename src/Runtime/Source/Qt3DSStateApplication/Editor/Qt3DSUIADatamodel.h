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
#ifndef UIA_DATAMODEL_H
#define UIA_DATAMODEL_H
#include "Qt3DSState.h"
#include "UICStateEditor.h"
#include "Qt3DSMetadata.h"
#include "Qt3DSStateInterpreter.h"
#include "UICStateEditorFoundation.h"

namespace qt3ds {
namespace app {
    using namespace qt3ds::state;
    using namespace qt3ds::state::editor;

    typedef eastl::pair<Q3DStudio::ERuntimeDataModelDataType,
                        Q3DStudio::ERuntimeAdditionalMetaDataType>
        TDataType;

    struct SDatamodelValue;

    struct SAppElement;

    struct SPresentation
    {
        eastl::string m_Id;
        eastl::string m_SrcPath;
        eastl::string m_Author;
        eastl::string m_Company;
        QT3DSU32 m_Width;
        QT3DSU32 m_Height;
        SAppElement *m_Scene;
        SPresentation()
            : m_Width(800)
            , m_Height(480)
            , m_Scene(NULL)
        {
        }
    };

    class IStateMachineChangeListener
    {
    public:
        virtual ~IStateMachineChangeListener() {}
        virtual bool OnDeleteState(TObjPtr inObject) = 0;
        virtual bool OnReloadStateMachine(TEditorPtr inStateMachineEditor) = 0;
    };

    class IStateMachineEditorManager
    {
    public:
        virtual ~IStateMachineEditorManager() {}
        virtual TEditorPtr GetOrCreateEditor(const TEditorStr &inFullPath, bool *outLoadStatus) = 0;
        virtual void RegisterChangeListener(IStateMachineChangeListener &inListener) = 0;
        virtual void UnregisterChangeListener(IStateMachineChangeListener &inListener) = 0;
    };

    class IDatamodel : public ITransactionManager,
                       public IStateMachineEditorManager,
                       public IStateMachineChangeListener
    {
    protected:
        virtual ~IDatamodel() {}
    public:
        // Returns true if we had to create the file.  Users should always keep track of the
        // transaction stack also
        // this does not include that.
        virtual bool IsDirty() const = 0;
        // General queries of the dataset defined by the uia file and all uip files and scxml files
        // it includes.

        virtual TEditorStrList GetComponents() = 0;
        virtual TEditorStrList GetComponentSlides(const TEditorStr &inComponent) = 0;

        virtual TEditorStrList GetBehaviors() = 0;

        virtual Q3DStudio::THandlerList GetHandlers(const TEditorStr &inBehavior) = 0;

        virtual Q3DStudio::TVisualEventList GetVisualEvents(const TEditorStr &inElement) = 0;

        virtual TEditorStrList GetElements() = 0;

        virtual Q3DStudio::TAttOrArgList GetElementAttributes(const TEditorStr &inElement) = 0;

        // Necessary to share transaction info and trigger deletes of related information but not
        // going to get
        // done right now.
        // virtual TEditorPtr GetOrCreateEditor( const TEditorStr& inFullPath ) = 0;

        // The editor obj has a remove from graph function that really is delete
        virtual TObjList
        GetVisualStateExecutableContent(TObjPtr inObject,
                                        InterpreterEventTypes::Enum inEventType) = 0;
        // Type name is the element name, so set-attribute, goto-slide, or fire-event
        virtual TObjPtr AppendVisualStateExecutableContent(TObjPtr inObject,
                                                           InterpreterEventTypes::Enum inEventType,
                                                           const char8_t *inElementName) = 0;
        virtual TObjPtr ChangeVisualStateExecutableContentName(TObjPtr inContent,
                                                               const char8_t *inElementName) = 0;

        // Called when the source uia changes.
        virtual void RefreshFile() = 0;
        virtual void RefreshFromStream(qt3ds::foundation::IInStream &inStream) = 0;
        // Returns the path that was passed in on create.
        virtual TEditorStr GetFilePath() = 0;

        // Returns false if unable to save, ask users to check the file out.
        virtual bool Save() = 0;
        virtual bool Save(qt3ds::foundation::IOutStream &inStream) = 0;

        // The section below allows someone to build an initial scene graph.
        // Note that components have additional properties
        // Get a list of presentations found while parsing uia file.
        virtual eastl::vector<SPresentation> GetPresentations() = 0;
        virtual eastl::string GetElementType(SAppElement &elem) = 0;
        virtual eastl::string GetElementId(SAppElement &elem) = 0;
        virtual bool IsComponent(SAppElement &elem) = 0;
        virtual Q3DStudio::TAttOrArgList GetElementAttributes(SAppElement &elem) = 0;
        // These are found either on the slide or on the component depending on if you are working
        // in uip space or runtime space.
        virtual Q3DStudio::TAttOrArgList GetSlideAttributes() = 0;
        virtual eastl::vector<SDatamodelValue>
        GetElementAttributeInitialValues(SAppElement &elem) = 0;
        virtual eastl::vector<SAppElement *> GetElementChildren(SAppElement &elem) = 0;

        virtual eastl::string GetLastLoadingErrorString() = 0;

        virtual NVFoundationBase &GetFoundation() = 0;
        virtual IStringTable &GetStringTable() = 0;

        // inPath may either exist or not.  You can pass in a uip file as well as a uia file.
        // application dir is so we can find the meta data.
        // inStream provides a way to load from memory, it will be used if it's not NULL.
        static IDatamodel &Create(qt3ds::state::editor::TFoundationPtr inFoundation,
                                  const TEditorStr &inPath, const TEditorStr &inApplicationDir,
                                  IStateMachineEditorManager &inStateMachineEditorManager,
                                  IInStream *inStream = 0);
        static IDatamodel &Create(qt3ds::state::editor::TFoundationPtr inFoundation,
                                  const TEditorStr &inPath, const TEditorStr &inApplicationDir,
                                  IStateMachineEditorManager &inStateMachineEditorManager,
                                  IStringTable &inStringTable, IInStream *inStream = 0);
    };

    typedef NVScopedRefCounted<IDatamodel> TDatamodelPtr;
}
}
#endif

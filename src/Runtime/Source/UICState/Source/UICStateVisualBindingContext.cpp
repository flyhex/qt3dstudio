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
#include "UICStateVisualBindingContext.h"

#include "UICStateVisualBindingContextValues.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "EASTL/string.h"
#include "EASTL/hash_map.h"
#include "UICStateInterpreter.h"
#include "UICStateContext.h"
#include "foundation/XML.h"
#include "foundation/FileTools.h"
#include "foundation/Qt3DSInvasiveLinkedList.h"
#include "foundation/SerializationTypes.h"
#include "foundation/StringConversionImpl.h"

using namespace uic::state;

namespace {

struct SStateEventKey
{
    InterpreterEventTypes::Enum m_Event;
    CRegisteredString m_Id;
    SStateEventKey(InterpreterEventTypes::Enum inEvt, CRegisteredString inId)
        : m_Event(inEvt)
        , m_Id(inId)
    {
    }
    bool operator==(const SStateEventKey &inOther) const
    {
        return m_Event == inOther.m_Event && m_Id == inOther.m_Id;
    }
};
}
namespace eastl {
template <>
struct hash<SStateEventKey>
{
    size_t operator()(const SStateEventKey &inKey) const
    {
        return hash<int>()((int)inKey.m_Event)
            ^ hash<qt3ds::foundation::CRegisteredString>()(inKey.m_Id);
    }
};
}

namespace {
struct SVisualStateCommandNode
{
    SVisualStateCommand m_Command;
    SVisualStateCommandNode *m_NextSibling;
    SVisualStateCommandNode(const SVisualStateCommand &inCommand = SVisualStateCommand())
        : m_Command(inCommand)
        , m_NextSibling(NULL)
    {
    }
};

DEFINE_INVASIVE_SINGLE_LIST(VisualStateCommandNode);
IMPLEMENT_INVASIVE_SINGLE_LIST(VisualStateCommandNode, m_NextSibling);

typedef TVisualStateCommandNodeList TCommandList;

// Apparently eastl::hash_multimap isn't order preserving for items.
typedef eastl::hash_map<SStateEventKey, TCommandList, eastl::hash<SStateEventKey>,
                        eastl::equal_to<SStateEventKey>, ForwardingAllocator>
    TStateEventCommandMap;

struct SStateMachineSystem : public IStateInterpreterEventHandler, public NVRefCounted
{
    NVAllocatorCallback &m_Allocator;
    CRegisteredString m_Path;
    CRegisteredString m_Id;
    CRegisteredString m_DatamodelFunction;

    NVScopedRefCounted<IStringTable> m_StringTable;
    NVScopedRefCounted<IStateInterpreter> m_Interpreter;
    TStateEventCommandMap m_CommandMap;
    QT3DSI32 mRefCount;
    bool m_Error;
    bool m_Running;

    TSignalConnectionPtr m_InterpreterEventConnection;
    NVScopedRefCounted<IVisualStateCommandHandler> m_CommandHandler;
    NVScopedRefCounted<IVisualStateInterpreterFactory> m_InterpreterFactory;

    SStateMachineSystem(const char8_t *inPath, const char8_t *inId, const char8_t *inFunction,
                        IStringTable &inStrTable, NVAllocatorCallback &inAlloc)
        : m_Allocator(inAlloc)
        , m_Path(inStrTable.RegisterStr(inPath))
        , m_Id(inStrTable.RegisterStr(inId))
        , m_DatamodelFunction(inStrTable.RegisterStr(inFunction))
        , m_StringTable(inStrTable)
        , m_CommandMap(ForwardingAllocator(inAlloc, "SStateMachineSystem::m_CommandMap"))
        , mRefCount(0)
        , m_Error(false)
        , m_Running(false)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Allocator)

    void Initialize()
    {
        if (!m_Interpreter) {
            m_Running = false;
            if (m_Error == false && m_InterpreterFactory) {
                m_Interpreter = m_InterpreterFactory->OnNewStateMachine(
                    m_Path.c_str(), m_Id.c_str(), m_DatamodelFunction.c_str());
                if (!m_Interpreter) {
                    m_Error = true;
                } else {
                    IStateContext *theContext = m_Interpreter->GetStateContext();
                    if (theContext) {
                        // Validate all of our ids in our command map.
                        nvvector<CRegisteredString> invalidStates(
                            m_Allocator, "SStateMachineSystem::Initialize::invalidStates");
                        for (TStateEventCommandMap::iterator iter = m_CommandMap.begin(),
                                                             end = m_CommandMap.end();
                             iter != end; ++iter) {
                            CRegisteredString testId = iter->first.m_Id;
                            if (theContext->FindStateNode(testId) == NULL) {
                                nvvector<CRegisteredString>::iterator theFind =
                                    eastl::find(invalidStates.begin(), invalidStates.end(), testId);
                                if (theFind == invalidStates.end()) {
                                    qCCritical(INVALID_OPERATION, m_Path.c_str(), 0,
                                        "State \"%s\" is not a valid state", testId.c_str());
                                    invalidStates.push_back(testId);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void Start()
    {
        if (m_Error || m_Running || !m_Interpreter)
            return;
        m_Running = true;
        m_Interpreter->Start();
        // Event handler must be replaced after interpreter started
        m_InterpreterEventConnection = m_Interpreter->RegisterEventHandler(*this);
        // Run throw the initial configuration and fire any enter events.  This takes care of
        // initial states.
        NVConstDataRef<SStateNode *> initialConfig = m_Interpreter->GetConfiguration();
        for (QT3DSU32 nodeIdx = 0, nodeEnd = initialConfig.size(); nodeIdx < nodeEnd; ++nodeIdx) {
            SStateNode *theNode(initialConfig[nodeIdx]);
            OnInterpreterEvent(InterpreterEventTypes::StateEnter, theNode->m_Id);
        }
    }

    void Update()
    {
        if (m_Interpreter) {
            m_Interpreter->Execute();
        }
    }

    void OnInterpreterEvent(InterpreterEventTypes::Enum inEvent,
                                    CRegisteredString inEventId) override
    {
        if (m_CommandHandler) {
            CRegisteredString theId = m_StringTable->RegisterStr(inEventId.c_str());
            SStateEventKey theKey(inEvent, theId);
            TStateEventCommandMap::iterator theItem = m_CommandMap.find(theKey);
            if (theItem != m_CommandMap.end()) {
                for (TCommandList::iterator iter = theItem->second.begin(),
                                            end = theItem->second.end();
                     iter != end; ++iter)
                    m_CommandHandler->Handle(iter->m_Command, m_Interpreter->GetScriptContext());
            }
        }
    }
};

typedef eastl::vector<NVScopedRefCounted<SStateMachineSystem>> TStateMachineList;

struct SVisualStateContext : public IVisualStateContext
{
    NVFoundationBase &m_Foundation;
    SSAutoDeallocatorAllocator m_DataAllocator;
    QT3DSI32 mRefCount;
    TStateMachineList m_StateMachines;
    NVScopedRefCounted<IVisualStateCommandHandler> m_CommandHandler;
    NVScopedRefCounted<IVisualStateInterpreterFactory> m_InterpreterFactory;
    NVScopedRefCounted<IStringTable> m_StringTable;
    nvvector<SElementReference> m_PreparseResults;

    SVisualStateContext(NVFoundationBase &fnd, IStringTable &inStrTable)
        : m_Foundation(fnd)
        , m_DataAllocator(fnd)
        , mRefCount(0)
        , m_StringTable(inStrTable)
        , m_PreparseResults(fnd.getAllocator(), "SVisualStateContext::m_PreparseResults")
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void LoadStateMachine(const char8_t *id, const char8_t *inRelativePath,
                                  const char8_t *inDatamodelFunction) override
    {
        SStateMachineSystem *theSystem = QT3DS_NEW(m_Foundation.getAllocator(), SStateMachineSystem)(
            inRelativePath, id, inDatamodelFunction, *m_StringTable, m_Foundation.getAllocator());
        theSystem->m_CommandHandler = m_CommandHandler;
        theSystem->m_InterpreterFactory = m_InterpreterFactory;
        m_StateMachines.push_back(theSystem);
    }

    CRegisteredString ParseStrAtt(IDOMReader &inReader, const char8_t *attName)
    {
        const char8_t *temp = "";
        if (inReader.UnregisteredAtt(attName, temp)) {
            return m_StringTable->RegisterStr(temp);
        }
        return CRegisteredString();
    }

    void PreparseExecutableContent(IDOMReader &inReader)
    {
        for (bool setatt = inReader.MoveToFirstChild(); setatt;
             setatt = inReader.MoveToNextSibling()) {
            IDOMReader::Scope __commandScope(inReader);
            const char8_t *elemName = inReader.GetElementName();

            SElementReference theReference;
            if (AreEqual(elemName, "goto-slide")) {
                theReference = SElementReference(ParseStrAtt(inReader, "element"));
            } else if (AreEqual(elemName, "call")) {
                theReference = SElementReference(ParseStrAtt(inReader, "element"));
            } else if (AreEqual(elemName, "set-attribute")) {
                theReference = SElementReference(ParseStrAtt(inReader, "element"),
                                                 ParseStrAtt(inReader, "attribute"));
            } else if (AreEqual(elemName, "fire-event")) {
                theReference = SElementReference(ParseStrAtt(inReader, "element"));
            } else if (AreEqual(elemName, "set-presentation")) {
            } else {
                qCCritical(INVALID_PARAMETER, "Unrecognized child in an enter/exit node: %s", elemName);
            }
            if (theReference.m_ElementPath.IsValid()) {
                m_PreparseResults.push_back(theReference);
            }
        }
    }

    NVConstDataRef<SElementReference> PreParseDocument(IDOMReader &inReader) override
    {
        m_PreparseResults.clear();
        for (bool success = inReader.MoveToFirstChild("statemachine"); success;
             success = inReader.MoveToNextSibling("statemachine")) {
            IDOMReader::Scope __readerScope(inReader);
            if (inReader.MoveToFirstChild("visual-states")) {
                IDOMReader::Scope __bindingsScope(inReader);
                for (bool success = inReader.MoveToFirstChild(); success;
                     success = inReader.MoveToNextSibling()) {
                    IDOMReader::Scope __stateScope(inReader);
                    if (AreEqual(inReader.GetElementName().c_str(), "transition")) {
                        PreparseExecutableContent(inReader);
                    } else {
                        for (bool enterExitSuccess = inReader.MoveToFirstChild(); enterExitSuccess;
                             enterExitSuccess = inReader.MoveToNextSibling()) {
                            IDOMReader::Scope __enterExitScope(inReader);
                            PreparseExecutableContent(inReader);
                        }
                    }
                }
            }
        }
        return m_PreparseResults;
    }

    void ParseGotoSlideData(IDOMReader &inReader, SGotoSlideData &inData)
    {
        const char *tempData;
        if (inReader.UnregisteredAtt("direction", tempData) && AreEqual(tempData, "reverse"))
            inData.m_Reverse = true;

        if (inReader.UnregisteredAtt("mode", tempData)) {
            if (AreEqual(tempData, "stopatend")) {
                inData.m_Mode = SlidePlaybackModes::StopAtEnd;
                inData.m_PlaythroughTo = CRegisteredString();
            } else if (AreEqual(tempData, "looping"))
                inData.m_Mode = SlidePlaybackModes::Looping;
            else if (AreEqual(tempData, "pingpong"))
                inData.m_Mode = SlidePlaybackModes::PingPong;
            else if (AreEqual(tempData, "ping"))
                inData.m_Mode = SlidePlaybackModes::Ping;
            else if (AreEqual(tempData, "playthrough")) {
                if (!inReader.UnregisteredAtt("playthroughto", tempData) || isTrivial(tempData)) {
                    qCCritical(INVALID_OPERATION, "Goto slide command has playthough "
                        "mode but no playthroughto attribute; mode will be ignored");
                } else {
                    inData.m_PlaythroughTo = m_StringTable->RegisterStr(tempData);
                }

                if (inData.m_PlaythroughTo.hasValue())
                    inData.m_Mode = SlidePlaybackModes::StopAtEnd;
            }
        }

        if (inReader.UnregisteredAtt("state", tempData))
            inData.m_Paused = AreEqual(tempData, "pause");

        if (inReader.UnregisteredAtt("rate", tempData)) {
            QT3DSF32 temp = 1.0f;
            if (StringConversion<QT3DSF32>().StrTo(tempData, temp))
                inData.m_Rate = temp;
        }

        if (inReader.UnregisteredAtt("time", tempData)) {
            QT3DSF32 temp = 0.0f;
            if (StringConversion<QT3DSF32>().StrTo(tempData, temp))
                inData.m_StartTime = static_cast<QT3DSU32>(NVMax(0.0f, temp) * 1000.0f + .5f);
        }
    }

    void ParseExecutableContent(IDOMReader &inReader, CRegisteredString &inStateId,
                                InterpreterEventTypes::Enum inEvent,
                                TStateEventCommandMap &inCommandMap)
    {
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            IDOMReader::Scope __itemScope(inReader);
            const char8_t *elemName = inReader.GetElementName();
            SVisualStateCommand theCommand;
            if (AreEqual(elemName, "goto-slide")) {
                const char8_t *rel;
                if (inReader.UnregisteredAtt("rel", rel)) {
                    const char8_t *wrap;
                    inReader.UnregisteredAtt("wrap", wrap);
                    SGotoSlideRelative::Enum direction = SGotoSlideRelative::Error;
                    if (AreEqual(rel, "next"))
                        direction = SGotoSlideRelative::Next;
                    else if (AreEqual(rel, "previous"))
                        direction = SGotoSlideRelative::Previous;
                    else {
                        qCCritical(INVALID_OPERATION, "Goto slide relative has invalid "
                            "attribute (neither 'next' nor 'previous')");
                    }
                    bool doWrap = AreEqual(wrap, "true") ? true : false;

                    SGotoSlideRelative theCommandData(ParseStrAtt(inReader, "element"), direction,
                                                      doWrap);
                    ParseGotoSlideData(inReader, theCommandData.m_GotoSlideData);
                    theCommand = SVisualStateCommand(theCommandData);
                } else {
                    SGotoSlide theCommandData(ParseStrAtt(inReader, "element"),
                                              ParseStrAtt(inReader, "slide"));
                    ParseGotoSlideData(inReader, theCommandData.m_GotoSlideData);
                    theCommand = SVisualStateCommand(theCommandData);
                }
            } else if (AreEqual(elemName, "call")) {
                theCommand = SVisualStateCommand(SCallFunction(ParseStrAtt(inReader, "element"),
                                                               ParseStrAtt(inReader, "handler"),
                                                               ParseStrAtt(inReader, "arguments")));
            } else if (AreEqual(elemName, "set-attribute")) {
                theCommand = SVisualStateCommand(SSetAttribute(ParseStrAtt(inReader, "element"),
                                                               ParseStrAtt(inReader, "attribute"),
                                                               ParseStrAtt(inReader, "value")));
            } else if (AreEqual(elemName, "fire-event")) {
                theCommand = SVisualStateCommand(
                    SFireEvent(ParseStrAtt(inReader, "element"), ParseStrAtt(inReader, "event")));
            } else if (AreEqual(elemName, "set-presentation")) {
                theCommand = SVisualStateCommand(SPresentationAttribute(
                    ParseStrAtt(inReader, "ref"), ParseStrAtt(inReader, "attribute"),
                    ParseStrAtt(inReader, "value")));
            } else if (AreEqual(elemName, "play-sound")) {
                theCommand = SVisualStateCommand(SPlaySound(ParseStrAtt(inReader, "file")));
            } else {
                qCCritical(INVALID_PARAMETER, "Unrecognized child in an enter/exit node: %s", elemName);
            }
            if (theCommand.getType() != VisualStateCommandTypes::NoVisualStateCommand) {
                TCommandList &theList = inCommandMap
                                            .insert(eastl::make_pair(
                                                SStateEventKey(inEvent, inStateId), TCommandList()))
                                            .first->second;
                theList.push_back(*QT3DS_NEW(m_DataAllocator, SVisualStateCommandNode)(theCommand));
            }
        }
    }

    SStateMachineSystem *FindStateMachine(const char8_t *inId)
    {
        if (isTrivial(inId))
            return NULL;
        if (inId[0] == '#')
            ++inId;
        for (QT3DSU32 idx = 0, end = m_StateMachines.size(); idx < end; ++idx)
            if (AreEqual(m_StateMachines[idx].mPtr->m_Id.c_str(), inId))
                return m_StateMachines[idx].mPtr;
        return NULL;
    }

    // We have to be careful of string table values coming from the state machine and from the
    // reader
    // because we don't necessarily share the same string table.
    void LoadVisualStateMapping(IDOMReader &inReader) override
    {
        for (bool success = inReader.MoveToFirstChild("statemachine"); success;
             success = inReader.MoveToNextSibling("statemachine")) {
            IDOMReader::Scope __readerScope(inReader);
            const char8_t *machineId = "";
            if (inReader.UnregisteredAtt("ref", machineId)) {
                SStateMachineSystem *theSystem = FindStateMachine(machineId + 1);
                if (theSystem == NULL) {
                    qCCritical(INVALID_OPERATION, "Unknown state machine id: %s",
                        nonNull(machineId));
                    continue;
                }

                if (inReader.MoveToFirstChild("visual-states")) {
                    IDOMReader::Scope __bindingsScope(inReader);
                    for (bool bindingsSuccess = inReader.MoveToFirstChild(); bindingsSuccess;
                         bindingsSuccess = inReader.MoveToNextSibling()) {
                        IDOMReader::Scope __stateScope(inReader);
                        const char8_t *rawStateId = "";
                        inReader.UnregisteredAtt("ref", rawStateId);
                        if (isTrivial(rawStateId))
                            continue;

                        if (rawStateId[0] == '#')
                            ++rawStateId;
                        CRegisteredString elemName = m_StringTable->RegisterStr(rawStateId);

                        if (AreEqual(inReader.GetElementName().c_str(), "transition")) {
                            ParseExecutableContent(inReader, elemName,
                                                   InterpreterEventTypes::Transition,
                                                   theSystem->m_CommandMap);
                        } else {
                            for (bool stateSuccess = inReader.MoveToFirstChild(); stateSuccess;
                                 stateSuccess = inReader.MoveToNextSibling()) {
                                IDOMReader::Scope __enterExitScope(inReader);
                                const char8_t *stateChildName = inReader.GetElementName();
                                if (AreEqual(stateChildName, "enter")) {
                                    ParseExecutableContent(inReader, elemName,
                                                           InterpreterEventTypes::StateEnter,
                                                           theSystem->m_CommandMap);
                                } else if (AreEqual(stateChildName, "exit")) {
                                    ParseExecutableContent(inReader, elemName,
                                                           InterpreterEventTypes::StateExit,
                                                           theSystem->m_CommandMap);
                                } else {
                                    qCCritical(INVALID_PARAMETER,
                                        "Unrecognized child in a visual state bindings state: %s",
                                        stateChildName);
                                }
                            }
                        }
                    }
                }
            } else {
                qCCritical(INVALID_OPERATION, "visual-state element has no machine attribute");
            }
        }
    }

    void SetCommandHandler(IVisualStateCommandHandler *inHandler) override
    {
        m_CommandHandler = inHandler;
        for (QT3DSU32 idx = 0, end = m_StateMachines.size(); idx < end; ++idx)
            m_StateMachines[idx]->m_CommandHandler = inHandler;
    }

    void SetInterpreterFactory(IVisualStateInterpreterFactory *inHandler) override
    {
        m_InterpreterFactory = inHandler;
        for (QT3DSU32 idx = 0, end = m_StateMachines.size(); idx < end; ++idx)
            m_StateMachines[idx]->m_InterpreterFactory = inHandler;
    }

    void Initialize() override
    {
        for (QT3DSU32 idx = 0, end = m_StateMachines.size(); idx < end; ++idx)
            m_StateMachines[idx]->Initialize();
    }

    void Start() override
    {
        for (QT3DSU32 idx = 0, end = m_StateMachines.size(); idx < end; ++idx)
            m_StateMachines[idx]->Start();
    }

    void Update() override
    {
        for (QT3DSU32 idx = 0, end = m_StateMachines.size(); idx < end; ++idx)
            m_StateMachines[idx]->Update();
    }

    QT3DSU32 CountItems(TCommandList &list)
    {
        QT3DSU32 retval = 0;
        for (TCommandList::iterator iter = list.begin(), end = list.end(); iter != end; ++iter)
            ++retval;
        return retval;
    }

    struct SSaveVisitor
    {
        const SStrRemapMap &m_RemapMap;
        SSaveVisitor(const SStrRemapMap &map)
            : m_RemapMap(map)
        {
        }
        template <typename TItemType>
        SVisualStateCommand operator()(const TItemType &item)
        {
            TItemType newItem(item);
            newItem.Remap(*this);
            return newItem;
        }
        void Remap(CRegisteredString &inStr) { inStr.Remap(m_RemapMap); }
        SVisualStateCommand operator()() { return SVisualStateCommand(); }
    };

    void BinarySave(IOutStream &stream) override
    {
        qt3ds::foundation::SWriteBuffer theWriteBuffer(m_Foundation.getAllocator(),
                                                    "BinarySave::writebuffer");
        // Allocate space for overall size of the data section
        theWriteBuffer.writeZeros(4);
        theWriteBuffer.write((QT3DSU32)m_StateMachines.size());
        const SStrRemapMap &theRemapMap(m_StringTable->GetRemapMap());
        for (QT3DSU32 idx = 0, end = m_StateMachines.size(); idx < end; ++idx) {
            SStateMachineSystem &theSystem = *m_StateMachines[idx];
            CRegisteredString path(theSystem.m_Path);
            CRegisteredString id(theSystem.m_Id);
            CRegisteredString fn(theSystem.m_DatamodelFunction);
            path.Remap(theRemapMap);
            id.Remap(theRemapMap);
            fn.Remap(theRemapMap);
            theWriteBuffer.write(path);
            theWriteBuffer.write(id);
            theWriteBuffer.write(fn);
            theWriteBuffer.write((QT3DSU32)theSystem.m_CommandMap.size());
            for (TStateEventCommandMap::iterator iter = theSystem.m_CommandMap.begin(),
                                                 mapEnd = theSystem.m_CommandMap.end();
                 iter != mapEnd; ++iter) {
                theWriteBuffer.write((QT3DSU32)iter->first.m_Event);
                CRegisteredString stateId(iter->first.m_Id);
                stateId.Remap(theRemapMap);
                theWriteBuffer.write(stateId);

                theWriteBuffer.write(CountItems(iter->second));
                for (TCommandList::iterator cmdIter = iter->second.begin(),
                                            cmdEnd = iter->second.end();
                     cmdIter != cmdEnd; ++cmdIter) {
                    SVisualStateCommand remapped =
                        cmdIter->m_Command.visit<SVisualStateCommand>(SSaveVisitor(theRemapMap));
                    SVisualStateCommandNode theNode(remapped);
                    theWriteBuffer.write(theNode);
                }
            }
        }
        QT3DSU32 totalSize = theWriteBuffer.size();
        QT3DSU32 *data = (QT3DSU32 *)theWriteBuffer.begin();
        data[0] = totalSize - 4;
        stream.Write((QT3DSU8 *)data, totalSize);
    }

    struct SLoadVisitor
    {
        NVDataRef<QT3DSU8> m_RemapMap;
        SLoadVisitor(const NVDataRef<QT3DSU8> map)
            : m_RemapMap(map)
        {
        }
        template <typename TItemType>
        void operator()(TItemType &item)
        {
            item.Remap(*this);
        }

        void Remap(CRegisteredString &inStr) { inStr.Remap(m_RemapMap); }
        void operator()() {}
    };

    void BinaryLoad(IInStream &stream, NVDataRef<QT3DSU8> inStringTableData) override
    {
        QT3DSU32 length;
        stream.Read(length);
        QT3DSU8 *data = (QT3DSU8 *)m_DataAllocator.allocate(length, "Binaryload", __FILE__, __LINE__);
        stream.Read(data, length);

        SDataReader theReader(data, data + length);
        QT3DSU32 numMachines = theReader.LoadRef<QT3DSU32>();
        m_StateMachines.clear();
        m_StateMachines.reserve(numMachines);
        for (QT3DSU32 idx = 0, end = numMachines; idx < end; ++idx) {
            CRegisteredString path = theReader.LoadRef<CRegisteredString>();
            CRegisteredString id = theReader.LoadRef<CRegisteredString>();
            CRegisteredString fn = theReader.LoadRef<CRegisteredString>();
            path.Remap(inStringTableData);
            id.Remap(inStringTableData);
            fn.Remap(inStringTableData);
            LoadStateMachine(id, path, fn);
            SStateMachineSystem &theSystem(*m_StateMachines.back());
            QT3DSU32 mapSize = theReader.LoadRef<QT3DSU32>();
            for (QT3DSU32 mapIdx = 0; mapIdx < mapSize; ++mapIdx) {
                InterpreterEventTypes::Enum evt =
                    static_cast<InterpreterEventTypes::Enum>(theReader.LoadRef<QT3DSU32>());
                CRegisteredString stateId = theReader.LoadRef<CRegisteredString>();
                stateId.Remap(inStringTableData);
                QT3DSU32 numCommands = theReader.LoadRef<QT3DSU32>();
                TCommandList &theList =
                    theSystem.m_CommandMap
                        .insert(eastl::make_pair(SStateEventKey(evt, stateId), TCommandList()))
                        .first->second;
                for (QT3DSU32 cmdIdx = 0, cmdEnd = numCommands; cmdIdx < cmdEnd; ++cmdIdx) {
                    SVisualStateCommandNode *nextNode = theReader.Load<SVisualStateCommandNode>();
                    nextNode->m_Command.visit<void>(SLoadVisitor(inStringTableData));
                    theList.push_back(*nextNode);
                }
            }
        }
    }
};
}

IVisualStateContext &IVisualStateContext::Create(NVFoundationBase &inFoundation,
                                                 IStringTable &inStrTable)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SVisualStateContext)(inFoundation, inStrTable);
}

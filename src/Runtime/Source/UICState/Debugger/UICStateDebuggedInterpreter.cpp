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
#include "UICStateDebugger.h"
#include "UICStateDebuggerProtocol.h"
#include "EASTL/map.h"
#include "EASTL/set.h"
#include "foundation/Qt3DSAtomic.h"

using namespace uic::state;
using namespace uic::state::debugger;

namespace {

// Horrible name, I know.
struct SDebuggedInterpreter : public IDebuggedInterpreter
{
    typedef eastl::set<TDebugStr> TDebugStrSet;
    typedef eastl::vector<SBreakpoint> TBreakpointList;
    typedef eastl::map<TDebugStr, SDatamodelValue> TDataModelTable;
    typedef eastl::hash_map<SDatamodelTable *, TDataModelTable> TTableMap;
    typedef eastl::vector<STableEntry> TTableEntryList;
    // Null is the datamodel table.

    NVAllocatorCallback &m_Allocator;
    NVScopedRefCounted<IDebugOutStream> m_Stream;
    QT3DSI32 mRefCount;
    TEditorPtr m_Editor;
    TDebugStr m_Filename;
    SMessageSerializer m_Serializer;
    QT3DSI32 m_StreamId;
    TBreakpointList m_Breakpoints;
    TDebugStrList m_StrList;
    TDebugStrSet m_Configuration;
    bool m_BreakOnMicrostep;
    bool m_IsBroken;
    Option<SBreakpoint> m_BrokenBreakpoint;
    IDebuggerMasterListener &m_Listener;
    TTableMap m_DatamodelValues;
    mutable TTableEntryList m_TempEntries;
    bool m_MicrostepHasData;
    SMicrostepData m_MicrostepData;
    TDebugStr m_MicrostepEvent;
    TDebugStrList m_ConfigurationList;
    TDebugStrList m_PreviousConfiguration;

    SDebuggedInterpreter(NVAllocatorCallback &inAlloc, IDebugOutStream &inStream,
                         TEditorPtr inEditor, const TDebugStr &inFname, QT3DSI32 inStreamId,
                         NVConstDataRef<TDebugStr> inConfiguration,
                         IDebuggerMasterListener &inListener)
        : m_Allocator(inAlloc)
        , m_Stream(inStream)
        , mRefCount(0)
        , m_Editor(inEditor)
        , m_Filename(inFname)
        , m_StreamId(inStreamId)
        , m_BreakOnMicrostep(false)
        , m_IsBroken(false)
        , m_Listener(inListener)
        , m_MicrostepHasData(false)
    {
        m_ConfigurationList.assign(inConfiguration.begin(), inConfiguration.end());
        m_Configuration.insert(inConfiguration.begin(), inConfiguration.end());
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Allocator)

    // Get the editor that represents the state graph for this debugged interpreter.
    TEditorPtr GetEditor() override { return m_Editor; }
    TDebugStr GetFilename() const override { return m_Filename; }

    template <typename TMessageType>
    void SendMessage(const TMessageType &inMessage)
    {
        m_Serializer.Serialize(m_StreamId, inMessage, 0);
        m_Stream->Write(m_Serializer.ToRawMessage());
    }

    // Used when connecting to a live session.
    void SetBreakpoint(const SBreakpoint &inBreakpoint) override
    {
        TBreakpointList::iterator iter =
            eastl::find(m_Breakpoints.begin(), m_Breakpoints.end(), inBreakpoint);
        if (iter == m_Breakpoints.end()) {
            SendMessage(SSetBreakpoint(inBreakpoint));
            m_Breakpoints.push_back(inBreakpoint);
        }
    }

    void ClearBreakpoint(const SBreakpoint &inBreakpoint) override
    {
        TBreakpointList::iterator iter =
            eastl::find(m_Breakpoints.begin(), m_Breakpoints.end(), inBreakpoint);
        if (iter != m_Breakpoints.end()) {
            SendMessage(SClearBreakpoint(inBreakpoint));
            m_Breakpoints.erase(iter);
        }
    }

    NVConstDataRef<SBreakpoint> GetBreakpoints() override
    {
        return toDataRef(m_Breakpoints.data(), m_Breakpoints.size());
    }

    void ClearAllBreakpoints() override
    {
        SendMessage(SClearAllBreakpoints());
        m_Breakpoints.clear();
    }

    // break at the *end* of the microstep so you can see the data of the microstep.
    void SetBreakOnMicrostep(bool inBreak) override
    {
        if (m_BreakOnMicrostep != inBreak) {
            m_BreakOnMicrostep = inBreak;
            SendMessage(SBreakOnMicrostep(inBreak));
        }
    }
    bool IsBreakOnMicrostep() const override { return m_BreakOnMicrostep; }

    NVConstDataRef<STableEntry> TableToList(const TDataModelTable &inTable) const
    {
        m_TempEntries.resize(inTable.size());
        QT3DSU32 idx = 0;
        for (TDataModelTable::const_iterator iter = inTable.begin(), end = inTable.end();
             iter != end; ++iter, ++idx)
            m_TempEntries[idx] = STableEntry(iter->first, iter->second);
        return toConstDataRef(m_TempEntries.data(), m_TempEntries.size());
    }

    NVConstDataRef<STableEntry> GetTableValues(SDatamodelTable *inTable) const override
    {
        TTableMap::const_iterator iter = m_DatamodelValues.find(inTable);
        if (iter != m_DatamodelValues.end())
            return TableToList(iter->second);
        return NVConstDataRef<STableEntry>();
    }

    NVConstDataRef<TDebugStr> GetPreviousConfiguration() const override
    {
        return toConstDataRef(m_PreviousConfiguration.data(), m_PreviousConfiguration.size());
    }
    NVConstDataRef<TDebugStr> GetConfiguration() const override
    {
        return toConstDataRef(m_ConfigurationList.data(), m_ConfigurationList.size());
    }
    const TDebugStr &GetMicrostepEvent() const override { return m_MicrostepEvent; }
    NVConstDataRef<STransitionId> GetMicrostepTransitions() const override
    {
        return toConstDataRef(m_MicrostepData.m_Transitions.data(),
                              m_MicrostepData.m_Transitions.size());
    }
    NVConstDataRef<TDebugStr> GetMicrostepEnterStates() const override
    {
        return toConstDataRef(m_MicrostepData.m_EnterStates.data(),
                              m_MicrostepData.m_EnterStates.size());
    }
    NVConstDataRef<TDebugStr> GetMicrostepExitStates() const override
    {
        return toConstDataRef(m_MicrostepData.m_ExitStates.data(),
                              m_MicrostepData.m_ExitStates.size());
    }
    void Continue() override
    {
        SendMessage(SContinue());
        m_IsBroken = false;
        m_BrokenBreakpoint = Option<SBreakpoint>();
    }

    void Disconnect() override
    {
        ClearAllBreakpoints();
        SetBreakOnMicrostep(false);
        if (m_IsBroken == true)
            Continue();

        SendMessage(SDisconnect());
    }

    void BreakpointHit(const SBreakpoint &inBreakpoint) override
    {
        m_BrokenBreakpoint = inBreakpoint;
        m_IsBroken = true;
        m_Listener.OnInterpreterBroken(*this, inBreakpoint);
    }

    void OnEventQueued(const SEventQueued &inMsg) override
    {
        m_Listener.OnEventQueued(inMsg.m_Event, inMsg.m_Internal);
    }

    void OnBeginStep(const SBeginStep &) override { m_Listener.OnBeginStep(*this); }

    void OnBeginMicrostep(const SBeginMicrostep &) override
    {
        m_MicrostepEvent.clear();
        m_MicrostepData.m_Transitions.clear();
        m_MicrostepData.m_EnterStates.clear();
        m_MicrostepData.m_ExitStates.clear();
        m_MicrostepHasData = false;
        m_Listener.OnBeginMicrostep(*this);
    }

    void OnMicrostepEvent(const SMicrostepEvent &inMsg) override
    {
        m_MicrostepEvent = inMsg.m_Event;
    }

    void OnMicrostepData(const SMicrostepData &inMsg) override
    {
        m_MicrostepData = inMsg;
        m_MicrostepHasData = true;
    }
    void OnEndMicrostep(const SEndMicrostep & /*inMsg*/) override
    {
        if (m_MicrostepHasData == false && m_MicrostepEvent.empty() == false) {
            m_Listener.OnEventProcessed(m_MicrostepEvent, false);
        } else {
            m_PreviousConfiguration = m_ConfigurationList;

            for (TDebugStrList::const_iterator iter = m_MicrostepData.m_ExitStates.begin(),
                                               end = m_MicrostepData.m_ExitStates.end();
                 iter != end; ++iter)
                m_Configuration.erase(*iter);

            m_Configuration.insert(m_MicrostepData.m_EnterStates.begin(),
                                   m_MicrostepData.m_EnterStates.end());

            m_ConfigurationList.clear();
            m_ConfigurationList.insert(m_ConfigurationList.end(), m_Configuration.begin(),
                                       m_Configuration.end());
            if (m_BreakOnMicrostep) {
                m_IsBroken = true;
                m_Listener.OnInterpreterBroken(*this, Option<SBreakpoint>());
            } else
                m_Listener.OnMicrostep(*this);
        }
    }

    void OnModifyTable(const SModifyTableValues &inValues) override
    {
        TTableMap::iterator iter =
            m_DatamodelValues.insert(eastl::make_pair(inValues.m_TablePtr, TDataModelTable()))
                .first;
        for (QT3DSU32 idx = 0, end = inValues.m_Modifications.size(); idx < end; ++idx) {
            const STableModification &theMod(inValues.m_Modifications[idx]);
            switch (theMod.m_Type) {
            case TableModificationType::RemoveKey:
                iter->second.erase(theMod.m_Entry.m_Key);
                break;
            case TableModificationType::SetKey:
                (iter->second)[theMod.m_Entry.m_Key] = theMod.m_Entry.m_Value;
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
        m_Listener.OnDatamodelChange(
            *this, inValues.m_TablePtr,
            toConstDataRef(inValues.m_Modifications.data(), inValues.m_Modifications.size()));
    }

    bool IsBroken() const override { return m_IsBroken; }
};
}

IDebuggedInterpreter &IDebuggedInterpreter::Create(NVAllocatorCallback &inAlloc,
                                                   IDebugOutStream &inStream, TEditorPtr inEditor,
                                                   const TDebugStr &inFilename, QT3DSI32 inStreamId,
                                                   NVConstDataRef<TDebugStr> inConfiguration,
                                                   IDebuggerMasterListener &inListener)
{
    return *QT3DS_NEW(inAlloc, SDebuggedInterpreter)(inAlloc, inStream, inEditor, inFilename,
                                                  inStreamId, inConfiguration, inListener);
}

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
#include "Qt3DSSceneGraphDebugger.h"
#include "Qt3DSSceneGraphDebuggerValue.h"
#include "Qt3DSSceneGraphDebuggerProtocol.h"
#include "Qt3DSUIADatamodel.h"
#include "Qt3DSStateEditorValue.h"
#include "Qt3DSUIADatamodelValue.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSContainers.h"
#include "Qt3DSKernelTypes.h"
#include "Qt3DSHash.h" //we need to duplicate the hash attribute calls
#include "EASTL/set.h"

using namespace uic;
using namespace uic::state;
using namespace uic::app;
using namespace uic::state::debugger;

namespace {

struct SAppElemVectorSet
{
    nvvector<SAppElement *> m_Elements;
    eastl::hash_set<SAppElement *, eastl::hash<SAppElement *>, eastl::equal_to<SAppElement *>,
                    ForwardingAllocator>
        m_Set;
    SAppElemVectorSet(NVAllocatorCallback &alloc)
        : m_Elements(alloc, "AppElemVectorSet")
        , m_Set(ForwardingAllocator(alloc, "AppElemVectorSet"))
    {
    }

    void insert(SAppElement *elem)
    {
        if (m_Set.insert(elem).second)
            m_Elements.push_back(elem);
    }

    void clear()
    {
        m_Elements.clear();
        m_Set.clear();
    }
    NVConstDataRef<SAppElement *> ToDataRef() { return m_Elements; }
};

struct SValueIndex
{
    QT3DSU32 m_ValueIndex;
    QT3DSU32 m_Component;
    SValueIndex(QT3DSU32 vi, QT3DSU32 component)
        : m_ValueIndex(vi)
        , m_Component(component)
    {
    }
};

struct SAppElementEntry
{
    Q3DStudio::TAttOrArgList m_AttList;
    eastl::hash_map<QT3DSI32, SValueIndex> m_HashToIndexMap;
    eastl::vector<app::SDatamodelValue> m_RuntimeValues;
};

struct SPresentationEntry
{
    CRegisteredString m_Id;
    eastl::hash_map<CRegisteredString, SAppElement *> m_ElementMap;
};

Option<float> ValueToFloat(const SSGValue &val)
{
    if (val.getType() == SGPropertyValueTypes::Float)
        return val.getData<float>();
    return Empty();
}

Option<QT3DSI32> ValueToInteger(const SSGValue &val)
{
    if (val.getType() == SGPropertyValueTypes::I32)
        return val.getData<QT3DSI32>();
    return Empty();
}

Option<CRegisteredString> ValueToString(const SSGValue &val)
{
    if (val.getType() == SGPropertyValueTypes::String)
        return val.getData<CRegisteredString>();
    return Empty();
}

template <typename TDtype>
struct SValueSetter
{
};

template <>
struct SValueSetter<float>
{
    static void SetValue(const SSGValue &incoming, float &outgoing, QT3DSU32 /*component*/)
    {
        Option<float> val = ValueToFloat(incoming);
        if (val.hasValue())
            outgoing = *val;
    }
};

template <>
struct SValueSetter<QT3DSVec2>
{
    static void SetValue(const SSGValue &incoming, QT3DSVec2 &outgoing, QT3DSU32 component)
    {
        Option<float> val = ValueToFloat(incoming);
        if (val.hasValue()) {
            switch (component) {
            case 0:
                outgoing.x = *val;
                break;
            case 1:
                outgoing.y = *val;
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
    }
};

template <>
struct SValueSetter<QT3DSVec3>
{
    static void SetValue(const SSGValue &incoming, QT3DSVec3 &outgoing, QT3DSU32 component)
    {
        Option<float> val = ValueToFloat(incoming);
        if (val.hasValue()) {
            switch (component) {
            case 0:
                outgoing.x = *val;
                break;
            case 1:
                outgoing.y = *val;
                break;
            case 2:
                outgoing.z = *val;
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
    }
};

template <>
struct SValueSetter<QT3DSI32>
{
    static void SetValue(const SSGValue &incoming, QT3DSI32 &outgoing, QT3DSU32 /*component*/)
    {
        Option<QT3DSI32> val = ValueToInteger(incoming);
        if (val.hasValue())
            outgoing = *val;
    }
};

template <>
struct SValueSetter<eastl::string>
{
    static void SetValue(const SSGValue &incoming, eastl::string &outgoing, QT3DSU32 /*component*/)
    {
        Option<CRegisteredString> val = ValueToString(incoming);
        if (val.hasValue())
            outgoing = eastl::string(val->c_str());
    }
};

template <>
struct SValueSetter<bool>
{
    static void SetValue(const SSGValue &incoming, bool &outgoing, QT3DSU32 /*component*/)
    {
        Option<QT3DSI32> val = ValueToInteger(incoming);
        if (val.hasValue())
            outgoing = (*val) != 0 ? true : false;
    }
};

// Element ref.  Not sure what to do about this right now
template <>
struct SValueSetter<SGuid>
{
    static void SetValue(const SSGValue & /*incoming*/, SGuid & /*outgoing*/, QT3DSU32 /*component*/)
    {
    }
};

template <>
struct SValueSetter<CRegisteredString>
{
    static void SetValue(const SSGValue & /*incoming*/, CRegisteredString & /*outgoing*/,
                         QT3DSU32 /*component*/)
    {
        QT3DS_ASSERT(false);
    }
};

template <>
struct SValueSetter<SObjectRef>
{
    static void SetValue(const SSGValue & /*incoming*/, SObjectRef & /*outgoing*/,
                         QT3DSU32 /*component*/)
    {
    }
};

template <>
struct SValueSetter<CStringOrInt>
{
    static void SetValue(const SSGValue & /*incoming*/, CStringOrInt & /*outgoing*/,
                         QT3DSU32 /*component*/)
    {
    }
};

struct SValueSetterVisitor
{
    const SSGValue &m_Incoming;
    QT3DSU32 m_Component;
    SValueSetterVisitor &operator=(const SValueSetterVisitor &);
    SValueSetterVisitor(const SSGValue &i, QT3DSU32 c)
        : m_Incoming(i)
        , m_Component(c)
    {
    }
    template <typename TDataType>
    void operator()(TDataType &dt)
    {
        SValueSetter<TDataType>::SetValue(m_Incoming, dt, m_Component);
    }
    // object refs can generate this response.
    void operator()() {}
};

void SetComponentValue(const SSGValue &incoming, app::SDatamodelValue &outgoing, QT3DSU32 component)
{
    SValueSetterVisitor visitor(incoming, component);
    outgoing.visit<void>(visitor);
}

struct SArchitectDebuggerImpl : public ISceneGraphArchitectDebugger
{
    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<IDatamodel> m_Datamodel;
    NVScopedRefCounted<IDebugOutStream> m_Stream;
    ISceneGraphArchitectDebuggerListener *m_Listener;
    nvhash_map<QT3DSU64, SAppElement *> m_ElemAppElemMap;
    nvhash_map<SAppElement *, SAppElementEntry> m_RuntimeValues;
    SAppElemVectorSet m_DirtySet;
    eastl::vector<SPresentationEntry> m_Presentations;
    bool m_Valid;
    QT3DSI32 mRefCount;

    SArchitectDebuggerImpl(IDatamodel &dm)
        : m_Foundation(dm.GetFoundation())
        , m_Datamodel(dm)
        , m_Stream(NULL)
        , m_Listener(NULL)
        , m_ElemAppElemMap(dm.GetFoundation().getAllocator(), "ElemAppMap")
        , m_RuntimeValues(dm.GetFoundation().getAllocator(), "RuntimeValues")
        , m_DirtySet(dm.GetFoundation().getAllocator())
        , m_Valid(true)
        , mRefCount(0)
    {
    }

    void addRef() { atomicIncrement(&mRefCount); }
    void release()
    {
        // ensure the datamodel sticks around until after we are destroyed
        NVScopedRefCounted<IDatamodel> dm(m_Datamodel);
        atomicDecrement(&mRefCount);
        if (mRefCount <= 0) {
            NVDelete(m_Foundation.getAllocator(), this);
        }
    }

    virtual IDatamodel &GetDatamodel() { return *m_Datamodel; }

    static QT3DSI32 GetHashValue(const char *name, const char *component, eastl::string &ioWorkspace)
    {
        ioWorkspace.assign(nonNull(name));
        ioWorkspace.append(".");
        ioWorkspace.append(component);
        return Q3DStudio::CHash::HashAttribute(ioWorkspace.c_str());
    }

    void AppendAttribute(const char *name, const char *formalName, ERuntimeDataModelDataType dtype,
                         SAppElementEntry &theEntry)
    {
        SAttOrArg newValue;
        newValue.m_Name = name;
        newValue.m_FormalName = formalName;
        newValue.m_DataType = dtype;
        theEntry.m_AttList.push_back(newValue);
        theEntry.m_RuntimeValues.push_back(uic::app::SDatamodelValue());
    }

    void LoadAppElement(SAppElement &elem, SPresentationEntry &entry)
    {
        entry.m_ElementMap.insert(eastl::make_pair(
            m_Datamodel->GetStringTable().RegisterStr(m_Datamodel->GetElementId(elem).c_str()),
            &elem));
        SAppElementEntry &theEntry = m_RuntimeValues[&elem];
        theEntry.m_AttList = m_Datamodel->GetElementAttributes(elem);
        theEntry.m_RuntimeValues = m_Datamodel->GetElementAttributeInitialValues(elem);
        AppendAttribute("active", "Active", ERuntimeDataModelDataTypeBool, theEntry);

        if (m_Datamodel->IsComponent(elem)) {
            AppendAttribute("slide", "(Slide)", ERuntimeDataModelDataTypeLong, theEntry);
            AppendAttribute("time", "(Time)", ERuntimeDataModelDataTypeLong, theEntry);
            AppendAttribute("paused", "(Mode)", ERuntimeDataModelDataTypeBool, theEntry);
        }
        eastl::string hashTemp;
        for (QT3DSU32 idx = 0, end = (QT3DSU32)theEntry.m_AttList.size(); idx < end; ++idx) {
            // Build out the component hash names.
            const Q3DStudio::SAttOrArg &theProp(theEntry.m_AttList[idx]);
            switch (theProp.m_DataType) {
            // one component, one hash, just hash the name
            default:
                theEntry.m_HashToIndexMap.insert(
                    eastl::make_pair((QT3DSI32)Q3DStudio::CHash::HashAttribute(theProp.m_Name.c_str()),
                                     SValueIndex(idx, 0)));
                break;
            case ERuntimeDataModelDataTypeFloat2: {
                theEntry.m_HashToIndexMap.insert(eastl::make_pair(
                    GetHashValue(theProp.m_Name.c_str(), "x", hashTemp), SValueIndex(idx, 0)));
                theEntry.m_HashToIndexMap.insert(eastl::make_pair(
                    GetHashValue(theProp.m_Name.c_str(), "y", hashTemp), SValueIndex(idx, 1)));
            } break;
            case ERuntimeDataModelDataTypeFloat3: {
                const char *compNames[3] = { "x", "y", "z" };
                if (theProp.m_MetaType == ERuntimeAdditionalMetaDataTypeColor) {
                    compNames[0] = "r";
                    compNames[1] = "g";
                    compNames[2] = "b";
                }

                theEntry.m_HashToIndexMap.insert(
                    eastl::make_pair(GetHashValue(theProp.m_Name.c_str(), compNames[0], hashTemp),
                                     SValueIndex(idx, 0)));
                theEntry.m_HashToIndexMap.insert(
                    eastl::make_pair(GetHashValue(theProp.m_Name.c_str(), compNames[1], hashTemp),
                                     SValueIndex(idx, 1)));
                theEntry.m_HashToIndexMap.insert(
                    eastl::make_pair(GetHashValue(theProp.m_Name.c_str(), compNames[2], hashTemp),
                                     SValueIndex(idx, 2)));
            } break;
            }
            // Set a value
            if (theEntry.m_RuntimeValues[idx].getType() == ERuntimeDataModelDataTypeNone) {
                switch (theProp.m_DataType) {
                case ERuntimeDataModelDataTypeFloat:
                    theEntry.m_RuntimeValues[idx] = 0.0f;
                    break;
                case ERuntimeDataModelDataTypeFloat2:
                    theEntry.m_RuntimeValues[idx] = QT3DSVec2(0, 0);
                    break;
                case ERuntimeDataModelDataTypeFloat3:
                    theEntry.m_RuntimeValues[idx] = QT3DSVec3(0, 0, 0);
                    break;
                case ERuntimeDataModelDataTypeLong:
                    theEntry.m_RuntimeValues[idx] = (QT3DSI32)0;
                    break;
                case ERuntimeDataModelDataTypeString:
                    theEntry.m_RuntimeValues[idx] = eastl::string();
                    break;
                case ERuntimeDataModelDataTypeBool:
                    theEntry.m_RuntimeValues[idx] = false;
                    break;
                case ERuntimeDataModelDataTypeStringRef:
                    theEntry.m_RuntimeValues[idx] = eastl::string();
                    break;
                // object references are stored as string values.
                case ERuntimeDataModelDataTypeObjectRef:
                    theEntry.m_RuntimeValues[idx] = eastl::string();
                    break;
                case ERuntimeDataModelDataTypeStringOrInt:
                    theEntry.m_RuntimeValues[idx] = eastl::string();
                    break;
                }
            }
        }
        eastl::vector<SAppElement *> children = m_Datamodel->GetElementChildren(elem);
        for (size_t childIdx = 0, childEnd = children.size(); childIdx < childEnd; ++childIdx) {
            LoadAppElement(*children[childIdx], entry);
        }
    }

    void LoadDatamodel()
    {
        eastl::vector<app::SPresentation> presentations = m_Datamodel->GetPresentations();
        m_Presentations.clear();
        m_RuntimeValues.clear();
        m_ElemAppElemMap.clear();
        m_Presentations.resize(presentations.size());
        for (size_t idx = 0, end = presentations.size(); idx < end; ++idx) {
            app::SPresentation &incoming(presentations[idx]);
            SPresentationEntry &pres = m_Presentations[idx];
            pres.m_Id = m_Datamodel->GetStringTable().RegisterStr(incoming.m_Id.c_str());
            if (incoming.m_Scene)
                LoadAppElement(*incoming.m_Scene, pres);
        }
    }

    SAppElementEntry *GetRuntimeValues(app::SAppElement &elem)
    {
        nvhash_map<SAppElement *, SAppElementEntry>::iterator iter = m_RuntimeValues.find(&elem);
        if (iter != m_RuntimeValues.end())
            return &iter->second;
        return NULL;
    }

    SPresentationEntry *FindPresentation(CRegisteredString &str)
    {
        for (size_t idx = 0, end = m_Presentations.size(); idx < end; ++idx)
            if (m_Presentations[idx].m_Id == str)
                return &m_Presentations[idx];
        return NULL;
    }

    /*External interface for clients*/

    void SetListener(ISceneGraphArchitectDebuggerListener *listener) { m_Listener = listener; }

    Q3DStudio::TAttOrArgList GetElementAttributes(app::SAppElement &elem)
    {
        SAppElementEntry *entry = GetRuntimeValues(elem);
        if (entry)
            return entry->m_AttList;
        return Q3DStudio::TAttOrArgList();
    }

    eastl::vector<app::SDatamodelValue> GetElementAttributeValues(app::SAppElement &elem)
    {
        SAppElementEntry *entry = GetRuntimeValues(elem);
        if (entry)
            return entry->m_RuntimeValues;
        return eastl::vector<app::SDatamodelValue>();
    }

    void OnMessageReceived(const SDebugStreamMessage &inMessage)
    {
        if (!m_Valid)
            return;
        SSGProtocolReader theReader(inMessage.m_Data, m_Datamodel->GetStringTable());
        while (theReader.Finished() == false && m_Valid) {
            SSGProtocolMessageTypes::Enum theMessageType = theReader.MessageType();
            switch (theMessageType) {
            case SSGProtocolMessageTypes::Initialization: {
                QT3DSU32 version = theReader.ReadInitialization();
                if (version > GetSceneGraphProtocolVersion()) {
                    QT3DS_ASSERT(false);
                    m_Foundation.error(QT3DS_INVALID_OPERATION,
                                       "Invalid scene graph debugger protocol version");
                    m_Valid = false;
                }
            } break;
            case SSGProtocolMessageTypes::IdUpdate: {
                SIdUpdate theUpdate = theReader.ReadIdUpdate();
                SPresentationEntry *theEntry = FindPresentation(theUpdate.m_PresentationId);
                if (theEntry) {
                    for (size_t idx = 0, end = theUpdate.m_IdUpdates.size(); idx < end; ++idx) {
                        const SElemMap &elemMap(theUpdate.m_IdUpdates[idx]);
                        eastl::hash_map<CRegisteredString, SAppElement *>::iterator iter =
                            theEntry->m_ElementMap.find(elemMap.m_Id);
                        if (iter != theEntry->m_ElementMap.end())
                            m_ElemAppElemMap[elemMap.m_Elem] = iter->second;
                        else {
                            QT3DS_ASSERT(false);
                            m_Foundation.error(QT3DS_WARN, "Failed to map element");
                        }
                    }
                }
            } break;
            case SSGProtocolMessageTypes::ElemUpdate: {
                SElemUpdate theUpdate = theReader.ReadElemUpdate();
                nvhash_map<QT3DSU64, SAppElement *>::iterator ptrToElem =
                    m_ElemAppElemMap.find(theUpdate.m_Elem);
                if (ptrToElem != m_ElemAppElemMap.end()) {
                    nvhash_map<SAppElement *, SAppElementEntry>::iterator elemToEntry =
                        m_RuntimeValues.find(ptrToElem->second);
                    if (elemToEntry != m_RuntimeValues.end()) {
                        SAppElementEntry &theEntry(elemToEntry->second);
                        m_DirtySet.insert(elemToEntry->first);
                        for (size_t idx = 0, end = theUpdate.m_Updates.size(); idx < end; ++idx) {
                            const SValueUpdate &theValue(theUpdate.m_Updates[idx]);
                            eastl::hash_map<QT3DSI32, SValueIndex>::iterator hashIndex =
                                theEntry.m_HashToIndexMap.find(theValue.m_Hash);
                            if (hashIndex != theEntry.m_HashToIndexMap.end()) {
                                const SValueIndex theIdx = hashIndex->second;
                                SetComponentValue(theValue.m_Value,
                                                  theEntry.m_RuntimeValues[theIdx.m_ValueIndex],
                                                  theIdx.m_Component);
                            }
                        }
                    } else {
                        QT3DS_ASSERT(false);
                        m_Foundation.error(QT3DS_WARN, "Failed to map element");
                    }
                } else {
                    QT3DS_ASSERT(false);
                    m_Foundation.error(QT3DS_WARN, "Failed to map element");
                }
            } break;
            case SSGProtocolMessageTypes::Frame: {
                NVConstDataRef<SAppElement *> dirtyItems = m_DirtySet.ToDataRef();
                if (dirtyItems.size() && m_Listener) {
                    m_Listener->OnItemsDirty(dirtyItems);
                    m_DirtySet.clear();
                }
            } break;
            }
        } // end while
    }

    void AttachToStream(IDebugOutStream &inStream) { m_Stream = &inStream; }

    void RefreshData(bool inNeedReloadData)
    {
        if (inNeedReloadData)
            m_Datamodel->RefreshFile();
        LoadDatamodel();
    }
};
}

ISceneGraphArchitectDebugger &
ISceneGraphArchitectDebugger::Create(uic::app::IDatamodel &inDatamodel)
{
    SArchitectDebuggerImpl &retval =
        *QT3DS_NEW(inDatamodel.GetFoundation().getAllocator(), SArchitectDebuggerImpl)(inDatamodel);
    retval.LoadDatamodel();
    return retval;
}

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
#include "UICStateXMLIO.h"
#include "foundation/XML.h"
#include "UICStateTypes.h"
#include "UICStateContext.h"
#include "foundation/Utils.h"
#include "foundation/StringConversionImpl.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "EASTL/hash_map.h"
#include "UICStateExecutionTypes.h"
#include "UICStateEditor.h"
#include "UICStateEditorValue.h"

using namespace uic::state;
using namespace uic::state::editor;

namespace {
#define ITERATE_XML_ELEMENT_NAMES                                                                  \
    HANDLE_XML_ELEMENT_NAME(scxml)                                                                 \
    HANDLE_XML_ELEMENT_NAME(state)                                                                 \
    HANDLE_XML_ELEMENT_NAME(parallel)                                                              \
    HANDLE_XML_ELEMENT_NAME(transition)                                                            \
    HANDLE_XML_ELEMENT_NAME(initial)                                                               \
    HANDLE_XML_ELEMENT_NAME(final)                                                                 \
    HANDLE_XML_ELEMENT_NAME(onentry)                                                               \
    HANDLE_XML_ELEMENT_NAME(onexit)                                                                \
    HANDLE_XML_ELEMENT_NAME(history)                                                               \
    HANDLE_XML_ELEMENT_NAME(raise)                                                                 \
    HANDLE_XML_ELEMENT_NAME(if)                                                                    \
    HANDLE_XML_ELEMENT_NAME(elseif)                                                                \
    HANDLE_XML_ELEMENT_NAME(else)                                                                  \
    HANDLE_XML_ELEMENT_NAME(foreach)                                                               \
    HANDLE_XML_ELEMENT_NAME(log)                                                                   \
    HANDLE_XML_ELEMENT_NAME(param)                                                                 \
    HANDLE_XML_ELEMENT_NAME(assign)                                                                \
    HANDLE_XML_ELEMENT_NAME(script)                                                                \
    HANDLE_XML_ELEMENT_NAME(send)                                                                  \
    HANDLE_XML_ELEMENT_NAME(cancel)                                                                \
    HANDLE_XML_ELEMENT_NAME(invoke)                                                                \
    HANDLE_XML_ELEMENT_NAME(finalize)                                                              \
    HANDLE_XML_ELEMENT_NAME(cond)                                                                  \
    HANDLE_XML_ELEMENT_NAME(event)                                                                 \
    HANDLE_XML_ELEMENT_NAME(datamodel)                                                             \
    HANDLE_XML_ELEMENT_NAME(data)                                                                  \
    HANDLE_XML_ELEMENT_NAME(content)                                                               \
    HANDLE_XML_ELEMENT_NAME(external_transition)

struct SXMLName
{
    enum Enum {
#define HANDLE_XML_ELEMENT_NAME(nm) e##nm,
        ITERATE_XML_ELEMENT_NAMES
#undef HANDLE_XML_ELEMENT_NAME
            LastName,
    };
    static const char *GetNameForElemName(Enum inName)
    {
        switch (inName) {
#define HANDLE_XML_ELEMENT_NAME(nm)                                                                \
    case e##nm:                                                                                    \
        return #nm;
            ITERATE_XML_ELEMENT_NAMES
#undef HANDLE_XML_ELEMENT_NAME
        default:
            break;
        }
        QT3DS_ASSERT(false);
        return "unknown element";
    }
};

const char8_t *GetSCXMLNamespace()
{
    return "http://www.w3.org/2005/07/scxml";
}
const char8_t *GetStudioStateNamespace()
{
    return "http://qt.io/qt3dstudio/uicstate";
}

typedef eastl::pair<const char8_t *, NVConstDataRef<SStateNode *> *> TIdListPtrPair;
typedef eastl::pair<const char8_t *, SSend **> TIdSendPair;

TEditorStr DoGenerateUniqueId(const char *inRoot, IStateContext &ioContext,
                              IStringTable &ioStringTable)
{
    if (isTrivial(inRoot))
        inRoot = "id";
    TEditorStr stem(inRoot);
    TEditorStr idStr(stem);
    // Make the stem a valid possible id according xml specifications
    if (idStr.size()) {
        // Replace all spaces with undre
        // Check that the first item isn't a space or a number.  We can't do a real check here
        // because we don't have unicode tables of letters and such.
        // replace spaces with underscores.
        for (TEditorStr::size_type thePos = idStr.find(' '); thePos != TEditorStr::npos;
             thePos = idStr.find(' ', thePos + 1))
            idStr.replace(thePos, 1, "_");
        if (idStr[0] >= '0' && idStr[0] <= '9')
            idStr.insert(idStr.begin(), 1, ':');
    }

    QT3DSU32 idx = 0;

    while (ioContext.ContainsId(ioStringTable.RegisterStr(idStr.c_str()))) {
        ++idx;
        char temp[64];
        sprintf(temp, "%d", idx);
        idStr.assign(stem);
        idStr.append("_");
        idStr.append(temp);
    }

    return idStr;
}

struct SParseContext
{
    typedef nvvector<eastl::pair<CRegisteredString, STransition *>> TExternalTransitionList;

    NVAllocatorCallback &m_GraphAllocator;
    NVFoundationBase &m_Foundation;
    IDOMReader &m_Reader;
    IStateContext &m_Context;
    IStringTable &m_StrTable;
    IEditor *m_Editor;
    CRegisteredString m_Names[SXMLName::LastName];
    MemoryBuffer<ForwardingAllocator> m_ParseBuffer;
    nvvector<char8_t> m_TempBuffer;
    // To be filled in on the second parse pass
    nvvector<TIdListPtrPair> m_References;
    nvvector<TIdSendPair> m_SendReferences;
    CXMLIO::TIdRemapMap m_RemapMap;
    nvvector<SIdValue> m_GenerateIdList;
    TExternalTransitionList m_ExternalTransitions;
    CRegisteredString m_SCXMLNamespace;
    CRegisteredString m_StudioNamespace;
    QT3DSI32 m_UICVersion;

    SParseContext(NVAllocatorCallback &inGraphAlloc, NVFoundationBase &inFnd, IDOMReader &inReader,
                  IStateContext &inCtx, IStringTable &inStrTable, IEditor *inEditor)
        : m_GraphAllocator(inGraphAlloc)
        , m_Foundation(inFnd)
        , m_Reader(inReader)
        , m_Context(inCtx)
        , m_StrTable(inStrTable)
        , m_Editor(inEditor)
        , m_ParseBuffer(ForwardingAllocator(inFnd.getAllocator(), "ParseBuffer"))
        , m_TempBuffer(inFnd.getAllocator(), "TempBuffer")
        , m_References(inFnd.getAllocator(), "m_References")
        , m_SendReferences(inFnd.getAllocator(), "m_StrReferences")
        , m_GenerateIdList(inFnd.getAllocator(), "m_GenerateIdList")
        , m_ExternalTransitions(inFnd.getAllocator(), "m_ExternalTransitions")
        , m_UICVersion(SSCXML::GetCurrentUICVersion())
    {
#define HANDLE_XML_ELEMENT_NAME(nm) m_Names[SXMLName::e##nm] = inStrTable.RegisterStr(#nm);
        ITERATE_XML_ELEMENT_NAMES
#undef HANDLE_XML_ELEMENT_NAME
        m_SCXMLNamespace = inStrTable.RegisterStr(GetSCXMLNamespace());
        m_StudioNamespace = inStrTable.RegisterStr(GetStudioStateNamespace());
    }

    inline bool eq(CRegisteredString lhs, SXMLName::Enum rhs) { return lhs == m_Names[rhs]; }
    const char8_t *ToGraphStr(const char8_t *temp)
    {
        temp = nonNull(temp);
        QT3DSU32 len = StrLen(temp) + 1;
        char8_t *retval =
            (char8_t *)m_GraphAllocator.allocate(len, "graph string", __FILE__, __LINE__);
        memCopy(retval, temp, len);
        // Force null termination regardless.
        retval[len] = 0;
        return retval;
    }
    const char8_t *ParseStrAtt(const char8_t *attName)
    {
        const char8_t *temp;
        if (m_Reader.UnregisteredAtt(attName, temp) && temp && *temp)
            return ToGraphStr(temp);
        return NULL;
    }
    void ParseStrAtt(const char8_t *attName, const char8_t *&outVal)
    {
        outVal = ParseStrAtt(attName);
    }
    void ParseStrAtt(const char8_t *attName, CRegisteredString &outVal)
    {
        m_Reader.Att(attName, outVal);
    }

    SItemExtensionInfo &GetExtensionInfo(void *inItem)
    {
        return m_Context.GetOrCreateExtensionInfo(inItem);
    }

    // Extension attributes are indicated by their namespace.  If they have a namespace
    // and they aren't scxml namespace and they aren't studio namespace, they are extension.
    void ParseExtensionAttributes(void *inItem)
    {
        for (SDOMAttribute *theAttribute = m_Reader.GetFirstAttribute(); theAttribute;
             theAttribute = theAttribute->m_NextAttribute) {
            if (theAttribute->m_Namespace != m_StudioNamespace
                && theAttribute->m_Namespace != m_SCXMLNamespace
                && theAttribute->m_Namespace.IsValid()) {
                GetExtensionInfo(inItem).m_ExtensionAttributes.push_back(
                    *QT3DS_NEW(m_GraphAllocator, SDOMAttributeNode)(theAttribute));
            }
        }
    }
    bool ParseExtensionElement(void *inItem)
    {
        SDOMElement &theElement(*m_Reader.GetElement());

        if (theElement.m_Namespace != m_StudioNamespace
            && theElement.m_Namespace != m_SCXMLNamespace && theElement.m_Namespace.IsValid()) {
            GetExtensionInfo(inItem).m_ExtensionNodes.push_back(
                *QT3DS_NEW(m_GraphAllocator, SDOMElementNode)(&theElement));
            return true;
        }
        return false;
    }

    void ParseExtensionElements(void *inItem)
    {
        IDOMReader::Scope _childElemScope(m_Reader);
        for (bool success = m_Reader.MoveToFirstChild(); success;
             success = m_Reader.MoveToNextSibling()) {
            ParseExtensionElement(inItem);
        }
    }

    NVConstDataRef<QT3DSF32> ParseFloats(const char8_t *inData)
    {
        size_t len = StrLen(inData) + 1;
        if (len == 1)
            return NVConstDataRef<QT3DSF32>();
        m_TempBuffer.resize((QT3DSU32)len);
        memCopy(m_TempBuffer.data(), inData, (QT3DSU32)len);
        m_ParseBuffer.clear();
        Char8TReader theReader(m_TempBuffer.data(), m_ParseBuffer);
        NVConstDataRef<QT3DSF32> retval;
        theReader.ReadBuffer(retval);
        return retval;
    }
    bool ParseVec2Att(const char8_t *attName, QT3DSVec2 &outVal)
    {
        const char8_t *tempVal;
        if (m_Reader.UnregisteredAtt(attName, tempVal, GetStudioStateNamespace())) {
            NVConstDataRef<QT3DSF32> floats = ParseFloats(tempVal);
            if (floats.mSize >= 2) {
                memCopy(&outVal.x, floats.mData, sizeof(outVal));
                return true;
            }
        }
        return false;
    }

    bool ParseVec3Att(const char8_t *attName, QT3DSVec3 &outVal)
    {
        const char8_t *tempVal;
        if (m_Reader.UnregisteredAtt(attName, tempVal, GetStudioStateNamespace())) {
            NVConstDataRef<QT3DSF32> floats = ParseFloats(tempVal);
            if (floats.mSize >= 3) {
                memCopy(&outVal.x, floats.mData, sizeof(outVal));
                return true;
            }
        }
        return false;
    }

    CRegisteredString RemapStr(CRegisteredString inStr)
    {
        CXMLIO::TIdRemapMap::iterator iter = m_RemapMap.find(inStr.c_str());
        if (iter != m_RemapMap.end())
            inStr = m_StrTable.RegisterStr(iter->second.c_str());
        return inStr;
    }

    SStateNode *FindStateNode(CRegisteredString inString)
    {
        return m_Context.FindStateNode(RemapStr(inString));
    }

    SSend *ParseSendIdSecondPass(const char8_t *inStr)
    {
        if (isTrivial(inStr))
            return NULL;
        return m_Context.FindSend(RemapStr(m_StrTable.RegisterStr(inStr)));
    }

    NVConstDataRef<SStateNode *> ParseIDRefSecondPass(const char8_t *inStr)
    {
        typedef eastl::basic_string<char8_t, ForwardingAllocator> TStrType;
        TStrType workspace(ForwardingAllocator(m_Foundation.getAllocator(), "ParseIDRef"));
        TStrType str(ForwardingAllocator(m_Foundation.getAllocator(), "ParseIDRef"));
        nvvector<SStateNode *> tempVal(m_Foundation.getAllocator(), "ParseIDRef");

        workspace.assign(inStr);
        for (TStrType::size_type startPos = workspace.find_first_not_of(' '),
                                 endPos = workspace.find_first_of(' ', startPos);
             startPos != TStrType::npos; startPos = workspace.find_first_not_of(' ', endPos),
                                 endPos = workspace.find_first_of(' ', startPos)) {
            if (endPos == TStrType::npos)
                endPos = workspace.size();
            str = workspace.substr(startPos, endPos - startPos);
            CRegisteredString theStr(m_StrTable.RegisterStr(str.c_str()));
            SStateNode *theNode = FindStateNode(theStr);
            if (theNode)
                tempVal.push_back(theNode);
        }
        if (tempVal.size() == 0)
            return NVConstDataRef<SStateNode *>();

        SStateNode **dataPtr = (SStateNode **)m_GraphAllocator.allocate(
            tempVal.size() * sizeof(SStateNode *), "IDRef", __FILE__, __LINE__);
        memCopy(dataPtr, tempVal.data(), tempVal.size() * sizeof(SStateNode *));
        return toDataRef(dataPtr, tempVal.size());
    }

    void ParseIDRef(const char8_t *inStr, NVConstDataRef<SStateNode *> &ioNodes)
    {
        if (inStr == NULL || *inStr == 0) {
            ioNodes = NVConstDataRef<SStateNode *>();
            return;
        }
        m_References.push_back(eastl::make_pair(inStr, &ioNodes));
    }

    static void AppendChild(SStateNode &inParent, TStateNodeList &outChildren, SStateNode &inChild)
    {
        inChild.m_Parent = &inParent;
        outChildren.push_back(inChild);
    }

    static void AppendChild(SStateNode *inNodeParent, SExecutableContent *inParent,
                            TExecutableContentList &outChildren, SExecutableContent &inChild)
    {
        if (inNodeParent) {
            QT3DS_ASSERT(inParent == NULL);
        } else {
            QT3DS_ASSERT(inParent);
        }
        inChild.m_StateNodeParent = inNodeParent;
        inChild.m_Parent = inParent;
        outChildren.push_back(inChild);
    }

    template <typename TStateType>
    void ParseEditorAttributes(TStateType &inNode)
    {
        ParseExtensionAttributes(&inNode);
        if (m_Editor) {
            const char8_t *name;
            if (m_Reader.UnregisteredAtt("id", name, GetStudioStateNamespace()))
                m_Editor->GetOrCreate(inNode)->SetPropertyValue("id", eastl::string(nonNull(name)));

            QT3DSVec2 temp;
            if (ParseVec2Att("position", temp))
                m_Editor->GetOrCreate(inNode)->SetPropertyValue("position", temp);

            if (ParseVec2Att("dimension", temp))
                m_Editor->GetOrCreate(inNode)->SetPropertyValue("dimension", temp);

            QT3DSVec3 tempv3;
            if (ParseVec3Att("color", tempv3))
                m_Editor->GetOrCreate(inNode)->SetPropertyValue("dimension", tempv3);
        }
    }

    void ParseStateNodeEditorAttributes(SStateNode &inNode)
    {
        ParseExtensionAttributes(&inNode);
        CRegisteredString Id;
        if (inNode.m_Id.IsValid() == false)
            m_Reader.Att("id", inNode.m_Id);

        QT3DSVec2 temp;
        if (ParseVec2Att("position", temp))
            inNode.SetPosition(temp);

        if (ParseVec2Att("dimension", temp))
            inNode.SetDimension(temp);

        QT3DSVec3 tempv3;
        if (ParseVec3Att("color", tempv3))
            inNode.SetColor(tempv3);

        if (m_Editor) {
            const char8_t *desc;
            if (m_Reader.UnregisteredAtt("description", desc, GetStudioStateNamespace()))
                m_Editor->ToEditor(inNode)->SetPropertyValue("description",
                                                             eastl::string(nonNull(desc)));
        }
    }

    // Parse the node id.  IF it exists, then schedule the id for remapping.
    template <typename TDataType>
    void ParseNodeId(TDataType &inNode)
    {
        m_Reader.Att("id", inNode.m_Id);
        if (inNode.m_Id.IsValid() == false || m_Context.ContainsId(inNode.m_Id)) {
            m_GenerateIdList.push_back(&inNode);
        } else {
            bool success = m_Context.InsertId(inNode.m_Id, &inNode);
            (void)success;
            QT3DS_ASSERT(success);
        }
    }

    SParam &ParseParam()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SParam *retval = QT3DS_NEW(m_GraphAllocator, SParam)();
        ParseStrAtt("name", retval->m_Name);
        ParseStrAtt("expr", retval->m_Expr);
        ParseStrAtt("location", retval->m_Location);
        ParseExtensionAttributes(retval);
        ParseExtensionElements(retval);
        return *retval;
    }

    SContent &ParseContent()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SContent *retval = QT3DS_NEW(m_GraphAllocator, SContent)();
        ParseStrAtt("expr", retval->m_Expr);
        if (isTrivial(retval->m_Expr)) {
            if (m_Reader.CountChildren() == 0) {
                const char8_t *val = NULL;
                m_Reader.Value(val);
                if (!isTrivial(val))
                    retval->m_ContentValue = this->ToGraphStr(val);
            }
            // We don't implement any extensions, so this is most certainly going into the
            // extensions bin for this content.
            else {
            }
        }
        ParseExtensionAttributes(retval);
        ParseExtensionElements(retval);
        return *retval;
    }

    SExecutableContent &ParseSend()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SSend *retval = QT3DS_NEW(m_GraphAllocator, SSend)();
        ParseStrAtt("event", retval->m_Event);
        ParseStrAtt("eventexpr", retval->m_EventExpr);
        ParseStrAtt("target", retval->m_Target);
        ParseStrAtt("targetexpr", retval->m_TargetExpr);
        ParseStrAtt("type", retval->m_Type);
        ParseStrAtt("typeExpr", retval->m_TypeExpr);
        ParseNodeId(*retval);
        ParseStrAtt("idlocation", retval->m_IdLocation);
        ParseStrAtt("delay", retval->m_Delay);
        ParseStrAtt("delayexpr", retval->m_DelayExpr);
        ParseStrAtt("namelist", retval->m_NameList);
        ParseExtensionAttributes(retval);
        for (bool success = m_Reader.MoveToFirstChild(); success;
             success = m_Reader.MoveToNextSibling()) {
            IDOMReader::Scope _loopScope(m_Reader);
            CRegisteredString elemName = m_Reader.GetElementName();
            if (eq(elemName, SXMLName::eparam))
                AppendChild(NULL, retval, retval->m_Children, ParseParam());
            else if (eq(elemName, SXMLName::econtent))
                retval->m_Children.push_back(ParseContent());
            else {
                if (!ParseExtensionElement(retval)) {
                    qCCritical(INTERNAL_ERROR, "Failed to parse send child %s", elemName.c_str());
                    QT3DS_ASSERT(false);
                }
            }
        }
        m_Context.AddSendToList(*retval);
        return *retval;
    }
    SExecutableContent &ParseIf()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SIf *retval = QT3DS_NEW(m_GraphAllocator, SIf)();
        ParseStrAtt("cond", retval->m_Cond);
        ParseExtensionAttributes(retval);
        ParseExecutableContent(NULL, retval, retval->m_Children, retval);
        return *retval;
    }

    SExecutableContent &ParseElseIf()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SElseIf *retval = QT3DS_NEW(m_GraphAllocator, SElseIf)();
        ParseStrAtt("cond", retval->m_Cond);
        ParseExtensionAttributes(retval);
        ParseExecutableContent(NULL, retval, retval->m_Children, retval);
        return *retval;
    }

    SExecutableContent &ParseElse()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SElse *retval = QT3DS_NEW(m_GraphAllocator, SElse)();
        ParseExtensionAttributes(retval);
        ParseExecutableContent(NULL, retval, retval->m_Children, retval);
        return *retval;
    }
    SExecutableContent &ParseForEach()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SForeach *retval = QT3DS_NEW(m_GraphAllocator, SForeach)();
        ParseStrAtt("array", retval->m_Array);
        ParseStrAtt("item", retval->m_Item);
        ParseStrAtt("index", retval->m_Index);
        ParseExtensionAttributes(retval);
        ParseExecutableContent(NULL, retval, retval->m_Children, retval);
        return *retval;
    }

    SExecutableContent &ParseRaise()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SRaise *retval = QT3DS_NEW(m_GraphAllocator, SRaise)();
        ParseStrAtt("event", retval->m_Event);
        ParseExtensionAttributes(retval);
        ParseExtensionElements(retval);
        return *retval;
    }

    SExecutableContent &ParseLog()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SLog *retval = QT3DS_NEW(m_GraphAllocator, SLog)();
        ParseStrAtt("label", retval->m_Label);
        ParseStrAtt("expr", retval->m_Expression);
        ParseExtensionAttributes(retval);
        ParseExtensionElements(retval);
        return *retval;
    }

    SData &ParseData()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SData *retval = QT3DS_NEW(m_GraphAllocator, SData)();
        ParseStrAtt("id", retval->m_Id);
        ParseStrAtt("src", retval->m_Source);
        ParseStrAtt("expr", retval->m_Expression);
        ParseEditorAttributes(*retval);
        ParseExtensionElements(retval);
        const char8_t *value;
        m_Reader.Value(value);
        // Not handling arbitrary content under data right now.
        if (isTrivial(retval->m_Expression) && !isTrivial(value)) {
            retval->m_Expression = ToGraphStr(value);
        }
        return *retval;
    }

    SDataModel *ParseDataModel()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SDataModel *retval = QT3DS_NEW(m_GraphAllocator, SDataModel)();
        ParseStrAtt("id", retval->m_Id);
        ParseStrAtt("src", retval->m_Source);
        ParseStrAtt("expr", retval->m_Expression);
        ParseEditorAttributes(*retval);
        for (bool success = m_Reader.MoveToFirstChild("data"); success;
             success = m_Reader.MoveToNextSibling("data"))
            retval->m_Data.push_back(ParseData());

        ParseExtensionElements(retval);

        return retval;
    }

    SExecutableContent &ParseAssign()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SAssign *retval = QT3DS_NEW(m_GraphAllocator, SAssign)();
        ParseStrAtt("location", retval->m_Location);
        ParseStrAtt("expr", retval->m_Expression);
        ParseExtensionAttributes(retval);
        ParseExtensionElements(retval);
        // Not dealing with children.
        return *retval;
    }

    SExecutableContent &ParseScript()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SScript *retval = QT3DS_NEW(m_GraphAllocator, SScript)();
        ParseStrAtt("src", retval->m_URL);
        ParseExtensionAttributes(retval);
        if (m_Reader.Value(retval->m_Data))
            retval->m_Data = ToGraphStr(retval->m_Data);
        ParseExtensionElements(retval);
        return *retval;
    }

    SExecutableContent &ParseCancel()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SCancel *retval = QT3DS_NEW(m_GraphAllocator, SCancel)();
        const char8_t *sendId;
        if (m_Reader.UnregisteredAtt("sendid", sendId))
            m_SendReferences.push_back(eastl::make_pair(sendId, &retval->m_Send));
        ParseStrAtt("sendidexpr", retval->m_IdExpression);
        ParseExtensionAttributes(retval);
        ParseExtensionElements(retval);
        return *retval;
    }
    struct SExecutableContentParseBinding
    {
        SXMLName::Enum m_Name;
        typedef SExecutableContent &(SParseContext::*TParseFunc)();
        TParseFunc m_ParseFunc;
    };

    static NVDataRef<SExecutableContentParseBinding> GetParseBindings()
    {
        static SExecutableContentParseBinding s_ExecContentParseBindings[10] = {
            { SXMLName::esend, &SParseContext::ParseSend },
            { SXMLName::eif, &SParseContext::ParseIf },
            { SXMLName::eelseif, &SParseContext::ParseElseIf },
            { SXMLName::eelse, &SParseContext::ParseElse },
            { SXMLName::eforeach, &SParseContext::ParseForEach },
            { SXMLName::eraise, &SParseContext::ParseRaise },
            { SXMLName::elog, &SParseContext::ParseLog },
            { SXMLName::eassign, &SParseContext::ParseAssign },
            { SXMLName::escript, &SParseContext::ParseScript },
            { SXMLName::ecancel, &SParseContext::ParseCancel },
        };

        return toDataRef(s_ExecContentParseBindings, 10);
    }

    bool ParseExecutableContentItem(SStateNode *inNodeParent, SExecutableContent *inParent,
                                    TExecutableContentList &outContent)
    {
        CRegisteredString elemName = m_Reader.GetElementName();
        NVDataRef<SExecutableContentParseBinding> theBindingList(GetParseBindings());
        for (QT3DSU32 idx = 0, end = theBindingList.size(); idx < end; ++idx) {
            SExecutableContentParseBinding theBinding(theBindingList[idx]);
            if (eq(elemName, theBinding.m_Name)) {
                AppendChild(inNodeParent, inParent, outContent, (this->*theBinding.m_ParseFunc)());
                return true;
            }
        }
        return false;
    }

    void ParseExecutableContent(SStateNode *inStateNodeParent, SExecutableContent *inParent,
                                TExecutableContentList &outContent, void *inExtensionPtr)
    {
        IDOMReader::Scope _itemScope(m_Reader);
        for (bool success = m_Reader.MoveToFirstChild(); success;
             success = m_Reader.MoveToNextSibling()) {
            IDOMReader::Scope _loopScope(m_Reader);
            CRegisteredString elemName = m_Reader.GetElementName();
            if (!ParseExecutableContentItem(inStateNodeParent, inParent, outContent)) {
                if (!ParseExtensionElement(inExtensionPtr)) {
                    qCCritical(INTERNAL_ERROR, "Failed to parse extension child %s", elemName.c_str());
                    QT3DS_ASSERT(false);
                }
            }
        }
    }

    SOnEntry &ParseOnEntry(SStateNode &inParent)
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SOnEntry *retval = QT3DS_NEW(m_GraphAllocator, SOnEntry)();
        ParseExecutableContent(&inParent, NULL, retval->m_ExecutableContent, retval);
        return *retval;
    }

    SOnExit &ParseOnExit(SStateNode &inParent)
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SOnExit *retval = QT3DS_NEW(m_GraphAllocator, SOnExit)();
        ParseExecutableContent(&inParent, NULL, retval->m_ExecutableContent, retval);
        return *retval;
    }

    SHistory &ParseHistory()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SHistory *retval = QT3DS_NEW(m_GraphAllocator, SHistory)();
        ParseNodeId(*retval);
        const char8_t *temp;
        if (m_Reader.UnregisteredAtt("type", temp) && AreEqual(temp, "deep"))
            retval->m_Flags.SetDeep(true);
        ParseStateNodeEditorAttributes(*retval);
        if (m_Reader.MoveToFirstChild("transition")) {
            retval->m_Transition = &ParseTransition();
            retval->m_Transition->m_Parent = retval;
        }
        ParseExtensionElements(retval);
        return *retval;
    }

    STransition &ParseTransition()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        STransition *retval = QT3DS_NEW(m_GraphAllocator, STransition)();
        m_Reader.Att("event", retval->m_Event);
        ParseNodeId(*retval);
        const char8_t *temp;
        if (m_Reader.UnregisteredAtt("target", temp)) {
            ParseIDRef(temp, retval->m_Target);
        }
        if (m_Reader.UnregisteredAtt("type", temp) && AreEqual(temp, "internal"))
            retval->m_Flags.SetInternal(true);
        ParseStateNodeEditorAttributes(*retval);

        retval->m_Condition = ParseStrAtt("cond");
        // Position and such is only valid for transitions after UIC version 0.
        if (m_UICVersion > 0) {
            QT3DSVec2 endPos;
            if (ParseVec2Att("end_position", endPos))
                retval->SetEndPosition(endPos);

            if (m_Reader.UnregisteredAtt("path", temp)) {
                NVConstDataRef<QT3DSF32> pathData = ParseFloats(temp);
                QT3DS_ASSERT((pathData.size() % 2) == 0);
                size_t newDataSize = pathData.size() * sizeof(QT3DSF32);
                QT3DSVec2 *newData = (QT3DSVec2 *)m_GraphAllocator.allocate(
                    newDataSize, "STransition::m_Path", __FILE__, __LINE__);
                memCopy(newData, pathData.begin(), (QT3DSU32)newDataSize);
                retval->m_Path = toDataRef(newData, (QT3DSU32)(newDataSize / sizeof(QT3DSVec2)));
            }
        } else {
            retval->SetPosition(Empty());
        }

        for (bool success = m_Reader.MoveToFirstChild(); success;
             success = m_Reader.MoveToNextSibling()) {
            IDOMReader::Scope _loopScope(m_Reader);
            CRegisteredString elemName = m_Reader.GetElementName();
            if (!ParseExecutableContentItem(retval, NULL, retval->m_ExecutableContent)) {
                if (!ParseExtensionElement(retval)) {
                    qCCritical(INTERNAL_ERROR, "Failed to parse transition child %s", elemName.c_str());
                    QT3DS_ASSERT(false);
                }
            }
        }
        return *retval;
    }

    SFinal &ParseFinal()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SFinal *retval = QT3DS_NEW(m_GraphAllocator, SFinal)();

        ParseNodeId(*retval);
        ParseStateNodeEditorAttributes(*retval);

        for (bool success = m_Reader.MoveToFirstChild(); success;
             success = m_Reader.MoveToNextSibling()) {
            IDOMReader::Scope _loopScope(m_Reader);
            CRegisteredString elemName = m_Reader.GetElementName();
            if (eq(elemName, SXMLName::eonentry))
                retval->m_OnEntry.push_back(ParseOnEntry(*retval));
            else if (eq(elemName, SXMLName::eonexit))
                retval->m_OnExit.push_back(ParseOnExit(*retval));
            else if (!ParseExtensionElement(retval)) {
                qCCritical(INTERNAL_ERROR, "Failed to parse final child %s", elemName.c_str());
                QT3DS_ASSERT(false);
            }
        }
        return *retval;
    }

    bool ParseStateParallelChildren(SStateNode &inParent, CRegisteredString &inElemName,
                                    TStateNodeList &outChildren, TOnEntryList &inOnEntry,
                                    TOnExitList &inOnExit, SDataModel *&dmPtr)
    {
        if (eq(inElemName, SXMLName::estate))
            AppendChild(inParent, outChildren, ParseState());
        else if (eq(inElemName, SXMLName::etransition))
            AppendChild(inParent, outChildren, ParseTransition());
        else if (eq(inElemName, SXMLName::eparallel))
            AppendChild(inParent, outChildren, ParseParallel());
        else if (eq(inElemName, SXMLName::eonentry))
            inOnEntry.push_back(ParseOnEntry(inParent));
        else if (eq(inElemName, SXMLName::eonexit))
            inOnExit.push_back(ParseOnExit(inParent));
        else if (eq(inElemName, SXMLName::ehistory))
            AppendChild(inParent, outChildren, ParseHistory());
        else if (eq(inElemName, SXMLName::efinal))
            AppendChild(inParent, outChildren, ParseFinal());
        else if (eq(inElemName, SXMLName::edatamodel))
            dmPtr = ParseDataModel();
        else {
            return false;
        }
        return true;
    }

    SStateNode &ParseParallel()
    {
        IDOMReader::Scope _itemScope(m_Reader);
        SParallel *retval = QT3DS_NEW(m_GraphAllocator, SParallel)();
        ParseNodeId(*retval);
        ParseStateNodeEditorAttributes(*retval);

        for (bool success = m_Reader.MoveToFirstChild(); success;
             success = m_Reader.MoveToNextSibling()) {
            IDOMReader::Scope _loopScope(m_Reader);
            CRegisteredString elemName = m_Reader.GetElementName();
            if (!ParseStateParallelChildren(*retval, elemName, retval->m_Children,
                                            retval->m_OnEntry, retval->m_OnExit,
                                            retval->m_DataModel)) {
                if (!ParseExtensionElement(retval)) {
                    qCCritical(INTERNAL_ERROR, "Failed to parse parallel child %s", elemName.c_str());
                    QT3DS_ASSERT(false);
                }
            }
        }
        return *retval;
    }

    void SetStateInitialTransition(STransition *&ioInitial, TStateNodeList &inChildren,
                                   SStateNode &inSource)
    {
        if (ioInitial) {
            ioInitial->m_Parent = &inSource;
            return;
        }
        SStateNode *firstStateChild = NULL;
        for (TStateNodeList::iterator iter = inChildren.begin();
             iter != inChildren.end() && firstStateChild == NULL; ++iter) {
            if (iter->m_Type == StateNodeTypes::State || iter->m_Type == StateNodeTypes::Parallel
                || iter->m_Type == StateNodeTypes::Final) {
                firstStateChild = &(*iter);
            }
        }

        if (firstStateChild) {
            STransition *theTransition = QT3DS_NEW(m_GraphAllocator, STransition)();
            size_t byteSize = sizeof(SStateNode *);
            SStateNode **theData = (SStateNode **)m_GraphAllocator.allocate(
                byteSize, "InitialTransition", __FILE__, __LINE__);
            *theData = firstStateChild;
            theTransition->m_Target = toDataRef(theData, 1);
            theTransition->m_Parent = &inSource;
            ioInitial = theTransition;
        }
    }

    STransition *ParseInitial()
    {
        if (m_Reader.MoveToFirstChild("transition"))
            return &ParseTransition();
        else {
            QT3DS_ASSERT(false);
        }
        return NULL;
    }

    SStateNode &ParseState()
    {
        IDOMReader::Scope _stateScope(m_Reader);
        SState *theNewState = QT3DS_NEW(m_GraphAllocator, SState)();
        ParseNodeId(*theNewState);
        const char8_t *initialAtt;
        if (m_Reader.UnregisteredAtt("initialexpr", initialAtt, GetStudioStateNamespace())) {
            theNewState->m_InitialExpr = ToGraphStr(initialAtt);
            const char8_t *errorTest;
            if (m_Reader.UnregisteredAtt("initial", errorTest)) {
                qCCritical(INVALID_OPERATION, "Attribute initial=\"%s\" conflicts with "
                    "attribute initialexpr=\"%s\" on <state "
                    "id=\"%s\">; using initialexpr.",
                    nonNull(errorTest), nonNull(initialAtt),
                    nonNull(theNewState->m_Id.c_str()));
            }
        } else if (m_Reader.UnregisteredAtt("initial", initialAtt) && !isTrivial(initialAtt)) {
            STransition *theTransition = QT3DS_NEW(m_GraphAllocator, STransition)();
            ParseIDRef(initialAtt, theTransition->m_Target);
            theTransition->m_Parent = theNewState;
            theNewState->m_Initial = theTransition;
        }

        ParseStateNodeEditorAttributes(*theNewState);

        for (bool success = m_Reader.MoveToFirstChild(); success;
             success = m_Reader.MoveToNextSibling()) {
            IDOMReader::Scope _loopScope(m_Reader);
            CRegisteredString elemName = m_Reader.GetElementName();
            if (!ParseStateParallelChildren(*theNewState, elemName, theNewState->m_Children,
                                            theNewState->m_OnEntry, theNewState->m_OnExit,
                                            theNewState->m_DataModel)) {
                // InitialExpr takes precedence over initial transition
                if (eq(elemName, SXMLName::einitial) && isTrivial(theNewState->m_InitialExpr)) {
                    if (!theNewState->m_Initial)
                        theNewState->m_Initial = ParseInitial();
                } else {
                    if (!ParseExtensionElement(theNewState)) {
                        qCCritical(INTERNAL_ERROR, "Failed to parse state child %s", elemName.c_str());
                        QT3DS_ASSERT(false);
                    }
                }
            }
        }
        if (isTrivial(theNewState->m_InitialExpr))
            SetStateInitialTransition(theNewState->m_Initial, theNewState->m_Children,
                                      *theNewState);
        return *theNewState;
    }

    static const char8_t *GetDefaultIdName(SStateNode &theNode)
    {
        const char8_t *theTemp = "id";
        switch (theNode.m_Type) {
        case StateNodeTypes::State:
            theTemp = "state";
            break;
        case StateNodeTypes::Parallel:
            theTemp = "parallel";
            break;
        case StateNodeTypes::Final:
            theTemp = "final";
            break;
        case StateNodeTypes::History:
            theTemp = "history";
            break;
        case StateNodeTypes::Transition:
            theTemp = "transition";
            break;
        default:
            break;
        }
        return theTemp;
    }

    static const char8_t *GetDefaultIdName(SSend &) { return "send"; }

    template <typename TDataType>
    void GenerateIdValue(TDataType &theNode)
    {
        if (theNode.m_Id.IsValid() == true) {
            bool preexisting = m_Context.ContainsId(theNode.m_Id);
            if (preexisting) {
                CRegisteredString oldId = theNode.m_Id;
                theNode.m_Id = CRegisteredString();
                CXMLIO::GenerateUniqueId(theNode, oldId.c_str(), m_Context, m_StrTable);
                bool success = m_RemapMap.insert(eastl::make_pair(oldId, theNode.m_Id)).second;
                (void)success;
                QT3DS_ASSERT(success);
            }
        } else {
            CXMLIO::GenerateUniqueId(theNode, GetDefaultIdName(theNode), m_Context, m_StrTable);
        }
    }

    void FinishParsing()
    {
        for (QT3DSU32 idx = 0, end = m_GenerateIdList.size(); idx < end; ++idx) {
            SIdValue &theNode(m_GenerateIdList[idx]);
            switch (theNode.getType()) {
            case IdValueTypes::StateNode:
                GenerateIdValue(*(theNode.getData<SStateNode *>()));
                break;
            case IdValueTypes::Send:
                GenerateIdValue(*(theNode.getData<SSend *>()));
                break;
            default:
                QT3DS_ASSERT(false);
                break;
            }
        }
        for (QT3DSU32 idx = 0, end = m_References.size(); idx < end; ++idx)
            *(m_References[idx].second) = ParseIDRefSecondPass(m_References[idx].first);

        for (QT3DSU32 idx = 0, end = m_SendReferences.size(); idx < end; ++idx)
            *(m_SendReferences[idx].second) = ParseSendIdSecondPass(m_SendReferences[idx].first);

        if (m_Editor) {
            for (QT3DSU32 idx = 0, end = m_ExternalTransitions.size(); idx < end; ++idx) {
                SStateNode *theNode = FindStateNode(m_ExternalTransitions[idx].first);
                if (theNode) {
                    TObjPtr transObj = m_Editor->GetOrCreate(*m_ExternalTransitions[idx].second);
                    TObjPtr stateObj = m_Editor->ToEditor(*theNode);
                    stateObj->Append("children", transObj);
                }
            }
        }
    }
    SSCXML &ParseSCXML()
    {
        IDOMReader::Scope _stateScope(m_Reader);
        SSCXML *retval = QT3DS_NEW(m_GraphAllocator, SSCXML)();

        if (m_Reader.Att("id", retval->m_Id))
            m_Context.InsertId(retval->m_Id, retval);
        const char8_t *initial;
        if (m_Reader.UnregisteredAtt("initialexpr", initial, GetStudioStateNamespace())) {
            retval->m_InitialExpr = ToGraphStr(initial);
        } else if (m_Reader.UnregisteredAtt("initial", initial)) {
            STransition *theTransition = QT3DS_NEW(m_GraphAllocator, STransition)();
            ParseIDRef(initial, theTransition->m_Target);
            theTransition->m_Parent = retval;
            retval->m_Initial = theTransition;
        }
        ParseStrAtt("name", retval->m_Name);
        const char8_t *uicVersion;
        if (!m_Reader.UnregisteredAtt("version", uicVersion, GetStudioStateNamespace()))
            retval->m_UICVersion = 0;
        else {
            StringConversion<QT3DSI32>().StrTo(uicVersion, retval->m_UICVersion);
        }
        m_UICVersion = retval->m_UICVersion;
        const char8_t *desc;
        if (m_Editor && m_Reader.UnregisteredAtt("description", desc))
            m_Editor->GetOrCreate(*retval)->SetPropertyValue("description", desc);

        const char8_t *temp;
        if (m_Reader.UnregisteredAtt("binding", temp) && AreEqual(temp, "late"))
            retval->m_Flags.SetLateBinding(true);

        ParseExtensionAttributes(retval);

        for (bool success = m_Reader.MoveToFirstChild(); success;
             success = m_Reader.MoveToNextSibling()) {
            IDOMReader::Scope _loopScope(m_Reader);
            CRegisteredString elemName = m_Reader.GetElementName();
            if (eq(elemName, SXMLName::estate))
                AppendChild(*retval, retval->m_Children, ParseState());
            else if (eq(elemName, SXMLName::eparallel))
                AppendChild(*retval, retval->m_Children, ParseParallel());
            else if (eq(elemName, SXMLName::etransition))
                AppendChild(*retval, retval->m_Children, ParseTransition());
            else if (eq(elemName, SXMLName::efinal))
                AppendChild(*retval, retval->m_Children, ParseFinal());
            else if (eq(elemName, SXMLName::einitial))
                retval->m_Initial = ParseInitial();
            else if (eq(elemName, SXMLName::edatamodel))
                retval->m_DataModel = ParseDataModel();
            else {
                if (!ParseExtensionElement(retval)) {
                    qCCritical(INTERNAL_ERROR, "Failed to parse scxml child %s", elemName.c_str());
                    QT3DS_ASSERT(false);
                }
            }
        }
        if (isTrivial(retval->m_InitialExpr)) {
            SetStateInitialTransition(retval->m_Initial, retval->m_Children, *retval);
        }
        FinishParsing();
        return *retval;
    }
    eastl::vector<SStateNode *> ParseFragment()
    {
        eastl::vector<SStateNode *> retval;

        for (bool success = m_Reader.MoveToFirstChild(); success;
             success = m_Reader.MoveToNextSibling()) {
            IDOMReader::Scope _loopScope(m_Reader);
            CRegisteredString elemName = m_Reader.GetElementName();
            if (eq(elemName, SXMLName::estate))
                retval.push_back(&ParseState());
            else if (eq(elemName, SXMLName::eparallel))
                retval.push_back(&ParseParallel());
            else if (eq(elemName, SXMLName::efinal))
                retval.push_back(&ParseFinal());
            else if (eq(elemName, SXMLName::ehistory))
                retval.push_back(&ParseHistory());
            else if (eq(elemName, SXMLName::eexternal_transition)) {
                CRegisteredString src;
                m_Reader.Att("source", src);
                if (src.IsValid())
                    m_ExternalTransitions.push_back(eastl::make_pair(src, &ParseTransition()));
            } else {
                qCCritical(INTERNAL_ERROR, "Failed to parse scxml child %s", elemName.c_str() );
                QT3DS_ASSERT(false);
            }
        }

        FinishParsing();
        return retval;
    }
};

struct SWriteContext
{
    IDOMWriter &m_Writer;
    IStateContext &m_Context;
    IStringTable &m_StringTable;
    const char8_t *m_CurrentNamespace;
    const char8_t *m_Names[SXMLName::LastName];
    eastl::string m_IdStr;
    eastl::string m_RefListWorkspace;
    IEditor *m_Editor;
    MemoryBuffer<ForwardingAllocator> m_Buffer;
    Option<QT3DSVec2> m_MousePos;
    nvvector<SStateNode *> m_WrittenNodes;
    nvvector<STransition *> m_ExternalTransitions;

    SWriteContext(IDOMWriter &inWriter, IStateContext &inContext, IStringTable &inStrTable,
                  IEditor *inEditor, NVAllocatorCallback &inAlloc)
        : m_Writer(inWriter)
        , m_Context(inContext)
        , m_StringTable(inStrTable)
        , m_CurrentNamespace(GetSCXMLNamespace())
        , m_Editor(inEditor)
        , m_Buffer(ForwardingAllocator(inAlloc, "WriteBuffer"))
        , m_WrittenNodes(inAlloc, "m_WrittenNodes")
        , m_ExternalTransitions(inAlloc, "m_ExternalTransitions")
    {
#define HANDLE_XML_ELEMENT_NAME(nm) m_Names[SXMLName::e##nm] = #nm;
        ITERATE_XML_ELEMENT_NAMES
#undef HANDLE_XML_ELEMENT_NAME
    }

    void GenerateId(SStateNode &inNode, const char8_t *stem)
    {
        if (inNode.m_Id.IsValid() == false)
            CXMLIO::GenerateUniqueId(inNode, stem, m_Context, m_StringTable);
    }

    void WriteExtensionElement(SDOMElement &elem)
    {
        IDOMWriter::Scope _elemScope(m_Writer, elem.m_Name.c_str(), elem.m_Namespace.c_str());
        for (TAttributeList::iterator iter = elem.m_Attributes.begin(),
                                      end = elem.m_Attributes.end();
             iter != end; ++iter) {
            SDOMAttribute &theAttribute(*iter);
            m_Writer.Att(theAttribute.m_Name, theAttribute.m_Value,
                         theAttribute.m_Namespace.c_str());
        }
        for (SDOMElement::TElementChildList::iterator iter = elem.m_Children.begin(),
                                                      end = elem.m_Children.end();
             iter != end; ++iter) {
            WriteExtensionElement(elem);
        }
        if (!isTrivial(elem.m_Value)) {
            m_Writer.Value(elem.m_Value);
        }
    }

    void WriteExtensionData(void *inItem)
    {
        SItemExtensionInfo *infoPtr = m_Context.GetExtensionInfo(inItem);
        if (infoPtr) {
            SItemExtensionInfo &theInfo(*infoPtr);
            for (TDOMAttributeNodeList::iterator iter = theInfo.m_ExtensionAttributes.begin(),
                                                 end = theInfo.m_ExtensionAttributes.end();
                 iter != end; ++iter) {
                SDOMAttribute &theAttribute(*iter->m_Attribute);
                m_Writer.Att(theAttribute.m_Name.c_str(), theAttribute.m_Value,
                             theAttribute.m_Namespace.c_str());
            }
            for (TDOMElementNodeList::iterator iter = theInfo.m_ExtensionNodes.begin(),
                                               end = theInfo.m_ExtensionNodes.end();
                 iter != end; ++iter) {
                WriteExtensionElement(*iter->m_Element);
            }
        }
    }

    void Att(const char8_t *inName, const char8_t *inData)
    {
        if (!isTrivial(inData))
            m_Writer.Att(inName, inData, m_CurrentNamespace);
    }

    void Att(const char8_t *inName, CRegisteredString inData)
    {
        if (inData.IsValid())
            m_Writer.Att(inName, inData.c_str(), m_CurrentNamespace);
    }

    void Att(const char8_t *inName, NVConstDataRef<CRegisteredString> inData)
    {
        m_RefListWorkspace.clear();
        for (QT3DSU32 idx = 0, end = inData.size(); idx < end; ++idx) {
            if (m_RefListWorkspace.size())
                m_RefListWorkspace.append(" ");
            m_RefListWorkspace.append(inData[idx].c_str());
        }
        if (m_RefListWorkspace.size())
            Att(inName, m_RefListWorkspace.c_str());
    }
    void Att(const char8_t *inName, NVConstDataRef<SStateNode *> inData)
    {
        m_RefListWorkspace.clear();
        for (QT3DSU32 idx = 0, end = inData.size(); idx < end; ++idx) {
            if (m_RefListWorkspace.size())
                m_RefListWorkspace.append(" ");
            if (inData[idx])
                m_RefListWorkspace.append(inData[idx]->m_Id.c_str());
        }
        if (m_RefListWorkspace.size())
            Att(inName, m_RefListWorkspace.c_str());
    }
    void Att(const char8_t *inName, const QT3DSVec2 &inData)
    {
        m_Buffer.clear();
        Char8TWriter writer(m_Buffer);
        writer.Write(NVConstDataRef<QT3DSF32>(&inData.x, 2));
        m_Buffer.writeZeros(1);
        m_Writer.Att(inName, (const char8_t *)m_Buffer.begin(), GetStudioStateNamespace());
    }

    void Att(const char8_t *inName, const QT3DSVec3 &inData)
    {
        m_Buffer.clear();
        Char8TWriter writer(m_Buffer);
        writer.Write(NVConstDataRef<QT3DSF32>(&inData.x, 3));
        m_Buffer.writeZeros(1);
        m_Writer.Att(inName, (const char8_t *)m_Buffer.begin(), GetStudioStateNamespace());
    }

    void WriteEditorAttributes(void *inType, bool inUICId, bool inAdjustPos = false)
    {
        if (m_Editor) {
            TObjPtr editorObj = m_Editor->GetEditor(inType);
            if (editorObj != NULL) {
                if (inUICId) {
                    eastl::string name = editorObj->GetId();
                    if (name.empty() == false)
                        m_Writer.Att("id", name.c_str(), GetStudioStateNamespace());
                }
                eastl::string description = editorObj->GetDescription();
                if (description.empty() == false)
                    m_Writer.Att("description", description.c_str(), GetStudioStateNamespace());

                Option<SValue> tempData = editorObj->GetPropertyValue("position");
                if (tempData.hasValue()) {
                    QT3DSVec2 thePos(tempData->getData<QT3DSVec2>());
                    if (inAdjustPos && m_MousePos.hasValue()) {
                        // Get the global pos, not the local position.
                        for (TObjPtr parentPtr = editorObj->Parent(); parentPtr;
                             parentPtr = parentPtr->Parent()) {
                            Option<SValue> parentPos = parentPtr->GetPropertyValue("position");
                            if (parentPos.hasValue())
                                thePos += parentPos->getData<QT3DSVec2>();
                        }
                        // Store pos in global coords adjusted.
                        thePos -= *m_MousePos;
                    }
                    Att("position", thePos);
                }

                tempData = editorObj->GetPropertyValue("dimension");
                if (tempData.hasValue())
                    Att("dimension", tempData->getData<QT3DSVec2>());

                tempData = editorObj->GetPropertyValue("color");
                if (tempData.hasValue())
                    Att("color", tempData->getData<QT3DSVec3>());
            }
        }
    }

    void WriteSend(SSend &inContent)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "send", m_CurrentNamespace);
        Att("event", inContent.m_Event);
        Att("eventexpr", inContent.m_EventExpr);
        Att("target", inContent.m_Target);
        Att("targetexpr", inContent.m_TargetExpr);
        Att("type", inContent.m_Type);
        Att("typeExpr", inContent.m_TypeExpr);
        Att("id", inContent.m_Id);
        Att("idlocation", inContent.m_IdLocation);
        Att("delay", inContent.m_Delay);
        Att("delayexpr", inContent.m_DelayExpr);
        Att("namelist", inContent.m_NameList);
        WriteExecutableContentList(inContent.m_Children);
        WriteEditorAttributes(&inContent, false);
        WriteExtensionData(&inContent);
    }

    void WriteParam(SParam &inContent)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "param", m_CurrentNamespace);
        Att("name", inContent.m_Name);

        if (!isTrivial(inContent.m_Expr))
            Att("expr", inContent.m_Expr);
        else if (!isTrivial(inContent.m_Location))
            Att("location", inContent.m_Location);

        WriteEditorAttributes(&inContent, false);
        WriteExtensionData(&inContent);
    }

    void WriteContent(SContent &inContent)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "content", m_CurrentNamespace);
        if (!isTrivial(inContent.m_Expr))
            Att("expr", inContent.m_Expr);
        else if (!isTrivial(inContent.m_ContentValue))
            m_Writer.Value(inContent.m_ContentValue);

        WriteEditorAttributes(&inContent, false);
        WriteExtensionData(&inContent);
    }

    void WriteRaise(SRaise &inContent)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "raise", m_CurrentNamespace);
        Att("event", inContent.m_Event);
        WriteEditorAttributes(&inContent, false);
        WriteExtensionData(&inContent);
    }

    void WriteAssign(SAssign &inContent)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "assign", m_CurrentNamespace);
        Att("location", inContent.m_Location);
        Att("expr", inContent.m_Expression);
        WriteEditorAttributes(&inContent, false);
        WriteExtensionData(&inContent);
    }

    void WriteIf(SIf &inContent)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "if", m_CurrentNamespace);
        Att("cond", inContent.m_Cond);
        WriteEditorAttributes(&inContent, false);
        WriteExecutableContentList(inContent.m_Children);
        WriteExtensionData(&inContent);
    }

    void WriteElseIf(SElseIf &inContent)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "elseif", m_CurrentNamespace);
        WriteEditorAttributes(&inContent, false);
        Att("cond", inContent.m_Cond);
        WriteExtensionData(&inContent);
    }

    void WriteElse(SElse &inContent)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "else", m_CurrentNamespace);
        WriteEditorAttributes(&inContent, false);
        WriteExtensionData(&inContent);
    }

    void WriteLog(SLog &inContent)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "log", m_CurrentNamespace);
        WriteEditorAttributes(&inContent, false);
        Att("label", inContent.m_Label);
        Att("expr", inContent.m_Expression);
        WriteExtensionData(&inContent);
    }

    void WriteScript(SScript &inContent)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "script", m_CurrentNamespace);
        WriteEditorAttributes(&inContent, false);
        if (isTrivial(inContent.m_Data) && !isTrivial(inContent.m_URL))
            Att("src", inContent.m_URL);
        else
            m_Writer.Value(inContent.m_Data);
        WriteExtensionData(&inContent);
    }

    void WriteCancel(SCancel &inContent)
    {
        // Do not serialize invalid cancel items.
        if (isTrivial(inContent.m_IdExpression) && inContent.m_Send == NULL)
            return;

        IDOMWriter::Scope _itemScope(m_Writer, "cancel", m_CurrentNamespace);
        if (!inContent.m_Send && inContent.m_IdExpression)
            Att("sendidexpr", inContent.m_IdExpression);
        else
            Att("sendid", inContent.m_Send->m_Id);

        WriteEditorAttributes(&inContent, true);
        WriteExtensionData(&inContent);
    }

    void WriteExecutableContent(SExecutableContent &inContent)
    {
        switch (inContent.m_Type) {
        case ExecutableContentTypes::Send:
            WriteSend(static_cast<SSend &>(inContent));
            break;
        case ExecutableContentTypes::Raise:
            WriteRaise(static_cast<SRaise &>(inContent));
            break;
        case ExecutableContentTypes::Assign:
            WriteAssign(static_cast<SAssign &>(inContent));
            break;
        case ExecutableContentTypes::If:
            WriteIf(static_cast<SIf &>(inContent));
            break;
        case ExecutableContentTypes::ElseIf:
            WriteElseIf(static_cast<SElseIf &>(inContent));
            break;
        case ExecutableContentTypes::Else:
            WriteElse(static_cast<SElse &>(inContent));
            break;
        case ExecutableContentTypes::Log:
            WriteLog(static_cast<SLog &>(inContent));
            break;
        case ExecutableContentTypes::Script:
            WriteScript(static_cast<SScript &>(inContent));
            break;
        case ExecutableContentTypes::Cancel:
            WriteCancel(static_cast<SCancel &>(inContent));
            break;
        case ExecutableContentTypes::Param:
            WriteParam(static_cast<SParam &>(inContent));
            break;
        case ExecutableContentTypes::Content:
            WriteContent(static_cast<SContent &>(inContent));
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
    }
    void WriteExecutableContentList(TExecutableContentList &inList)
    {
        for (TExecutableContentList::iterator contentIter = inList.begin(),
                                              contentEnd = inList.end();
             contentIter != contentEnd; ++contentIter)
            WriteExecutableContent(*contentIter);
    }

    void WriteOnEntry(SOnEntry &inItem)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "onentry", m_CurrentNamespace);
        WriteEditorAttributes(&inItem, true);
        WriteExecutableContentList(inItem.m_ExecutableContent);
        WriteExtensionData(&inItem);
    }

    void WriteOnEntryList(TOnEntryList &inList)
    {
        for (TOnEntryList::iterator iter = inList.begin(), end = inList.end(); iter != end; ++iter)
            WriteOnEntry(*iter);
    }

    void WriteOnExit(SOnExit &inItem)
    {
        IDOMWriter::Scope _itemScope(m_Writer, "onexit", m_CurrentNamespace);
        WriteEditorAttributes(&inItem, true);
        WriteExecutableContentList(inItem.m_ExecutableContent);
        WriteExtensionData(&inItem);
    }

    void WriteOnExitList(TOnExitList &inList)
    {
        for (TOnExitList::iterator iter = inList.begin(), end = inList.end(); iter != end; ++iter)
            WriteOnExit(*iter);
    }

    void WriteDataModel(SDataModel &inDataModel)
    {
        IDOMWriter::Scope _transitionScope(m_Writer, "datamodel", m_CurrentNamespace);
        WriteEditorAttributes(&inDataModel, true);
        for (TDataList::iterator iter = inDataModel.m_Data.begin(), end = inDataModel.m_Data.end();
             iter != end; ++iter) {
            IDOMWriter::Scope _transitionScope(m_Writer, "data", m_CurrentNamespace);
            Att("id", iter->m_Id);
            Att("expr", iter->m_Expression);
            WriteEditorAttributes(&(*iter), true);
            WriteExtensionData(&(*iter));
        }
        WriteExtensionData(&inDataModel);
    }

    static inline SStateNode *FirstValidChild(SState &inNode)
    {
        for (TStateNodeList::iterator iter = inNode.m_Children.begin(),
                                      end = inNode.m_Children.end();
             iter != end; ++iter) {
            switch (iter->m_Type) {
            case StateNodeTypes::State:
            case StateNodeTypes::Parallel:
            case StateNodeTypes::Final:
                return iter.m_Obj;
            default:
                break;
            }
        }
        return NULL;
    }

    void WriteState(SState &inNode, bool inAdjustPos)
    {
        m_WrittenNodes.push_back(&inNode);
        IDOMWriter::Scope _transitionScope(m_Writer, "state", m_CurrentNamespace);
        GenerateId(inNode, "state");
        Att("id", inNode.m_Id);
        WriteEditorAttributes(&inNode, false, inAdjustPos);
        if (!isTrivial(inNode.m_InitialExpr)) {
            m_Writer.Att("initialexpr", inNode.m_InitialExpr, GetStudioStateNamespace());
        } else if (inNode.m_Initial) {
            // First check to see if this could be an attribute
            if (inNode.m_Initial->m_ExecutableContent.empty()) {
                // Now check if it has one child and if it has only one child and that child is the
                // first valid possible child
                // then we don't write out the attribute
                bool canElideInitial = inNode.m_Initial->m_Target.size() == 0
                    || (inNode.m_Initial->m_Target.size() == 1
                        && inNode.m_Initial->m_Target[0] == FirstValidChild(inNode));
                if (!canElideInitial)
                    Att("initial", inNode.m_Initial->m_Target);
            } else {
                IDOMWriter::Scope _transitionScope(m_Writer, "initial", m_CurrentNamespace);
                WriteTransition(*inNode.m_Initial, false);
            }
        }
        WriteOnEntryList(inNode.m_OnEntry);
        WriteOnExitList(inNode.m_OnExit);
        WriteStateNodeList(inNode.m_Children);
        if (inNode.m_DataModel)
            WriteDataModel(*inNode.m_DataModel);
        WriteExtensionData(&inNode);
    }

    void WriteParallel(SParallel &inNode, bool inAdjustPos)
    {
        m_WrittenNodes.push_back(&inNode);
        IDOMWriter::Scope _transitionScope(m_Writer, "parallel", m_CurrentNamespace);
        GenerateId(inNode, "parallel");
        Att("id", inNode.m_Id);
        WriteEditorAttributes(&inNode, false, inAdjustPos);
        WriteOnEntryList(inNode.m_OnEntry);
        WriteOnExitList(inNode.m_OnExit);
        WriteStateNodeList(inNode.m_Children);
        if (inNode.m_DataModel)
            WriteDataModel(*inNode.m_DataModel);
        WriteExtensionData(&inNode);
    }

    void WriteHistory(SHistory &inNode, bool inAdjustPos)
    {
        m_WrittenNodes.push_back(&inNode);
        IDOMWriter::Scope _transitionScope(m_Writer, "history", m_CurrentNamespace);
        GenerateId(inNode, "history");
        Att("id", inNode.m_Id);
        if (inNode.m_Flags.IsDeep())
            Att("type", "deep");
        WriteEditorAttributes(&inNode, false, inAdjustPos);
        if (inNode.m_Transition)
            WriteTransition(*inNode.m_Transition, false);
        WriteExtensionData(&inNode);
    }
    void WriteTransitionData(STransition &inNode)
    {
        Att("event", inNode.m_Event);
        Att("cond", inNode.m_Condition);
        Att("target", inNode.m_Target);
        if (inNode.m_Flags.IsInternal())
            Att("type", "internal");
        WriteEditorAttributes(&inNode, true, false);
        if (inNode.m_StateNodeFlags.HasEndPosition()) {
            Att("end_position", inNode.m_EndPosition);
        }
        if (inNode.m_Path.mSize) {
            m_Buffer.clear();
            Char8TWriter writer(m_Buffer);
            writer.Write(
                NVConstDataRef<QT3DSF32>((const QT3DSF32 *)inNode.m_Path.mData, 2 * inNode.m_Path.mSize),
                QT3DS_MAX_U32);
            m_Buffer.writeZeros(1);
            m_Writer.Att("path", (const char8_t *)m_Buffer.begin(), GetStudioStateNamespace());
        }
        WriteExecutableContentList(inNode.m_ExecutableContent);
        WriteExtensionData(&inNode);
    }
    void WriteTransition(STransition &inNode, bool /*inAdjustPos*/)
    {
        IDOMWriter::Scope _transitionScope(m_Writer, "transition", m_CurrentNamespace);
        WriteTransitionData(inNode);
    }
    void WriteExternalTransition(STransition &inNode)
    {
        if (inNode.m_Parent != NULL) {
            IDOMWriter::Scope _transitionScope(m_Writer, "external_transition",
                                               GetStudioStateNamespace());
            m_Writer.Att("source", inNode.m_Parent->m_Id.c_str(), GetStudioStateNamespace());
            WriteTransitionData(inNode);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    void WriteFinal(SFinal &inNode, bool inAdjustPos)
    {
        m_WrittenNodes.push_back(&inNode);
        IDOMWriter::Scope _transitionScope(m_Writer, "final", m_CurrentNamespace);
        GenerateId(inNode, "history");
        Att("id", inNode.m_Id);
        WriteEditorAttributes(&inNode, false, inAdjustPos);
        WriteOnEntryList(inNode.m_OnEntry);
        WriteOnExitList(inNode.m_OnExit);
        WriteExtensionData(&inNode);
    }

    void Write(SStateNode &inNode, bool inAdjustPos)
    {
        switch (inNode.m_Type) {
        case StateNodeTypes::State:
            WriteState(static_cast<SState &>(inNode), inAdjustPos);
            break;
        case StateNodeTypes::Parallel:
            WriteParallel(static_cast<SParallel &>(inNode), inAdjustPos);
            break;
        case StateNodeTypes::History:
            WriteHistory(static_cast<SHistory &>(inNode), inAdjustPos);
            break;
        case StateNodeTypes::Transition:
            WriteTransition(static_cast<STransition &>(inNode), inAdjustPos);
            break;
        case StateNodeTypes::Final:
            WriteFinal(static_cast<SFinal &>(inNode), inAdjustPos);
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
    }
    void WriteStateNodeList(TStateNodeList &inNodeList)
    {
        for (TStateNodeList::iterator iter = inNodeList.begin(), end = inNodeList.end();
             iter != end; ++iter) {
            Write((*iter), false);
        }
    }

    void Write()
    {
        SSCXML &item = *m_Context.GetRoot();
        GenerateId(item, "scxml");
        Att("name", item.m_Name);
        if (item.m_Flags.IsLateBinding())
            Att("binding", "late");
        Att("version", "1");
        m_Writer.Att("version", SSCXML::GetCurrentUICVersion(), GetStudioStateNamespace());

        if (!isTrivial(item.m_InitialExpr)) {
            m_Writer.Att("initialexpr", item.m_InitialExpr, GetStudioStateNamespace());
        } else if (item.m_Initial)
            Att("initial", item.m_Initial->m_Target);

        WriteStateNodeList(item.m_Children);
        if (item.m_DataModel)
            WriteDataModel(*item.m_DataModel);
        WriteExtensionData(&item);
    }

    void CheckTransitionForWrittenNodesList(STransition *transition)
    {
        if (transition == NULL)
            return;
        for (QT3DSU32 targetIdx = 0, targetEnd = transition->m_Target.size(); targetIdx < targetEnd;
             ++targetIdx) {
            if (eastl::find(m_WrittenNodes.begin(), m_WrittenNodes.end(),
                            transition->m_Target[targetIdx])
                != m_WrittenNodes.end()) {
                m_ExternalTransitions.push_back(transition);
                return;
            }
        }
    }

    // Not sure the right answer here.  I know you can't just blindly work with initials and such
    // because you can't create new transitions for them.
    void CheckForExternalTransitions(SStateNode &inNode, const eastl::vector<SStateNode *> &inRoots)
    {
        switch (inNode.m_Type) {
        case StateNodeTypes::SCXML: {
            SSCXML &theNode(static_cast<SSCXML &>(inNode));
            // CheckTransitionForWrittenNodesList( theNode.m_Initial );
            CheckForExternalTransitions(theNode.m_Children, inRoots);
        } break;
        case StateNodeTypes::State: {
            SState &theNode(static_cast<SState &>(inNode));
            // CheckTransitionForWrittenNodesList( theNode.m_Initial );
            CheckForExternalTransitions(theNode.m_Children, inRoots);
        } break;
        case StateNodeTypes::Parallel: {
            SParallel &theNode(static_cast<SParallel &>(inNode));
            CheckForExternalTransitions(theNode.m_Children, inRoots);
        } break;
        case StateNodeTypes::Final:
            break;
        case StateNodeTypes::History: {
            // CheckTransitionForWrittenNodesList( theNode.m_Transition );
        } break;
        case StateNodeTypes::Transition: {
            STransition &theNode(static_cast<STransition &>(inNode));
            CheckTransitionForWrittenNodesList(&theNode);
        } break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
    }

    void CheckForExternalTransitions(TStateNodeList &ioList,
                                     const eastl::vector<SStateNode *> &inRoots)
    {
        for (TStateNodeList::iterator iter = ioList.begin(), end = ioList.end(); iter != end;
             ++iter) {
            if (eastl::find(inRoots.begin(), inRoots.end(), &(*iter)) == inRoots.end())
                CheckForExternalTransitions(*iter, inRoots);
        }
    }

    void WriteFragment(eastl::vector<SStateNode *> &inRoots, const QT3DSVec2 &inMousePos)
    {
        m_MousePos = inMousePos;
        for (QT3DSU32 idx = 0, end = inRoots.size(); idx < end; ++idx) {
            Write(*inRoots[idx], true);
        }
        if (m_Context.GetRoot())
            CheckForExternalTransitions(*m_Context.GetRoot(), inRoots);
        m_CurrentNamespace = GetStudioStateNamespace();
        for (QT3DSU32 idx = 0, end = m_ExternalTransitions.size(); idx < end; ++idx) {
            WriteExternalTransition(*m_ExternalTransitions[idx]);
        }
    }
};

// True if child is a descedent of parent.  False otherwise.
inline bool IsDescendent(SStateNode &parent, SStateNode &child)
{
    if (&parent == &child)
        return false;
    if (&parent == child.m_Parent)
        return true;

    if (child.m_Parent)
        return IsDescendent(parent, *child.m_Parent);

    return false;
}

template <typename TDataType>
void GenerateUniqueIdT(TDataType &inNode, const char8_t *inStem, IStateContext &ioContext,
                       IStringTable &ioStringTable)
{
    QT3DS_ASSERT(inNode.m_Id.IsValid() == false);
    inNode.m_Id =
        ioStringTable.RegisterStr(DoGenerateUniqueId(inStem, ioContext, ioStringTable).c_str());
    bool insertResult = ioContext.InsertId(inNode.m_Id, &inNode);
    QT3DS_ASSERT(insertResult);
    (void)insertResult;
}
}

namespace uic {
namespace state {

    void CXMLIO::GenerateUniqueId(SStateNode &inNode, const char8_t *inStem,
                                  IStateContext &ioContext, IStringTable &ioStringTable)
    {
        GenerateUniqueIdT(inNode, inStem, ioContext, ioStringTable);
    }

    void CXMLIO::GenerateUniqueId(SSend &inNode, const char8_t *inStem, IStateContext &ioContext,
                                  IStringTable &ioStringTable)
    {
        GenerateUniqueIdT(inNode, inStem, ioContext, ioStringTable);
    }

    bool CXMLIO::LoadSCXMLFile(NVAllocatorCallback &inGraphAllocator, NVFoundationBase &inFnd,
                               IDOMReader &inReader, IStringTable &inStringTable,
                               const char8_t *inFilename, IStateContext &outContext,
                               editor::IEditor *inEditor)
    {
        // the topmost scxml node is a state, so go from there.
        if (AreEqual(inReader.GetElementName().c_str(), "scxml")) {
            SParseContext theParseContext(inGraphAllocator, inFnd, inReader, outContext,
                                          inStringTable, inEditor);
            outContext.SetRoot(theParseContext.ParseSCXML());
            if (outContext.GetRoot()) {
                inFilename = nonNull(inFilename);
                outContext.GetRoot()->m_Filename = theParseContext.ToGraphStr(inFilename);
            }
            return true;
        } else {
            return false;
        }
    }

    eastl::pair<eastl::vector<SStateNode *>, CXMLIO::TIdRemapMap>
    CXMLIO::LoadSCXMLFragment(NVAllocatorCallback &inGraphAllocator, NVFoundationBase &inFnd,
                              IDOMReader &inReader, IStringTable &inStringTable,
                              IStateContext &ioContext, editor::IEditor &inEditor)
    {
        if (AreEqual(inReader.GetElementName().c_str(), "scxml_fragment")) {
            SParseContext theParseContext(inGraphAllocator, inFnd, inReader, ioContext,
                                          inStringTable, &inEditor);
            eastl::vector<SStateNode *> retval = theParseContext.ParseFragment();
            return eastl::make_pair(retval, theParseContext.m_RemapMap);
        }
        return eastl::make_pair(eastl::vector<SStateNode *>(), CXMLIO::TIdRemapMap());
    }

    void CXMLIO::SaveSCXMLFile(IStateContext &inContext, NVFoundationBase &inFnd,
                               IStringTable &inStringTable, IOutStream &outStream,
                               editor::IEditor *inEditor)
    {
        NVScopedRefCounted<IDOMFactory> theFactory =
            IDOMFactory::CreateDOMFactory(inFnd.getAllocator(), inStringTable);
        NVScopedRefCounted<IDOMWriter> theWriter =
            IDOMWriter::CreateDOMWriter(inFnd.getAllocator(), "scxml", inStringTable,
                                        GetSCXMLNamespace())
                .first;
        SWriteContext theWriteContext(*theWriter, inContext, inStringTable, inEditor,
                                      inFnd.getAllocator());
        theWriteContext.Write();

        // Now we actually serialize the data.
        eastl::vector<SNamespacePair> thePairs;
        thePairs.push_back(
            SNamespacePair(inStringTable.RegisterStr(GetSCXMLNamespace()), CRegisteredString()));
        thePairs.push_back(SNamespacePair(inStringTable.RegisterStr(GetStudioStateNamespace()),
                                          inStringTable.RegisterStr("uic")));

        for (SNamespacePairNode *theNode = inContext.GetFirstNSNode(); theNode;
             theNode = theNode->m_NextNode) {
            if (AreEqual(theNode->m_Namespace.c_str(), GetSCXMLNamespace()) == false
                && AreEqual(theNode->m_Namespace.c_str(), GetStudioStateNamespace()) == false
                && theNode->m_Abbreviation.IsValid())
                thePairs.push_back(*theNode);
        }

        CDOMSerializer::WriteXMLHeader(outStream);
        CDOMSerializer::Write(inFnd.getAllocator(), *theWriter->GetTopElement(), outStream,
                              inStringTable, toDataRef(thePairs.data(), thePairs.size()), false);
    }

    void CXMLIO::FindRoots(NVConstDataRef<SStateNode *> inObjects,
                           eastl::vector<SStateNode *> &outRoots)
    {
        // Stupid but effective algorithm.
        for (QT3DSU32 idx = 0, end = inObjects.size(); idx < end; ++idx) {
            // Transitions never make it into the root, nor does the top scxml object
            if (inObjects[idx]->m_Type == StateNodeTypes::Transition
                || inObjects[idx]->m_Type == StateNodeTypes::SCXML)
                continue;
            SStateNode *newNode(inObjects[idx]);
            // Note that re-querying size is important.
            bool skip = false;
            for (QT3DSU32 existingIdx = 0; existingIdx < outRoots.size() && skip == false;
                 ++existingIdx) {
                SStateNode *existingNode(outRoots[existingIdx]);
                if (newNode == existingNode || IsDescendent(*existingNode, *newNode))
                    skip = true;

                // If the existing item is a descendent of new node,
                // then get the existing item out of the list.
                if (IsDescendent(*newNode, *existingNode)) {
                    // Get the existing item out of the list.
                    outRoots.erase(outRoots.begin() + existingIdx);
                    --existingIdx;
                }
            }
            if (!skip)
                outRoots.push_back(newNode);
        }
    }

    eastl::vector<SStateNode *> CXMLIO::SaveSCXMLFragment(
        IStateContext &inContext, NVFoundationBase &inFnd, IStringTable &inStringTable,
        IDOMWriter &ioWriter, NVDataRef<SStateNode *> inObjects, editor::IEditor &inEditor,
        const QT3DSVec2 &inMousePos, eastl::vector<SNamespacePair> &outNamespaces)
    {
        eastl::vector<SStateNode *> theRoots;
        FindRoots(inObjects, theRoots);
        NVScopedRefCounted<IDOMWriter> theWriter(ioWriter);
        SWriteContext theWriteContext(*theWriter, inContext, inStringTable, &inEditor,
                                      inFnd.getAllocator());
        theWriteContext.WriteFragment(theRoots, inMousePos);

        outNamespaces.push_back(
            SNamespacePair(inStringTable.RegisterStr(GetSCXMLNamespace()), CRegisteredString()));
        outNamespaces.push_back(SNamespacePair(inStringTable.RegisterStr(GetStudioStateNamespace()),
                                               inStringTable.RegisterStr("uic")));
        return theRoots;
    }

    void CXMLIO::ToEditableXml(IStateContext &inContext, NVFoundationBase &inFnd,
                               IStringTable &inStringTable, IDOMWriter &ioWriter,
                               SExecutableContent &inContent, editor::IEditor &inEditor)
    {
        SWriteContext theWriteContext(ioWriter, inContext, inStringTable, &inEditor,
                                      inFnd.getAllocator());
        theWriteContext.WriteExecutableContent(inContent);
    }

    SExecutableContent *
    CXMLIO::FromEditableXML(IDOMReader &inReader, NVFoundationBase &inFnd, IStateContext &inContext,
                            IStringTable &inStringTable, NVAllocatorCallback &inGraphAllocator,
                            editor::IEditor &inEditor, SStateNode *inStateNodeParent,
                            SExecutableContent *inExecContentParent)
    {
        SParseContext theParseContext(inGraphAllocator, inFnd, inReader, inContext, inStringTable,
                                      &inEditor);
        TExecutableContentList theContentList;
        theParseContext.ParseExecutableContentItem(inStateNodeParent, inExecContentParent,
                                                   theContentList);
        return &(*theContentList.begin());
    }

    eastl::vector<eastl::string> CXMLIO::GetSupportedExecutableContentNames()
    {
        eastl::vector<eastl::string> retval;
        NVConstDataRef<SParseContext::SExecutableContentParseBinding> theBindingList =
            SParseContext::GetParseBindings();
        for (QT3DSU32 idx = 0, end = theBindingList.size(); idx < end; ++idx)
            retval.push_back(
                eastl::string(SXMLName::GetNameForElemName(theBindingList[idx].m_Name)));
        return retval;
    }
}
}

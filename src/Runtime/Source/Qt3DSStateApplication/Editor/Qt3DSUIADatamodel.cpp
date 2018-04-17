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
#include "Qt3DSUIADatamodel.h"
#include "foundation/IOStreams.h"
#include "foundation/XML.h"
#include "foundation/FileTools.h"
#include "Qt3DSStateEditorFoundation.h"
#include "Qt3DSStateApplication.h"
#include "Qt3DSRenderInputStreamFactory.h"
#include "EASTL/map.h"
#include "EASTL/sort.h"
#include "Qt3DSStateEditorTransactionImpl.h"
#include "Qt3DSUIADatamodelValue.h"
#include "foundation/StringConversion.h"
#include "foundation/StringConversionImpl.h"
#include "Qt3DSDMStringTable.h"

using namespace qt3ds::app;
using qt3ds::render::IInputStreamFactory;

namespace qt3ds {
namespace app {

    struct ElementSubTypes
    {
        enum Enum {
            NoSubType = 0,
            Component = 1,
            Behavior = 2,
        };
    };

    struct SAppElement
    {
        TEditorStr m_Path;
        TEditorStr m_Type;
        TEditorStr m_Name;
        TEditorStr m_Id;
        Q3DStudio::TAttOrArgList m_Attributes;
        Q3DStudio::TVisualEventList m_VisualEvents;
        eastl::vector<NVScopedRefCounted<SAppElement>> m_Children;
        eastl::vector<SDatamodelValue> m_InitialValues;

    private:
        QT3DSI32 m_RefCount;

    public:
        SAppElement()
            : m_RefCount(0)
        {
        }

        virtual ~SAppElement() {}

        void addRef() { ++m_RefCount; }

        void release()
        {
            --m_RefCount;
            if (m_RefCount <= 0)
                delete this;
        }

        virtual ElementSubTypes::Enum GetSubType() { return ElementSubTypes::NoSubType; }
    };
}
}

namespace {

typedef eastl::basic_string<char8_t> TStrType;
struct IPropertyParser
{
    virtual ~IPropertyParser() {}
    virtual Option<TStrType> ParseStr(const char8_t *inName) = 0;
    virtual Option<QT3DSF32> ParseFloat(const char8_t *inName) = 0;
    virtual Option<QT3DSVec2> ParseVec2(const char8_t *inName) = 0;
    virtual Option<QT3DSVec3> ParseVec3(const char8_t *inName) = 0;
    virtual Option<bool> ParseBool(const char8_t *inName) = 0;
    virtual Option<QT3DSU32> ParseU32(const char8_t *inName) = 0;
    virtual Option<QT3DSI32> ParseI32(const char8_t *inName) = 0;
};

struct SMetaPropertyParser : public IPropertyParser
{
    Q3DStudio::IRuntimeMetaData &m_MetaData;
    IStringTable &m_StringTable;
    TStrType m_TempStr;
    qt3ds::foundation::CRegisteredString m_Type;
    qt3ds::foundation::CRegisteredString m_ClassId;

    SMetaPropertyParser(const char8_t *inType, const char8_t *inClass,
                        Q3DStudio::IRuntimeMetaData &inMeta, IStringTable &stringTable)
        : m_MetaData(inMeta)
        , m_StringTable(stringTable)
        , m_Type(stringTable.RegisterStr(inType))
        , m_ClassId(stringTable.RegisterStr(inClass))
    {
    }

    qt3ds::foundation::CRegisteredString Register(const char8_t *inName)
    {
        return m_StringTable.RegisterStr(inName);
    }

    virtual Option<TStrType> ParseStr(const char8_t *inName)
    {
        qt3ds::foundation::CRegisteredString theName(Register(inName));
        Q3DStudio::ERuntimeDataModelDataType theType(
            m_MetaData.GetPropertyType(m_Type, theName, m_ClassId));
        if (theType != Q3DStudio::ERuntimeDataModelDataTypeObjectRef
            && theType != Q3DStudio::ERuntimeDataModelDataTypeLong4) {
            return m_MetaData.GetPropertyValueString(m_Type, theName, m_ClassId);
        }
        return Empty();
    }
    virtual Option<QT3DSF32> ParseFloat(const char8_t *inName)
    {
        return m_MetaData.GetPropertyValueFloat(m_Type, Register(inName), m_ClassId);
    }
    virtual Option<QT3DSVec2> ParseVec2(const char8_t *inName)
    {
        Option<qt3ds::QT3DSVec3> theProperty =
            m_MetaData.GetPropertyValueVector2(m_Type, Register(inName), m_ClassId);
        if (theProperty.hasValue()) {
            return QT3DSVec2(theProperty->x, theProperty->y);
        }
        return Empty();
    }
    virtual Option<QT3DSVec3> ParseVec3(const char8_t *inName)
    {
        Option<qt3ds::QT3DSVec3> theProperty =
            m_MetaData.GetPropertyValueVector3(m_Type, Register(inName), m_ClassId);
        if (theProperty.hasValue()) {
            return *theProperty;
        }
        return Empty();
    }
    virtual Option<bool> ParseBool(const char8_t *inName)
    {
        return m_MetaData.GetPropertyValueBool(m_Type, Register(inName), m_ClassId);
    }

    virtual Option<QT3DSU32> ParseU32(const char8_t *inName)
    {
        Option<long> retval = m_MetaData.GetPropertyValueLong(m_Type, Register(inName), m_ClassId);
        if (retval.hasValue())
            return (QT3DSU32)retval.getValue();
        return Empty();
    }

    virtual Option<QT3DSI32> ParseI32(const char8_t *inName)
    {
        Option<long> retval = m_MetaData.GetPropertyValueLong(m_Type, Register(inName), m_ClassId);
        if (retval.hasValue())
            return (QT3DSI32)retval.getValue();
        return Empty();
    }
};

struct SDomReaderPropertyParser : public IPropertyParser
{
    IDOMReader &m_Reader;
    nvvector<char8_t> &m_TempBuf;
    MemoryBuffer<ForwardingAllocator> &m_ReadBuffer;

    SDomReaderPropertyParser(IDOMReader &reader, nvvector<char8_t> &inTempBuf,
                             MemoryBuffer<ForwardingAllocator> &readBuf)
        : m_Reader(reader)
        , m_TempBuf(inTempBuf)
        , m_ReadBuffer(readBuf)
    {
    }
    virtual Option<TStrType> ParseStr(const char8_t *inName)
    {
        const char8_t *retval;
        if (m_Reader.UnregisteredAtt(inName, retval))
            return TStrType(retval);
        return Empty();
    }
    virtual Option<QT3DSF32> ParseFloat(const char8_t *inName)
    {
        QT3DSF32 retval;
        if (m_Reader.Att(inName, retval))
            return retval;
        return Empty();
    }
    virtual Option<QT3DSVec2> ParseVec2(const char8_t *inName)
    {
        QT3DSVec2 retval;
        const char8_t *tempVal;

        if (m_Reader.UnregisteredAtt(inName, tempVal)) {
            eastl::string tempBuffer(tempVal);
            Char8TReader theReader(const_cast<char *>(tempBuffer.c_str()), m_ReadBuffer);
            NVDataRef<QT3DSF32> theDataRef(toDataRef(&retval.x, 2));
            theReader.ReadRef(theDataRef);
            return retval;
        }
        return Empty();
    }
    virtual Option<QT3DSVec3> ParseVec3(const char8_t *inName)
    {
        QT3DSVec3 retval;
        const char8_t *tempVal;
        if (m_Reader.UnregisteredAtt(inName, tempVal)) {
            eastl::string tempBuffer(tempVal);
            Char8TReader theReader(const_cast<char *>(tempBuffer.c_str()), m_ReadBuffer);
            NVDataRef<QT3DSF32> theDataRef(toDataRef(&retval.x, 3));
            theReader.ReadRef(theDataRef);
            return retval;
        }
        return Empty();
    }
    virtual Option<bool> ParseBool(const char8_t *inName)
    {
        bool retval;
        if (m_Reader.Att(inName, retval))
            return retval;
        return Empty();
    }

    virtual Option<QT3DSU32> ParseU32(const char8_t *inName)
    {
        QT3DSU32 retval;
        if (m_Reader.Att(inName, retval))
            return retval;
        return Empty();
    }

    virtual Option<QT3DSI32> ParseI32(const char8_t *inName)
    {
        QT3DSI32 retval;
        if (m_Reader.Att(inName, retval))
            return retval;
        return Empty();
    }
};

template <typename TDataType>
struct SParserHelper
{
};
template <>
struct SParserHelper<TStrType>
{
    static Option<TStrType> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseStr(inName);
    }
};
template <>
struct SParserHelper<QT3DSF32>
{
    static Option<QT3DSF32> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseFloat(inName);
    }
};
template <>
struct SParserHelper<QT3DSVec2>
{
    static Option<QT3DSVec2> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseVec2(inName);
    }
};
template <>
struct SParserHelper<QT3DSVec3>
{
    static Option<QT3DSVec3> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseVec3(inName);
    }
};
template <>
struct SParserHelper<bool>
{
    static Option<bool> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseBool(inName);
    }
};
template <>
struct SParserHelper<QT3DSU32>
{
    static Option<QT3DSU32> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseU32(inName);
    }
};
template <>
struct SParserHelper<QT3DSI32>
{
    static Option<QT3DSI32> Parse(const char8_t *inName, IPropertyParser &inParser)
    {
        return inParser.ParseI32(inName);
    }
};

struct SSlideInfo
{
    TEditorStr m_Name;
};

struct SAppComponent : public SAppElement
{
    TEditorStrList m_Slides;
    virtual ElementSubTypes::Enum GetSubType() { return ElementSubTypes::Component; }
};

struct SAppBehavior : public SAppElement
{
    Q3DStudio::THandlerList m_Handlers;
    virtual ElementSubTypes::Enum GetSubType() { return ElementSubTypes::Behavior; }
};

struct SVSEditorObject;

struct SVSEditor
{
    TFoundationPtr m_Foundation;
    NVScopedRefCounted<STransactionManagerImpl> m_TransactionManager;
    NVScopedRefCounted<IStringTable> m_StringTable;
    SVSEditor(TFoundationPtr fnd, IStringTable &inStringTable)
        : m_Foundation(fnd)
        , m_TransactionManager(QT3DS_NEW(m_Foundation->getAllocator(), STransactionManagerImpl)(fnd))
        , m_StringTable(inStringTable)
    {
    }

    virtual void RemoveObjectFromGraph(SVSEditorObject &inObj) = 0;
    STransaction *GetOpenTransactionImpl()
    {
        return m_TransactionManager->GetOpenTransactionImpl();
    }
};

struct SVSEditorObject : public IEditorObject
{
    TFoundationPtr m_Foundation;
    SVSEditor &m_Editor;
    TObjPtr m_ParentObject;
    TPropertyAccessorList m_PropertyAccessors;
    QT3DSI32 mRefCount;
    SVSEditorObject(SVSEditor &editor, TObjPtr parent, const char8_t *typeName,
                    const TPropertyAccessorList &ioList)
        : IEditorObject(typeName)
        , m_Foundation(editor.m_Foundation)
        , m_Editor(editor)
        , m_ParentObject(parent)
        , m_PropertyAccessors(ioList)
        , mRefCount(0)
    {
    }

    virtual TEditorStr GetId() { return TEditorStr(); }
    virtual TEditorStr GetDescription() { return TEditorStr(); }
    virtual void SetId(const TEditorStr &) { QT3DS_ASSERT(false); }
    virtual void SetDescription(const TEditorStr &) { QT3DS_ASSERT(false); }

    virtual TObjPtr Parent() { return m_ParentObject; }

    IPropertyAccessor *FindPropertyAccessor(const char8_t *inName)
    {
        CRegisteredString nameStr = m_Editor.m_StringTable->RegisterStr(inName);
        for (size_t idx = 0, end = m_PropertyAccessors.size(); idx < end; ++idx)
            if (m_PropertyAccessors[idx]->m_Declaration.m_Name == nameStr)
                return m_PropertyAccessors[idx].mPtr;
        return NULL;
    }

    virtual void GetProperties(eastl::vector<SPropertyDeclaration> &outProperties)
    {
        outProperties.clear();
        for (size_t idx = 0, end = m_PropertyAccessors.size(); idx < end; ++idx)
            outProperties.push_back(m_PropertyAccessors[idx]->m_Declaration);
    }

    virtual Option<SPropertyDeclaration> FindProperty(const char8_t *propName)
    {
        for (size_t idx = 0, end = m_PropertyAccessors.size(); idx < end; ++idx) {
            if (AreEqual(m_PropertyAccessors[idx]->m_Declaration.m_Name.c_str(), propName))
                return m_PropertyAccessors[idx]->m_Declaration;
        }
        return Empty();
    }

    virtual eastl::vector<CRegisteredString> GetLegalValues(const char8_t *)
    {
        return eastl::vector<CRegisteredString>();
    }

    virtual Option<SValue> GetPropertyValue(const char8_t *inPropName)
    {
        IPropertyAccessor *accessor = FindPropertyAccessor(inPropName);
        if (accessor) {
            return accessor->Get(*this);
        }
        return Empty();
    }
    virtual void SetPropertyValue(const char8_t *inPropName, const SValueOpt &inValue)
    {
        IPropertyAccessor *accessor = FindPropertyAccessor(inPropName);
        if (accessor) {
            STransaction *theTransaction = m_Editor.GetOpenTransactionImpl();
            if (theTransaction) {
                Option<SValue> existing = accessor->Get(*this);
                theTransaction->m_Changes.push_back(
                    new SChange(existing, inValue, *accessor, *this));
            }
            accessor->Set(*this, inValue);
        } else {
            QT3DS_ASSERT(false);
        }
    }

    virtual void RemoveObjectFromGraph() { m_Editor.RemoveObjectFromGraph(*this); }
    virtual void RemoveIdFromContext() {}
    virtual void AddIdToContext() {}
    virtual IEditor &GetEditor() { return m_ParentObject->GetEditor(); }
};

template <typename TEditorType>
struct SGenericPropertyStringAccessor : public SPropertyAccessorBase<TEditorType>
{
    typedef TEditorStr TDataType;
    typedef TDataType TEditorType::*TPropertyPtr;
    typedef SPropertyAccessorBase<TEditorType> TBaseType;

    TPropertyPtr m_Property;

    SGenericPropertyStringAccessor(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec,
                                   TPropertyPtr inPtr)
        : TBaseType(inAlloc, inDec)
        , m_Property(inPtr)
    {
    }

    QT3DS_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

    virtual Option<SValue> DoGet(TEditorType &inObj)
    {
        TDataType &dtype = inObj.*(this->m_Property);
        return SValue(dtype);
    }

    virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
    {
        TDataType &dtype = inObj.*(this->m_Property);
        dtype.clear();
        if (inValue.hasValue() && inValue->getType() == ValueTypes::String)
            dtype = inValue->getData<TEditorStr>();
    }
};

template <typename TEditorType>
struct SGenericPropertyStringFunctionAccessor : public SPropertyAccessorBase<TEditorType>
{
    typedef TEditorStr TDataType;
    typedef TDataType (TEditorType::*TGetter)() const;
    typedef void (TEditorType::*TSetter)(const TDataType &);
    typedef SPropertyAccessorBase<TEditorType> TBaseType;

    TGetter m_Getter;
    TSetter m_Setter;

    SGenericPropertyStringFunctionAccessor(TFoundationPtr inAlloc,
                                           const SPropertyDeclaration &inDec, TGetter inGetter,
                                           TSetter inSetter)
        : TBaseType(inAlloc, inDec)
        , m_Getter(inGetter)
        , m_Setter(inSetter)
    {
    }
    QT3DS_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

    virtual Option<SValue> DoGet(TEditorType &inObj)
    {
        TDataType dtype = (inObj.*m_Getter)();
        return SValue(dtype);
    }

    virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
    {
        TDataType dtype;
        dtype.clear();
        if (inValue.hasValue() && inValue->getType() == ValueTypes::String)
            dtype = inValue->getData<TEditorStr>();

        (inObj.*m_Setter)(dtype);
    }
};

struct SGotoSlideEditor : public SVSEditorObject
{
    TEditorStr m_Component;
    TEditorStr m_Slide;
    TEditorStr m_Rel;
    TEditorStr m_Wrap;
    TEditorStr m_Direction;
    TEditorStr m_State;
    TEditorStr m_Mode;
    TEditorStr m_PlayThroughTo;
    TEditorStr m_Rate;
    TEditorStr m_Time;
    static void CreateProperties(TPropertyAccessorList &ioList, SVSEditor &editor)
    {
        if (ioList.size())
            return;

        typedef SGenericPropertyStringAccessor<SGotoSlideEditor> TStrProp;
        typedef SGenericPropertyStringFunctionAccessor<SGotoSlideEditor> TStrFunProp;
        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("element"),
                                                      EditorPropertyTypes::String),
            &SGotoSlideEditor::m_Component));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrFunProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("slide"),
                                                      EditorPropertyTypes::String),
            &SGotoSlideEditor::GetSlide, &SGotoSlideEditor::SetSlide));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrFunProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("rel"),
                                                      EditorPropertyTypes::String),
            &SGotoSlideEditor::GetRel, &SGotoSlideEditor::SetRel));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("wrap"),
                                                      EditorPropertyTypes::String),
            &SGotoSlideEditor::m_Wrap));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation,
            SPropertyDeclaration(editor.m_StringTable->RegisterStr("direction"),
                                 EditorPropertyTypes::String),
            &SGotoSlideEditor::m_Direction));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("state"),
                                                      EditorPropertyTypes::String),
            &SGotoSlideEditor::m_State));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("mode"),
                                                      EditorPropertyTypes::String),
            &SGotoSlideEditor::m_Mode));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation,
            SPropertyDeclaration(editor.m_StringTable->RegisterStr("playthroughto"),
                                 EditorPropertyTypes::String),
            &SGotoSlideEditor::m_PlayThroughTo));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("rate"),
                                                      EditorPropertyTypes::String),
            &SGotoSlideEditor::m_Rate));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("time"),
                                                      EditorPropertyTypes::String),
            &SGotoSlideEditor::m_Time));
    }

    SGotoSlideEditor(SVSEditor &editor, TObjPtr parent, const TPropertyAccessorList &ioList)
        : SVSEditorObject(editor, parent, ElementName(), ioList)
    {
    }

    QT3DS_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Foundation, this->mRefCount);

    static const char8_t *ElementName() { return "goto-slide"; }

    void SetSlide(const TEditorStr &inSlide)
    {
        if (inSlide.empty() == false) {
            if (m_Editor.GetOpenTransactionImpl())
                SetPropertyValue("rel", SValueOpt());
            else
                m_Rel.clear();
        }
        m_Slide = inSlide;
    }

    TEditorStr GetSlide() const { return m_Slide; }

    void SetRel(const TEditorStr &inRel)
    {
        if (inRel.empty() == false) {
            if (m_Editor.GetOpenTransactionImpl())
                SetPropertyValue("slide", SValueOpt());
            else
                m_Slide.clear();
        }
        m_Rel = inRel;
    }

    TEditorStr GetRel() const { return m_Rel; }
};

struct SRunHandlerEditor : public SVSEditorObject
{
    TEditorStr m_Behavior;
    TEditorStr m_Handler;
    TEditorStr m_ArgumentStr;
    static void CreateProperties(TPropertyAccessorList &ioList, SVSEditor &editor)
    {
        if (ioList.size())
            return;

        typedef SGenericPropertyStringAccessor<SRunHandlerEditor> TStrProp;
        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("element"),
                                                      EditorPropertyTypes::String),
            &SRunHandlerEditor::m_Behavior));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("handler"),
                                                      EditorPropertyTypes::String),
            &SRunHandlerEditor::m_Handler));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation,
            SPropertyDeclaration(editor.m_StringTable->RegisterStr("arguments"),
                                 EditorPropertyTypes::String),
            &SRunHandlerEditor::m_ArgumentStr));
    }
    SRunHandlerEditor(SVSEditor &editor, TObjPtr parent, const TPropertyAccessorList &ioList)
        : SVSEditorObject(editor, parent, ElementName(), ioList)
    {
    }
    QT3DS_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Foundation, this->mRefCount);
    static const char8_t *ElementName() { return "call"; }
};

struct SSetAttributeEditor : public SVSEditorObject
{
    TEditorStr m_Element;
    TEditorStr m_Attribute;
    TEditorStr m_Value;

    static void CreateProperties(TPropertyAccessorList &ioList, SVSEditor &editor)
    {
        if (ioList.size())
            return;

        typedef SGenericPropertyStringAccessor<SSetAttributeEditor> TStrProp;
        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("element"),
                                                      EditorPropertyTypes::String),
            &SSetAttributeEditor::m_Element));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation,
            SPropertyDeclaration(editor.m_StringTable->RegisterStr("attribute"),
                                 EditorPropertyTypes::String),
            &SSetAttributeEditor::m_Attribute));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("value"),
                                                      EditorPropertyTypes::String),
            &SSetAttributeEditor::m_Value));
    }
    SSetAttributeEditor(SVSEditor &editor, TObjPtr parent, const TPropertyAccessorList &ioList)
        : SVSEditorObject(editor, parent, ElementName(), ioList)
    {
    }
    QT3DS_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Foundation, this->mRefCount);
    static const char8_t *ElementName() { return "set-attribute"; }
};

struct SFireEventEditor : public SVSEditorObject
{
    TEditorStr m_Element;
    TEditorStr m_Event;

    static void CreateProperties(TPropertyAccessorList &ioList, SVSEditor &editor)
    {
        if (ioList.size())
            return;

        typedef SGenericPropertyStringAccessor<SFireEventEditor> TStrProp;
        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("element"),
                                                      EditorPropertyTypes::String),
            &SFireEventEditor::m_Element));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("event"),
                                                      EditorPropertyTypes::String),
            &SFireEventEditor::m_Event));
    }
    SFireEventEditor(SVSEditor &editor, TObjPtr parent, const TPropertyAccessorList &ioList)
        : SVSEditorObject(editor, parent, ElementName(), ioList)
    {
    }
    QT3DS_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Foundation, this->mRefCount);
    static const char8_t *ElementName() { return "fire-event"; }
};

struct SSetPresentationEditor : public SVSEditorObject
{
    TEditorStr m_Ref;
    TEditorStr m_Attribute;
    TEditorStr m_Value;

    static void CreateProperties(TPropertyAccessorList &ioList, SVSEditor &editor)
    {
        if (ioList.size())
            return;

        typedef SGenericPropertyStringAccessor<SSetPresentationEditor> TStrProp;
        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("ref"),
                                                      EditorPropertyTypes::String),
            &SSetPresentationEditor::m_Ref));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation,
            SPropertyDeclaration(editor.m_StringTable->RegisterStr("attribute"),
                                 EditorPropertyTypes::String),
            &SSetPresentationEditor::m_Attribute));

        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("value"),
                                                      EditorPropertyTypes::String),
            &SSetPresentationEditor::m_Value));
    }
    SSetPresentationEditor(SVSEditor &editor, TObjPtr parent, const TPropertyAccessorList &ioList)
        : SVSEditorObject(editor, parent, ElementName(), ioList)
    {
    }
    QT3DS_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Foundation, this->mRefCount);
    static const char8_t *ElementName() { return "set-presentation"; }
};

struct SPlaySoundEditor : public SVSEditorObject
{
    TEditorStr m_FilePath;

    static void CreateProperties(TPropertyAccessorList &ioList, SVSEditor &editor)
    {
        if (ioList.size())
            return;

        typedef SGenericPropertyStringAccessor<SPlaySoundEditor> TStrProp;
        ioList.push_back(QT3DS_NEW(editor.m_Foundation->getAllocator(), TStrProp)(
            editor.m_Foundation, SPropertyDeclaration(editor.m_StringTable->RegisterStr("file"),
                                                      EditorPropertyTypes::String),
            &SPlaySoundEditor::m_FilePath));
    }
    SPlaySoundEditor(SVSEditor &editor, TObjPtr parent, const TPropertyAccessorList &ioList)
        : SVSEditorObject(editor, parent, ElementName(), ioList)
    {
    }
    QT3DS_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Foundation, this->mRefCount);
    static const char8_t *ElementName() { return "play-sound"; }
};

struct SVSEntry
{
    QT3DSI32 mRefCount;
    InterpreterEventTypes::Enum m_EventType;
    TObjList m_Editors;

    SVSEntry(InterpreterEventTypes::Enum evtType = InterpreterEventTypes::UnknownInterpreterEvent)
        : mRefCount(0)
        , m_EventType(evtType)
    {
    }
    void addRef() { ++mRefCount; }
    void release()
    {
        --mRefCount;
        if (mRefCount <= 0)
            delete this;
    }
};

struct SVSEntryListChange : public IChange
{
    TObjPtr m_Object;
    NVScopedRefCounted<SVSEntry> m_Entry;
    QT3DSI32 m_Index;
    bool m_AddOnDo;
    TObjPtr m_EditorObj;
    QT3DSI32 m_RefCount;

    SVSEntryListChange(TObjPtr inObj, TObjPtr inPositionObj, SVSEntry &entry, bool addOnDo,
                       TObjPtr inEditorObj)
        : m_Object(inObj)
        , m_Entry(entry)
        , m_AddOnDo(addOnDo)
        , m_EditorObj(inEditorObj)
        , m_RefCount(0)
    {
        TObjList::iterator iter =
            eastl::find(entry.m_Editors.begin(), entry.m_Editors.end(), inPositionObj);
        m_Index = iter - entry.m_Editors.begin();
    }
    void addRef() { ++m_RefCount; }
    void release()
    {
        --m_RefCount;
        if (m_RefCount <= 0)
            delete this;
    }
    void add() { m_Entry->m_Editors.insert(m_Entry->m_Editors.begin() + m_Index, m_Object); }
    void remove()
    {
        TObjList::iterator iter =
            eastl::find(m_Entry->m_Editors.begin(), m_Entry->m_Editors.end(), m_Object);
        if (iter != m_Entry->m_Editors.end())
            m_Entry->m_Editors.erase(iter);
        else {
            QT3DS_ASSERT(false);
        }
    }

    virtual void Do()
    {
        if (m_AddOnDo)
            add();
        else
            remove();
    }
    virtual void Undo()
    {
        if (m_AddOnDo)
            remove();
        else
            add();
    }
    virtual TObjPtr GetEditor() { return m_EditorObj; }
};

typedef eastl::pair<NVScopedRefCounted<SVSEntry>, NVScopedRefCounted<SVSEntry>> TEntryExitPair;
typedef eastl::map<TEditorStr, TEntryExitPair> TIdEntryExitMap;
typedef eastl::map<TEditorStr, TIdEntryExitMap> TIdStateMapMap;

struct SIdEntryExitDeleteChange : public IChange
{
    TIdEntryExitMap &m_Map;
    TEditorStr m_Id;
    TEntryExitPair m_Data;
    QT3DSI32 m_RefCount;
    TObjPtr m_EditorObject;
    bool m_RemoveOnDo;
    SIdEntryExitDeleteChange(TIdEntryExitMap &inMap, const TEditorStr &inId, TObjPtr inObj,
                             bool removeOnDo = true)
        : m_Map(inMap)
        , m_Id(inId)
        , m_RefCount(0)
        , m_EditorObject(inObj)
        , m_RemoveOnDo(removeOnDo)
    {
        m_Data = m_Map[m_Id];
    }
    virtual void addRef() { ++m_RefCount; }
    virtual void release()
    {
        --m_RefCount;
        if (m_RefCount <= 0)
            delete this;
    }
    void remove()
    {
        TIdEntryExitMap::iterator iter = m_Map.find(m_Id);
        if (iter != m_Map.end())
            m_Map.erase(iter);
        else {
            QT3DS_ASSERT(false);
        }
    }
    void insert() { m_Map[m_Id] = m_Data; }
    virtual void Do()
    {
        if (m_RemoveOnDo)
            remove();
        else
            insert();
    }
    virtual void Undo()
    {
        if (m_RemoveOnDo)
            insert();
        else
            remove();
    }
    virtual TObjPtr GetEditor() { return m_EditorObject; }
};

struct SEditorEntry
{
    TEditorPtr m_Editor;
    TEditorStr m_Id;
    TEditorStr m_FilePath;
};

template <typename TDataType>
SDatamodelValue ParseInitialProperty(const char8_t *inName, IPropertyParser &metaParser,
                                     IPropertyParser &runtimeParser)
{
    Option<TDataType> val = SParserHelper<TDataType>::Parse(inName, runtimeParser);
    if (val.isEmpty())
        val = SParserHelper<TDataType>::Parse(inName, metaParser);
    if (val.hasValue())
        return SDatamodelValue(*val);
    return SDatamodelValue();
}

template <typename TDataType>
SDatamodelValue ParseSlideProperty(const char8_t *inName, IPropertyParser &runtimeParser)
{
    Option<TDataType> val = SParserHelper<TDataType>::Parse(inName, runtimeParser);
    if (val.hasValue())
        return *val;
    return SDatamodelValue();
}

struct DatamodelImpl : public IDatamodel,
                       public ITransManagerImplListener,
                       public SVSEditor,
                       public IEditorCopyPasteListener
{
    TEditorStr m_FilePath;
    QT3DSI32 mRefCount;
    NVScopedRefCounted<IInputStreamFactory> m_InputStreamFactory;
    eastl::pair<NVScopedRefCounted<IDOMWriter>, NVScopedRefCounted<IDOMReader>> m_UIADocument;
    nvvector<SNamespacePair> m_UIANamespaces;
    bool m_Dirty;
    nvvector<NVScopedRefCounted<SAppElement>> m_Elements;
    NVScopedRefCounted<SAppElement> m_FinderElement;
    nvvector<SEditorEntry> m_Editors;
    TIdStateMapMap m_IdToStateMaps;
    eastl::vector<SPresentation> m_Presentations;
    nvvector<char8_t> m_ParseBuf;
    Q3DStudio::TAttOrArgList m_SlideProperties;
    eastl::string m_LoadingErrorString;
    IStateMachineEditorManager &m_StateMachineEditorManager;

    DatamodelImpl(TFoundationPtr inFoundation, const TEditorStr &inPath, const TEditorStr &inAppDir,
                  IStateMachineEditorManager &inStateMachineEditorManager,
                  IStringTable &inStringTable)
        : SVSEditor(inFoundation, inStringTable)
        , m_FilePath(inPath)
        , mRefCount(0)
        , m_InputStreamFactory(IInputStreamFactory::Create(m_Foundation->getFoundation()))
        , m_UIANamespaces(m_Foundation->getAllocator(), "m_UIANamespaces")
        , m_Dirty(false)
        , m_Elements(m_Foundation->getAllocator(), "m_Elements")
        , m_Editors(m_Foundation->getAllocator(), "m_Editors")
        , m_ParseBuf(inFoundation->getAllocator(), "tempbuf")
        , m_StateMachineEditorManager(inStateMachineEditorManager)
    {
        TEditorStr ext;
        CFileTools::GetExtension(m_FilePath.c_str(), ext);
        if (ext.comparei("uia") != 0) {
            m_FilePath = IApplication::GetLaunchFile(inPath.c_str());
        }
        // Check extension, if it isn't what we expect then just set it to uia
        ext.clear();
        CFileTools::GetExtension(m_FilePath.c_str(), ext);
        if (ext.comparei("uia") != 0)
            CFileTools::SetExtension(m_FilePath, "uia");

        m_InputStreamFactory->AddSearchDirectory(inAppDir.c_str());

        m_TransactionManager->m_ObjListener = this;
    }

    ~DatamodelImpl() {}

    void addRef() { atomicIncrement(&mRefCount); }

    void release()
    {
        TFoundationPtr tempFoundation(m_Foundation);
        atomicDecrement(&mRefCount);
        if (mRefCount <= 0) {
            NVDelete(m_Foundation->getAllocator(), this);
        }
    }
    typedef eastl::map<TEditorStr, Q3DStudio::THandlerList> TIdHandlerMap;
    typedef eastl::map<TEditorStr, Q3DStudio::TVisualEventList> TIdVisualEventMap;
    typedef eastl::map<TEditorStr, NVScopedRefCounted<SAppElement>> TIdElemMap;

    SAppElement &ParseElement(IDOMReader &inReader, Q3DStudio::IRuntimeMetaData &inMetaData,
                              eastl::vector<NVScopedRefCounted<SAppElement>> &ioChildList,
                              TIdElemMap &ioElemMap)
    {
        IDOMReader::Scope __elemScope(inReader);
        const char8_t *className = inReader.GetElementName();
        const char8_t *classRef;
        inReader.UnregisteredAtt("class", classRef);
        const char8_t *id;
        inReader.UnregisteredAtt("id", id);
        Q3DStudio::SElementInfo elemInfo = inMetaData.LoadElement(className, classRef, id);
        NVScopedRefCounted<SAppElement> newElem;
        if (elemInfo.m_ClassName.compare("Behavior") == 0) {
            SAppBehavior *behavior = new SAppBehavior();
            behavior->m_Handlers = inMetaData.GetCustomHandlers(id);
            newElem = behavior;
        } else if (elemInfo.m_IsComponent) {
            SAppComponent *component = new SAppComponent();
            newElem = component;
        } else {
            newElem = new SAppElement();
        }
        newElem->m_Attributes = elemInfo.m_Attributes;
        newElem->m_VisualEvents = inMetaData.GetVisualEvents(id);
        newElem->m_Type = elemInfo.m_ClassName;
        newElem->m_Id = id;
        const char8_t *elemName;
        if (inReader.UnregisteredAtt("name", elemName))
            newElem->m_Name.assign(elemName);
        else {
            Q3DStudio::TRuntimeMetaDataStrType str = inMetaData.GetPropertyValueString(
                inMetaData.Register(className), inMetaData.Register("name"),
                inMetaData.Register(id));
            newElem->m_Name = str;
        }

        ioChildList.push_back(newElem);
        m_Elements.push_back(newElem);
        ioElemMap.insert(eastl::make_pair(TEditorStr(id), newElem));
        {
            SMetaPropertyParser theMetaParser(newElem->m_Type.c_str(), classRef, inMetaData,
                                              inMetaData.GetStringTable()->GetRenderStringTable());
            SDomReaderPropertyParser theDOMParser(inReader, m_ParseBuf, inReader.m_TempBuf);
            for (size_t idx = 0, end = newElem->m_Attributes.size(); idx < end; ++idx) {
                const SAttOrArg &theAtt(newElem->m_Attributes[idx]);
                SDatamodelValue newValue;
                switch (theAtt.m_DataType) {
                case ERuntimeDataModelDataTypeFloat:
                    newValue = ParseInitialProperty<float>(theAtt.m_Name.c_str(), theMetaParser,
                                                           theDOMParser);
                    break;
                case ERuntimeDataModelDataTypeFloat2:
                    newValue = ParseInitialProperty<QT3DSVec2>(theAtt.m_Name.c_str(), theMetaParser,
                                                            theDOMParser);
                    break;
                case ERuntimeDataModelDataTypeFloat3:
                    newValue = ParseInitialProperty<QT3DSVec3>(theAtt.m_Name.c_str(), theMetaParser,
                                                            theDOMParser);
                    break;
                case ERuntimeDataModelDataTypeLong:
                    newValue = ParseInitialProperty<QT3DSI32>(theAtt.m_Name.c_str(), theMetaParser,
                                                           theDOMParser);
                    break;
                case ERuntimeDataModelDataTypeString:
                    newValue = ParseInitialProperty<eastl::string>(theAtt.m_Name.c_str(),
                                                                   theMetaParser, theDOMParser);
                    break;
                case ERuntimeDataModelDataTypeBool:
                    newValue = ParseInitialProperty<bool>(theAtt.m_Name.c_str(), theMetaParser,
                                                          theDOMParser);
                    break;
                case ERuntimeDataModelDataTypeStringRef:
                    newValue = ParseInitialProperty<eastl::string>(theAtt.m_Name.c_str(),
                                                                   theMetaParser, theDOMParser);
                    break;
                case ERuntimeDataModelDataTypeObjectRef:
                    newValue =
                        ParseSlideProperty<eastl::string>(theAtt.m_Name.c_str(), theDOMParser);
                    break;
                default:
                    break;
                }
                newElem->m_InitialValues.push_back(newValue);
            }
        }
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling())
            ParseElement(inReader, inMetaData, newElem->m_Children, ioElemMap);

        return *newElem;
    }

    // Get the item names if not overridden
    // Pull the names off so we can construct the item path.
    void LoadSlide(IDOMReader &inReader, TIdElemMap &inMap)
    {
        IDOMReader::Scope __topScope(inReader);
        for (bool commandSuccess = inReader.MoveToFirstChild(); commandSuccess;
             commandSuccess = inReader.MoveToNextSibling()) {
            IDOMReader::Scope __commandScope(inReader);
            const char8_t *elemName = inReader.GetElementName();
            if (AreEqual(elemName, "Set") || AreEqual(elemName, "Add")) {
                const char8_t *name;
                TIdElemMap::iterator iter = inMap.end();
                const char8_t *itemRef;
                inReader.UnregisteredAtt("ref", itemRef);
                if (!isTrivial(itemRef)) {
                    if (itemRef[0] == '#')
                        ++itemRef;
                    iter = inMap.find(TEditorStr(itemRef));
                }

                if (inReader.UnregisteredAtt("name", name)) {
                    if (iter != inMap.end()) {
                        iter->second->m_Name.assign(name);
                    } else {
                        QT3DS_ASSERT(false);
                    }
                }
                if (iter != inMap.end() && AreEqual(elemName, "Add")) {
                    nvvector<char8_t> theTempBuf(this->m_Foundation->getAllocator(), "tempbuf");
                    SDomReaderPropertyParser theDOMParser(inReader, m_ParseBuf, inReader.m_TempBuf);
                    SAppElement *newElem = iter->second.mPtr;
                    for (size_t idx = 0, end = newElem->m_Attributes.size(); idx < end; ++idx) {
                        const SAttOrArg &theAtt(newElem->m_Attributes[idx]);
                        SDatamodelValue newValue;
                        switch (theAtt.m_DataType) {
                        case ERuntimeDataModelDataTypeFloat:
                            newValue =
                                ParseSlideProperty<float>(theAtt.m_Name.c_str(), theDOMParser);
                            break;
                        case ERuntimeDataModelDataTypeFloat2:
                            newValue =
                                ParseSlideProperty<QT3DSVec2>(theAtt.m_Name.c_str(), theDOMParser);
                            break;
                        case ERuntimeDataModelDataTypeFloat3:
                            newValue =
                                ParseSlideProperty<QT3DSVec3>(theAtt.m_Name.c_str(), theDOMParser);
                            break;
                        case ERuntimeDataModelDataTypeLong:
                            newValue =
                                ParseSlideProperty<QT3DSI32>(theAtt.m_Name.c_str(), theDOMParser);
                            break;
                        case ERuntimeDataModelDataTypeString:
                            newValue = ParseSlideProperty<eastl::string>(theAtt.m_Name.c_str(),
                                                                         theDOMParser);
                            break;
                        case ERuntimeDataModelDataTypeBool:
                            newValue =
                                ParseSlideProperty<bool>(theAtt.m_Name.c_str(), theDOMParser);
                            break;
                        case ERuntimeDataModelDataTypeStringRef:
                            newValue = ParseSlideProperty<eastl::string>(theAtt.m_Name.c_str(),
                                                                         theDOMParser);
                            break;
                        case ERuntimeDataModelDataTypeObjectRef:
                            newValue = ParseSlideProperty<eastl::string>(theAtt.m_Name.c_str(),
                                                                         theDOMParser);
                            break;
                        default:
                            break;
                        }
                        if (newValue.getType() != ERuntimeDataModelDataTypeNone)
                            newElem->m_InitialValues[idx] = newValue;
                    }
                }
            }
        }
    }

    void ResolveElemPath(SAppElement &inElem, const eastl::string &parentId)
    {
        inElem.m_Path = parentId;
        if (inElem.m_Path.back() != ':')
            inElem.m_Path.append(1, '.');
        inElem.m_Path.append(inElem.m_Name);
        for (size_t idx = 0, end = inElem.m_Children.size(); idx < end; ++idx)
            ResolveElemPath(*inElem.m_Children[idx], inElem.m_Path);
    }

    void LoadUIPFile(const char *inRelativePath, const char *inUIPId,
                     Q3DStudio::IRuntimeMetaData &inMetaData)
    {
        eastl::string appDir(m_FilePath);
        CFileTools::GetDirectory(appDir);
        eastl::string uipPath;
        CFileTools::CombineBaseAndRelative(appDir.c_str(), inRelativePath, uipPath);
        CFileSeekableIOStream uipStream(uipPath.c_str(), FileReadFlags());
        if (!uipStream.IsOpen()) {
            QT3DS_ASSERT(false);
            return;
        }
        NVScopedRefCounted<IDOMFactory> theFactory(
            IDOMFactory::CreateDOMFactory(m_Foundation->getAllocator(), m_StringTable));
        SDOMElement *domElem = CDOMSerializer::Read(*theFactory, uipStream).second;
        if (!domElem) {
            QT3DS_ASSERT(false);
            return;
        }

        NVScopedRefCounted<IDOMReader> theReader(IDOMReader::CreateDOMReader(
            m_Foundation->getAllocator(), *domElem, m_StringTable, theFactory));
        if (!theReader->MoveToFirstChild("Project")) {
            QT3DS_ASSERT(false);
            return;
        }

        Q3DStudio::IRuntimeMetaData &theMetaData(inMetaData);
        theMetaData.ClearPerProjectData();

        TEditorStr rootPath(inUIPId);
        rootPath.append(1, ':');
        eastl::vector<NVScopedRefCounted<SAppElement>> topElements;
        TIdElemMap idElemMap;
        m_Presentations.push_back(SPresentation());
        SPresentation &currentPresentation(m_Presentations.back());
        currentPresentation.m_Id.assign(inUIPId);
        currentPresentation.m_SrcPath.assign(inRelativePath);
        {
            IDOMReader::Scope __projectScope(*theReader);
            if (theReader->MoveToFirstChild("ProjectSettings")) {
                const char8_t *temp;
                theReader->UnregisteredAtt("author", temp);
                currentPresentation.m_Author.assign(nonNull(temp));
                theReader->UnregisteredAtt("company", temp);
                currentPresentation.m_Company.assign(nonNull(temp));
                theReader->Att("presentationWidth", currentPresentation.m_Width);
                theReader->Att("presentationHeight", currentPresentation.m_Height);
            }
            // Presentation width and height as specified in the file.
        }
        // Classes/handlers
        {
            IDOMReader::Scope __projectScope(*theReader);
            if (theReader->MoveToFirstChild("Classes")) {
                // Load each class system into the meta data and then pull back the handlers
                for (bool success = theReader->MoveToFirstChild(); success;
                     success = theReader->MoveToNextSibling()) {
                    const char8_t *id, *srcPath, *name;
                    theReader->UnregisteredAtt("id", id);
                    theReader->UnregisteredAtt("sourcepath", srcPath);
                    theReader->UnregisteredAtt("name", name);
                    eastl::string classItemPath;
                    CFileTools::CombineBaseAndRelative(appDir.c_str(), srcPath, classItemPath);

                    if (AreEqual(theReader->GetElementName().c_str(), "Behavior")) {
                        bool theScriptFile =
                            theMetaData.LoadScriptFile("Behavior", id, name, classItemPath.c_str());
                        QT3DS_ASSERT(theScriptFile);
                        (void)theScriptFile;
                    } else if (AreEqual(theReader->GetElementName().c_str(), "Effect")) {
                        bool theEffectFile = theMetaData.LoadEffectXMLFile("Effect", id, name,
                                                                           classItemPath.c_str());
                        QT3DS_ASSERT(theEffectFile);
                        (void)theEffectFile;
                    } else if (AreEqual(theReader->GetElementName().c_str(), "RenderPlugin")) {
                        theMetaData.LoadPluginXMLFile("RenderPlugin", id, name,
                                                      classItemPath.c_str());
                    } else if (AreEqual(theReader->GetElementName().c_str(), "CustomMaterial")) {
                        theMetaData.LoadMaterialXMLFile("CustomMaterial", id, name,
                                                        classItemPath.c_str());
                    } else {
                        QT3DS_ASSERT(false);
                    }
                }
            }
        }

        // Graph
        {
            IDOMReader::Scope __projectScope(*theReader);
            if (theReader->MoveToFirstChild("Graph")) {
                for (bool success = theReader->MoveToFirstChild(); success;
                     success = theReader->MoveToNextSibling()) {
                    currentPresentation.m_Scene =
                        &ParseElement(*theReader, theMetaData, topElements, idElemMap);
                }
            }
        }
        // States/Slides
        {
            // This is where the name *may* be set,
            IDOMReader::Scope __projectScope(*theReader);
            if (theReader->MoveToFirstChild("Logic")) {
                // Slides are just slightly hierarchical, master and then nonmaster children.
                for (bool success = theReader->MoveToFirstChild(); success;
                     success = theReader->MoveToNextSibling()) {
                    IDOMReader::Scope __masterScope(*theReader);
                    LoadSlide(*theReader, idElemMap);
                    const char8_t *component;
                    theReader->UnregisteredAtt("component", component);
                    NVScopedRefCounted<SAppComponent> theComponent;
                    if (!isTrivial(component)) {
                        if (component[0] == '#')
                            ++component;
                        TIdElemMap::iterator iter = idElemMap.find(TEditorStr(component));
                        if (iter != idElemMap.end()) {
                            if (iter->second->GetSubType() == ElementSubTypes::Component)
                                theComponent = static_cast<SAppComponent *>(iter->second.mPtr);
                            else {
                                QT3DS_ASSERT(false);
                            }
                        } else {
                            QT3DS_ASSERT(false);
                        }
                    }

                    for (bool childSuccess = theReader->MoveToFirstChild("State"); childSuccess;
                         childSuccess = theReader->MoveToNextSibling("State")) {
                        IDOMReader::Scope __slideScope(*theReader);
                        const char8_t *slideName;
                        if (theReader->UnregisteredAtt("name", slideName))
                            theComponent->m_Slides.push_back(TEditorStr(slideName));
                        LoadSlide(*theReader, idElemMap);
                    }
                }
            }
        }
        // Now resolve all the names to create full paths.
        for (size_t idx = 0, end = topElements.size(); idx < end; ++idx) {
            ResolveElemPath(*topElements[idx], rootPath);
        }
    }

    struct SElemLessThan
    {
        bool operator()(NVScopedRefCounted<SAppElement> lhs,
                        NVScopedRefCounted<SAppElement> rhs) const
        {
            return lhs->m_Path < rhs->m_Path;
        }
    };

    static const char *GetNamespace() { return "http://qt.io/qt3dstudio/uia"; }
    static const char *GetOldNamespace() { return "http://qt.io/qt3dstudio/uicomposer"; }

    void LoadUIADatabase()
    {
        CFileSeekableIOStream theStream(m_FilePath.c_str(), FileReadFlags());
        if (theStream.IsOpen()) {
            LoadUIADatabaseFromStream(theStream);
        }
    }

    struct SXmlErrorHandler : public CXmlErrorHandler
    {
        SXmlErrorHandler(const eastl::string &inFilePath, eastl::string &inErrorString)
            : m_FilePath(inFilePath)
            , m_ErrorString(inErrorString)
        {
        }
        virtual ~SXmlErrorHandler() {}
        virtual void OnXmlError(TXMLCharPtr errorName, int line, int column)
        {
            if (m_ErrorString.empty())
                m_ErrorString.sprintf("%s(%d, %d): %s", m_FilePath.c_str(), line, column,
                                      errorName);
        }
        eastl::string m_FilePath;
        eastl::string &m_ErrorString;
    };

    void LoadUIADatabaseFromStream(IInStream &inStream)
    {
        NVScopedRefCounted<IDOMFactory> theFactory(
            IDOMFactory::CreateDOMFactory(m_Foundation->getAllocator(), m_StringTable));
        m_UIADocument.first = NULL;
        m_UIADocument.second = NULL;
        m_UIANamespaces.clear();
        m_Elements.clear();
        m_IdToStateMaps.clear();
        m_Editors.clear();
        m_Presentations.clear();
        m_SlideProperties.clear();
        m_LoadingErrorString.clear();
        eastl::string appDir(m_FilePath);
        CFileTools::GetDirectory(appDir);

        SXmlErrorHandler theXmlErrorWriter(m_FilePath, m_LoadingErrorString);
        eastl::pair<SNamespacePairNode *, SDOMElement *> parseResult =
            CDOMSerializer::Read(*theFactory, inStream, &theXmlErrorWriter);
        if (parseResult.second != NULL) {
            qt3ds::foundation::CRegisteredString theRegisteredOldNamespace =
                m_StringTable->RegisterStr(GetOldNamespace());
            qt3ds::foundation::CRegisteredString theRegisteredNamespace =
                m_StringTable->RegisterStr(GetNamespace());

            ReplaceDOMNamespace(*parseResult.second, theRegisteredOldNamespace,
                                theRegisteredNamespace);

            m_UIADocument =
                IDOMWriter::CreateDOMWriter(theFactory, *parseResult.second, m_StringTable);
            for (SNamespacePairNode *nodePtr = parseResult.first; nodePtr;
                 nodePtr = nodePtr->m_NextNode) {
                if (nodePtr->m_Namespace == theRegisteredOldNamespace)
                    nodePtr->m_Namespace = theRegisteredNamespace;
                m_UIANamespaces.push_back(*nodePtr);
            }
        }

        if (m_UIADocument.first.mPtr == NULL) {
            m_UIADocument = IDOMWriter::CreateDOMWriter(m_Foundation->getAllocator(), "application",
                                                        m_StringTable, GetNamespace());
            m_Dirty = true;
        }
        if (m_UIANamespaces.empty())
            m_UIANamespaces.push_back(
                SNamespacePair(m_StringTable->RegisterStr(GetNamespace())));

        {
            NVScopedReleasable<Q3DStudio::IRuntimeMetaData> theMetaData(
                Q3DStudio::IRuntimeMetaData::Create(*m_InputStreamFactory));

            if (theMetaData) {
                m_SlideProperties = theMetaData->GetSlideAttributes();

                eastl::vector<eastl::pair<TEditorStr, TEditorStr>> machinePathsAndIds;
                IDOMReader::Scope __appScope(m_UIADocument.second);

                if (m_UIADocument.second->MoveToFirstChild("assets")) {
                    IDOMReader::Scope __assetsScope(m_UIADocument.second);
                    for (bool success = m_UIADocument.second->MoveToFirstChild(); success;
                         success = m_UIADocument.second->MoveToNextSibling()) {
                        IDOMReader::Scope __assetScope(m_UIADocument.second);
                        const char8_t *elemName = m_UIADocument.second->GetElementName();
                        if (AreEqual(elemName, "presentation")) {
                            const char8_t *id, *relativePath;
                            m_UIADocument.second->UnregisteredAtt("id", id);
                            m_UIADocument.second->UnregisteredAtt("src", relativePath);
                            LoadUIPFile(relativePath, id, *theMetaData);
                        } else if (AreEqual(elemName, "statemachine")) {
                            const char8_t *id, *relativePath;
                            m_UIADocument.second->UnregisteredAtt("id", id);
                            m_UIADocument.second->UnregisteredAtt("src", relativePath);
                            TEditorStr fullPath;
                            CFileTools::CombineBaseAndRelative(appDir.c_str(), relativePath,
                                                               fullPath);
                            CreateSCXMLEditor(fullPath, nonNull(id));
                        }
                    }
                }

                // Now sort our list of elements by path after we have loaded all uip files.
                eastl::sort(m_Elements.begin(), m_Elements.end(), SElemLessThan());
            } else {
                QT3DS_ASSERT(false);
            }
        }
        {
            IDOMReader::Scope __appScope(m_UIADocument.second);
            for (bool success = m_UIADocument.second->MoveToFirstChild("statemachine"); success;
                 success = m_UIADocument.second->MoveToNextSibling("statemachine")) {
                IDOMReader::Scope __machineScope(m_UIADocument.second);
                const char8_t *idref;
                m_UIADocument.second->UnregisteredAtt("ref", idref);
                if (!isTrivial(idref)) {
                    if (idref[0] == '#')
                        ++idref;
                    for (size_t idx = 0, end = m_Editors.size(); idx < end; ++idx) {
                        if (m_Editors[idx].m_Id.compare(idref) == 0)
                            ParseStateMachine(m_Editors[idx].m_Id, m_Editors[idx].m_Editor);
                    }
                }
            }
        }
    }

    virtual bool IsDirty() const { return m_Dirty; }
    // General queries of the dataset defined by the uia file and all uip files and scxml files it
    // includes.

    SAppElement *FindElementByPath(const TEditorStr &inPath)
    {
        if (!m_FinderElement)
            m_FinderElement = new SAppElement();

        m_FinderElement->m_Path = inPath;

        nvvector<NVScopedRefCounted<SAppElement>>::iterator iter = eastl::lower_bound(
            m_Elements.begin(), m_Elements.end(), m_FinderElement, SElemLessThan());
        if (iter != m_Elements.end() && (*iter)->m_Path == m_FinderElement->m_Path) {
            return iter->mPtr;
        }
        return NVScopedRefCounted<SAppElement>();
    }

    virtual TEditorStrList GetComponents()
    {
        TEditorStrList retval;
        for (size_t idx = 0, end = m_Elements.size(); idx < end; ++idx) {
            if (m_Elements[idx]->GetSubType() == ElementSubTypes::Component)
                retval.push_back(m_Elements[idx]->m_Path);
        }
        return retval;
    }

    virtual TEditorStrList GetComponentSlides(const TEditorStr &inComponent)
    {
        SAppElement *elem = FindElementByPath(inComponent);
        if (elem != NULL && elem->GetSubType() == ElementSubTypes::Component)
            return static_cast<SAppComponent *>(elem)->m_Slides;
        return TEditorStrList();
    }

    virtual TEditorStrList GetBehaviors()
    {
        TEditorStrList retval;
        for (size_t idx = 0, end = m_Elements.size(); idx < end; ++idx) {
            if (m_Elements[idx]->GetSubType() == ElementSubTypes::Behavior)
                retval.push_back(m_Elements[idx]->m_Path);
        }
        return retval;
    }

    virtual Q3DStudio::THandlerList GetHandlers(const TEditorStr &inBehavior)
    {
        SAppElement *elem = FindElementByPath(inBehavior);
        if (elem != NULL && elem->GetSubType() == ElementSubTypes::Behavior)
            return static_cast<SAppBehavior *>(elem)->m_Handlers;
        return Q3DStudio::THandlerList();
    }

    virtual Q3DStudio::TVisualEventList GetVisualEvents(const TEditorStr &inElement)
    {
        SAppElement *elem = FindElementByPath(inElement);
        if (elem)
            return elem->m_VisualEvents;
        return Q3DStudio::TVisualEventList();
    }

    virtual TEditorStrList GetElements()
    {
        TEditorStrList retval;
        for (size_t idx = 0, end = m_Elements.size(); idx < end; ++idx)
            retval.push_back(m_Elements[idx]->m_Path);
        return retval;
    }

    virtual Q3DStudio::TAttOrArgList GetElementAttributes(const TEditorStr &inElement)
    {
        SAppElement *elem = FindElementByPath(inElement);
        if (elem)
            return elem->m_Attributes;
        return Q3DStudio::TAttOrArgList();
    }

    TEditorPtr CreateSCXMLEditor(const TEditorStr &inPath, const TEditorStr &inId)
    {
        TEditorPtr newEditor = m_StateMachineEditorManager.GetOrCreateEditor(inPath, 0);
        if (newEditor) {
            SEditorEntry theEntry;
            theEntry.m_Editor = newEditor.mPtr;
            theEntry.m_FilePath = inPath;
            theEntry.m_Id = inId;
            m_Editors.push_back(theEntry);
        }
        return newEditor.mPtr;
    }

    virtual TEditorPtr GetOrCreateEditor(const TEditorStr &inFullPath, bool *outLoadStatus)
    {
        (void)outLoadStatus;
        TEditorStr normalizedPath(inFullPath);
        CFileTools::NormalizePath(normalizedPath);
        for (size_t idx = 0, end = m_Editors.size(); idx < end; ++idx) {
            if (m_Editors[idx].m_FilePath == normalizedPath)
                return m_Editors[idx].m_Editor;
        }

        // Is file full path under our application directory;
        TEditorStr appDir(m_FilePath);
        CFileTools::GetDirectory(appDir);
        TEditorStr editorDir(inFullPath);
        CFileTools::GetDirectory(editorDir);

        // For scxml files outside our app dir, let the user create an editor manually.
        // We will have nothing to do with it.
        if (editorDir.find(appDir) == eastl::string::npos)
            return TEditorPtr();

        // Get an ID for the editor
        IDOMReader::Scope __appScope(*m_UIADocument.second);
        if (!m_UIADocument.second->MoveToFirstChild("assets")) {
            m_UIADocument.first->Begin("assets", GetNamespace());
        }

        {
            IDOMReader::Scope __assetsScope(*m_UIADocument.second);
            for (bool success = m_UIADocument.second->MoveToFirstChild("statemachine"); success;
                 success = m_UIADocument.second->MoveToNextSibling("statemachine")) {
                const char8_t *srcPath;
                const char8_t *id;
                m_UIADocument.second->UnregisteredAtt("src", srcPath);
                m_UIADocument.second->UnregisteredAtt("id", id);
                TEditorStr docPath;
                CFileTools::CombineBaseAndRelative(appDir.c_str(), srcPath, docPath);
                CFileTools::NormalizePath(docPath);
                if (docPath == normalizedPath) {
                    return CreateSCXMLEditor(normalizedPath, nonNull(id));
                }
            }
        }

        eastl::string dirname, fname, extension;
        CFileTools::Split(normalizedPath.c_str(), dirname, fname, extension);
        m_UIADocument.first->Begin("statemachine", GetNamespace());
        m_UIADocument.first->Att("id", fname.c_str(), GetNamespace());
        eastl::string relativePath;
        CFileTools::GetRelativeFromBase(appDir, normalizedPath, relativePath);
        m_UIADocument.first->Att("src", relativePath.c_str());
        m_Dirty = true;
        return CreateSCXMLEditor(normalizedPath, nonNull(fname.c_str()));
    }

    static bool CouldHaveVisualStateExecutableContent(TObjPtr inObject)
    {
        return inObject->GetExecutableContentTypes().size() > 0;
    }
    template <typename TEditorType>
    TEditorType &CreateEditor(TObjPtr inParent)
    {
        TPropertyAccessorList accessors;
        TEditorType::CreateProperties(accessors, *this);
        return *QT3DS_NEW(m_Foundation->getAllocator(), TEditorType)(*this, inParent, accessors);
    }

    SVSEntry *GetEntryForPair(TEntryExitPair &inPair, InterpreterEventTypes::Enum inType)
    {
        SVSEntry *theEntry;
        if (inType == InterpreterEventTypes::StateEnter
            || inType == InterpreterEventTypes::Transition) {
            if (inPair.first == NULL)
                inPair.first = new SVSEntry(inType);
            theEntry = inPair.first.mPtr;
        } else {
            if (inPair.second == NULL)
                inPair.second = new SVSEntry(inType);
            theEntry = inPair.second.mPtr;
        }
        return theEntry;
    }

    void ParseExecutableContent(IDOMReader &inReader, const TEditorStr &inId,
                                TIdEntryExitMap &ioMap, InterpreterEventTypes::Enum inType,
                                TObjPtr parentPtr)
    {
        IDOMReader::Scope __contentScope(inReader);
        {
            IDOMReader::Scope __testScope(inReader);
            // See if there is any executable content to begin with.
            if (inReader.MoveToFirstChild() == false)
                return;
        }

        TEntryExitPair &thePair =
            ioMap.insert(eastl::make_pair(inId, TEntryExitPair())).first->second;
        SVSEntry *theEntry = GetEntryForPair(thePair, inType);
        QT3DS_ASSERT(theEntry->m_EventType == inType);
        for (bool success = inReader.MoveToFirstChild(); success;
             success = inReader.MoveToNextSibling()) {
            IDOMReader::Scope __elemScope(inReader);
            const char8_t *elemName(inReader.GetElementName());
            if (AreEqual(elemName, SGotoSlideEditor::ElementName())) {
                const char8_t *component, *slide, *rel, *wrap, *direction, *state, *mode,
                    *playthroughto, *rate, *time;
                inReader.UnregisteredAtt("element", component);
                if (inReader.UnregisteredAtt("slide", slide) == false)
                    inReader.UnregisteredAtt("rel", rel);
                else
                    rel = "";
                inReader.UnregisteredAtt("wrap", wrap);
                inReader.UnregisteredAtt("direction", direction);
                inReader.UnregisteredAtt("state", state);
                inReader.UnregisteredAtt("mode", mode);
                inReader.UnregisteredAtt("playthroughto", playthroughto);
                inReader.UnregisteredAtt("rate", rate);
                inReader.UnregisteredAtt("time", time);
                SGotoSlideEditor &theEditor = CreateEditor<SGotoSlideEditor>(parentPtr);
                theEditor.m_Component.assign(nonNull(component));
                theEditor.m_Slide.assign(nonNull(slide));
                theEditor.m_Rel.assign(nonNull(rel));
                theEditor.m_Wrap.assign(nonNull(wrap));
                theEditor.m_Direction.assign(nonNull(direction));
                theEditor.m_State.assign(nonNull(state));
                theEditor.m_Mode.assign(nonNull(mode));
                theEditor.m_PlayThroughTo.assign(nonNull(playthroughto));
                theEditor.m_Rate.assign(nonNull(rate));
                theEditor.m_Time.assign(nonNull(time));
                theEntry->m_Editors.push_back(theEditor);
            } else if (AreEqual(elemName, SSetAttributeEditor::ElementName())) {
                const char8_t *element, *att, *val;
                inReader.UnregisteredAtt("element", element);
                inReader.UnregisteredAtt("attribute", att);
                inReader.UnregisteredAtt("value", val);
                SSetAttributeEditor &theEditor = CreateEditor<SSetAttributeEditor>(parentPtr);
                theEditor.m_Element.assign(nonNull(element));
                theEditor.m_Attribute.assign(nonNull(att));
                theEditor.m_Value.assign(nonNull(val));
                theEntry->m_Editors.push_back(theEditor);
            } else if (AreEqual(elemName, SFireEventEditor::ElementName())) {
                const char8_t *element, *evt;
                inReader.UnregisteredAtt("element", element);
                inReader.UnregisteredAtt("event", evt);
                SFireEventEditor &theEditor = CreateEditor<SFireEventEditor>(parentPtr);
                theEditor.m_Element.assign(nonNull(element));
                theEditor.m_Event.assign(nonNull(evt));
                theEntry->m_Editors.push_back(theEditor);
            } else if (AreEqual(elemName, SRunHandlerEditor::ElementName())) {
                const char8_t *element, *handler, *args;
                inReader.UnregisteredAtt("element", element);
                inReader.UnregisteredAtt("handler", handler);
                inReader.UnregisteredAtt("arguments", args);
                SRunHandlerEditor &theEditor = CreateEditor<SRunHandlerEditor>(parentPtr);
                theEditor.m_Behavior.assign(nonNull(element));
                theEditor.m_Handler.assign(nonNull(handler));
                theEditor.m_ArgumentStr.assign(nonNull(args));
                theEntry->m_Editors.push_back(theEditor);
            } else if (AreEqual(elemName, SSetPresentationEditor::ElementName())) {
                const char8_t *ref, *attribute, *value;
                inReader.UnregisteredAtt("ref", ref);
                inReader.UnregisteredAtt("attribute", attribute);
                inReader.UnregisteredAtt("value", value);
                SSetPresentationEditor &theEditor = CreateEditor<SSetPresentationEditor>(parentPtr);
                theEditor.m_Ref.assign(nonNull(ref));
                theEditor.m_Attribute.assign(nonNull(attribute));
                theEditor.m_Value.assign(nonNull(value));
                theEntry->m_Editors.push_back(theEditor);
            } else if (AreEqual(elemName, SPlaySoundEditor::ElementName())) {
                const char8_t *file;
                inReader.UnregisteredAtt("file", file);
                SPlaySoundEditor &theEditor = CreateEditor<SPlaySoundEditor>(parentPtr);
                theEditor.m_FilePath.assign(nonNull(file));
                theEntry->m_Editors.push_back(theEditor);
            }
        }
    }

    void ParseVisualStateEntry(IDOMReader &inReader, TEditorPtr inEditor, TIdEntryExitMap &retval,
                               const char8_t *stateId)
    {
        TObjPtr parentPtr = inEditor->GetObjectById(stateId);
        if (parentPtr) {
            if (AreEqual(inReader.GetElementName().c_str(), "state")) {
                for (bool entryExitSucces = inReader.MoveToFirstChild(); entryExitSucces;
                     entryExitSucces = inReader.MoveToNextSibling()) {
                    const char8_t *signalName(inReader.GetElementName());
                    if (AreEqual(signalName, "enter")) {
                        ParseExecutableContent(inReader, stateId, retval,
                                               InterpreterEventTypes::StateEnter, parentPtr);
                    } else if (AreEqual(signalName, "exit")) {
                        ParseExecutableContent(inReader, stateId, retval,
                                               InterpreterEventTypes::StateExit, parentPtr);
                    } else {
                        QT3DS_ASSERT(false);
                    }
                }
            } else if (AreEqual(inReader.GetElementName().c_str(), "transition")) {
                ParseExecutableContent(inReader, stateId, retval, InterpreterEventTypes::Transition,
                                       parentPtr);
            } else {
                QT3DS_ASSERT(false);
            }
        }
    }

    TIdEntryExitMap &ParseStateMachine(const TEditorStr &inId, TEditorPtr inEditor)
    {
        TIdEntryExitMap &retval =
            m_IdToStateMaps.insert(eastl::make_pair(inId, TIdEntryExitMap())).first->second;
        if (m_UIADocument.second->MoveToFirstChild("visual-states")) {
            for (bool stateSuccess = m_UIADocument.second->MoveToFirstChild(); stateSuccess;
                 stateSuccess = m_UIADocument.second->MoveToNextSibling()) {
                IDOMReader::Scope __stateScope(*m_UIADocument.second);
                const char8_t *stateId;
                m_UIADocument.second->UnregisteredAtt("ref", stateId);
                if (!isTrivial(stateId)) {
                    if (stateId[0] == '#')
                        ++stateId;

                    ParseVisualStateEntry(*m_UIADocument.second, inEditor, retval, stateId);
                }
            }
        }
        return retval; // found a statemachine with the right id.
    }

    TIdEntryExitMap &GetOrCreateStateMap(const TEditorStr &inId, TEditorPtr inEditor)
    {
        TIdEntryExitMap &retval =
            m_IdToStateMaps.insert(eastl::make_pair(inId, TIdEntryExitMap())).first->second;

        if (retval.empty()) {
            IDOMReader::Scope __appScope(*m_UIADocument.second);
            for (bool success = m_UIADocument.second->MoveToFirstChild("statemachine");
                 success && retval.empty();
                 success = m_UIADocument.second->MoveToNextSibling("statemachine")) {
                IDOMReader::Scope __machineScope(*m_UIADocument.second);
                const char8_t *idref;
                m_UIADocument.second->UnregisteredAtt("ref", idref);
                if (!isTrivial(idref)) {
                    if (idref[0] == '#')
                        ++idref;
                    if (inId.compare(idref) == 0) {
                        return ParseStateMachine(inId, inEditor);
                    }
                }
            } // for (statemachines)
        }

        return retval;
    }

    TIdEntryExitMap *GetStateMapForObject(TObjPtr inObject)
    {
        if (!CouldHaveVisualStateExecutableContent(inObject))
            return NULL;

        TEditorPtr theEditor = inObject->GetEditor();
        // Find the id for the editor
        TIdEntryExitMap *stateMap(NULL);

        for (size_t idx = 0, end = m_Editors.size(); idx < end && stateMap == NULL; ++idx) {
            if (m_Editors[idx].m_Editor == theEditor)
                return &GetOrCreateStateMap(m_Editors[idx].m_Id, m_Editors[idx].m_Editor);
        }
        return NULL;
    }

    // The editor obj has a remove from graph function that really is delete
    virtual TObjList GetVisualStateExecutableContent(TObjPtr inObject,
                                                     InterpreterEventTypes::Enum inEventType)
    {
        TIdEntryExitMap *stateMap = GetStateMapForObject(inObject);
        if (stateMap == NULL)
            return TObjList();

        TEditorStr objId(inObject->GetId());
        TIdEntryExitMap::iterator iter = stateMap->find(objId);
        for (TIdEntryExitMap::iterator temp = stateMap->begin(), end = stateMap->end(); temp != end;
             ++temp) {
            objId = temp->first;
            objId.clear();
        }
        if (iter != stateMap->end()) {
            TEntryExitPair thePair = iter->second;
            if (thePair.first && thePair.first->m_EventType == inEventType)
                return thePair.first->m_Editors;
            else if (thePair.second && thePair.second->m_EventType == inEventType)
                return thePair.second->m_Editors;
        }

        return TObjList();
    }

    // Type name is the element name, so set-attribute, goto-slide, or fire-event
    virtual TObjPtr AppendVisualStateExecutableContent(TObjPtr inObject,
                                                       InterpreterEventTypes::Enum inEventType,
                                                       const char8_t *inElementName)
    {
        TIdEntryExitMap *stateMap = GetStateMapForObject(inObject);
        if (stateMap == NULL)
            return TObjPtr();

        NVConstDataRef<InterpreterEventTypes::Enum> supportedTypes =
            inObject->GetExecutableContentTypes();
        bool foundEventType = false;
        for (size_t idx = 0, end = supportedTypes.size(); idx < end && foundEventType == false;
             ++idx)
            if (inEventType == supportedTypes[idx])
                foundEventType = true;

        if (foundEventType == false) {
            QT3DS_ASSERT(false);
            return TObjPtr();
        }

        TEntryExitPair &thePair =
            stateMap->insert(eastl::make_pair(inObject->GetId(), TEntryExitPair())).first->second;
        SVSEntry *entry = GetEntryForPair(thePair, inEventType);
        if (!entry) {
            QT3DS_ASSERT(false);
            return TObjPtr();
        }

        if (AreEqual(inElementName, SGotoSlideEditor::ElementName())) {
            entry->m_Editors.push_back(CreateEditor<SGotoSlideEditor>(inObject));
        } else if (AreEqual(inElementName, SRunHandlerEditor::ElementName())) {
            entry->m_Editors.push_back(CreateEditor<SRunHandlerEditor>(inObject));
        } else if (AreEqual(inElementName, SSetAttributeEditor::ElementName())) {
            entry->m_Editors.push_back(CreateEditor<SSetAttributeEditor>(inObject));
        } else if (AreEqual(inElementName, SFireEventEditor::ElementName())) {
            entry->m_Editors.push_back(CreateEditor<SFireEventEditor>(inObject));
        } else if (AreEqual(inElementName, SSetPresentationEditor::ElementName())) {
            entry->m_Editors.push_back(CreateEditor<SSetPresentationEditor>(inObject));
        } else if (AreEqual(inElementName, SPlaySoundEditor::ElementName())) {
            entry->m_Editors.push_back(CreateEditor<SPlaySoundEditor>(inObject));
        } else {
            QT3DS_ASSERT(false);
            return TObjPtr();
        }
        TObjPtr retval = entry->m_Editors.back();
        STransaction *theTrans = GetOpenTransactionImpl();
        if (theTrans) {
            SVSEntryListChange *theChange =
                new SVSEntryListChange(TObjPtr(), retval, *entry, true, inObject);
            theTrans->m_Changes.push_back(theChange);
        }
        return retval;
    }

    virtual TObjPtr ChangeVisualStateExecutableContentName(TObjPtr inContent,
                                                           const char8_t *inElementName)
    {
        TObjPtr theParent = inContent->Parent();
        if (!CouldHaveVisualStateExecutableContent(theParent)) {
            QT3DS_ASSERT(false);
            return TObjPtr();
        }
        TIdEntryExitMap *stateMap = GetStateMapForObject(theParent);
        if (stateMap == NULL) {
            QT3DS_ASSERT(false);
            return TObjPtr();
        }

        TIdEntryExitMap::iterator iter = stateMap->find(theParent->GetId());
        if (iter == stateMap->end()) {
            QT3DS_ASSERT(false);
            return TObjPtr();
        }
        TEntryExitPair thePair = iter->second;
        NVScopedRefCounted<SVSEntry> theEntryFound;
        if (!theEntryFound) {
            NVScopedRefCounted<SVSEntry> theEntry = thePair.first;
            TObjPtr theObj = inContent;
            if (theEntry) {
                TObjList::iterator iter =
                    eastl::find(theEntry->m_Editors.begin(), theEntry->m_Editors.end(), theObj);
                if (iter != theEntry->m_Editors.end()) {
                    theEntryFound = theEntry;
                }
            }
        }
        if (!theEntryFound) {
            NVScopedRefCounted<SVSEntry> theEntry = thePair.second;
            TObjPtr theObj = inContent;
            if (theEntry) {
                TObjList::iterator iter =
                    eastl::find(theEntry->m_Editors.begin(), theEntry->m_Editors.end(), theObj);
                if (iter != theEntry->m_Editors.end()) {
                    theEntryFound = theEntry;
                }
            }
        }
        if (!theEntryFound)
            return TObjPtr();

        TObjPtr theRetval;
        if (AreEqual(inElementName, SGotoSlideEditor::ElementName())) {
            theRetval = CreateEditor<SGotoSlideEditor>(theParent);
        } else if (AreEqual(inElementName, SRunHandlerEditor::ElementName())) {
            theRetval = CreateEditor<SRunHandlerEditor>(theParent);
        } else if (AreEqual(inElementName, SSetAttributeEditor::ElementName())) {
            theRetval = CreateEditor<SSetAttributeEditor>(theParent);
        } else if (AreEqual(inElementName, SFireEventEditor::ElementName())) {
            theRetval = CreateEditor<SFireEventEditor>(theParent);
        } else if (AreEqual(inElementName, SSetPresentationEditor::ElementName())) {
            theRetval = CreateEditor<SSetPresentationEditor>(theParent);
        } else if (AreEqual(inElementName, SPlaySoundEditor::ElementName())) {
            theRetval = CreateEditor<SPlaySoundEditor>(theParent);
        } else {
            QT3DS_ASSERT(false);
            return TObjPtr();
        }

        NVScopedRefCounted<SVSEntryListChange> theOldChange =
            new SVSEntryListChange(inContent, inContent, *theEntryFound, false, theParent);
        NVScopedRefCounted<SVSEntryListChange> theNewChange =
            new SVSEntryListChange(theRetval, inContent, *theEntryFound, true, theParent);
        theOldChange->Do();
        theNewChange->Do();
        if (GetOpenTransactionImpl()) {
            GetOpenTransactionImpl()->m_Changes.push_back(theOldChange.mPtr);
            GetOpenTransactionImpl()->m_Changes.push_back(theNewChange.mPtr);
        }

        return theRetval;
    }

    // Called when the source uia changes.
    virtual void RefreshFile() { LoadUIADatabase(); }

    virtual void RefreshFromStream(qt3ds::foundation::IInStream &inStream)
    {
        LoadUIADatabaseFromStream(inStream);
    }

    // Returns the path that was passed in on create.
    virtual TEditorStr GetFilePath() { return m_FilePath; }

    void CopyDOM(IDOMReader &inReader, IDOMWriter &inWriter)
    {
        IDOMReader::Scope __itemScope(inReader);
        const SDOMElement &theElement(*inReader.GetElement());
        IDOMWriter::Scope __writeScope(inWriter, theElement.m_Name, theElement.m_Namespace);
        if (theElement.m_Attributes.empty() && theElement.m_Children.empty()) {
            if (!isTrivial(theElement.m_Value))
                inWriter.Value(theElement.m_Value);
        } else {
            for (TAttributeList::iterator iter = theElement.m_Attributes.begin(),
                                          end = theElement.m_Attributes.end();
                 iter != end; ++iter) {
                inWriter.Att(iter->m_Name, iter->m_Value, iter->m_Namespace);
            }

            for (bool success = inReader.MoveToFirstChild(); success;
                 success = inReader.MoveToNextSibling()) {
                IDOMReader::Scope __loopScope(inReader);
                CopyDOM(inReader, inWriter);
            }
        }
    }
    void ReplaceDOMNamespace(SDOMElement &inDom,
                             qt3ds::foundation::CRegisteredString &inNamespaceToReplace,
                             qt3ds::foundation::CRegisteredString &inNewNamespace)
    {
        if (!inNamespaceToReplace.IsValid() || !inNewNamespace.IsValid())
            return;
        if (inDom.m_Namespace == inNamespaceToReplace)
            inDom.m_Namespace = inNewNamespace;
        // Set attributes namespace
        for (qt3ds::foundation::TAttributeList::iterator theIter = inDom.m_Attributes.begin(),
                                                      theEnd = inDom.m_Attributes.end();
             theIter != theEnd; ++theIter) {
            if (theIter->m_Namespace == inNamespaceToReplace)
                theIter->m_Namespace = inNewNamespace;
        }
        // Recursive
        for (qt3ds::foundation::SDOMElement::TElementChildList::iterator
                 theIter = inDom.m_Children.begin(),
                 theEnd = inDom.m_Children.end();
             theIter != theEnd; ++theIter) {
            ReplaceDOMNamespace(*theIter, inNamespaceToReplace, inNewNamespace);
        }
    }
    struct SEditorIDFinder
    {
        TEditorStr m_Id;
        SEditorIDFinder(const TEditorStr &id)
            : m_Id(id)
        {
        }
        bool operator()(const SEditorEntry &entry) const { return m_Id == entry.m_Id; }
    };

    void WriteExecutableContent(IDOMWriter &inWriter, NVScopedRefCounted<SVSEntry> inEntry)
    {
        if (inEntry == NULL)
            return;
        const char *elemName = "";
        switch (inEntry->m_EventType) {
        case InterpreterEventTypes::Transition:
            break;
        case InterpreterEventTypes::StateEnter:
            elemName = "enter";
            break;
        case InterpreterEventTypes::StateExit:
            elemName = "exit";
            break;
        default:
            QT3DS_ASSERT(false);
            break;
        }
        if (!isTrivial(elemName))
            inWriter.Begin(elemName, GetNamespace());

        eastl::vector<SPropertyDeclaration> properties;
        for (size_t idx = 0, end = inEntry->m_Editors.size(); idx < end; ++idx) {
            TObjPtr editor(inEntry->m_Editors[idx]);
            IDOMWriter::Scope __contentScope(inWriter, editor->TypeName(), GetNamespace());
            editor->GetProperties(properties);
            bool theNoProperty = true;
            for (size_t idx = 0, end = properties.size(); idx < end; ++idx) {
                TEditorStr theProp =
                    editor->GetPropertyValue(properties[idx].m_Name)->getData<TEditorStr>();
                if (theProp.empty() == false) {
                    inWriter.Att(properties[idx].m_Name.c_str(), theProp.c_str(),
                                 GetNamespace());
                    theNoProperty = false;
                }
            }
            if (theNoProperty && properties.size() > 0)
                inWriter.Att(properties[0].m_Name.c_str(), "", GetNamespace());
        }

        if (!isTrivial(elemName))
            inWriter.End();
    }

    void WriteStateEntry(TIdEntryExitMap::const_iterator stateIter, IDOMWriter &writer,
                         const char *typeName, const char *id)
    {
        const char8_t *stateName = "state";
        if (AreEqual(typeName, "transition"))
            stateName = "transition";
        eastl::string tempAtt;
        IDOMWriter::Scope __stateScope(writer, stateName, GetNamespace());
        tempAtt.assign("#");
        tempAtt.append(id);
        writer.Att("ref", tempAtt.c_str());
        WriteExecutableContent(writer, stateIter->second.first);
        WriteExecutableContent(writer, stateIter->second.second);
    }

    // Returns false if unable to save, ask users to check the file out.
    bool SaveInner(qt3ds::foundation::IOutStream &inStream)
    {
        IDOMReader::Scope __saveScope(m_UIADocument.second);
        NVScopedRefCounted<IDOMFactory> theFactory(
            IDOMFactory::CreateDOMFactory(m_Foundation->getAllocator(), m_StringTable));
        NVScopedRefCounted<IDOMWriter> outgoingDoc =
            IDOMWriter::CreateDOMWriter(m_Foundation->getAllocator(), "application", m_StringTable,
                                        GetNamespace())
                .first;
        {
            m_UIADocument.second->MoveToFirstChild("application");
            for (SDOMAttribute *theAtt = m_UIADocument.second->GetFirstAttribute(); theAtt;
                 theAtt = m_UIADocument.second->GetNextAttribute()) {
                outgoingDoc->Att(theAtt->m_Name, theAtt->m_Value, theAtt->m_Namespace);
            }
        }

        {
            IDOMReader::Scope __appScope(m_UIADocument.second);
            if (m_UIADocument.second->MoveToFirstChild("assets")) {
                CopyDOM(*m_UIADocument.second, *outgoingDoc);
            }
        }
        {
            eastl::string tempAtt;
            for (TIdStateMapMap::const_iterator iter = m_IdToStateMaps.begin(),
                                                end = m_IdToStateMaps.end();
                 iter != end; ++iter) {
                nvvector<SEditorEntry>::iterator editorEntry = eastl::find_if(
                    m_Editors.begin(), m_Editors.end(), SEditorIDFinder(iter->first));
                if (editorEntry == m_Editors.end()) {
                    QT3DS_ASSERT(false);
                    return false;
                }
                IDOMWriter::Scope __machineScope(*outgoingDoc, "statemachine", GetNamespace());
                tempAtt.assign("#");
                tempAtt.append(iter->first);
                outgoingDoc->Att("ref", tempAtt.c_str());
                IDOMWriter::Scope __vsScope(*outgoingDoc, "visual-states", GetNamespace());
                const TIdEntryExitMap &itemMap = iter->second;
                for (TIdEntryExitMap::const_iterator stateIter = itemMap.begin(),
                                                     stateEnd = itemMap.end();
                     stateIter != stateEnd; ++stateIter) {
                    TObjPtr editorObj =
                        editorEntry->m_Editor->GetObjectById(stateIter->first.c_str());
                    if (!editorObj) {
                        QT3DS_ASSERT(false);
                        continue;
                    }
                    WriteStateEntry(stateIter, *outgoingDoc, editorObj->TypeName(),
                                    stateIter->first.c_str());
                }
            }
        }

        SDOMElement *topElem = outgoingDoc->GetTopElement();
        CDOMSerializer::WriteXMLHeader(inStream);
        CDOMSerializer::Write(m_Foundation->getAllocator(), *topElem, inStream, *m_StringTable,
                              m_UIANamespaces);
        m_Dirty = false;
        return true;
    }

    virtual bool Save()
    {
        CFileSeekableIOStream theStream(m_FilePath.c_str(), FileWriteFlags());
        if (!theStream.IsOpen())
            return false;
        return SaveInner(theStream);
    }

    virtual bool Save(qt3ds::foundation::IOutStream &inStream) { return SaveInner(inStream); }

    virtual eastl::vector<SPresentation> GetPresentations() { return m_Presentations; }
    virtual eastl::string GetElementType(SAppElement &elem) { return elem.m_Type; }

    virtual eastl::string GetElementId(SAppElement &elem) { return elem.m_Id; }
    virtual bool IsComponent(SAppElement &elem)
    {
        return elem.GetSubType() == ElementSubTypes::Component;
    }
    virtual Q3DStudio::TAttOrArgList GetElementAttributes(SAppElement &elem)
    {
        return elem.m_Attributes;
    }
    virtual Q3DStudio::TAttOrArgList GetSlideAttributes() { return m_SlideProperties; }
    virtual eastl::vector<SDatamodelValue> GetElementAttributeInitialValues(SAppElement &elem)
    {
        return elem.m_InitialValues;
    }
    virtual eastl::vector<SAppElement *> GetElementChildren(SAppElement &elem)
    {
        eastl::vector<SAppElement *> retval;
        retval.resize(elem.m_Children.size());
        for (size_t idx = 0, end = elem.m_Children.size(); idx < end; ++idx)
            retval[idx] = elem.m_Children[idx].mPtr;
        return retval;
    }
    virtual eastl::string GetLastLoadingErrorString() { return m_LoadingErrorString; }
    // The file may either exist or not.  You can pass in a uip file as well as a uia file.

    virtual NVFoundationBase &GetFoundation() { return m_Foundation->getFoundation(); }

    virtual IStringTable &GetStringTable() { return *m_StringTable; }

    // ITransactionManager
    // Undo/redo is supported via a transparent transaction system.
    // calls are reentrant but last call will close the transaction object.
    // Any changes to any editor objects will go through this transaction when they happen.
    virtual TSignalConnectionPtr AddChangeListener(IEditorChangeListener &inListener)
    {
        return m_TransactionManager->AddChangeListener(inListener);
    }
    virtual TTransactionPtr BeginTransaction(const TEditorStr &inName)
    {
        return m_TransactionManager->BeginTransaction(inName);
    }

    virtual TTransactionPtr GetOpenTransaction()
    {
        return m_TransactionManager->GetOpenTransaction();
    }

    virtual void RollbackTransaction() { m_TransactionManager->RollbackTransaction(); }
    virtual void EndTransaction() { m_TransactionManager->EndTransaction(); }

    virtual void OnObjectCreated(TObjPtr) {}

    virtual void OnObjectDeleted(TObjPtr inObject)
    {
        // don't care.
        if (!CouldHaveVisualStateExecutableContent(inObject))
            return;

        TIdEntryExitMap *stateMap = GetStateMapForObject(inObject);
        if (stateMap == NULL)
            return;

        TIdEntryExitMap::iterator iter = stateMap->find(inObject->GetId());
        if (iter == stateMap->end())
            return;
        NVScopedRefCounted<SIdEntryExitDeleteChange> theChange =
            new SIdEntryExitDeleteChange(*stateMap, inObject->GetId(), inObject);
        theChange->Do();
        if (GetOpenTransactionImpl())
            GetOpenTransactionImpl()->m_Changes.push_back(theChange.mPtr);
    }
    bool RemoveObjectFromEntry(NVScopedRefCounted<SVSEntry> inEntry, SVSEditorObject &inObj)
    {
        if (inEntry) {
            TObjList::iterator iter =
                eastl::find(inEntry->m_Editors.begin(), inEntry->m_Editors.end(), TObjPtr(inObj));
            if (iter != inEntry->m_Editors.end()) {
                NVScopedRefCounted<SVSEntryListChange> theChange = new SVSEntryListChange(
                    TObjPtr(inObj), TObjPtr(inObj), *inEntry, false, inObj.m_ParentObject);
                theChange->Do();
                if (GetOpenTransactionImpl())
                    GetOpenTransactionImpl()->m_Changes.push_back(theChange.mPtr);
                return true;
            }
        }
        return false;
    }

    virtual void RemoveObjectFromGraph(SVSEditorObject &inObj)
    {
        TObjPtr parentPtr = inObj.m_ParentObject;
        if (!CouldHaveVisualStateExecutableContent(parentPtr)) {
            QT3DS_ASSERT(false);
            return;
        }
        TIdEntryExitMap *stateMap = GetStateMapForObject(parentPtr);
        if (stateMap == NULL) {
            QT3DS_ASSERT(false);
            return;
        }

        TIdEntryExitMap::iterator iter = stateMap->find(parentPtr->GetId());
        if (iter == stateMap->end()) {
            QT3DS_ASSERT(false);
            return;
        }
        TEntryExitPair thePair = iter->second;
        if (!RemoveObjectFromEntry(thePair.first, inObj)) {
            RemoveObjectFromEntry(thePair.second, inObj);
        }
    }

    struct SEditorFinder
    {
        TEditorPtr m_Editor;
        SEditorFinder(TEditorPtr editor)
            : m_Editor(editor)
        {
        }
        bool operator()(const SEditorEntry &entry) const { return entry.m_Editor == m_Editor; }
    };

    void CopyStateNode(SStateNode &inNode, TIdEntryExitMap &idMap, IDOMWriter &writer)
    {
        TIdEntryExitMap::iterator iter = idMap.find(inNode.m_Id.c_str());
        if (iter != idMap.end()) {
            const char *typeName = "state";
            if (inNode.m_Type == StateNodeTypes::Transition)
                typeName = "transition";
            WriteStateEntry(iter, writer, typeName, inNode.m_Id.c_str());
        }
        TStateNodeList *childList = inNode.GetChildren();
        if (childList) {
            for (TStateNodeList::iterator iter = childList->begin(), end = childList->end();
                 iter != end; ++iter) {
                CopyStateNode(*iter, idMap, writer);
            }
        }
    }

    virtual void OnCopy(TEditorPtr inEditor, eastl::vector<SStateNode *> &ioCopiedRoots,
                        IDOMWriter &ioWriter, eastl::vector<SNamespacePair> &ioNamespaces)
    {
        ioNamespaces.push_back(SNamespacePair(m_StringTable->RegisterStr(GetNamespace()),
                                              m_StringTable->RegisterStr("uia")));
        eastl::vector<SEditorEntry>::iterator entry =
            eastl::find_if(m_Editors.begin(), m_Editors.end(), SEditorFinder(inEditor));
        if (entry == m_Editors.end())
            return;
        TIdStateMapMap::iterator mapEntry = m_IdToStateMaps.find(entry->m_Id);
        if (mapEntry == m_IdToStateMaps.end())
            return;
        IDOMWriter::Scope __modelScope(ioWriter, "datamodel_fragment", GetNamespace());
        for (size_t idx = 0, end = ioCopiedRoots.size(); idx < end; ++idx) {
            CopyStateNode(*ioCopiedRoots[idx], mapEntry->second, ioWriter);
        }
    }

    virtual void OnPaste(TEditorPtr inEditor, IDOMReader &ioReader,
                         CXMLIO::TIdRemapMap &inStateIdRemapMap)
    {
        eastl::vector<SEditorEntry>::iterator entry =
            eastl::find_if(m_Editors.begin(), m_Editors.end(), SEditorFinder(inEditor));
        if (entry == m_Editors.end())
            return;

        IDOMReader::Scope __fragmentScope(ioReader);
        if (ioReader.MoveToFirstChild("datamodel_fragment")) {
            TIdEntryExitMap &stateMap =
                m_IdToStateMaps.insert(eastl::make_pair(entry->m_Id, TIdEntryExitMap()))
                    .first->second;
            for (bool success = ioReader.MoveToFirstChild(); success;
                 success = ioReader.MoveToNextSibling()) {
                IDOMReader::Scope __childScope(ioReader);
                const char8_t *idRef;
                ioReader.UnregisteredAtt("ref", idRef);
                if (isTrivial(idRef))
                    continue;
                if (idRef[0] == '#')
                    ++idRef;

                CXMLIO::TIdRemapMap::iterator finder = inStateIdRemapMap.find(idRef);
                if (finder != inStateIdRemapMap.end())
                    idRef = finder->second.c_str();

                TEditorStr idStr(idRef);
                TObjPtr parentObj = inEditor->GetObjectById(idRef);
                if (parentObj) {
                    ParseVisualStateEntry(ioReader, inEditor, stateMap, idRef);
                    if (stateMap.find(idStr) != stateMap.end()
                        && m_TransactionManager->GetOpenTransactionImpl()) {
                        m_TransactionManager->GetOpenTransactionImpl()->m_Changes.push_back(
                            new SIdEntryExitDeleteChange(stateMap, idStr, parentObj, false));
                    }
                }
            }
        }
    }

    virtual void OnIDChange(TEditorPtr inEditor, SStateNode &inNode, const char8_t *inOldId)
    {
        eastl::vector<SEditorEntry>::iterator entry =
            eastl::find_if(m_Editors.begin(), m_Editors.end(), SEditorFinder(inEditor));
        if (entry == m_Editors.end())
            return;

        TIdEntryExitMap &stateMap =
            m_IdToStateMaps.insert(eastl::make_pair(entry->m_Id, TIdEntryExitMap())).first->second;
        TEditorStr oldIdStr(inOldId);
        TEditorStr newIdStr(inNode.m_Id.c_str());
        TIdEntryExitMap::iterator iter = stateMap.find(oldIdStr);
        if (iter != stateMap.end()) {
            TEntryExitPair thePair(iter->second);
            stateMap.erase(iter);
            stateMap.insert(eastl::make_pair(newIdStr, thePair));
        }
    }
    virtual bool OnDeleteState(TObjPtr inObject)
    {
        // don't care.
        if (!CouldHaveVisualStateExecutableContent(inObject))
            return false;

        TIdEntryExitMap *stateMap = GetStateMapForObject(inObject);
        if (stateMap == NULL)
            return false;

        TIdEntryExitMap::iterator iter = stateMap->find(inObject->GetId());
        if (iter == stateMap->end())
            return false;
        NVScopedRefCounted<SIdEntryExitDeleteChange> theChange =
            new SIdEntryExitDeleteChange(*stateMap, inObject->GetId(), inObject);
        theChange->Do();
        if (GetOpenTransactionImpl())
            GetOpenTransactionImpl()->m_Changes.push_back(theChange.mPtr);
        return true;
    }
    virtual bool OnReloadStateMachine(TEditorPtr inStateMachineEditor)
    {
        bool theRetval = false;
        TEditorPtr theEditor = inStateMachineEditor;

        for (size_t idx = 0, end = m_Editors.size(); idx < end; ++idx) {
            if (m_Editors[idx].m_Editor == theEditor) {
                TIdStateMapMap::iterator theFind = m_IdToStateMaps.find(m_Editors[idx].m_Id);
                if (theFind != m_IdToStateMaps.end()) {
                    TIdEntryExitMap &theStateMap = theFind->second;
                    for (TIdEntryExitMap::iterator theIter = theStateMap.begin(),
                                                   theEnd = theStateMap.end();
                         theIter != theEnd;) {
                        TEditorStr theStateId = theIter->first;
                        TObjPtr theState = inStateMachineEditor->GetObjectById(theStateId.c_str());
                        if (theState) {
                            {
                                NVScopedRefCounted<SVSEntry> theEntry = theIter->second.first;
                                if (theEntry) {
                                    for (TObjList::iterator theIter = theEntry->m_Editors.begin(),
                                                            theEnd = theEntry->m_Editors.end();
                                         theIter != theEnd; ++theIter) {
                                        SVSEditorObject *theExecutableContent =
                                            static_cast<SVSEditorObject *>(theIter->mPtr);
                                        if (theExecutableContent) {
                                            theExecutableContent->m_ParentObject = theState;
                                        }
                                    }
                                }
                            }
                            {
                                NVScopedRefCounted<SVSEntry> theEntry = theIter->second.second;
                                if (theEntry) {
                                    for (TObjList::iterator theIter = theEntry->m_Editors.begin(),
                                                            theEnd = theEntry->m_Editors.end();
                                         theIter != theEnd; ++theIter) {
                                        SVSEditorObject *theExecutableContent =
                                            static_cast<SVSEditorObject *>(theIter->mPtr);
                                        if (theExecutableContent) {
                                            theExecutableContent->m_ParentObject = theState;
                                        }
                                    }
                                }
                            }
                            ++theIter;
                        } else {
                            theStateMap.erase(theIter++);
                            theRetval = true;
                        }
                    }
                }
                break;
            }
        }
        return theRetval;
    }
    virtual void RegisterChangeListener(IStateMachineChangeListener &) {}
    virtual void UnregisterChangeListener(IStateMachineChangeListener &) {}
};
}

IDatamodel &IDatamodel::Create(qt3ds::state::editor::TFoundationPtr inFoundation,
                               const TEditorStr &inPath, const TEditorStr &inAppDir,
                               IStateMachineEditorManager &inStateMachineEditorManager,
                               IInStream *inStream)
{
    return IDatamodel::Create(inFoundation, inPath, inAppDir, inStateMachineEditorManager,
                              IStringTable::CreateStringTable(inFoundation->getAllocator()),
                              inStream);
}

IDatamodel &IDatamodel::Create(qt3ds::state::editor::TFoundationPtr inFoundation,
                               const TEditorStr &inPath, const TEditorStr &inAppDir,
                               IStateMachineEditorManager &inStateMachineEditorManager,
                               IStringTable &inStringTable, IInStream *inStream)
{
    DatamodelImpl &theDatamodel = *QT3DS_NEW(inFoundation->getAllocator(), DatamodelImpl)(
        inFoundation, inPath, inAppDir, inStateMachineEditorManager, inStringTable);
    if (inStream)
        theDatamodel.LoadUIADatabaseFromStream(*inStream);
    else
        theDatamodel.LoadUIADatabase();
    return theDatamodel;
}

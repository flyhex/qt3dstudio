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
#ifndef UICSTATE_EDITOR_PROPERTIES_H
#define UICSTATE_EDITOR_PROPERTIES_H
#pragma once

#include "UICStateEditorFoundation.h"
#include "UICStateEditorImpl.h"

namespace uic {
namespace state {
    namespace editor {

        template <typename TEditorType>
        struct SPropertyAccessorBase : public IPropertyAccessor
        {
            TFoundationPtr m_Allocator;
            volatile QT3DSI32 mRefCount;

            SPropertyAccessorBase(TFoundationPtr alloc, const SPropertyDeclaration &inDec)
                : IPropertyAccessor(inDec)
                , m_Allocator(alloc)
                , mRefCount(0)
            {
            }

            virtual Option<SValue> Get(IEditorObject &inObj)
            {
                return DoGet(static_cast<TEditorType &>(inObj));
            }
            virtual void Set(IEditorObject &inObj, const Option<SValue> &inValue)
            {
                DoSet(static_cast<TEditorType &>(inObj), inValue);
            }

            virtual Option<SValue> DoGet(TEditorType &inObj) = 0;
            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue) = 0;
        };

        template <typename TEditorType>
        struct SInitialProperty : SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;
            static SPropertyDeclaration CreatePropertyDeclaration(IStringTable &inStrTable)
            {
                SPropertyDeclaration theDeclaration;
                theDeclaration.m_Name = inStrTable.RegisterStr("initial");
                theDeclaration.m_Type = EditorPropertyTypes::Object;
                return theDeclaration;
            }
            SInitialProperty(TFoundationPtr inAlloc, IStringTable &inStrTable)
                : TBaseType(inAlloc, CreatePropertyDeclaration(inStrTable))
            {
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                return SValue(inObj.Get(inObj.m_Data.m_Initial));
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                inObj.m_Initial =
                    inObj.template FromEditor<STransition>(inValue->getData<TObjPtr>());
            }
        };

        template <typename TEditorType, typename TFlagsType>
        struct SFlagBooleanProperty : public SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;
            typedef bool (TFlagsType::*TGetPropPtr)() const;
            typedef void (TFlagsType::*TSetPropPtr)(bool);

            TGetPropPtr m_GetProp;
            TSetPropPtr m_SetProp;
            eastl::vector<CRegisteredString> m_LegalValues;
            virtual eastl::vector<CRegisteredString> GetLegalValues(IEditorObject & /*inObj*/)
            {
                return m_LegalValues;
            }

            static SPropertyDeclaration CreatePropertyDeclaration(IStringTable &inStrTable,
                                                                  const char8_t *propName)
            {
                SPropertyDeclaration theDeclaration;
                theDeclaration.m_Name = inStrTable.RegisterStr(propName);
                theDeclaration.m_Type = EditorPropertyTypes::StringSet;
                return theDeclaration;
            }
            SFlagBooleanProperty(TFoundationPtr inFnd, IStringTable &inStrTable,
                                 const char8_t *inPropName, const char8_t *falseName,
                                 const char8_t *trueName, TGetPropPtr inGetProp,
                                 TSetPropPtr inSetProp)
                : TBaseType(inFnd, CreatePropertyDeclaration(inStrTable, inPropName))
                , m_GetProp(inGetProp)
                , m_SetProp(inSetProp)
            {
                m_LegalValues.push_back(inStrTable.RegisterStr(falseName));
                m_LegalValues.push_back(inStrTable.RegisterStr(trueName));
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                bool boolVal = (inObj.m_Data.m_Flags.*m_GetProp)();
                TEditorStr retval = boolVal ? TEditorStr(m_LegalValues[1].c_str())
                                            : TEditorStr(m_LegalValues[0].c_str());
                return SValue(retval);
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValueOpt)
            {
                if (inValueOpt.hasValue()) {
                    TEditorStr data = inValueOpt->getData<TEditorStr>();

                    if (AreEqual(data.c_str(), m_LegalValues[1].c_str()))
                        (inObj.m_Data.m_Flags.*m_SetProp)(true);

                    else if (AreEqual(data.c_str(), m_LegalValues[0].c_str()))
                        (inObj.m_Data.m_Flags.*m_SetProp)(false);

                    else {
                        QT3DS_ASSERT(false);
                    }
                }
            }
        };

        template <typename TEditorType>
        struct SChildrenProperty : SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;
            static SPropertyDeclaration CreatePropertyDeclaration(IStringTable &inStrTable)
            {
                SPropertyDeclaration theDeclaration;
                theDeclaration.m_Name = inStrTable.RegisterStr("children");
                theDeclaration.m_Type = EditorPropertyTypes::ObjectList;
                return theDeclaration;
            }

            SChildrenProperty(TFoundationPtr inAlloc, IStringTable &inStrTable)
                : TBaseType(inAlloc, CreatePropertyDeclaration(inStrTable))
            {
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                TObjList retval;
                for (TStateNodeList::iterator iter = inObj.m_Data.m_Children.begin(),
                                              end = inObj.m_Data.m_Children.end();
                     iter != end; ++iter)
                    retval.push_back(inObj.m_Editor.ToEditor(*iter));
                return SValue(retval);
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                if (inValue.hasValue()) {
                    const TObjList &data = inValue->getData<TObjList>();
                    TStateNodeList &theChildren(inObj.m_Data.m_Children);
                    // De-set all children.
                    while (theChildren.empty() == false)
                        inObj.m_Data.RemoveChild(theChildren.front());

                    for (TObjList::const_iterator iter = data.begin(), end = data.end();
                         iter != end; ++iter) {
                        SStateNode *theNode = inObj.m_Editor.StateNodeFromEditor(*iter);
                        if (theNode)
                            inObj.m_Data.AppendChild(*theNode);
                        else {
                            QT3DS_ASSERT(false);
                        }
                    }
                }
            }
        };

        // Property that is represented by a data item's member.
        template <typename TEditorType, typename TStateType, typename TDataType>
        struct SDataProp : public SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;
            typedef TDataType TStateType::*TPropertyPtr;
            TPropertyPtr m_Ptr;
            eastl::vector<CRegisteredString> m_LegalValues;

            SDataProp(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec, TPropertyPtr inPtr)
                : TBaseType(inAlloc, inDec)
                , m_Ptr(inPtr)
            {
            }
            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual eastl::vector<CRegisteredString> GetLegalValues(IEditorObject & /*inObj*/)
            {
                return m_LegalValues;
            }

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                return inObj.m_Editor.Get(inObj.m_Data.*(this->m_Ptr));
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                inObj.m_Editor.Set(inValue, inObj.m_Data.*(this->m_Ptr));
            }
        };

        template <typename TEditorType, typename TStateType>
        struct SInitialTargetProp : public SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;

            SInitialTargetProp(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec)
                : TBaseType(inAlloc, inDec)
            {
            }
            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                return inObj.m_Editor.IsInitialTarget(inObj.m_Data);
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                inObj.m_Editor.SetInitialTarget(inValue, inObj.m_Data);
            }

            virtual bool HandlesTransaction() { return true; }
        };

        template <typename TEditorType, typename TStateType>
        struct SInitialComboProp : public SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;

            SInitialComboProp(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec)
                : TBaseType(inAlloc, inDec)
            {
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual eastl::vector<CRegisteredString> GetLegalValues(IEditorObject &inEditor)
            {
                eastl::vector<CRegisteredString> retval;
                TEditorType &theEditor = static_cast<TEditorType &>(inEditor);
                retval.push_back(theEditor.m_Editor.RegisterStr("(script expression)"));
                theEditor.m_Editor.GetLegalInitialValues(theEditor.m_Data, retval);
                return retval;
            }

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                TEditorType &theEditor = inObj;
                if (theEditor.m_InitialComboValue.size() == 0) {
                    if (!isTrivial(inObj.m_Data.GetInitialExpression()))
                        theEditor.m_InitialComboValue = "(script expression)";
                    else if (inObj.m_Data.GetInitialTransition()
                             && inObj.m_Data.GetInitialTransition()->m_Target.size())
                        theEditor.m_InitialComboValue.assign(
                            inObj.m_Data.GetInitialTransition()->m_Target[0]->m_Id.c_str());
                    else
                        theEditor.m_InitialComboValue =
                            theEditor.m_Editor.GetDefaultInitialValue(theEditor.m_Data);
                }
                return theEditor.m_InitialComboValue;
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                if (inValue->getData<TEditorStr>() != "(script expression)") {
                    inObj.m_Editor.SetInitialAttribute(inValue, inObj.m_Data);
                } else {
                    inObj.m_Editor.SetInitialAttribute(Option<SValue>(), inObj.m_Data);
                }
            }
            virtual bool HandlesTransaction() { return true; }
        };

        template <typename TEditorType, typename TStateType, typename TDataType>
        struct SOptionAccessorProp : public SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;
            typedef Option<TDataType> TOptType;
            typedef TOptType (TStateType::*TGetPtr)() const;
            typedef void (TStateType::*TSetPtr)(const TOptType &inOpt);
            TGetPtr m_Getter;
            TSetPtr m_Setter;

            SOptionAccessorProp(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec,
                                TGetPtr inPtr, TSetPtr inSetPtr)
                : TBaseType(inAlloc, inDec)
                , m_Getter(inPtr)
                , m_Setter(inSetPtr)
            {
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                TOptType theOpt = (inObj.m_Data.*m_Getter)();
                if (theOpt.hasValue())
                    return SValue(*theOpt);
                return Empty();
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                TOptType theNewValue;
                if (inValue.hasValue())
                    theNewValue = TOptType(inValue->getData<TDataType>());
                (inObj.m_Data.*m_Setter)(theNewValue);
            }
        };

        template <typename TEditorType, typename TStateType>
        struct SDataIdProp : SDataProp<TEditorType, TStateType, CRegisteredString>
        {
            typedef SDataProp<TEditorType, TStateType, CRegisteredString> TBaseType;
            typedef CRegisteredString TStateType::*TPropertyPtr;

            SDataIdProp(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec,
                        TPropertyPtr inPtr)
                : TBaseType(inAlloc, inDec, inPtr)
            {
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                // This may mangle the id in order to find a unique id.
                inObj.m_Editor.SetIdProperty(inObj.m_Data, inValue, inObj.m_Data.*(this->m_Ptr));
            }
        };

        template <typename TEditorObject, typename TStateType, typename TDataType>
        IPropertyAccessor *CreateDataAccessor(SEditorImpl &inData, TDataType TStateType::*inDataPtr,
                                              const char8_t *inName,
                                              EditorPropertyTypes::Enum inPropType)
        {
            typedef SDataProp<TEditorObject, TStateType, TDataType> TPropType;
            return QT3DS_NEW(inData.m_EditorFoundation->getAllocator(), TPropType)(
                inData.m_EditorFoundation,
                SPropertyDeclaration(inData.RegisterStr(inName), inPropType), inDataPtr);
        }

        template <typename TEditorType, typename TDataType>
        struct SEditorImplProp : public SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;
            typedef TDataType TEditorType::*TPropertyPtr;
            TPropertyPtr m_Ptr;

            SEditorImplProp(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec,
                            TPropertyPtr inPtr)
                : TBaseType(inAlloc, inDec)
                , m_Ptr(inPtr)
            {
            }
            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                return inObj.m_Editor.Get(inObj.*(this->m_Ptr));
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                inObj.m_Editor.Set(inValue, inObj.*(this->m_Ptr));
            }
        };

        template <typename TEditorObject, typename TDataType>
        IPropertyAccessor *
        CreateEditorAccessor(SEditorImpl &inData, TDataType TEditorObject::*inDataPtr,
                             const char8_t *inName, EditorPropertyTypes::Enum inPropType)
        {
            typedef SEditorImplProp<TEditorObject, TDataType> TPropType;
            return QT3DS_NEW(inData.m_EditorFoundation->getAllocator(), TPropType)(
                inData.m_EditorFoundation,
                SPropertyDeclaration(inData.RegisterStr(inName), inPropType), inDataPtr);
        }

        // General property for initial and history transition properties
        template <typename TEditorType, typename TStateType>
        struct SSCXMLInitialPtr : public SPropertyAccessorBase<TEditorType>
        {
            typedef STransition *TStateType::*TPropertyType;
            typedef SPropertyAccessorBase<TEditorType> TBaseType;
            TPropertyType m_PropPtr;

            SSCXMLInitialPtr(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec,
                             TPropertyType inPropPtr)
                : TBaseType(inAlloc, inDec)
                , m_PropPtr(inPropPtr)
            {
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                return inObj.GetInitial(inObj.m_Data.*m_PropPtr);
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                inObj.SetInitial(inValue->getData<TObjList>(), inObj.m_Data.*m_PropPtr);
            }
        };

        template <typename TEditorType, typename TStateType>
        struct SSCXMLInitialContent : public SPropertyAccessorBase<TEditorType>
        {
            typedef STransition *TStateType::*TPropertyType;
            typedef SPropertyAccessorBase<TEditorType> TBaseType;
            TPropertyType m_PropPtr;

            SSCXMLInitialContent(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec,
                                 TPropertyType inPropPtr)
                : TBaseType(inAlloc, inDec)
                , m_PropPtr(inPropPtr)
            {
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                return inObj.GetInitialContent(inObj.m_Data.*m_PropPtr);
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                if (inValue.hasValue()) {
                    inObj.SetInitialContent(inValue->getData<TObjList>(), inObj.m_Data.*m_PropPtr);
                } else {
                    inObj.SetInitialContent(TObjList(), inObj.m_Data.*m_PropPtr);
                }
            }
        };

        template <typename TEditorType>
        struct SDelayProp : public SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;

            SDelayProp(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec)
                : TBaseType(inAlloc, inDec)
            {
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                if (!isTrivial(inObj.m_Data.m_DelayExpr))
                    return Empty();

                return inObj.m_Editor.GetSendId(inObj.m_Data.m_Delay);
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                inObj.m_Data.m_DelayExpr = "";
                inObj.m_Editor.SetSendId(inValue, inObj.m_Data.m_Delay);
            }
        };

        template <typename TEditorType>
        struct SHistoryTransitionProp : public SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;

            SHistoryTransitionProp(TFoundationPtr inAlloc, const SPropertyDeclaration &inDec)
                : TBaseType(inAlloc, inDec)
            {
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            virtual eastl::vector<CRegisteredString> GetLegalValues(IEditorObject &inEditor)
            {
                TEditorType &theEditor = static_cast<TEditorType &>(inEditor);
                return theEditor.m_Editor.GetLegalHistoryDefaultValues(theEditor.m_Data);
            }

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                if (inObj.m_Data.m_Transition)
                    return inObj.m_Editor.Get(inObj.m_Data.m_Transition->m_Target);

                return SValue(TObjList());
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                TObjList newObjects;

                if (inValue.hasValue())
                    newObjects = inValue->getData<TObjList>();

                if (newObjects.size()) {
                    if (!inObj.m_Data.m_Transition) {
                        inObj.m_Data.m_Transition =
                            (STransition *)inObj.m_Editor.m_AutoAllocator.allocate(
                                sizeof(STransition), "transition", __FILE__, __LINE__);
                        new (inObj.m_Data.m_Transition) STransition();
                        inObj.m_Data.m_Transition->m_Parent = &inObj.m_Data;
                    }
                    inObj.m_Editor.Set(inValue, inObj.m_Data.m_Transition->m_Target);
                } else
                    inObj.m_Data.m_Transition = NULL;
            }
        };

        template <typename TEditorType>
        struct SParentProp : public SPropertyAccessorBase<TEditorType>
        {
            typedef SPropertyAccessorBase<TEditorType> TBaseType;

            SParentProp(TFoundationPtr inAlloc, IStringTable &inStrTable)
                : TBaseType(inAlloc, SPropertyDeclaration(inStrTable.RegisterStr("parent"),
                                                          EditorPropertyTypes::StringSet))
            {
            }

            UIC_STATE_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(this->m_Allocator, this->mRefCount);

            static const char *SCXMLParentName() { return "(none)"; }

            virtual Option<SValue> DoGet(TEditorType &inObj)
            {
                TObjPtr current = inObj.Parent();
                if (current && AreEqual(current->TypeName(), "scxml") == false)
                    return SValue(current->GetId());
                return Option<SValue>(SValue(eastl::string(SCXMLParentName())));
            }

            virtual eastl::vector<CRegisteredString> GetLegalValues(IEditorObject &inEditor)
            {
                TEditorType &theEditor = static_cast<TEditorType &>(inEditor);
                eastl::vector<CRegisteredString> retval =
                    theEditor.m_Editor.GetLegalParentIds(theEditor.m_Data);
                CRegisteredString parentName(theEditor.m_Editor.RegisterStr(SCXMLParentName()));
                retval.insert(retval.begin(), parentName);
                return retval;
            }

            virtual void DoSet(TEditorType &inObj, const Option<SValue> &inValue)
            {
                inObj.m_Editor.SetParent(inObj.m_Data, inValue);
            }

            virtual bool HandlesTransaction() { return true; }
        };
    }
}
}

#endif

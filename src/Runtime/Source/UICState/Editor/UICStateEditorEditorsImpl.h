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
#ifndef UIC_STATE_EDITOR_EDITORS_IMPL_H
#define UIC_STATE_EDITOR_EDITORS_IMPL_H
#include "UICState.h"
#include "UICStateEditor.h"
#include "UICStateEditorValue.h"
#include "UICStateEditorTransactionImpl.h"
#include "UICStateTypes.h"
#include "UICStateEditorFoundation.h"
#include "UICStateEditorProperties.h"
#include "foundation/Utils.h"
#include "foundation/XML.h"
#include "UICStateXMLIO.h"
#include "foundation/IOStreams.h"
#include "UICStateExecutionTypes.h"
#include "UICStateContext.h"
#include "UICStateEditorImpl.h"

namespace uic {
namespace state {
    namespace editor {

        struct EditorTypes
        {
            enum Enum {
                UnknownEditor = 0,
                SCXML,
                State,
                Parallel,
                Transition,
                History,
                Final,
                ExecutableContent,
                OnEntry,
                OnExit,
                Send,
                Raise,
                If,
                Else,
                ElseIf,
                Log,
                Assign,
                Script,
                DataModel,
                Data,
                Cancel,
            };
        };

        struct SEditorImplObject : public IEditorObject
        {
            TFoundationPtr m_Foundation;
            SEditorImpl &m_Editor;
            volatile QT3DSI32 mRefCount;
            eastl::string m_Id;
            eastl::string m_Description;
            Option<QT3DSVec3> m_Color;
            TPropertyAccessorList m_PropertyAccessors;

            static void CreateAccessors(SEditorImpl &inData, TPropertyAccessorList &outAccessors,
                                        bool includeId, bool includeColor)
            {
                if (includeId)
                    outAccessors.push_back(CreateEditorAccessor(inData, &SEditorImplObject::m_Id,
                                                                "id", EditorPropertyTypes::String));

                outAccessors.push_back(
                    CreateEditorAccessor(inData, &SEditorImplObject::m_Description, "description",
                                         EditorPropertyTypes::String));
                if (includeColor)
                    outAccessors.push_back(CreateEditorAccessor(
                        inData, &SEditorImplObject::m_Color, "color", EditorPropertyTypes::Color));
            }

            SEditorImplObject(const char8_t *tn, SEditorImpl &inData,
                              const TPropertyAccessorList &inAccessors)
                : IEditorObject(tn)
                , m_Foundation(inData.m_EditorFoundation)
                , m_Editor(inData)
                , mRefCount(0)
                , m_PropertyAccessors(inAccessors)
            {
            }

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

            virtual eastl::vector<CRegisteredString> GetLegalValues(const char8_t *propName)
            {
                IPropertyAccessor *accessor = FindPropertyAccessor(propName);
                if (accessor)
                    return accessor->GetLegalValues(*this);
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
                    if (accessor->HandlesTransaction() == false) {
                        STransaction *theTransaction = m_Editor.GetOpenTransactionImpl();
                        if (theTransaction) {
                            Option<SValue> existing = accessor->Get(*this);
                            theTransaction->m_Changes.push_back(
                                new SChange(existing, inValue, *accessor, *this));
                        }
                    }
                    accessor->Set(*this, inValue);
                } else {
                    QT3DS_ASSERT(false);
                }
            }

            virtual TEditorStr GetId()
            {
                Option<SValue> propValue = GetPropertyValue("id");
                if (propValue.hasValue())
                    return propValue->getData<TEditorStr>();
                return TEditorStr();
            }
            virtual TEditorStr GetDescription() { return m_Description; }
            // We have to go through the SPV interface to take transactions into account.

            virtual void SetId(const TEditorStr &inName) { SetPropertyValue("id", SValue(inName)); }
            virtual void SetDescription(const TEditorStr &inName)
            {
                SetPropertyValue("description", SValue(inName));
            }

            virtual TObjPtr Parent() { return TObjPtr(); }
            virtual void RemoveObjectFromGraph() { QT3DS_ASSERT(false); }

            virtual IEditor &GetEditor() { return m_Editor; }
        };

#define UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE                                            \
    void addRef() { atomicIncrement(&mRefCount); }                                                 \
    void release()                                                                                 \
    {                                                                                              \
        TFoundationPtr fnd(m_Foundation);                                                          \
        /*Ensure the editor sticks around till *after* we are gone. */                             \
        /*This is because our objects will keep several bits of the editor around */               \
        QT3DS_IMPLEMENT_REF_COUNT_RELEASE(m_Foundation->getAllocator());                              \
    }

        struct SSCXMLEditor : public SEditorImplObject
        {
            typedef SSCXML TStateType;
            enum { EditorType = EditorTypes::SCXML };

            SSCXML &m_Data;
            eastl::string m_InitialComboValue;
            bool m_endEdit;

            static const char8_t *GetTypeStr() { return "scxml"; }

            static TPropertyAccessorList CreateAccessors(SEditorImpl &inEditorData,
                                                         TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                typedef SDataProp<SSCXMLEditor, SSCXML, STransition *> TInitialProp;
                typedef SFlagBooleanProperty<SSCXMLEditor, SSCXMLFlags> TBindingProp;
                TPropertyAccessorList &retval(inList);
                NVAllocatorCallback &alloc(inEditorData.m_EditorFoundation->getAllocator());
                typedef SDataProp<SSCXMLEditor, SSCXML, CRegisteredString> TNameProp;
                typedef SInitialComboProp<SSCXMLEditor, SSCXML> TInitialComboProp;
                typedef SDataProp<SSCXMLEditor, SSCXML, const char8_t *> TCharProp;
                IPropertyAccessor *newAccessor =
                    QT3DS_NEW(alloc, TNameProp)(inEditorData.m_EditorFoundation,
                                             SPropertyDeclaration(inEditorData.RegisterStr("name"),
                                                                  EditorPropertyTypes::String),
                                             &SSCXML::m_Name);
                retval.push_back(newAccessor);
                retval.push_back(CreateEditorAccessor(inEditorData,
                                                      &SEditorImplObject::m_Description,
                                                      "description", EditorPropertyTypes::String));
                newAccessor = QT3DS_NEW(alloc, TBindingProp)(
                    inEditorData.m_EditorFoundation, *inEditorData.m_StringTable, "binding",
                    "early", "late", &SSCXMLFlags::IsLateBinding, &SSCXMLFlags::SetLateBinding);
                retval.push_back(newAccessor);

                TInitialComboProp *theInitialCombo = QT3DS_NEW(alloc, TInitialComboProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("initial"),
                                         EditorPropertyTypes::StringSet));
                retval.push_back(theInitialCombo);

                TCharProp *theInitialExprProp = QT3DS_NEW(alloc, TCharProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("initialexpr"),
                                         EditorPropertyTypes::BigString),
                    &SSCXML::m_InitialExpr);
                retval.push_back(theInitialExprProp);

                retval.push_back(CreateDataAccessor<SSCXMLEditor>(
                    inEditorData, &SSCXML::m_DataModel, "datamodel", EditorPropertyTypes::Object));
                newAccessor = QT3DS_NEW(alloc, SChildrenProperty<SSCXMLEditor>)(
                    inEditorData.m_EditorFoundation, *inEditorData.m_StringTable);
                retval.push_back(newAccessor);
                // Replace the name property with one that writes to our data
                return retval;
            }

            SSCXMLEditor(SSCXML &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SEditorImplObject(GetTypeStr(), inEditorData,
                                    CreateAccessors(inEditorData, inList))
                , m_Data(inData)
                , m_endEdit(false)
            {
            }

            void ApplyInitial(const TEditorStr &comboValue)
            {
                m_endEdit = false;

                if (m_InitialComboValue == "(script expression)"
                    || comboValue == "(script expression)")
                    m_endEdit = true;

                m_InitialComboValue = comboValue;
            }

            virtual bool endEdit()
            {
                bool bTemp = m_endEdit;
                m_endEdit = false;
                return bTemp;
            }

            virtual CRegisteredString GetId() const { return m_Data.m_Id; }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual void *GetWrappedObject() { return &m_Data; }

            virtual void RemoveObjectFromGraph() { QT3DS_ASSERT(false); }

            virtual void RemoveIdFromContext() { m_Editor.GetStateContext().EraseId(m_Data.m_Id); }

            virtual void AddIdToContext()
            {
                if (m_Data.m_Id.IsValid())
                    m_Editor.GetStateContext().InsertId(m_Data.m_Id, &m_Data);
            }

            virtual void GetProperties(eastl::vector<SPropertyDeclaration> &outProperties)
            {
                outProperties.clear();
                eastl::vector<CRegisteredString> temp;
                m_Editor.GetLegalInitialValues(m_Data, temp);
                bool hasInitialState = temp.size() > 1;
                bool hasInitialExpr = false;
                CRegisteredString nameStr = m_Editor.RegisterStr("initial");
                CRegisteredString initialExprStr = m_Editor.RegisterStr("initialexpr");

                for (size_t idx = 0, end = m_PropertyAccessors.size(); idx < end; ++idx) {
                    if (m_PropertyAccessors[idx]->m_Declaration.m_Name == nameStr) {
                        if (hasInitialState)
                            outProperties.push_back(m_PropertyAccessors[idx]->m_Declaration);
                    } else if (m_PropertyAccessors[idx]->m_Declaration.m_Name == initialExprStr) {
                        if (hasInitialExpr)
                            outProperties.push_back(m_PropertyAccessors[idx]->m_Declaration);
                    } else {
                        outProperties.push_back(m_PropertyAccessors[idx]->m_Declaration);
                    }

                    if (m_PropertyAccessors[idx]->m_Declaration.m_Name == nameStr) {
                        TEditorStr initialValue =
                            m_PropertyAccessors[idx]->Get(*this)->getData<TEditorStr>();
                        if (initialValue == "(script expression)")
                            hasInitialExpr = true;
                    }
                }
            }
        };

        struct SPositionalEditor : public SEditorImplObject
        {
            SPositionalEditor(const char8_t *inTypeName, SEditorImpl &inData,
                              const TPropertyAccessorList &inAccessors)
                : SEditorImplObject(inTypeName, inData, inAccessors)
            {
            }
        };

        template <typename TListType, typename TListItem>
        struct TListInsertDeleteChange : public IChange
        {
            TObjPtr m_EditorObject;
            QT3DSI32 mRefCount;
            TListType *m_ParentList;
            TListItem *m_TargetContent;
            QT3DSI32 m_ContentIdx;
            bool m_AddOnDo;
            // The item must be in the list for this to work.
            TListInsertDeleteChange(TObjPtr inEditor, TListType *plist, TListItem *tc, bool addOnDo)
                : m_EditorObject(inEditor)
                , mRefCount(0)
                , m_ParentList(plist)
                , m_TargetContent(tc)
                , m_ContentIdx(-1)
                , m_AddOnDo(addOnDo)
            {
                for (typename TListType::iterator iter = m_ParentList->begin(),
                                                  end = m_ParentList->end();
                     iter != end; ++iter) {
                    if (&(*iter) == m_TargetContent)
                        break;
                    // setup content idx to point to the item just our target item.
                    ++m_ContentIdx;
                }
            }
            virtual void addRef() { atomicIncrement(&mRefCount); }
            virtual void release()
            {
                atomicDecrement(&mRefCount);
                if (mRefCount <= 0) {
                    delete this;
                }
            }
            void add()
            {
                if (m_ContentIdx > -1) {
                    QT3DSI32 idx = m_ContentIdx - 1;
                    typename TListType::iterator iter = m_ParentList->begin();

                    for (typename TListType::iterator end = m_ParentList->end();
                         iter != end && idx > -1; ++iter, --idx) {
                    };

                    if (iter != m_ParentList->end())
                        m_ParentList->insert_after(*iter, *m_TargetContent);
                    else
                        m_ParentList->push_back(*m_TargetContent);
                } else
                    m_ParentList->push_front(*m_TargetContent);
            }
            void remove() { m_ParentList->remove(*m_TargetContent); }

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
            virtual TObjPtr GetEditor() { return m_EditorObject; }
        };

        struct SExecutableContentChange
            : public TListInsertDeleteChange<TExecutableContentList, SExecutableContent>
        {
            typedef TListInsertDeleteChange<TExecutableContentList, SExecutableContent> TBase;
            SExecutableContentChange(TObjPtr inEditor, TExecutableContentList *plist,
                                     SExecutableContent *tc, bool addOnDo)
                : TBase(inEditor, plist, tc, addOnDo)
            {
            }
        };

        struct SStateEditor : public SPositionalEditor
        {
            typedef SState TStateType;
            enum { EditorType = EditorTypes::State };
            SState &m_Data;
            eastl::string m_InitialComboValue;
            bool m_endEdit;
            static const char8_t *GetTypeStr() { return "state"; }

            template <typename TEditorType>
            static void CreateStateNodeChildren(SEditorImpl &inEditorData,
                                                TPropertyAccessorList &retval)
            {
                NVAllocatorCallback &alloc(inEditorData.m_EditorFoundation->getAllocator());
                IPropertyAccessor *newAccessor = QT3DS_NEW(alloc, SChildrenProperty<TEditorType>)(
                    inEditorData.m_EditorFoundation, *inEditorData.m_StringTable);
                retval.push_back(newAccessor);
            }

            template <typename TEditorType>
            static void CreatePositionColorAccessors(SEditorImpl &inEditorData,
                                                     TPropertyAccessorList &retval)
            {
                typedef SOptionAccessorProp<TEditorType, SStateNode, QT3DSVec2> TVec2Access;
                typedef SOptionAccessorProp<TEditorType, SStateNode, QT3DSVec3> TVec3Access;
                NVAllocatorCallback &alloc(inEditorData.m_EditorFoundation->getAllocator());
                retval.push_back(QT3DS_NEW(alloc, TVec2Access)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("position"),
                                         EditorPropertyTypes::Position),
                    &SStateNode::GetPosition, &SStateNode::SetPosition));
                retval.push_back(QT3DS_NEW(alloc, TVec2Access)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("dimension"),
                                         EditorPropertyTypes::Position),
                    &SStateNode::GetDimension, &SStateNode::SetDimension));

                retval.push_back(QT3DS_NEW(alloc, TVec3Access)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("color"),
                                         EditorPropertyTypes::Color),
                    &SStateNode::GetColor, &SStateNode::SetColor));
            }

            static TPropertyAccessorList CreateAccessors(SEditorImpl &inEditorData,
                                                         TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                typedef SDataProp<SStateEditor, SState, STransition *> TInitialProp;
                typedef SDataIdProp<SStateEditor, SState> TIdPropType;
                typedef SInitialTargetProp<SStateEditor, SState> TInitialTargetProp;
                typedef SInitialComboProp<SStateEditor, SState> TInitialComboProp;
                typedef SDataProp<SStateEditor, SState, const char8_t *> TCharProp;

                NVAllocatorCallback &alloc(inEditorData.m_EditorFoundation->getAllocator());

                TPropertyAccessorList &retval(inList);
                retval.push_back(QT3DS_NEW(alloc, TIdPropType)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("id"), EditorPropertyTypes::Id),
                    &SState::m_Id));

                typedef SParentProp<SStateEditor> TParentPropType;
                retval.push_back(QT3DS_NEW(alloc, TParentPropType)(inEditorData.m_EditorFoundation,
                                                                *inEditorData.m_StringTable));
                SEditorImplObject::CreateAccessors(inEditorData, retval, false, false);

                TInitialComboProp *theInitialCombo = QT3DS_NEW(alloc, TInitialComboProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("initial"),
                                         EditorPropertyTypes::StringSet));
                retval.push_back(theInitialCombo);

                TCharProp *theInitialExprProp = QT3DS_NEW(alloc, TCharProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("initialexpr"),
                                         EditorPropertyTypes::BigString),
                    &SState::m_InitialExpr);
                retval.push_back(theInitialExprProp);
                retval.push_back(CreateDataAccessor<SStateEditor>(
                    inEditorData, &SState::m_DataModel, "datamodel", EditorPropertyTypes::Object));
                CreateStateNodeChildren<SStateEditor>(inEditorData, retval);
                CreatePositionColorAccessors<SStateEditor>(inEditorData, retval);
                retval.push_back(QT3DS_NEW(alloc, TInitialTargetProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("is initial target"),
                                         EditorPropertyTypes::Boolean)));
                // Replace the name property with one that writes to our data
                return retval;
            }
            SStateEditor(SState &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SPositionalEditor(GetTypeStr(), inEditorData,
                                    CreateAccessors(inEditorData, inList))
                , m_Data(inData)
                , m_endEdit(false)
            {
            }

            virtual void GetProperties(eastl::vector<SPropertyDeclaration> &outProperties)
            {
                outProperties.clear();
                eastl::vector<CRegisteredString> temp;
                m_Editor.GetLegalInitialValues(m_Data, temp);
                bool hasInitialState = temp.size() > 1;
                bool hasInitialExpr = false;
                CRegisteredString nameStr = m_Editor.RegisterStr("initial");
                CRegisteredString initialExprStr = m_Editor.RegisterStr("initialexpr");
                for (size_t idx = 0, end = m_PropertyAccessors.size(); idx < end; ++idx) {
                    if (m_PropertyAccessors[idx]->m_Declaration.m_Name == nameStr) {
                        if (hasInitialState)
                            outProperties.push_back(m_PropertyAccessors[idx]->m_Declaration);
                    } else if (m_PropertyAccessors[idx]->m_Declaration.m_Name == initialExprStr) {
                        if (hasInitialExpr)
                            outProperties.push_back(m_PropertyAccessors[idx]->m_Declaration);
                    } else
                        outProperties.push_back(m_PropertyAccessors[idx]->m_Declaration);

                    if (hasInitialState
                        && m_PropertyAccessors[idx]->m_Declaration.m_Name == nameStr) {
                        TEditorStr initialValue =
                            m_PropertyAccessors[idx]->Get(*this)->getData<TEditorStr>();
                        if (initialValue == "(script expression)")
                            hasInitialExpr = true;
                    }
                }
            }

            void ApplyInitial(const TEditorStr &comboValue)
            {
                m_endEdit = false;

                if (m_InitialComboValue == "(script expression)"
                    || comboValue == "(script expression)")
                    m_endEdit = true;

                m_InitialComboValue = comboValue;
            }

            virtual bool endEdit()
            {
                bool bTemp = m_endEdit;
                m_endEdit = false;
                return bTemp;
            }

            virtual CRegisteredString GetId() const { return m_Data.m_Id; }
            virtual void RemoveIdFromContext() { m_Editor.GetStateContext().EraseId(m_Data.m_Id); }
            virtual void AddIdToContext()
            {
                if (m_Data.m_Id.IsValid())
                    m_Editor.GetStateContext().InsertId(m_Data.m_Id, &m_Data);
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual void *GetWrappedObject() { return &m_Data; }

            struct SObjListContains
            {
                const TObjList &m_Targets;
                SObjListContains(const TObjList &t)
                    : m_Targets(t)
                {
                }
                bool operator()(const TObjPtr &inObj) const
                {
                    return eastl::find(m_Targets.begin(), m_Targets.end(), inObj.mPtr)
                        != m_Targets.end();
                }
            };

            static void CheckAndRemoveStateFromTransitionProperty(TObjList &removedItems,
                                                                  IEditorObject &inEditorObj,
                                                                  const char8_t *propName)
            {
                Option<SValue> propVal = inEditorObj.GetPropertyValue(propName);
                if (propVal.hasValue()) {
                    if (propVal->getType() == ValueTypes::ObjPtr) {
                        TObjPtr transProp = propVal->getData<TObjPtr>();
                        if (transProp) {
                            TObjList propValValues =
                                transProp->GetPropertyValue("target")->getData<TObjList>();
                            TObjList::iterator lastIter =
                                eastl::remove_if(propValValues.begin(), propValValues.end(),
                                                 SObjListContains(removedItems));
                            if (lastIter != propValValues.end()) {
                                propValValues.erase(lastIter, propValValues.end());
                                if (propValValues.empty())
                                    inEditorObj.SetPropertyValue(propName, TObjPtr());
                                else
                                    transProp->SetPropertyValue("target", propValValues);
                            }
                        }
                    } else if (propVal->getType() == ValueTypes::String) {
                        TEditorStr theIdStr = propVal->getData<TEditorStr>();
                        for (size_t idx = 0, end = removedItems.size(); idx < end; ++idx) {
                            if (removedItems[idx]->GetId() == theIdStr) {
                                inEditorObj.SetPropertyValue(propName, TEditorStr());
                                break;
                            }
                        }
                    }
                }
            }

            static void RemoveTransitionsPointingTo(TObjList &removedItems,
                                                    IEditorObject &inEditorObj)
            {
                if (AreEqual(inEditorObj.TypeName(), "transition")) {
                    TObjList targets = inEditorObj.GetPropertyValue("target")->getData<TObjList>();
                    TObjList::iterator lastIter = eastl::remove_if(targets.begin(), targets.end(),
                                                                   SObjListContains(removedItems));
                    if (lastIter != targets.end()) {
                        targets.erase(lastIter, targets.end());
                        inEditorObj.SetPropertyValue("target", targets);
                        if (targets.size() == 0)
                            inEditorObj.RemoveObjectFromGraph();
                    }
                } else {
                    // state and parallel
                    CheckAndRemoveStateFromTransitionProperty(removedItems, inEditorObj, "initial");
                    // history
                    CheckAndRemoveStateFromTransitionProperty(removedItems, inEditorObj,
                                                              "transition");
                }
                Option<SValue> childListOpt = inEditorObj.GetPropertyValue("children");
                if (childListOpt.hasValue()) {
                    TObjList childList = childListOpt->getData<TObjList>();
                    for (size_t idx = 0, end = childList.size(); idx < end; ++idx)
                        RemoveTransitionsPointingTo(removedItems, *childList[idx]);
                }
            }
            static void RemoveObjectFromGraph(IEditorObject &inEditorObj,
                                              TObjList &outRemovedObjects)
            {
                TObjPtr parentPtr = inEditorObj.Parent();
                outRemovedObjects.push_back(&inEditorObj);
                if (parentPtr)
                    parentPtr->Remove("children", &inEditorObj);

                Option<SValue> nodeChildrenOpt = inEditorObj.GetPropertyValue("children");
                if (nodeChildrenOpt.hasValue()) {
                    TObjList nodeChildren(nodeChildrenOpt->getData<TObjList>());
                    for (size_t idx = 0, end = nodeChildren.size(); idx < end; ++idx)
                        RemoveObjectFromGraph(*nodeChildren[idx], outRemovedObjects);
                }
            }

            virtual TObjPtr Parent() { return m_Editor.ToEditor(m_Data.m_Parent); }

            // Simple and slow as hell.
            static void RemoveObjectFromGraph(SEditorImplObject &inEditorObj)
            {
                TObjList removedItems;
                TObjPtr oldParent = inEditorObj.Parent();
                RemoveObjectFromGraph(inEditorObj, removedItems);
                RemoveTransitionsPointingTo(removedItems, *inEditorObj.m_Editor.GetRoot());
                for (size_t idx = 0, end = removedItems.size(); idx < end; ++idx) {
                    inEditorObj.m_Editor.m_TransactionManager->OnObjectDeleted(removedItems[idx]);
                    inEditorObj.RemoveIdFromContext();
                }
                if (oldParent) {
                    TObjList children =
                        oldParent->GetPropertyValue("children")->getData<TObjList>();
                    for (size_t childIdx = 0, childEnd = children.size(); childIdx < childEnd;
                         ++childIdx) {
                        inEditorObj.m_Editor.CheckAndSetValidHistoryDefault(children[childIdx]);
                    }
                }
            }

            virtual void RemoveObjectFromGraph() { RemoveObjectFromGraph(*this); }

            virtual TEditorStr GetId() { return m_Data.m_Id.c_str(); }

            virtual NVConstDataRef<InterpreterEventTypes::Enum> GetExecutableContentTypes()
            {
                static InterpreterEventTypes::Enum retval[] = { InterpreterEventTypes::StateEnter,
                                                                InterpreterEventTypes::StateExit };
                return toConstDataRef(retval, 2);
            }

            template <typename TListType>
            static TObjList ListDataToEditor(TListType &inList, SEditorImplObject &ioObj)
            {
                TObjList retval;
                typename TListType::iterator iter = inList.begin();
                // Take the first one.
                if (iter != inList.end()) {
                    for (TExecutableContentList::iterator
                             contentIter = iter->m_ExecutableContent.begin(),
                             contentEnd = iter->m_ExecutableContent.end();
                         contentIter != contentEnd; ++contentIter) {
                        TObjPtr editorData = ioObj.m_Editor.ExecutableContentToEditor(*contentIter);
                        retval.push_back(editorData);
                    }
                }
                return retval;
            }

            static TObjList ListDataToEditor(InterpreterEventTypes::Enum inType,
                                             SEntryExitBase &inItem, SEditorImplObject &ioObj)
            {
                switch (inType) {
                case InterpreterEventTypes::StateEnter:
                    return ListDataToEditor(inItem.m_OnEntry, ioObj);
                case InterpreterEventTypes::StateExit:
                    return ListDataToEditor(inItem.m_OnExit, ioObj);
                default:
                    break;
                }
                QT3DS_ASSERT(false);
                return TObjList();
            }

            virtual TObjList GetExecutableContent(InterpreterEventTypes::Enum inType)
            {
                return SStateEditor::ListDataToEditor(inType, m_Data, *this);
            }

            template <typename TListItemType, typename TListType>
            static TObjPtr DoCreateAppendExecutableContent(SEditorImplObject &inObj,
                                                           TListType &inList, const char8_t *inName,
                                                           SStateNode &inNode)
            {
                NVScopedRefCounted<IChange> theNewListItemChange;
                if (inList.empty()) {
                    TListItemType *newType =
                        QT3DS_NEW(inObj.m_Editor.m_AutoAllocator, TListItemType)();
                    inList.push_back(*newType);
                    theNewListItemChange = new TListInsertDeleteChange<TListType, TListItemType>(
                        inObj, &inList, newType, true);
                }
                TListItemType &theFront = *inList.begin();
                TObjPtr retval = inObj.m_Editor.CreateExecutableContent(inNode, inName);
                SExecutableContent *theContent = inObj.m_Editor.ExecutableContentFromEditor(retval);
                theFront.m_ExecutableContent.push_back(*theContent);
                if (inObj.m_Editor.GetOpenTransactionImpl()) {
                    if (theNewListItemChange)
                        inObj.m_Editor.GetOpenTransactionImpl()->m_Changes.push_back(
                            theNewListItemChange);

                    NVScopedRefCounted<SExecutableContentChange> newChange =
                        new SExecutableContentChange(inObj, &theFront.m_ExecutableContent,
                                                     theContent, true);
                    inObj.m_Editor.GetOpenTransactionImpl()->m_Changes.push_back(newChange.mPtr);
                }
                return retval;
            }

            static TObjPtr DoCreateAppendExecutableContent(InterpreterEventTypes::Enum inType,
                                                           const char8_t *inName,
                                                           SEditorImplObject &inObj,
                                                           SEntryExitBase &inItem)
            {
                switch (inType) {
                case InterpreterEventTypes::StateEnter:
                    return DoCreateAppendExecutableContent<SOnEntry>(inObj, inItem.m_OnEntry,
                                                                     inName, inItem);
                case InterpreterEventTypes::StateExit:
                    return DoCreateAppendExecutableContent<SOnExit>(inObj, inItem.m_OnExit, inName,
                                                                    inItem);
                default:
                    break;
                }

                QT3DS_ASSERT(false);
                return TObjPtr();
            }

            virtual TObjPtr CreateAndAppendExecutableContent(InterpreterEventTypes::Enum inType,
                                                             const char8_t *inName)
            {
                return SStateEditor::DoCreateAppendExecutableContent(inType, inName, *this, m_Data);
            }
        };

        struct SParallelEditor : public SPositionalEditor
        {
            typedef SParallel TStateType;
            enum { EditorType = EditorTypes::Parallel };
            SParallel &m_Data;
            static const char8_t *GetTypeStr() { return "parallel"; }

            static TPropertyAccessorList CreateAccessors(SEditorImpl &inEditorData,
                                                         TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;

                typedef SInitialTargetProp<SParallelEditor, SParallel> TInitialTargetProp;
                TPropertyAccessorList &retval(inList);
                typedef SDataIdProp<SParallelEditor, SParallel> TIdPropType;
                typedef SParentProp<SParallelEditor> TParallelParentProp;
                NVAllocatorCallback &alloc(inEditorData.m_EditorFoundation->getAllocator());
                retval.push_back(QT3DS_NEW(alloc, TIdPropType)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("id"), EditorPropertyTypes::Id),
                    &SParallel::m_Id));

                retval.push_back(QT3DS_NEW(alloc, TParallelParentProp)(inEditorData.m_EditorFoundation,
                                                                    *inEditorData.m_StringTable));

                SEditorImplObject::CreateAccessors(inEditorData, retval, false, false);
                retval.push_back(CreateDataAccessor<SParallelEditor>(
                    inEditorData, &SState::m_DataModel, "datamodel", EditorPropertyTypes::Object));
                SStateEditor::CreateStateNodeChildren<SParallelEditor>(inEditorData, retval);
                SStateEditor::CreatePositionColorAccessors<SParallelEditor>(inEditorData, retval);
                // Replace the name property with one that writes to our data
                retval.push_back(QT3DS_NEW(alloc, TInitialTargetProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("is initial target"),
                                         EditorPropertyTypes::Boolean)));
                return retval;
            }

            SParallelEditor(SParallel &inData, SEditorImpl &inEditorData,
                            TPropertyAccessorList &inList)
                : SPositionalEditor(GetTypeStr(), inEditorData,
                                    CreateAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }
            virtual CRegisteredString GetId() const { return m_Data.m_Id; }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual void *GetWrappedObject() { return &m_Data; }

            virtual TObjPtr Parent() { return m_Editor.ToEditor(m_Data.m_Parent); }

            virtual void RemoveObjectFromGraph() { SStateEditor::RemoveObjectFromGraph(*this); }

            virtual TEditorStr GetId() { return m_Data.m_Id.c_str(); }
            virtual void RemoveIdFromContext() { m_Editor.GetStateContext().EraseId(m_Data.m_Id); }

            virtual void AddIdToContext()
            {
                if (m_Data.m_Id.IsValid())
                    m_Editor.GetStateContext().InsertId(m_Data.m_Id, &m_Data);
            }

            virtual NVConstDataRef<InterpreterEventTypes::Enum> GetExecutableContentTypes()
            {
                static InterpreterEventTypes::Enum retval[] = { InterpreterEventTypes::StateEnter,
                                                                InterpreterEventTypes::StateExit };
                return toConstDataRef(retval, 2);
            }

            virtual TObjList GetExecutableContent(InterpreterEventTypes::Enum inType)
            {
                return SStateEditor::ListDataToEditor(inType, m_Data, *this);
            }

            virtual TObjPtr CreateAndAppendExecutableContent(InterpreterEventTypes::Enum inType,
                                                             const char8_t *inName)
            {
                return SStateEditor::DoCreateAppendExecutableContent(inType, inName, *this, m_Data);
            }
        };

        struct SFinalEditor : public SPositionalEditor
        {
            typedef SFinal TStateType;
            enum { EditorType = EditorTypes::Final };

            SFinal &m_Data;
            static const char8_t *GetTypeStr() { return "final"; }

            static TPropertyAccessorList CreateAccessors(SEditorImpl &inEditorData,
                                                         TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                TPropertyAccessorList &retval(inList);
                typedef SDataIdProp<SFinalEditor, SFinal> TIdPropType;
                typedef SInitialTargetProp<SFinalEditor, SFinal> TInitialTargetProp;
                NVAllocatorCallback &alloc(inEditorData.m_EditorFoundation->getAllocator());
                typedef SParentProp<SFinalEditor> TFinalParentProp;
                retval.push_back(QT3DS_NEW(alloc, TIdPropType)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("id"), EditorPropertyTypes::Id),
                    &SFinal::m_Id));
                retval.push_back(QT3DS_NEW(alloc, TFinalParentProp)(inEditorData.m_EditorFoundation,
                                                                 *inEditorData.m_StringTable));
                SEditorImplObject::CreateAccessors(inEditorData, retval, false, false);
                SStateEditor::CreatePositionColorAccessors<SFinalEditor>(inEditorData, retval);
                retval.push_back(QT3DS_NEW(alloc, TInitialTargetProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("is initial target"),
                                         EditorPropertyTypes::Boolean)));
                // Replace the name property with one that writes to our data
                return retval;
            }

            SFinalEditor(SFinal &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SPositionalEditor(GetTypeStr(), inEditorData,
                                    CreateAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }
            virtual CRegisteredString GetId() const { return m_Data.m_Id; }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual void *GetWrappedObject() { return &m_Data; }

            virtual TObjPtr Parent() { return m_Editor.ToEditor(m_Data.m_Parent); }

            virtual void RemoveObjectFromGraph() { SStateEditor::RemoveObjectFromGraph(*this); }

            virtual TEditorStr GetId() { return m_Data.m_Id.c_str(); }
            virtual void RemoveIdFromContext() { m_Editor.GetStateContext().EraseId(m_Data.m_Id); }

            virtual void AddIdToContext()
            {
                if (m_Data.m_Id.IsValid())
                    m_Editor.GetStateContext().InsertId(m_Data.m_Id, &m_Data);
            }

            virtual NVConstDataRef<InterpreterEventTypes::Enum> GetExecutableContentTypes()
            {
                static InterpreterEventTypes::Enum retval[] = { InterpreterEventTypes::StateEnter,
                                                                InterpreterEventTypes::StateExit };
                return toConstDataRef(retval, 2);
            }

            virtual TObjList GetExecutableContent(InterpreterEventTypes::Enum inType)
            {
                return SStateEditor::ListDataToEditor(inType, m_Data, *this);
            }

            virtual TObjPtr CreateAndAppendExecutableContent(InterpreterEventTypes::Enum inType,
                                                             const char8_t *inName)
            {
                return SStateEditor::DoCreateAppendExecutableContent(inType, inName, *this, m_Data);
            }
        };

        struct SHistoryEditor : public SPositionalEditor
        {
            typedef SHistory TStateType;
            enum { EditorType = EditorTypes::History };
            SHistory &m_Data;
            static const char8_t *GetTypeStr() { return "history"; }

            static TPropertyAccessorList CreateAccessors(SEditorImpl &inEditorData,
                                                         TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                typedef SDataProp<SHistoryEditor, SHistory, STransition *> TTransitionProp;

                typedef SFlagBooleanProperty<SHistoryEditor, SHistoryFlags> THistoryFlagsProp;
                typedef SDataIdProp<SHistoryEditor, SHistory> TIdPropType;
                typedef SHistoryTransitionProp<SHistoryEditor> THistoryTransitionProp;
                typedef SParentProp<SHistoryEditor> THistoryParentProp;
                typedef SInitialTargetProp<SHistoryEditor, SHistory> TInitialTargetProp;

                TPropertyAccessorList &retval(inList);
                NVAllocatorCallback &alloc(inEditorData.m_EditorFoundation->getAllocator());
                retval.push_back(QT3DS_NEW(alloc, TIdPropType)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("id"), EditorPropertyTypes::Id),
                    &SHistory::m_Id));
                retval.push_back(QT3DS_NEW(alloc, THistoryParentProp)(inEditorData.m_EditorFoundation,
                                                                   *inEditorData.m_StringTable));
                SEditorImplObject::CreateAccessors(inEditorData, retval, false, false);
                SStateEditor::CreatePositionColorAccessors<SHistoryEditor>(inEditorData, retval);
                retval.push_back(QT3DS_NEW(
                    alloc, THistoryFlagsProp(inEditorData.m_EditorFoundation,
                                             *inEditorData.m_StringTable, "type", "shallow", "deep",
                                             &SHistoryFlags::IsDeep, &SHistoryFlags::SetDeep)));

                retval.push_back(QT3DS_NEW(alloc, TTransitionProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("transition"),
                                         EditorPropertyTypes::ObjectList),
                    &SHistory::m_Transition));

                retval.push_back(QT3DS_NEW(alloc, THistoryTransitionProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("default"),
                                         EditorPropertyTypes::ObjectList)));

                retval.push_back(QT3DS_NEW(alloc, TInitialTargetProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("is initial target"),
                                         EditorPropertyTypes::Boolean)));

                return retval;
            }

            SHistoryEditor(SHistory &inData, SEditorImpl &inEditorData,
                           TPropertyAccessorList &inList)
                : SPositionalEditor(GetTypeStr(), inEditorData,
                                    CreateAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            virtual CRegisteredString GetId() const { return m_Data.m_Id; }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual void *GetWrappedObject() { return &m_Data; }

            virtual void RemoveObjectFromGraph() { SStateEditor::RemoveObjectFromGraph(*this); }

            virtual TObjPtr Parent() { return m_Editor.ToEditor(m_Data.m_Parent); }

            virtual TEditorStr GetId() { return m_Data.m_Id.c_str(); }
            virtual void RemoveIdFromContext() { m_Editor.GetStateContext().EraseId(m_Data.m_Id); }

            virtual void AddIdToContext()
            {
                if (m_Data.m_Id.IsValid())
                    m_Editor.GetStateContext().InsertId(m_Data.m_Id, &m_Data);
            }
        };

        struct STransitionEditor : public SEditorImplObject
        {
            typedef STransition TStateType;
            STransition &m_Data;
            enum { EditorType = EditorTypes::Transition };
            static const char8_t *GetTypeStr() { return "transition"; }
            TVec2List m_PathList;

            static TPropertyAccessorList CreateAccessors(SEditorImpl &inEditorData,
                                                         TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                typedef SDataProp<STransitionEditor, STransition, const char8_t *> TTransCharProp;
                typedef SDataProp<STransitionEditor, STransition, CRegisteredString>
                    TTransRegStrProp;
                typedef SDataProp<STransitionEditor, STransition, NVConstDataRef<SStateNode *>>
                    TTransTargetProp;
                typedef SDataProp<STransitionEditor, STransition, NVConstDataRef<QT3DSVec2>>
                    TTransPathProp;
                typedef SFlagBooleanProperty<STransitionEditor, STransitionFlags>
                    TTransitionFlagsProp;
                typedef SDataIdProp<STransitionEditor, STransition> TIdPropType;
                typedef SOptionAccessorProp<STransitionEditor, STransition, QT3DSVec2> TVec2Access;
                typedef SOptionAccessorProp<STransitionEditor, STransition, QT3DSVec3> TVec3Access;
                typedef SDataIdProp<STransitionEditor, STransition> TIdPropType;

                TPropertyAccessorList &retval(inList);
                NVAllocatorCallback &alloc(inEditorData.m_EditorFoundation->getAllocator());

                retval.push_back(
                    QT3DS_NEW(alloc, TIdPropType)(inEditorData.m_EditorFoundation,
                                               SPropertyDeclaration(inEditorData.RegisterStr("id"),
                                                                    EditorPropertyTypes::String),
                                               &STransition::m_Id));

                retval.push_back(QT3DS_NEW(alloc, TTransRegStrProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("event"),
                                         EditorPropertyTypes::String),
                    &STransition::m_Event));

                SEditorImplObject::CreateAccessors(inEditorData, retval, false, false);
                IPropertyAccessor *accessor = QT3DS_NEW(alloc, TTransCharProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("cond"),
                                         EditorPropertyTypes::BigString),
                    &STransition::m_Condition);

                retval.push_back(accessor);
                accessor = QT3DS_NEW(alloc, TTransTargetProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("target"),
                                         EditorPropertyTypes::ObjectList),
                    &STransition::m_Target);
                retval.push_back(accessor);
                retval.push_back(QT3DS_NEW(alloc, TTransitionFlagsProp)(
                    inEditorData.m_EditorFoundation, *inEditorData.m_StringTable, "type",
                    "external", "internal", &STransitionFlags::IsInternal,
                    &STransitionFlags::SetInternal));

                accessor = QT3DS_NEW(alloc, TTransPathProp)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("path"),
                                         EditorPropertyTypes::PositionList),
                    &STransition::m_Path);
                retval.push_back(accessor);
                retval.push_back(QT3DS_NEW(alloc, TVec2Access)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("position"),
                                         EditorPropertyTypes::PositionList),
                    &STransition::GetPosition, &STransition::SetPosition));

                retval.push_back(QT3DS_NEW(alloc, TVec2Access)(
                    inEditorData.m_EditorFoundation,
                    SPropertyDeclaration(inEditorData.RegisterStr("end position"),
                                         EditorPropertyTypes::PositionList),
                    &STransition::GetEndPosition, &STransition::SetEndPosition));

                return retval;
            }

            STransitionEditor(STransition &inData, SEditorImpl &inEditorData,
                              TPropertyAccessorList &inList)
                : SEditorImplObject(GetTypeStr(), inEditorData,
                                    CreateAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual void *GetWrappedObject() { return &m_Data; }

            virtual void RemoveObjectFromGraph() { SStateEditor::RemoveObjectFromGraph(*this); }

            virtual TEditorStr GetId() { return m_Data.m_Id.c_str(); }
            virtual void RemoveIdFromContext() { m_Editor.GetStateContext().EraseId(m_Data.m_Id); }

            virtual void AddIdToContext()
            {
                if (m_Data.m_Id.IsValid())
                    m_Editor.GetStateContext().InsertId(m_Data.m_Id, &m_Data);
            }

            virtual TObjPtr Parent() { return m_Editor.ToEditor(m_Data.m_Parent); }

            virtual NVConstDataRef<InterpreterEventTypes::Enum> GetExecutableContentTypes()
            {
                static InterpreterEventTypes::Enum data[] = { InterpreterEventTypes::Transition };
                return toConstDataRef(data, 1);
            }

            virtual TObjList GetExecutableContent(InterpreterEventTypes::Enum inType)
            {
                TObjList retval;
                if (inType == InterpreterEventTypes::Transition) {
                    for (TExecutableContentList::iterator iter = m_Data.m_ExecutableContent.begin(),
                                                          end = m_Data.m_ExecutableContent.end();
                         iter != end; ++iter) {
                        retval.push_back(m_Editor.ExecutableContentToEditor(*iter));
                    }
                }
                return retval;
            }
            virtual TObjPtr CreateAndAppendExecutableContent(InterpreterEventTypes::Enum inType,
                                                             const char8_t *inName)
            {
                TObjPtr retval;
                if (inType == InterpreterEventTypes::Transition) {
                    retval = m_Editor.CreateExecutableContent(m_Data, inName);
                    SExecutableContent *theContent = m_Editor.ExecutableContentFromEditor(retval);
                    m_Data.m_ExecutableContent.push_back(*theContent);
                    if (m_Editor.GetOpenTransactionImpl()) {
                        NVScopedRefCounted<SExecutableContentChange> newChange =
                            new SExecutableContentChange(*this, &m_Data.m_ExecutableContent,
                                                         theContent, true);
                        m_Editor.GetOpenTransactionImpl()->m_Changes.push_back(newChange.mPtr);
                    }
                }
                return retval;
            }
        };

        struct SExecutableContentParentInfo
        {
            SExecutableContent &content;
            SEditorImpl &m_Editor;
            TExecutableContentList *parentList;
            TObjPtr editorObj;
            TOnEntryList *entryList;
            SOnEntry *entryItem;
            TOnExitList *exitList;
            SOnExit *exitItem;
            SExecutableContentParentInfo(SExecutableContent &c, SEditorImpl &e)
                : content(c)
                , m_Editor(e)
                , parentList(NULL)
                , entryList(NULL)
                , entryItem(NULL)
                , exitList(NULL)
                , exitItem(NULL)
            {
                if (GetContent().m_StateNodeParent) {
                    editorObj = m_Editor.ToEditor(*GetContent().m_StateNodeParent);
                    if (GetContent().m_StateNodeParent->m_Type == StateNodeTypes::Transition)
                        parentList = &static_cast<STransition *>(GetContent().m_StateNodeParent)
                                          ->m_ExecutableContent;
                    else {
                        SExecutableContent *targetContent = &GetContent();
                        entryList = GetContent().m_StateNodeParent->GetOnEntryList();
                        exitList = GetContent().m_StateNodeParent->GetOnExitList();
                        if (entryList) {
                            for (TOnEntryList::iterator iter = entryList->begin(),
                                                        end = entryList->end();
                                 iter != end && parentList == NULL; ++iter) {
                                for (TExecutableContentList::iterator
                                         contentIter = iter->m_ExecutableContent.begin(),
                                         contentEnd = iter->m_ExecutableContent.end();
                                     contentIter != contentEnd && parentList == NULL;
                                     ++contentIter) {
                                    if (&(*contentIter) == targetContent) {
                                        parentList = &iter->m_ExecutableContent;
                                        exitList = NULL;
                                        entryItem = &(*iter);
                                    }
                                }
                            }
                        }
                        if (parentList == NULL && exitList != NULL) {
                            for (TOnExitList::iterator iter = exitList->begin(),
                                                       end = exitList->end();
                                 iter != end && parentList == NULL; ++iter) {
                                for (TExecutableContentList::iterator
                                         contentIter = iter->m_ExecutableContent.begin(),
                                         contentEnd = iter->m_ExecutableContent.end();
                                     contentIter != contentEnd && parentList == NULL;
                                     ++contentIter) {
                                    if (&(*contentIter) == targetContent) {
                                        parentList = &iter->m_ExecutableContent;
                                        entryList = NULL;
                                        exitItem = &(*iter);
                                    }
                                }
                            }
                        }
                    }
                } else {
                    editorObj = m_Editor.ToEditor(GetContent().m_Parent);
                    parentList = &GetContent().m_Parent->m_Children;
                }
            }
            SExecutableContent &GetContent() { return content; }
        };

        struct SExecutableContentEditor : public SEditorImplObject
        {
            SExecutableContentEditor(const char8_t *inTypeStr, SEditorImpl &inEditorData,
                                     const TPropertyAccessorList &inList)
                : SEditorImplObject(inTypeStr, inEditorData, inList)
            {
            }

            virtual SExecutableContent &GetContent() = 0;

            virtual void RemoveObjectFromGraph()
            {
                SExecutableContentParentInfo parentInfo(GetContent(), m_Editor);
                TExecutableContentList *parentList = parentInfo.parentList;
                TObjPtr editorObj = parentInfo.editorObj;
                TOnEntryList *entryList = parentInfo.entryList;
                SOnEntry *entryItem = parentInfo.entryItem;
                TOnExitList *exitList = parentInfo.exitList;
                SOnExit *exitItem = parentInfo.exitItem;

                if (parentList) {
                    NVScopedRefCounted<SExecutableContentChange> change =
                        new SExecutableContentChange(editorObj, parentList, &GetContent(), false);
                    change->Do();
                    if (m_Editor.GetOpenTransactionImpl())
                        m_Editor.GetOpenTransactionImpl()->m_Changes.push_back(change.mPtr);
                    // Perhaps remove the item itself
                    if (parentList->empty()) {
                        NVScopedRefCounted<IChange> newChange;
                        if (entryItem)
                            newChange = new TListInsertDeleteChange<TOnEntryList, SOnEntry>(
                                editorObj, entryList, entryItem, false);
                        else if (exitItem)
                            newChange = new TListInsertDeleteChange<TOnExitList, SOnExit>(
                                editorObj, exitList, exitItem, false);

                        if (newChange) {
                            newChange->Do();
                            if (m_Editor.GetOpenTransactionImpl())
                                m_Editor.GetOpenTransactionImpl()->m_Changes.push_back(newChange);
                        }
                    }
                }
            }
        };

        struct SSendEditor : public SExecutableContentEditor
        {
            typedef SSend TStateType;
            enum { EditorType = EditorTypes::Send };
            SSend &m_Data;
            static const char8_t *GetTypeStr() { return "send"; }
            static TPropertyAccessorList GetPropertyAccessors(SEditorImpl &inData,
                                                              TPropertyAccessorList &inList)
            {
                typedef SDataIdProp<SSendEditor, SSend> TIdPropType;

                if (inList.size())
                    return inList;
                TPropertyAccessorList &retval(inList);
                retval.push_back(CreateDataAccessor<SSendEditor>(inData, &SSend::m_Event, "event",
                                                                 EditorPropertyTypes::String));
                NVFoundationBase &fnd = inData.m_EditorFoundation->getFoundation();

                retval.push_back(QT3DS_NEW(fnd.getAllocator(), TIdPropType)(
                    inData.m_EditorFoundation,
                    SPropertyDeclaration(inData.RegisterStr("id"), EditorPropertyTypes::Id),
                    &SSend::m_Id));

                retval.push_back(QT3DS_NEW(fnd.getAllocator(), SDelayProp<SSendEditor>)(
                    inData.m_EditorFoundation,
                    SPropertyDeclaration(inData.RegisterStr("delay"), EditorPropertyTypes::U32)));
                return retval;
            }
            SSendEditor(SSend &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SExecutableContentEditor(GetTypeStr(), inEditorData,
                                           GetPropertyAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual SExecutableContent &GetContent() { return m_Data; }
            virtual void *GetWrappedObject() { return &m_Data; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SRaiseEditor : public SExecutableContentEditor
        {
            typedef SRaise TStateType;
            enum { EditorType = EditorTypes::Raise };
            SRaise &m_Data;
            static const char8_t *GetTypeStr() { return "raise"; }
            static TPropertyAccessorList GetPropertyAccessors(SEditorImpl &inData,
                                                              TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                TPropertyAccessorList &retval(inList);
                retval.push_back(CreateDataAccessor<SRaiseEditor>(inData, &SRaise::m_Event, "event",
                                                                  EditorPropertyTypes::String));
                return retval;
            }
            SRaiseEditor(SRaise &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SExecutableContentEditor(GetTypeStr(), inEditorData,
                                           GetPropertyAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual SExecutableContent &GetContent() { return m_Data; }
            virtual void *GetWrappedObject() { return &m_Data; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SLogEditor : public SExecutableContentEditor
        {
            typedef SLog TStateType;
            enum { EditorType = EditorTypes::Log };
            SLog &m_Data;
            static const char8_t *GetTypeStr() { return "log"; }
            static TPropertyAccessorList GetPropertyAccessors(SEditorImpl &inData,
                                                              TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                TPropertyAccessorList &retval(inList);
                retval.push_back(CreateDataAccessor<SLogEditor>(inData, &SLog::m_Label, "label",
                                                                EditorPropertyTypes::String));
                retval.push_back(CreateDataAccessor<SLogEditor>(inData, &SLog::m_Expression, "expr",
                                                                EditorPropertyTypes::String));
                return retval;
            }
            SLogEditor(SLog &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SExecutableContentEditor(GetTypeStr(), inEditorData,
                                           GetPropertyAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual SExecutableContent &GetContent() { return m_Data; }
            virtual void *GetWrappedObject() { return &m_Data; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SAssignEditor : public SExecutableContentEditor
        {
            typedef SAssign TStateType;
            enum { EditorType = EditorTypes::Assign };
            SAssign &m_Data;
            static const char8_t *GetTypeStr() { return "assign"; }
            static TPropertyAccessorList GetPropertyAccessors(SEditorImpl &inData,
                                                              TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                TPropertyAccessorList &retval(inList);
                retval.push_back(CreateDataAccessor<SAssignEditor>(
                    inData, &SAssign::m_Location, "location", EditorPropertyTypes::String));
                retval.push_back(CreateDataAccessor<SAssignEditor>(
                    inData, &SAssign::m_Expression, "expr", EditorPropertyTypes::String));
                return retval;
            }
            SAssignEditor(SAssign &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SExecutableContentEditor(GetTypeStr(), inEditorData,
                                           GetPropertyAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual SExecutableContent &GetContent() { return m_Data; }
            virtual void *GetWrappedObject() { return &m_Data; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SIfEditor : public SExecutableContentEditor
        {
            typedef SIf TStateType;
            enum { EditorType = EditorTypes::If };
            SIf &m_Data;
            static const char8_t *GetTypeStr() { return "if"; }
            static TPropertyAccessorList GetPropertyAccessors(SEditorImpl &inData,
                                                              TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                TPropertyAccessorList &retval(inList);
                retval.push_back(CreateDataAccessor<SIfEditor>(inData, &SIf::m_Cond, "cond",
                                                               EditorPropertyTypes::String));
                return retval;
            }

            SIfEditor(SIf &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SExecutableContentEditor(GetTypeStr(), inEditorData,
                                           GetPropertyAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual SExecutableContent &GetContent() { return m_Data; }
            virtual void *GetWrappedObject() { return &m_Data; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SElseIfEditor : public SExecutableContentEditor
        {
            typedef SElseIf TStateType;
            enum { EditorType = EditorTypes::ElseIf };
            SElseIf &m_Data;
            static const char8_t *GetTypeStr() { return "elseif"; }
            static TPropertyAccessorList GetPropertyAccessors(SEditorImpl &inData,
                                                              TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                TPropertyAccessorList &retval(inList);
                retval.push_back(CreateDataAccessor<SElseIfEditor>(inData, &SElseIf::m_Cond, "cond",
                                                                   EditorPropertyTypes::String));
                return retval;
            }

            SElseIfEditor(SElseIf &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SExecutableContentEditor(GetTypeStr(), inEditorData,
                                           GetPropertyAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual SExecutableContent &GetContent() { return m_Data; }
            virtual void *GetWrappedObject() { return &m_Data; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SElseEditor : public SExecutableContentEditor
        {
            typedef SElse TStateType;
            enum { EditorType = EditorTypes::Else };
            SElse &m_Data;
            static const char8_t *GetTypeStr() { return "else"; }

            SElseEditor(SElse &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SExecutableContentEditor(GetTypeStr(), inEditorData, inList)
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual SExecutableContent &GetContent() { return m_Data; }
            virtual void *GetWrappedObject() { return &m_Data; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SScriptEditor : public SExecutableContentEditor
        {
            typedef SScript TStateType;
            enum { EditorType = EditorTypes::Script };
            SScript &m_Data;
            static const char8_t *GetTypeStr() { return "script"; }

            static TPropertyAccessorList GetPropertyAccessors(SEditorImpl &inData,
                                                              TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                TPropertyAccessorList &retval(inList);
                retval.push_back(CreateDataAccessor<SScriptEditor>(
                    inData, &SScript::m_Data, "content", EditorPropertyTypes::BigString));
                return retval;
            }

            SScriptEditor(SScript &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SExecutableContentEditor(GetTypeStr(), inEditorData,
                                           GetPropertyAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual SExecutableContent &GetContent() { return m_Data; }
            virtual void *GetWrappedObject() { return &m_Data; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SCancelEditor : public SExecutableContentEditor
        {
            typedef SData TStateType;
            enum { EditorType = EditorTypes::Cancel };
            SCancel &m_Data;
            static const char8_t *GetTypeStr() { return "cancel"; }

            static TPropertyAccessorList GetPropertyAccessors(SEditorImpl &inData,
                                                              TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                TPropertyAccessorList retval;
                retval.push_back(CreateDataAccessor<SCancelEditor>(
                    inData, &SCancel::m_Send, "sendid", EditorPropertyTypes::Object));
                retval.push_back(CreateDataAccessor<SCancelEditor>(
                    inData, &SCancel::m_IdExpression, "sendidexpr", EditorPropertyTypes::String));
                return retval;
            }

            SCancelEditor(SCancel &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SExecutableContentEditor(GetTypeStr(), inEditorData,
                                           GetPropertyAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual SExecutableContent &GetContent() { return m_Data; }
            virtual void *GetWrappedObject() { return &m_Data; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SDataModelEditor : public SEditorImplObject
        {
            typedef SDataModel TStateType;
            enum { EditorType = EditorTypes::DataModel };
            SDataModel &m_Data;
            static const char8_t *GetTypeStr() { return "datamodel"; }

            static TPropertyAccessorList GetPropertyAccessors(SEditorImpl &inData,
                                                              TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                TPropertyAccessorList &retval(inList);
                retval.push_back(CreateDataAccessor<SDataModelEditor>(
                    inData, &SDataModel::m_Data, "data", EditorPropertyTypes::ObjectList));
                return retval;
            }

            SDataModelEditor(SDataModel &inData, SEditorImpl &inEditorData,
                             TPropertyAccessorList &inList)
                : SEditorImplObject(GetTypeStr(), inEditorData,
                                    GetPropertyAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual void *GetWrappedObject() { return &m_Data; }

            virtual CRegisteredString GetId() const { return m_Data.m_Id; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SDataEditor : public SEditorImplObject
        {
            typedef SData TStateType;
            enum { EditorType = EditorTypes::Data };
            SData &m_Data;
            static const char8_t *GetTypeStr() { return "data"; }
            static TPropertyAccessorList GetPropertyAccessors(SEditorImpl &inData,
                                                              TPropertyAccessorList &inList)
            {
                if (inList.size())
                    return inList;
                TPropertyAccessorList retval;
                retval.push_back(CreateDataAccessor<SDataEditor>(inData, &SData::m_Id, "id",
                                                                 EditorPropertyTypes::String));
                retval.push_back(CreateDataAccessor<SDataEditor>(
                    inData, &SData::m_Expression, "expr", EditorPropertyTypes::String));
                return retval;
            }

            SDataEditor(SData &inData, SEditorImpl &inEditorData, TPropertyAccessorList &inList)
                : SEditorImplObject(GetTypeStr(), inEditorData,
                                    GetPropertyAccessors(inEditorData, inList))
                , m_Data(inData)
            {
            }

            UICSTATE_EDITOR_OBJECT_IMPLEMENT_ADDREF_RELEASE;

            virtual void *GetWrappedObject() { return &m_Data; }

            virtual CRegisteredString GetId() const { return m_Data.m_Id; }
            virtual void RemoveIdFromContext() {}
            virtual void AddIdToContext() {}
        };

        struct SNullEditor
        {
            int m_Data;
            static const char8_t *GetTypeStr() { return ""; }
        };

        template <typename TStateType>
        struct SStateEditorMap
        {
            typedef SNullEditor TEditorType;
            template <typename TIgnored>
            static TObjPtr CreateEditor(TIgnored &, SEditorImpl &, TAccessorMap &)
            {
                return TObjPtr();
            }
        };

        template <typename TEditorType>
        struct SEditorImplStateMap
        {
            typedef int TStateType;
        };

        static inline TPropertyAccessorList &GetAccessorList(TAccessorMap &inMap, int inAccessor)
        {
            return inMap.insert(eastl::make_pair(inAccessor, TPropertyAccessorList()))
                .first->second;
        }

        template <>
        struct SStateEditorMap<STransition>
        {
            typedef STransition stateType;
            typedef STransitionEditor TEditorType;
            typedef STransitionEditor editorType;
            static TObjPtr CreateEditor(stateType &inData, SEditorImpl &inEditorData,
                                        TAccessorMap &inAccessors)
            {
                return QT3DS_NEW(inEditorData.m_EditorFoundation->getAllocator(), TEditorType)(
                    inData, inEditorData,
                    GetAccessorList(inAccessors, (int)editorType::EditorType));
            }
            static TObjPtr CreateEditor(SEditorImpl &inEditorData, TAccessorMap &inAccessors)
            {
                STransition *newTransition = QT3DS_NEW(inEditorData.m_AutoAllocator, stateType)();
                newTransition->m_Flags.SetInternal(true);
                return CreateEditor(*newTransition, inEditorData, inAccessors);
            }
        };
        template <>
        struct SEditorImplStateMap<STransitionEditor>
        {
            typedef STransition TStateType;
        };

#define DEFINE_STATE_EDITOR_TYPE_MAP(stateType, editorType)                                        \
    template <>                                                                                    \
    struct SStateEditorMap<stateType>                                                              \
    {                                                                                              \
        typedef editorType TEditorType;                                                            \
        static TObjPtr CreateEditor(stateType &inData, SEditorImpl &inEditorData,                  \
                                    TAccessorMap &inAccessors)                                     \
        {                                                                                          \
            return QT3DS_NEW(inEditorData.m_EditorFoundation->getAllocator(), TEditorType)(           \
                inData, inEditorData, GetAccessorList(inAccessors, (int)editorType::EditorType));  \
        }                                                                                          \
        static TObjPtr CreateEditor(SEditorImpl &inEditorData, TAccessorMap &inAccessors)          \
        {                                                                                          \
            return CreateEditor(*QT3DS_NEW(inEditorData.m_AutoAllocator, stateType)(), inEditorData,  \
                                inAccessors);                                                      \
        }                                                                                          \
    };                                                                                             \
    template <>                                                                                    \
    struct SEditorImplStateMap<editorType>                                                         \
    {                                                                                              \
        typedef stateType TStateType;                                                              \
    };

        DEFINE_STATE_EDITOR_TYPE_MAP(SSCXML, SSCXMLEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SState, SStateEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SParallel, SParallelEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SHistory, SHistoryEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SFinal, SFinalEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SSend, SSendEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SRaise, SRaiseEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SLog, SLogEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SAssign, SAssignEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SIf, SIfEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SElseIf, SElseIfEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SElse, SElseEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SScript, SScriptEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SDataModel, SDataModelEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SData, SDataEditor);
        DEFINE_STATE_EDITOR_TYPE_MAP(SCancel, SCancelEditor);

#undef DEFINE_STATE_EDITOR_TYPE_MAP
    }
}
}
#endif

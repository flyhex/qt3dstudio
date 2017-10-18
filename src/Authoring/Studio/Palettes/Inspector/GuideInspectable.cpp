/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "stdafx.h"
#include "GuideInspectable.h"
#include "InspectableBase.h"
#include "Core.h"
#include "Doc.h"
#include "UICDMGuides.h"
#include "EasyInspectorGroup.h"
#include "IDocumentEditor.h"
#include "UICDMDataTypes.h"
#include "IInspectableItem.h"
#include "UICDMValue.h"

typedef std::function<qt3dsdm::SValue()> TGetterFunc;
typedef std::function<void(qt3dsdm::SValue)> TSetterFunc;
typedef std::function<void()> TCommitFunc;
typedef std::function<void()> TCancelFunc;

struct SInspectableDataInfo
{
    Q3DStudio::CString m_Name;
    Q3DStudio::CString m_FormalName;
    Q3DStudio::CString m_Description;
    TGetterFunc m_Getter;
    TSetterFunc m_Setter;
    TCommitFunc m_Commit;
    TCancelFunc m_Cancel;

    SInspectableDataInfo(const Q3DStudio::CString &name, const Q3DStudio::CString &formalName,
                         const Q3DStudio::CString &description, TGetterFunc getter, TSetterFunc setter,
                         TCommitFunc commit, TCancelFunc inCancel)
        : m_Name(name)
        , m_FormalName(formalName)
        , m_Description(description)
        , m_Getter(getter)
        , m_Setter(setter)
        , m_Commit(commit)
        , m_Cancel(inCancel)
    {
    }
};

struct SComboAttItem : public IInspectableAttributeItem
{
    SInspectableDataInfo m_BaseInspectableInfo;
    qt3dsdm::TMetaDataStringList m_MetaDataTypes;
    SComboAttItem(const SInspectableDataInfo &inInfo, const qt3dsdm::TMetaDataStringList &inTypes)
        : m_BaseInspectableInfo(inInfo)
        , m_MetaDataTypes(inTypes)
    {
    }
    qt3dsdm::HandlerArgumentType::Value GetInspectableSubType() const override
    {
        return qt3dsdm::HandlerArgumentType::Property;
    }
    Q3DStudio::CString GetInspectableName() const override { return m_BaseInspectableInfo.m_Name; }
    Q3DStudio::CString GetInspectableFormalName() const override
    {
        return m_BaseInspectableInfo.m_FormalName;
    }
    Q3DStudio::CString GetInspectableDescription() const override
    {
        return m_BaseInspectableInfo.m_Description;
    }

    qt3dsdm::SValue GetInspectableData() const override { return m_BaseInspectableInfo.m_Getter(); }
    void SetInspectableData(const qt3dsdm::SValue &inValue) override
    {
        m_BaseInspectableInfo.m_Setter(inValue);
        m_BaseInspectableInfo.m_Commit();
    }

    float GetInspectableMin() const override { return 0; }
    float GetInspectableMax() const override { return 0; }
    qt3dsdm::TMetaDataStringList GetInspectableList() const override { return m_MetaDataTypes; }
    qt3dsdm::DataModelDataType::Value GetInspectableType() const override
    {
        return qt3dsdm::DataModelDataType::String;
    }
    qt3dsdm::AdditionalMetaDataType::Value GetInspectableAdditionalType() const override
    {
        return qt3dsdm::AdditionalMetaDataType::StringList;
    }
};

struct SFloatIntItem : public IInspectableAttributeItem
{
    SInspectableDataInfo m_BaseInspectableInfo;
    qt3dsdm::DataModelDataType::Value m_DataType;
    float m_Min;
    float m_Max;
    SFloatIntItem(const SInspectableDataInfo &inInfo, qt3dsdm::DataModelDataType::Value inType,
                  float inMin = 0, float inMax = 0)
        : m_BaseInspectableInfo(inInfo)
        , m_DataType(inType)
        , m_Min(inMin)
        , m_Max(inMax)
    {
    }
    qt3dsdm::HandlerArgumentType::Value GetInspectableSubType() const override
    {
        return qt3dsdm::HandlerArgumentType::Property;
    }
    Q3DStudio::CString GetInspectableName() const override { return m_BaseInspectableInfo.m_Name; }
    Q3DStudio::CString GetInspectableFormalName() const override
    {
        return m_BaseInspectableInfo.m_FormalName;
    }
    Q3DStudio::CString GetInspectableDescription() const override
    {
        return m_BaseInspectableInfo.m_Description;
    }

    qt3dsdm::SValue GetInspectableData() const override { return m_BaseInspectableInfo.m_Getter(); }
    void SetInspectableData(const qt3dsdm::SValue &inValue) override
    {
        m_BaseInspectableInfo.m_Setter(inValue);
        m_BaseInspectableInfo.m_Commit();
    }

    void ChangeInspectableData(const qt3dsdm::SValue &inValue) override
    {
        m_BaseInspectableInfo.m_Setter(inValue);
    }
    void CancelInspectableData() override { m_BaseInspectableInfo.m_Cancel(); }

    float GetInspectableMin() const override { return m_Min; }
    float GetInspectableMax() const override { return m_Max; }
    qt3dsdm::TMetaDataStringList GetInspectableList() const override
    {
        return qt3dsdm::TMetaDataStringList();
    }
    qt3dsdm::DataModelDataType::Value GetInspectableType() const override { return m_DataType; }
    qt3dsdm::AdditionalMetaDataType::Value GetInspectableAdditionalType() const override
    {
        return qt3dsdm::AdditionalMetaDataType::None;
    }
};


CInspectableBase *CGuideInspectable::CreateInspectable(CCore &inCore,
                                                       qt3dsdm::CUICDMGuideHandle inGuide)
{
    return new SGuideInspectableImpl(inCore, inGuide);
}

SGuideInspectableImpl::SGuideInspectableImpl(CCore &inCore, qt3dsdm::CUICDMGuideHandle inGuide)
    : CInspectableBase(&inCore)
    , m_Guide(inGuide)
    , m_Editor(*inCore.GetDoc())
{
}

Q3DStudio::IDocumentReader &SGuideInspectableImpl::Reader() const
{
    return m_Core->GetDoc()->GetDocumentReader();
}

EStudioObjectType SGuideInspectableImpl::GetObjectType()
{
    return OBJTYPE_GUIDE;
}

Q3DStudio::CString SGuideInspectableImpl::GetName()
{
    return L"Guide";
}

long SGuideInspectableImpl::GetGroupCount()
{
    return 1;
}

CInspectorGroup *SGuideInspectableImpl::GetGroup(long)
{
    CDoc *theDoc = m_Core->GetDoc();
    TCommitFunc theCommiter = std::bind(&SGuideInspectableImpl::Commit, this);
    TCancelFunc theCanceler = std::bind(&SGuideInspectableImpl::Rollback, this);
    m_Properties.push_back(std::make_shared<SFloatIntItem>(
                               SInspectableDataInfo("Position", "Position", "Position of the guide",
                                                    std::bind(&SGuideInspectableImpl::GetPosition, this),
                                                    std::bind(&SGuideInspectableImpl::SetPosition, this,
                                                              std::placeholders::_1),
                                                    theCommiter, theCanceler),
                               qt3dsdm::DataModelDataType::Float));
    qt3dsdm::TMetaDataStringList theComboItems;
    theComboItems.push_back(L"Horizontal");
    theComboItems.push_back(L"Vertical");

    m_Properties.push_back(std::make_shared<SComboAttItem>(
                               SInspectableDataInfo("Direction", "Direction", "Direction of the guide",
                                                    std::bind(&SGuideInspectableImpl::GetDirection, this),
                                                    std::bind(&SGuideInspectableImpl::SetDirection, this,
                                                              std::placeholders::_1),
                                                    theCommiter, theCanceler),
                               theComboItems));

    m_Properties.push_back(std::make_shared<SFloatIntItem>(
                               SInspectableDataInfo("Width", "Width", "Width of the guide",
                                                    std::bind(&SGuideInspectableImpl::GetWidth, this),
                                                    std::bind(&SGuideInspectableImpl::SetWidth, this, std::placeholders::_1),
                                                    theCommiter, theCanceler),
                               qt3dsdm::DataModelDataType::Long, 1.0f, 50.0f));

    CEasyInspectorGroup *theNewGroup = new CEasyInspectorGroup(QObject::tr("Basic"));
    return theNewGroup;
}

bool SGuideInspectableImpl::IsValid() const
{
    return Reader().IsGuideValid(m_Guide);
}

bool SGuideInspectableImpl::IsMaster()
{
    return true;
}

void SGuideInspectableImpl::SetDirection(const qt3dsdm::SValue &inValue)
{
    qt3dsdm::TDataStrPtr theData = inValue.getData<qt3dsdm::TDataStrPtr>();
    qt3dsdm::SGuideInfo theSetter(Reader().GetGuideInfo(m_Guide));
    if (theData) {
        if (qt3dsdm::AreEqual(theData->GetData(), L"Horizontal"))
            theSetter.m_Direction = qt3dsdm::GuideDirections::Horizontal;
        else if (qt3dsdm::AreEqual(theData->GetData(), L"Vertical"))
            theSetter.m_Direction = qt3dsdm::GuideDirections::Vertical;
    }
    Editor().UpdateGuide(m_Guide, theSetter);
    FireRefresh();
}

qt3dsdm::SValue SGuideInspectableImpl::GetDirection()
{
    switch (Reader().GetGuideInfo(m_Guide).m_Direction) {
    case qt3dsdm::GuideDirections::Horizontal:
        return std::make_shared<qt3dsdm::CDataStr>(L"Horizontal");
    case qt3dsdm::GuideDirections::Vertical:
        return std::make_shared<qt3dsdm::CDataStr>(L"Vertical");
    default:
        return std::make_shared<qt3dsdm::CDataStr>(L"");
    }
}

void SGuideInspectableImpl::SetPosition(const qt3dsdm::SValue &inValue)
{
    float thePos = inValue.getData<float>();
    qt3dsdm::SGuideInfo theSetter(Reader().GetGuideInfo(m_Guide));
    theSetter.m_Position = thePos;
    Editor().UpdateGuide(m_Guide, theSetter);
    FireRefresh();
}

qt3dsdm::SValue SGuideInspectableImpl::GetPosition()
{
    qt3dsdm::SGuideInfo theSetter(Reader().GetGuideInfo(m_Guide));
    return theSetter.m_Position;
}

void SGuideInspectableImpl::SetWidth(const qt3dsdm::SValue &inValue)
{
    auto theData = inValue.getData<qt3ds::QT3DSI32>();

    qt3dsdm::SGuideInfo theSetter(Reader().GetGuideInfo(m_Guide));
    theSetter.m_Width = theData;
    Editor().UpdateGuide(m_Guide, theSetter);
    FireRefresh();

}

qt3dsdm::SValue SGuideInspectableImpl::GetWidth()
{
    qt3dsdm::SGuideInfo theSetter(Reader().GetGuideInfo(m_Guide));
    return theSetter.m_Width;
}

Q3DStudio::IDocumentEditor &SGuideInspectableImpl::Editor()
{
    return m_Editor.EnsureEditor(L"Set Property", __FILE__, __LINE__);
}

void SGuideInspectableImpl::Commit()
{
    m_Editor.CommitEditor();
}

void SGuideInspectableImpl::Rollback()
{
    m_Editor.RollbackEditor();
}

void SGuideInspectableImpl::FireRefresh()
{
    m_Editor.FireImmediateRefresh(qt3dsdm::Qt3DSDMInstanceHandle());
}

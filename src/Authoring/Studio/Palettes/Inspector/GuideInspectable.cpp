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
#include "GuideInspectable.h"
#include "InspectableBase.h"
#include "StudioApp.h"
#include "Core.h"
#include "Doc.h"
#include "Qt3DSDMGuides.h"
#include "InspectorGroup.h"
#include "IDocumentEditor.h"
#include "Qt3DSDMDataTypes.h"
#include "IInspectableItem.h"
#include "Qt3DSDMValue.h"

typedef std::function<qt3dsdm::SValue()> TGetterFunc;
typedef std::function<void(qt3dsdm::SValue)> TSetterFunc;
typedef std::function<void()> TCommitFunc;
typedef std::function<void()> TCancelFunc;

struct SInspectableDataInfo
{
    QString m_Name;
    QString m_FormalName;
    QString m_Description;
    TGetterFunc m_Getter;
    TSetterFunc m_Setter;
    TCommitFunc m_Commit;
    TCancelFunc m_Cancel;

    SInspectableDataInfo(const QString &name, const QString &formalName,
                         const QString &description, TGetterFunc getter, TSetterFunc setter,
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
    QString GetInspectableName() const override { return m_BaseInspectableInfo.m_Name; }
    QString GetInspectableFormalName() const override
    {
        return m_BaseInspectableInfo.m_FormalName;
    }
    QString GetInspectableDescription() const override
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
    QString GetInspectableName() const override { return m_BaseInspectableInfo.m_Name; }
    QString GetInspectableFormalName() const override
    {
        return m_BaseInspectableInfo.m_FormalName;
    }
    QString GetInspectableDescription() const override
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
        if (m_Max > 0)
            return qt3dsdm::AdditionalMetaDataType::Range;
        else
            return qt3dsdm::AdditionalMetaDataType::None;
    }
};

GuideInspectable::GuideInspectable(qt3dsdm::Qt3DSDMGuideHandle inGuide)
    : m_Guide(inGuide)
    , m_Editor(*g_StudioApp.GetCore()->GetDoc())
{
}

Q3DStudio::IDocumentReader &GuideInspectable::Reader() const
{
    return g_StudioApp.GetCore()->GetDoc()->GetDocumentReader();
}

EStudioObjectType GuideInspectable::getObjectType() const
{
    return OBJTYPE_GUIDE;
}

Q3DStudio::CString GuideInspectable::getName()
{
    return L"Guide";
}

long GuideInspectable::getGroupCount() const
{
    return 1;
}

CInspectorGroup *GuideInspectable::getGroup(long)
{
    TCommitFunc theCommiter = std::bind(&GuideInspectable::Commit, this);
    TCancelFunc theCanceler = std::bind(&GuideInspectable::Rollback, this);
    m_Properties.push_back(
                std::make_shared<SFloatIntItem>(
                    SInspectableDataInfo(QStringLiteral("Position"), QObject::tr("Position"),
                                         QObject::tr("Position of the guide"),
                                         std::bind(&GuideInspectable::GetPosition, this),
                                         std::bind(&GuideInspectable::SetPosition, this,
                                                   std::placeholders::_1),
                                         theCommiter, theCanceler),
                    qt3dsdm::DataModelDataType::Float));
    qt3dsdm::TMetaDataStringList theComboItems;
    theComboItems.push_back(L"Horizontal");
    theComboItems.push_back(L"Vertical");

    m_Properties.push_back(
                std::make_shared<SComboAttItem>(
                    SInspectableDataInfo(QStringLiteral("Orientation"), QObject::tr("Orientation"),
                                         QObject::tr("Orientation of the guide"),
                                         std::bind(&GuideInspectable::GetDirection, this),
                                         std::bind(&GuideInspectable::SetDirection, this,
                                                   std::placeholders::_1),
                                         theCommiter, theCanceler),
                    theComboItems));

    m_Properties.push_back(
                std::make_shared<SFloatIntItem>(
                    SInspectableDataInfo(QStringLiteral("Width"), QObject::tr("Width"),
                                         QObject::tr("Width of the guide"),
                                         std::bind(&GuideInspectable::GetWidth, this),
                                         std::bind(&GuideInspectable::SetWidth, this,
                                                   std::placeholders::_1),
                                         theCommiter, theCanceler),
                    qt3dsdm::DataModelDataType::Long, 1.0f, 50.0f));

    CInspectorGroup *theNewGroup = new CInspectorGroup(QObject::tr("Basic"));
    return theNewGroup;
}

bool GuideInspectable::isValid() const
{
    return Reader().IsGuideValid(m_Guide);
}

bool GuideInspectable::isMaster() const
{
    return true;
}

qt3dsdm::Qt3DSDMInstanceHandle GuideInspectable::getInstance() const
{
    return 0; // guide has no instance
}

void GuideInspectable::SetDirection(const qt3dsdm::SValue &inValue)
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

qt3dsdm::SValue GuideInspectable::GetDirection()
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

void GuideInspectable::SetPosition(const qt3dsdm::SValue &inValue)
{
    float thePos = inValue.getData<float>();
    qt3dsdm::SGuideInfo theSetter(Reader().GetGuideInfo(m_Guide));
    theSetter.m_Position = thePos;
    Editor().UpdateGuide(m_Guide, theSetter);
    FireRefresh();
}

qt3dsdm::SValue GuideInspectable::GetPosition()
{
    qt3dsdm::SGuideInfo theSetter(Reader().GetGuideInfo(m_Guide));
    return theSetter.m_Position;
}

void GuideInspectable::SetWidth(const qt3dsdm::SValue &inValue)
{
    auto theData = inValue.getData<qt3ds::QT3DSI32>();

    qt3dsdm::SGuideInfo theSetter(Reader().GetGuideInfo(m_Guide));
    theSetter.m_Width = theData;
    Editor().UpdateGuide(m_Guide, theSetter);
    FireRefresh();

}

qt3dsdm::SValue GuideInspectable::GetWidth()
{
    qt3dsdm::SGuideInfo theSetter(Reader().GetGuideInfo(m_Guide));
    return theSetter.m_Width;
}

Q3DStudio::IDocumentEditor &GuideInspectable::Editor()
{
    return m_Editor.EnsureEditor(QObject::tr("Set Property"), __FILE__, __LINE__);
}

void GuideInspectable::Commit()
{
    m_Editor.CommitEditor();
}

void GuideInspectable::Rollback()
{
    m_Editor.RollbackEditor();
}

void GuideInspectable::FireRefresh()
{
    m_Editor.FireImmediateRefresh(qt3dsdm::Qt3DSDMInstanceHandle());
}

void GuideInspectable::Destroy()
{
    m_Editor.EnsureEditor(QObject::tr("Delete Guide"), __FILE__, __LINE__).DeleteGuide(m_Guide);
    m_Editor.CommitEditor();
}

bool GuideInspectable::isHorizontal() const
{
    return Reader().GetGuideInfo(m_Guide).m_Direction == qt3dsdm::GuideDirections::Horizontal;
}

const std::vector<std::shared_ptr<IInspectableAttributeItem>> &
GuideInspectable::properties() const
{
    return m_Properties;
}

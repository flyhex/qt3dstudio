/****************************************************************************
**
** Copyright (C) 2008 NVIDIA Corporation.
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
#include "UICDMMaterialInspectable.h"
#include "UICDMInspectorGroup.h"
#include "UICDMInspectorRow.h"
#include "Core.h"
#include "IDocumentEditor.h"
#include "GenericComboDropDown.h"
#include "UICDMHandles.h"
#include "Doc.h"
#include "GenericFunctor.h"
#include "StudioApp.h"
#include "UICDMStudioSystem.h"
#include "ClientDataModelBridge.h"
#include "IDocumentReader.h"
#include "Dispatch.h"
#include "IDirectoryWatchingSystem.h"
#include "UICDMSignals.h"
#include "UICString.h"

using namespace UICDM;

struct SMaterialTypeDropDown : public CGenericComboDropDown
{
    struct SMaterialEntry
    {
        Q3DStudio::CString m_Name;
        Q3DStudio::CString m_RelativePath;
        bool operator<(const SMaterialEntry &inOther) const { return m_Name < inOther.m_Name; }
    };
    CUICDMInstanceHandle m_Instance;
    Q3DStudio::CAutoMemPtr<CGenericEditCommitListener> m_CommitListener;
    vector<SMaterialEntry> m_Materials;
    CDoc &m_Doc;
    TSignalConnectionPtr m_FileListPtr;

    SMaterialTypeDropDown(CDoc &inDoc, CUICDMInstanceHandle inInstance)
        : m_Instance(inInstance)
        , m_Doc(inDoc)
    {
        using Q3DStudio::CString;
        using Q3DStudio::CFilePath;
        m_CommitListener =
            CREATE_LISTENER(CGenericEditCommitListener, SMaterialTypeDropDown, OnDataCommit);
        AddCommitListener(m_CommitListener);
        m_FileListPtr = m_Doc.GetDirectoryWatchingSystem()->AddDirectory(
            m_Doc.GetDocumentDirectory().toQString(),
            std::bind(&SMaterialTypeDropDown::OnFilesChanged, this, std::placeholders::_1));
    }

    vector<SMaterialEntry>::iterator GetMaterialEntry(const Q3DStudio::CString &inRelativePath)
    {
        for (size_t idx = 0, end = m_Materials.size(); idx < end; ++idx)
            if (m_Materials[idx].m_RelativePath == inRelativePath)
                return m_Materials.begin() + idx;
        return m_Materials.end();
    }

    vector<SMaterialEntry>::iterator
    GetOrCreateMaterialEntry(const Q3DStudio::CString &inRelativePath)
    {
        vector<SMaterialEntry>::iterator retval = GetMaterialEntry(inRelativePath);
        if (retval == m_Materials.end()) {
            m_Materials.push_back(SMaterialEntry());
            m_Materials.back().m_RelativePath = inRelativePath;
            return m_Materials.begin() + m_Materials.size() - 1;
        } else
            return retval;
    }

    void OnFilesChanged(const Q3DStudio::TFileModificationList &inFileModificationList)
    {
        for (size_t idx = 0, end = inFileModificationList.size(); idx < end; ++idx) {
            const Q3DStudio::SFileModificationRecord &theRecord = inFileModificationList[idx];
            Q3DStudio::CFilePath relativePath(Q3DStudio::CFilePath::GetRelativePathFromBase(
                m_Doc.GetDocumentDirectory(), theRecord.m_File));
            Q3DStudio::CString extension = relativePath.GetExtension();
            if (extension.CompareNoCase("material")) {
                switch (theRecord.m_ModificationType) {
                case Q3DStudio::FileModificationType::Created:
                case Q3DStudio::FileModificationType::Modified: {
                    SMaterialEntry &theEntry =
                        *GetOrCreateMaterialEntry(Q3DStudio::CString(relativePath));
                    theEntry.m_Name =
                        m_Doc.GetDocumentReader().GetCustomMaterialName(theRecord.m_File);
                } break;
                case Q3DStudio::FileModificationType::Destroyed: {
                    vector<SMaterialEntry>::iterator theEntry = GetMaterialEntry(relativePath);
                    if (theEntry != m_Materials.end())
                        m_Materials.erase(theEntry);
                } break;

                default: // don't care.
                    break;
                }
            }
        }

        std::sort(m_Materials.begin(), m_Materials.end());

        RefreshAllItems();
    }

    void RefreshAllItems()
    {
        RemoveAllItems();
        AddItem("Standard Material");
        AddItem("Referenced Material");
        CClientDataModelBridge *theBridge = m_Doc.GetStudioSystem()->GetClientDataModelBridge();
        long selectIdx = 0;
        EStudioObjectType theType = theBridge->GetObjectType(m_Instance);

        if (theType == OBJTYPE_REFERENCEDMATERIAL)
            selectIdx = 1;

        Q3DStudio::CString sourcePath = theBridge->GetSourcePath(m_Instance);

        for (size_t matIdx = 0, end = m_Materials.size(); matIdx < end; ++matIdx) {
            AddItem(m_Materials[matIdx].m_Name);
            if (m_Materials[matIdx].m_RelativePath.Compare(sourcePath))
                selectIdx = (long)matIdx + 2;
        }

        SelectItem(selectIdx, false);
    }

    // Note that the this object is probably deleted when this happens or will be during its
    // execution.
    static void DoChangeObjectType(CDoc *inDoc, const Q3DStudio::CString &inNewType,
                                   CUICDMInstanceHandle instance)
    {
        using namespace Q3DStudio;
        SCOPED_DOCUMENT_EDITOR(*inDoc, QObject::tr("Set Property"))->SetMaterialType(instance, inNewType);
    }

    void OnDataCommit()
    {
        using Q3DStudio::CString;
        size_t item = this->GetSelectedItem();
        if (item >= 0) {
            CString selectedType = this->GetItemText(this->GetSelectedItem());
            if (item > 1) {
                size_t matIdx = item - 2;
                if (matIdx < m_Materials.size())
                    selectedType = m_Materials[matIdx].m_RelativePath;
            }
            // Fire a command to do this later because we will get screwed if we don't as we will be
            // deleted
            // during this process.
            g_StudioApp.GetCore()->GetDispatch()->FireOnAsynchronousCommand(
                std::bind(&DoChangeObjectType, &m_Doc, selectedType, m_Instance));
        }
    }
};

UICDMMaterialInspectorGroup::UICDMMaterialInspectorGroup(
        CStudioApp &inApp,
        const Q3DStudio::CString &inName,
        CUICDMInspectable &inInspectable,
        long inIndex)
    : CUICDMInspectorGroup(inApp, inName.toQString(), inInspectable, inIndex)
{
}

struct SUICDMMaterialInspectorGroup : public UICDMMaterialInspectorGroup
{
    SUICDMMaterialInspectorGroup(CStudioApp &inApp, const Q3DStudio::CString &inName,
                                 CUICDMInspectable &inInspectable, long inIndex)
        : UICDMMaterialInspectorGroup(inApp, inName, inInspectable, inIndex)
    {
        Q3DStudio::CString theMaterialGroupName = L"Material";
        m_isMaterialGroup = (inName == theMaterialGroupName);
    }

    bool isMaterialGroup() const override
    {
        return m_isMaterialGroup;
    }

private:
    bool m_isMaterialGroup;
};

CInspectorGroup *CUICDMMaterialInspectable::GetGroup(long inIndex)
{
    Q3DStudio::CString theGroupName = GetGroupName(inIndex);
    Q3DStudio::CString theMaterialGroupName = L"Material";

    CUICDMInspectorGroup *theGroup =
        new SUICDMMaterialInspectorGroup(m_App, theGroupName, *this, inIndex);

    TMetaDataPropertyHandleList theProperties = GetGroupProperties(inIndex);
    size_t thePropertyCount = theProperties.size();

    for (size_t thePropertyIndex = 0; thePropertyIndex < thePropertyCount; ++thePropertyIndex)
        theGroup->CreateRow(m_Core->GetDoc(), theProperties[thePropertyIndex]);

    return theGroup;
}

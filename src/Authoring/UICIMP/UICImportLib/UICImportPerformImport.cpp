/****************************************************************************
**
** Copyright (C) 1993-2009 NVIDIA Corporation.
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
#include "UICImportLibPrecompile.h"
#include "UICImport.h"
#include "UICImportPerformImport.h"
#include "UICImportImpl.h"
#include "UICFileTools.h"

using namespace UICIMP;
using namespace Q3DStudio;

namespace {
template <typename TDataType>
struct ScopedReleaser
{
    TDataType *dtype;
    ScopedReleaser(TDataType &dt)
        : dtype(&dt)
    {
    }
    ~ScopedReleaser()
    {
        if (dtype)
            dtype->Release();
    }
};

void DoUpdateInstances(Import &import, IComposerEditor &composer,
                       NVConstDataRef<InstanceDesc> instances,
                       ImportArray<PropertyValue> &properties)
{
    QT3DSIMP_FOREACH(idx, instances.size())
    {
        // We have to re-lookup instances here because the instance data may have changed since it
        // was put
        // into the import report.  For instance, you get an import report, then you add instances.
        // This adds
        // user ids.  But the import report is already created so the instance descriptions in the
        // report's
        // add list won't reflect the new reality.
        const InstanceDesc &desc(instances[idx]);
        QT3DSU32 numProps = import.GetNumProperties(desc.m_Handle);
        properties.resize(numProps);
        import.GetProperties(desc.m_Handle, properties);
        composer.UpdateInstanceProperties(desc.m_Id, properties.data(), (QT3DSU32)properties.size());
    }
}
}

SImportResult::SImportResult(const Q3DStudio::CFilePath &inFilePath, QT3DSU32 inFileVersion)
    : m_Error(ImportErrorCodes::NoError)
    , m_FilePath(inFilePath)
{
    m_FilePath.SetIdentifier(inFileVersion);
}

/**
 *	This function has a simple concept but really complex implications.
 *	We are taking a document with perhaps multiple user-id, import-id pairs
 *	and applying it to another document where any one of those pairs may
 *	or may not apply.
 *
 *	Just for import, you can consider three different cases:
 *	1.  Document imported into project once.
 *	2.  Document *that has been imported* imported into project again.
 *	3.  Document *that has been imported* imported into new, different project.
 *
 *	And then consider what correct refresh behavior should be in each one of those
 *	cases.
 */
void CPerformImport::DoImportToComposer(Import &import, IComposerEditor &composer)
{
    ImportReport report(import.CompileReport());
    composer.BeginImport(import);
    // Remove links.
    QT3DSIMP_FOREACH(idx, report.m_Links.m_Removed.size())
    {
        const ParentChildLink link(report.m_Links.m_Removed[idx]);
        composer.RemoveChild(link.m_Parent, link.m_Child);
    }

    // Remove instances
    QT3DSIMP_FOREACH(idx, report.m_Instances.m_Removed.size())
    {
        composer.RemoveInstance(report.m_Instances.m_Removed[idx].m_Id);
    }

    // Create the new instances.
    QT3DSIMP_FOREACH(idx, report.m_Instances.m_Added.size())
    {
        const InstanceDesc &desc(report.m_Instances.m_Added[idx]);
        if (AreEqual(desc.m_Id, L"__import__root__"))
            composer.CreateRootInstance(desc.m_Id, desc.m_Type);
        else {
            Option<InstanceDesc> parent(import.GetInstanceByHandle(desc.m_Parent));
            TImportId parentId = L"";
            if (parent.hasValue())
                parentId = parent->m_Id;
            composer.CreateInstance(desc.m_Id, desc.m_Type, parentId);
        }
    }

    ImportArray<PropertyValue> properties;
    DoUpdateInstances(import, composer, report.m_Instances.m_Added, properties);
    DoUpdateInstances(import, composer, report.m_Instances.m_Existing, properties);

    // Add links
    QT3DSIMP_FOREACH(idx, report.m_Links.m_Added.size())
    {
        const ParentChildLink link(report.m_Links.m_Added[idx]);
        composer.AddChild(link.m_Parent, link.m_Child, link.m_NextSibling);
    }

    // Remove animations (some of these may be to removed objects).
    QT3DSIMP_FOREACH(idx, report.m_Animations.m_Removed.size())
    {
        const Animation &anim(report.m_Animations.m_Removed[idx]);
        composer.RemoveAnimation(anim.m_InstanceId, anim.m_PropertyName, anim.m_SubPropertyIndex);
    }

    QT3DSIMP_FOREACH(idx, report.m_Animations.m_Added.size())
    {
        const Animation &anim(report.m_Animations.m_Added[idx]);
        composer.AddAnimation(anim.m_InstanceId, anim.m_PropertyName, anim.m_SubPropertyIndex,
                              anim.m_Type, anim.m_Keyframes.begin(), anim.m_Keyframes.size());
    }

    QT3DSIMP_FOREACH(idx, report.m_Animations.m_Existing.size())
    {
        const Animation &anim(report.m_Animations.m_Existing[idx]);
        composer.UpdateAnimation(anim.m_InstanceId, anim.m_PropertyName, anim.m_SubPropertyIndex,
                                 anim.m_Type, anim.m_Keyframes.begin(), anim.m_Keyframes.size());
    }

    composer.EndImport();
}

SImportResult CPerformImport::RefreshToComposer(ITranslator &translator,
                                                IComposerEditor &inComposer,
                                                Import &inOriginalImport,
                                                const Q3DStudio::CFilePath &ioImportFile)
{
    ScopedReleaser<Import> __originalScope(inOriginalImport);

    ScopedReleaser<ITranslator> __translatorScope(translator);
    const CFilePath &importDestFile(ioImportFile);

    if (importDestFile.IsFile() && importDestFile.GetFileFlags().CanWrite() == false)
        return ImportErrorCodes::ResourceNotWriteable;

    __originalScope.dtype = NULL;

    ImportPtrOrError importPtr =
        Import::CreateForRefresh(inOriginalImport, translator.GetSourceFile().toStdWString().c_str());
    if (importPtr.m_Value == NULL)
        return importPtr.m_ErrorData.m_Error;

    ScopedReleaser<Import> __importScope(*importPtr.m_Value);
    if (translator.PerformTranslation(*importPtr.m_Value)) {
        DoImportToComposer(*importPtr.m_Value, inComposer);
        Q3DStudio::CString fname = Q3DStudio::CFilePath(importDestFile).GetFileName();
        QT3DSU32 importId = importPtr.m_Value->Save(fname.c_str());
        return SImportResult(importDestFile, importId);
    }
    return ImportErrorCodes::TranslationToImportFailed;
}
SImportResult CPerformImport::RefreshToComposer(ITranslator &translator, IComposerEditor &composer,
                                                const CFilePath &ioImportFile)
{
    ImportPtrOrError originalPtr = Import::Load(ioImportFile.c_str());
    if (originalPtr.m_Value == NULL)
        return originalPtr.m_ErrorData.m_Error;

    return RefreshToComposer(translator, composer, *originalPtr.m_Value, ioImportFile);
}

SImportResult CPerformImport::ImportToComposer(ITranslator &translator, IComposerEditor &composer,
                                               const Q3DStudio::CFilePath &inImportFile)
{
    ScopedReleaser<ITranslator> __translatorScope(translator);
    const CFilePath &importDestFile(inImportFile);
    if (importDestFile.IsFile() && importDestFile.GetFileFlags().CanWrite() == false)
        return ImportErrorCodes::ResourceNotWriteable;
    ImportPtrOrError importPtr =
        Import::Create(translator.GetSourceFile().toStdWString().c_str(), importDestFile.GetDirectory());
    if (importPtr.m_Value == NULL)
        return importPtr.m_ErrorData.m_Error;
    ScopedReleaser<Import> __importScope(*importPtr.m_Value);
    if (translator.PerformTranslation(*importPtr.m_Value)) {
        DoImportToComposer(*importPtr.m_Value, composer);
        Q3DStudio::CString fname = importDestFile.GetFileName();
        QT3DSU32 importId = importPtr.m_Value->Save(fname.c_str());
        return SImportResult(importDestFile, importId);
    }
    return ImportErrorCodes::TranslationToImportFailed;
}

SImportResult
CPerformImport::ImportToComposerFromImportFile(IComposerEditor &composer,
                                               const Q3DStudio::CFilePath &inImportFile)
{
    const CFilePath &importDestFile(inImportFile);
    if (importDestFile.IsFile() == false)
        return ImportErrorCodes::SourceFileDoesNotExist;

    QT3DSU32 fileId = Import::GetHighestImportRevision(inImportFile.c_str());
    if (fileId == 0)
        return ImportErrorCodes::SourceFileNotReadable;

    ImportPtrOrError importPtr = Import::Load(importDestFile.c_str(), fileId);
    if (importPtr.m_Value == NULL)
        return importPtr.m_ErrorData.m_Error;
    ScopedReleaser<Import> __importScope(*importPtr.m_Value);
    DoImportToComposer(*importPtr.m_Value, composer);
    return SImportResult(inImportFile, fileId);
}

SImportResult CPerformImport::TranslateToImportFile(ITranslator &translator,
                                                    const Q3DStudio::CFilePath &inImportFile)
{
    ScopedReleaser<ITranslator> __translatorScope(translator);
    const CFilePath &importDestFile(inImportFile);
    if (importDestFile.IsFile() && importDestFile.GetFileFlags().CanWrite() == false)
        return ImportErrorCodes::ResourceNotWriteable;

    ImportPtrOrError importPtr =
        Import::Create(translator.GetSourceFile().toStdWString().c_str(), importDestFile.GetDirectory());
    if (importPtr.m_Value == NULL)
        return importPtr.m_ErrorData.m_Error;
    ScopedReleaser<Import> __importScope(*importPtr.m_Value);
    if (translator.PerformTranslation(*importPtr.m_Value)) {
        Q3DStudio::CString fname = importDestFile.GetFileName();
        QT3DSU32 importId = importPtr.m_Value->Save(fname.c_str());
        return SImportResult(inImportFile, importId);
    }
    return ImportErrorCodes::TranslationToImportFailed;
}

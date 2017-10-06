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
#ifndef PATH_IMPORT_TRANSLATOR_H
#define PATH_IMPORT_TRANSLATOR_H
#include "UICImportTranslation.h"
#include "UICImportComposerTypes.h"
#include "UICImport.h"
#include "foundation/Qt3DSRefCounted.h"
namespace UICIMP {
class IPathBufferBuilder;
}

namespace qt3ds {
class NVFoundationBase;
}

namespace Q3DStudio {
class IDynamicLua;
struct SPathImportTranslator : public UICIMP::ITranslator
{
    typedef eastl::vector<Q3DStudio::CString> TNameList;
    typedef QT3DSU64 TInstanceHandle;

    qt3ds::NVFoundationBase &m_Foundation;
    QString m_SourceFile;
    UICIMP::STranslationLog m_TranslationLog;
    IDynamicLua &m_LuaState;
    UICIMP::Import *m_Import;
    UICIMP::SImportComposerTypes m_ObjectTypes;
    eastl::vector<UICIMP::InstanceDesc> m_ChildVector;
    qt3ds::foundation::NVScopedRefCounted<UICIMP::IPathBufferBuilder> m_Builder;

    SPathImportTranslator(const QString &srcFile, IDynamicLua &inLuaState,
                          qt3ds::NVFoundationBase &inFoundation);
    ~SPathImportTranslator();

    // Object is created on the stack, no reason to release.
    void Release() override {}

    const QString &GetSourceFile() override { return m_SourceFile; }
    // Returning false causes the rest of the import or refresh process
    // to fail.
    bool PerformTranslation(UICIMP::Import &import) override;

protected:
    void SetName(TInstanceHandle inItem, const wchar_t *inNameBase);
    void SetUniqueName(TInstanceHandle inItem, const char8_t *inNameBase,
                       eastl::vector<Q3DStudio::CString> &inExistingNames);
    TInstanceHandle GetChild(TInstanceHandle inItem, qt3ds::QT3DSI32 inIndex);
    TInstanceHandle CreateSceneGraphInstance(UICDM::ComposerObjectTypes::Enum inObjectTypes,
                                             TInstanceHandle inParent,
                                             const Q3DStudio::CString &inId);
    TInstanceHandle ParseSVGItem(TInstanceHandle parentHandle, TNameList &existingChildNames,
                                 bool inRoot = false);
    void ParseSVGGroupChildren(TInstanceHandle inNewItem);
    TInstanceHandle ParseSVGGroup(TInstanceHandle inParent,
                                  eastl::vector<Q3DStudio::CString> &inExistingNames, bool inRoot);
    TInstanceHandle ParseSVGPath(TInstanceHandle inParent,
                                 eastl::vector<Q3DStudio::CString> &inExistingNames);
};
}

#endif

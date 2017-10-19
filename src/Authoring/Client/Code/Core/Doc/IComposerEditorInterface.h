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
#pragma once
#ifndef ICOMPOSERIMPORTINTERFACEH
#define ICOMPOSERIMPORTINTERFACEH
#include "IDocumentEditor.h"
#include "Qt3DSImportPerformImport.h"

namespace qt3dsdm {
class IStringTable;
};

namespace Q3DStudio {
class CGraph;

namespace ComposerImport {

    using namespace Q3DStudio;
    using namespace qt3dsdm;
    using namespace qt3dsimp;
    using std::unordered_map;
    using std::vector;
    using std::pair;

    // For the children of this instance that are associated with this slide,
    // update their information.
    typedef unordered_map<const wchar_t *, vector<pair<CUICDMSlideHandle, Qt3DSDMInstanceHandle>>>
        TIdMultiMap;

    // Interface between the import library (which defines IComposerEditor)
    // and the document (which provides IDocumentEditor)
    class IComposerEditorInterface : public IComposerEditor
    {
    protected:
        ~IComposerEditorInterface(){}

    public:
        friend class std::shared_ptr<IComposerEditorInterface>;

        virtual bool HasError() = 0;
        // This file path contains the import document id.
        virtual void Finalize(const Q3DStudio::CFilePath &inDestFilePath) = 0;
        virtual Qt3DSDMInstanceHandle FindInstance(TImportId inImportHdl) = 0;
        virtual Qt3DSDMInstanceHandle GetRoot() = 0;
        virtual const Q3DStudio::CFilePath &GetDestImportFile() = 0;
        virtual void AddInstanceMap(Qt3DSDMInstanceHandle instanceHandle, TImportId inImportId) = 0;

        static std::shared_ptr<IComposerEditorInterface> CreateEditorInterface(
            Q3DStudio::IDocumentEditor &editor, qt3dsdm::CDataModelHandle parent // Parent object
            ,
            qt3dsdm::CDataModelHandle root, qt3dsdm::CUICDMSlideHandle slide,
            const Q3DStudio::CFilePath &docPath, const Q3DStudio::CFilePath &fullPathToImportFile,
            long inStartTime, qt3dsdm::IStringTable &inStringTable);

        // The refresh interface is setup to refresh multiple trees automatically
        static std::shared_ptr<IComposerEditor>
        CreateEditorInterface(Q3DStudio::IDocumentEditor &editor, TIdMultiMap &inRoots,
                              const Q3DStudio::CFilePath &docPath,
                              const Q3DStudio::CFilePath &fullPathToImportFile, long inStartTime,
                              qt3dsdm::IStringTable &inStringTable, CGraph &inAssetGraph);
    };
}
}
#endif

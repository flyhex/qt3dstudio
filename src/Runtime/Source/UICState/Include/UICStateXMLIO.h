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
#ifndef UIC_STATE_XML_IO_H
#define UIC_STATE_XML_IO_H
#pragma once
#include "UICState.h"
#include "foundation/Qt3DSDataRef.h"
#include "foundation/Qt3DSContainers.h"
#include "foundation/StringTable.h"
#include "foundation/XML.h"
#include "EASTL/map.h"
#include "EASTL/string.h"

namespace qt3ds {
namespace foundation {
    class IDOMReader;
    class IDOMWriter;
}
}

namespace uic {
namespace state {

    struct SExecutableContent;
    class CXMLIO
    {
    public:
        typedef eastl::map<eastl::string, eastl::string> TIdRemapMap;

        static void GenerateUniqueId(SStateNode &inNode, const char8_t *inStem,
                                     IStateContext &ioContext, IStringTable &ioStringTable);
        static void GenerateUniqueId(SSend &inNode, const char8_t *inStem, IStateContext &ioContext,
                                     IStringTable &ioStringTable);
        // Load an SCXML file and return the root states
        // All states, transitions, and memory used in the graph is allocated using the graph
        // allocator.
        // This makes freeing the memory much easier because you can just free it by releasing the
        // graph
        // allocator and you don't have to go object by object.
        // Filename is just used as another tag or name on the scxml object.
        static bool LoadSCXMLFile(NVAllocatorCallback &inGraphAllocator, NVFoundationBase &inFnd,
                                  IDOMReader &inReader, IStringTable &inStringTable,
                                  const char8_t *inFilename, IStateContext &outContext,
                                  editor::IEditor *inEditor = NULL);

        // Loading fragments remaps their ids to avoid conflics.  Returns the top nodes from the
        // scxml graph.
        static eastl::pair<eastl::vector<SStateNode *>, TIdRemapMap>
        LoadSCXMLFragment(NVAllocatorCallback &inGraphAllocator, NVFoundationBase &inFnd,
                          IDOMReader &inReader, IStringTable &inStringTable,
                          IStateContext &ioContext, editor::IEditor &inEditor);

        // We write all the way to file instead of a DOM writer because we have to add xml
        // namespaces and those can only
        // be added during the actual serialization process.
        static void SaveSCXMLFile(IStateContext &inContext, NVFoundationBase &inFnd,
                                  IStringTable &inStringTable, IOutStream &outStream,
                                  editor::IEditor *inEditor = NULL);

        static void FindRoots(NVConstDataRef<SStateNode *> inObjects,
                              eastl::vector<SStateNode *> &outRoots);

        // Returns the roots of the copied list
        static eastl::vector<SStateNode *>
        SaveSCXMLFragment(IStateContext &inContext, NVFoundationBase &inFnd,
                          IStringTable &inStringTable, IDOMWriter &ioWriter,
                          NVDataRef<SStateNode *> inObjects, editor::IEditor &inEditor,
                          const QT3DSVec2 &inMousePos, eastl::vector<SNamespacePair> &outNamespaces);

        static void ToEditableXml(IStateContext &inContext, NVFoundationBase &inFnd,
                                  IStringTable &inStringTable, IDOMWriter &ioWriter,
                                  SExecutableContent &inContent, editor::IEditor &inEditor);

        static SExecutableContent *
        FromEditableXML(IDOMReader &inReader, NVFoundationBase &inFnd, IStateContext &inContext,
                        IStringTable &inStringTable, NVAllocatorCallback &inGraphAllocator,
                        editor::IEditor &inEditor, SStateNode *inStateNodeParent,
                        SExecutableContent *inExecContentParent);

        static eastl::vector<eastl::string> GetSupportedExecutableContentNames();
    };
}
}

#endif
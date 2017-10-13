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
#ifndef UIC_SCENE_GRAPH_DEBUGGER_H
#define UIC_SCENE_GRAPH_DEBUGGER_H
#pragma once
#include "UICStateDebugger.h"
#include "UICUIADatamodel.h"

namespace qt3ds {
namespace state {
    namespace debugger {

        struct SSGValue;

        struct SSGPropertyChange;

        struct SGElemIdMap
        {
            void *m_Elem;
            const char *m_Id;
        };

        // Persistant item that sticks around and appends some information to the binary file.
        // The runtime debugger must exist for the entire time the runtime is running, not just
        // during connection because the mapping from element->id only exists at file loading time.
        class ISceneGraphRuntimeDebugger : public NVRefCounted, public IDebugStreamListener
        {
        public:
            static const char *GetProtocolName() { return "Scene Graph Debugger"; }
            // Nothing is returned if the object isn't connected.  The returned value may not be
            // valid
            // after next GetOrCreateCall, so don't hold on to it.
            virtual void MapPresentationId(void *presentation, const char *id) = 0;
            virtual void MapElementIds(void *presentation, NVConstDataRef<SGElemIdMap> inIds) = 0;
            virtual void OnPropertyChanged(void *elem,
                                           NVConstDataRef<SSGPropertyChange> changes) = 0;
            virtual void OnConnection(IDebugOutStream &outStream) = 0;
            virtual void BinarySave(IOutStream &stream) = 0;
            // Load the main datastructures, although we know the ids are wrong
            virtual void BinaryLoad(IInStream &stream, NVDataRef<QT3DSU8> inStringTableData) = 0;
            // Remap the presentation element points using id to map old presentation ptr to new
            // presentation ptr.
            virtual void BinaryLoadPresentation(void *presentation, const char *id,
                                                size_t inElemOffset) = 0;
            virtual void EndFrame() = 0;
            virtual bool IsConnected() = 0;

            static ISceneGraphRuntimeDebugger &Create(NVFoundationBase &fnd,
                                                      IStringTable &strTable);
        };

        class ISceneGraphArchitectDebuggerListener
        {
        public:
            virtual void OnItemsDirty(NVConstDataRef<app::SAppElement *> inDirtySet) = 0;
        };

        // The architect debugger only exists when debugging.
        class ISceneGraphArchitectDebugger : public NVRefCounted, public IDebugStreamListener
        {
        public:
            virtual void SetListener(ISceneGraphArchitectDebuggerListener *listener) = 0;
            // Note that we wrap the att or arg list and the initial values to provide extra
            // information.
            virtual Q3DStudio::TAttOrArgList GetElementAttributes(app::SAppElement &elem) = 0;

            // These may be empty, so don't expect them.  Also they are all string, no registered
            // strings
            // regardless of the type.
            virtual eastl::vector<app::SDatamodelValue>
            GetElementAttributeValues(app::SAppElement &elem) = 0;
            virtual app::IDatamodel &GetDatamodel() = 0;

            virtual void AttachToStream(IDebugOutStream &inStream) = 0;

            virtual void RefreshData(bool inNeedReloadData) = 0;

            static ISceneGraphArchitectDebugger &Create(qt3ds::app::IDatamodel &inDatamodel);
        };
    }
}
}

#endif
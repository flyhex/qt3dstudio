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
#ifndef UIC_STATE_CONTEXT_H
#define UIC_STATE_CONTEXT_H
#include "Qt3DSState.h"
#include "Qt3DSStateTypes.h"

namespace qt3ds {
namespace state {

    // Thanks to the crazy world we live in, we have to store xml extension information
    // on the state context.
    struct SItemExtensionInfo
    {
        void *m_ItemPtr;
        TDOMElementNodeList m_ExtensionNodes;
        TDOMAttributeNodeList m_ExtensionAttributes;
        SItemExtensionInfo(void *inItemPtr)
            : m_ItemPtr(inItemPtr)
        {
        }
    };
    typedef nvhash_map<void *, SItemExtensionInfo> TPtrExtensionMap;
    // Parsing produces a list of send objects that need to be added into the script
    // context if they have their idlocation attribute set.
    typedef nvvector<SSend *> TSendList;

    class IStateContext : public NVRefCounted
    {
    public:
        // Can only be done once.
        virtual void SetRoot(SSCXML &inRoot) = 0;
        virtual SSCXML *GetRoot() = 0;
        virtual void AddSendToList(SSend &inSend) = 0;
        virtual NVConstDataRef<SSend *> GetSendList() = 0;
        virtual void SetDOMFactory(IDOMFactory *inFactory) = 0;
        virtual IDOMFactory *GetDOMFactory() = 0;
        virtual NVConstDataRef<NVScopedRefCounted<IStateContext>> GetSubContexts() = 0;
        virtual bool ContainsId(const CRegisteredString &inId) = 0;
        virtual bool InsertId(const CRegisteredString &inId, const SIdValue &inValue) = 0;
        virtual void EraseId(const CRegisteredString &inId) = 0;
        virtual SStateNode *FindStateNode(const CRegisteredString &inStr) = 0;
        virtual SSend *FindSend(const CRegisteredString &inStr) = 0;

        virtual SItemExtensionInfo *GetExtensionInfo(void *inItem) = 0;
        virtual SItemExtensionInfo &GetOrCreateExtensionInfo(void *inItem) = 0;

        virtual void SetFirstNSNode(SNamespacePairNode &inNode) = 0;
        virtual SNamespacePairNode *GetFirstNSNode() = 0;

        virtual void Save(IOutStream &inOutStream, editor::IEditor *inEditor = NULL) = 0;

        // The graph allocator is expected to release anything allocated via it; the general
        // allocator doesn't need to do this.
        // Filename is stored on the context for debugging purposes, editor is used if during
        // loading so we can load extra
        // information that is associated with but not stored on the state graph.
        // String table will be created if not passed in, editor is optional.
        static IStateContext *Load(NVAllocatorCallback &inGraphAllocator,
                                   NVFoundationBase &inFoundation, IInStream &inStream,
                                   const char8_t *inFilename, IStringTable *inStrTable = NULL,
                                   editor::IEditor *inEditor = NULL);

        static IStateContext *Create(NVFoundationBase &inGeneralAlloc);
    };
}
}
#endif

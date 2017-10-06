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
#pragma once
#ifndef UICFILETOOLSSEEKABLEMESHBUFIOSTREAM
#define UICFILETOOLSSEEKABLEMESHBUFIOSTREAM
#include "foundation/IOStreams.h"

namespace Q3DStudio {
struct SFile;
}

namespace UICIMP {
class CUICFileToolsSeekableMeshBufIOStream : public qt3ds::foundation::ISeekableIOStream
{
protected:
    std::shared_ptr<Q3DStudio::SFile> m_File;

public:
    CUICFileToolsSeekableMeshBufIOStream(std::shared_ptr<Q3DStudio::SFile> inFile)
        : m_File(inFile)
    {
    }
    virtual bool IsOpen() { return m_File != NULL; }

    void SetPosition(qt3ds::QT3DSI64 inOffset, qt3ds::foundation::SeekPosition::Enum inEnum) override;
    qt3ds::QT3DSI64 GetPosition() const override;
    qt3ds::QT3DSU32 Read(qt3ds::foundation::NVDataRef<QT3DSU8> data) override;
    bool Write(qt3ds::foundation::NVConstDataRef<QT3DSU8> data) override;
};
}

#endif

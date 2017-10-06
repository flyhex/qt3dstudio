/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
#pragma once
#ifndef DOCUMENTRESOURCEMANAGERRENDERPLUGINPARSERH
#define DOCUMENTRESOURCEMANAGERRENDERPLUGINPARSERH

#include "foundation/Qt3DS.h"
#include "foundation/Qt3DSAssert.h"
#include "UICDMXML.h"
#include "UICDMDataTypes.h"
#include "UICDMMetaData.h"
#include "UICDMWStrOpsImpl.h"
#include "foundation/StrConvertUTF.h"
#include <EASTL/string.h>
#include "UICRenderInputStreamFactory.h"

namespace Q3DStudio {
struct CRenderPluginParser
{

    static bool NavigateToMetadata(std::shared_ptr<UICDM::IDOMReader> inReader)
    {
        return inReader->MoveToFirstChild("metadata");
    }

    static std::shared_ptr<UICDM::IDOMReader>
    ParseFile(std::shared_ptr<UICDM::IDOMFactory> inFactory,
              std::shared_ptr<UICDM::IStringTable> inStringTable, const char8_t *inFileData,
              UICDM::CXmlErrorHandler &inErrorHandler,
              uic::render::IInputStreamFactory &inStreamFactory)
    {
        using namespace qt3ds;
        using namespace qt3ds::foundation;

        NVScopedRefCounted<uic::render::IRefCountedInputStream> theStream(
            inStreamFactory.GetStreamForFile(inFileData));
        if (!theStream) {
            QT3DS_ASSERT(0);
            return std::shared_ptr<UICDM::IDOMReader>();
        }

        UICDM::SDOMElement *theElem(
            UICDM::CDOMSerializer::Read(*inFactory, *theStream, &inErrorHandler));

        if (theElem == NULL) {
            return std::shared_ptr<UICDM::IDOMReader>();
        } else
            return UICDM::IDOMReader::CreateDOMReader(*theElem, inStringTable, inFactory);
    }
};
}

#endif

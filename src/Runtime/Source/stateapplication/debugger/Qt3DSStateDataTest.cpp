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
#include "Qt3DSStateTest.h"
#include "foundation/IOStreams.h"
#include "EASTL/string.h"
#include "foundation/Utils.h"
#include "foundation/FileTools.h"
#include "foundation/XML.h"
#include "foundation/Qt3DSAllocator.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/TrackingAllocator.h"
#include "foundation/StringTable.h"
#include "Qt3DSStateContext.h"
#include "foundation/AutoDeallocatorAllocator.h"
#include "Qt3DSStateExecutionContext.h"
#include "Qt3DSStateInterpreter.h"

using namespace qt3ds::state::test;
using namespace qt3ds::state;

namespace {

struct XMLHandler : public qt3ds::foundation::CXmlErrorHandler
{
    IDataLogger &m_Logger;
    const char8_t *m_File;
    eastl::string m_ErrorString;
    XMLHandler(IDataLogger &logger, const char8_t *fname)
        : m_Logger(logger)
        , m_File(fname)
    {
    }

    void OnXmlError(qt3ds::foundation::TXMLCharPtr errorName, int line, int /*column*/) override
    {
        m_ErrorString.assign("Failed to parse test file: ");
        m_ErrorString.append(m_File);
        m_Logger.Log(LogType::Error, m_File, line, errorName);
    }
};

Option<STestResults> RunTest(const char8_t *inFullPath, const char8_t *inRoot,
                             IDataLogger &inLogger)
{
    return STestResults(1, 1);
}
}

Option<STestResults> IDataTest::RunFile(const char8_t *fname, const char8_t *inRootDir,
                                        IDataLogger &inLogger)
{
    return RunTest(fname, inRootDir, inLogger);
}

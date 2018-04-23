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
#ifndef QT3DS_STATE_TEST_H
#define QT3DS_STATE_TEST_H
#include "Qt3DSState.h"
#include "foundation/Qt3DSRefCounted.h"
#include "foundation/Qt3DSOption.h"

namespace qt3ds {
namespace state {
    namespace test {

        struct LogType
        {
            enum Enum {
                Error = 0,
                Info = 1,
            };
        };

        struct STestResults
        {
            QT3DSU32 m_TotalTests;
            QT3DSU32 m_PassingTests;
            STestResults(QT3DSU32 totalTests = 0, QT3DSU32 testPassed = 0)
                : m_TotalTests(totalTests)
                , m_PassingTests(testPassed)
            {
            }
            STestResults &operator+=(const STestResults &inOther)
            {
                m_TotalTests += inOther.m_TotalTests;
                m_PassingTests += inOther.m_PassingTests;
                return *this;
            }

            bool Failed() const { return m_TotalTests != m_PassingTests; }
            bool Passed() const { return !Failed(); }
        };

        class IDataLogger
        {
        protected:
            virtual ~IDataLogger() {}
        public:
            virtual void Log(LogType::Enum inLogType, const char8_t *file, int line,
                             const char8_t *message) = 0;
        };

        // Run a data test returning a true/false result for pass/fail.
        class IDataTest
        {
        public:
            static Option<STestResults> RunFile(const char8_t *fname, const char8_t *inRootDir,
                                                IDataLogger &inLogger);
        };
    }
}
}

#endif

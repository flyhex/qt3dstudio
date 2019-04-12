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
#ifndef QT3DS_STATE_EXECUTION_CONTEXT_H
#define QT3DS_STATE_EXECUTION_CONTEXT_H
#pragma once
#include "Qt3DSState.h"
#include "Qt3DSStateTypes.h"
#include "foundation/Qt3DSRefCounted.h"

namespace qt3ds {
namespace state {

    class IStateLogger : public NVRefCounted
    {
    protected:
        virtual ~IStateLogger() {}
    public:
        virtual void Log(const char8_t *inLabel, const char8_t *inExpression) = 0;
    };
    // Implementation of the execution context of the scxml state specification.
    // Implementations if,send,raise,foreach,etc working closely with the scripting
    // system.
    class IExecutionContext : public NVRefCounted
    {
    protected:
        virtual ~IExecutionContext() {}
    public:
        virtual void SetInterpreter(IStateInterpreter &inInterpreter) = 0;
        virtual void SetMachineDebugLogger(IStateLogger &inDebugLogger) = 0;
        virtual void Execute(STransition &inTransaction) = 0;
        // These functions take the node as well as the list so a context can cache a fast
        // execution path if necessary.
        virtual void Execute(SStateNode &inNode, TOnEntryList &inList) = 0;
        virtual void Execute(SStateNode &inNode, TOnExitList &inList) = 0;

        // Returns the time string in milliseconds.
        static QT3DSU64 ParseTimeStrToMilliseconds(const char8_t *timeStr);

        static IExecutionContext &Create(NVFoundationBase &inFoundation,
                                         IStringTable &inStringTable, IStateLogger &inLogger,
                                         IScriptContext &inScriptContext);
    };
}
}

#endif

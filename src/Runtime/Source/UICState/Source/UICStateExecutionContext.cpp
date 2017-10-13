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
#include "UICStateExecutionContext.h"
#include "UICStateExecutionTypes.h"
#include "UICStateInterpreter.h"
#include "foundation/Qt3DSFoundation.h"
#include "foundation/Qt3DSBroadcastingAllocator.h"
#include "foundation/Qt3DSAtomic.h"
#include "foundation/StringConversionImpl.h"
#include "UICStateScriptContext.h"
#include "foundation/Utils.h"
#include "EASTL/string.h"

using namespace qt3ds::state;

namespace {
struct SExecContext : public IExecutionContext
{
    NVFoundationBase &m_Foundation;
    NVScopedRefCounted<IStringTable> m_StringTable;
    // Referencing this here would create circular references
    IStateInterpreter *m_Interpreter;
    IStateLogger *m_DebugLogger;
    NVScopedRefCounted<IStateLogger> m_Logger;
    NVScopedRefCounted<IScriptContext> m_ScriptContext;
    bool m_Error;
    QT3DSI32 mRefCount;
    eastl::string m_DelayStr;

    SExecContext(NVFoundationBase &inFnd, IStringTable &inStrTable, IStateLogger &inLogger,
                 IScriptContext &inContext)
        : m_Foundation(inFnd)
        , m_StringTable(inStrTable)
        , m_Interpreter(NULL)
        , m_DebugLogger(NULL)
        , m_Logger(inLogger)
        , m_ScriptContext(inContext)
        , m_Error(false)
        , mRefCount(0)
    {
    }

    QT3DS_IMPLEMENT_REF_COUNT_ADDREF_RELEASE(m_Foundation.getAllocator())

    void SetInterpreter(IStateInterpreter &inInterpreter) override
    {
        m_Interpreter = &inInterpreter;
    }

    void SetMachineDebugLogger(IStateLogger &inDebugLogger) override
    {
        m_DebugLogger = &inDebugLogger;
    }

    void SignalError()
    {
        const char8_t *errorInfo = m_ScriptContext->GetErrorInfo();
        if (errorInfo && *errorInfo) {
            m_Interpreter->QueueEvent("error.execution", false);
        }
        m_Error = true;
    }
    void ExecuteContent(SExecutableContent &content)
    {
        switch (content.m_Type) {
        case ExecutableContentTypes::Send: {
            SSend &theSend = *content.CastTo<SSend>();
            CRegisteredString theEvent;
            if (!isTrivial(theSend.m_EventExpr)) {
                SScriptExecutionResult theStr =
                    m_ScriptContext->ExecuteExpressionToString(theSend.m_EventExpr);
                if (theStr.Valid())
                    theEvent = m_StringTable->RegisterStr(theStr.Result());
            } else
                theEvent = theSend.m_Event;

            QT3DSU64 theDelay(0);
            m_DelayStr.clear();
            if (!isTrivial(theSend.m_DelayExpr)) {
                SScriptExecutionResult theStr =
                    m_ScriptContext->ExecuteExpressionToString(theSend.m_DelayExpr);
                if (theStr.Valid())
                    m_DelayStr.assign(nonNull(theStr.Result()));
            } else
                m_DelayStr.assign(nonNull(theSend.m_Delay));
            if (m_DelayStr.size())
                theDelay = ParseTimeStrToMilliseconds(m_DelayStr.c_str());

            if (theEvent.IsValid()) {
                TEventPtr theEventPtr;
                if (theSend.m_Children.empty() == false) {
                    IScriptEvent *theNewEvent = m_ScriptContext->CreateScriptEvent(theEvent);
                    theEventPtr = theNewEvent;
                    for (TExecutableContentList::iterator iter = theSend.m_Children.begin(),
                                                          end = theSend.m_Children.end();
                         iter != end && m_Error == false; ++iter) {
                        if (iter->m_Type == ExecutableContentTypes::Param) {
                            SParam &theParam = static_cast<SParam &>(*iter);
                            if (theParam.m_Location.IsValid() == false) {
                                bool success =
                                    theNewEvent->SetParam(theParam.m_Name, theParam.m_Expr);
                                if (!success)
                                    SignalError();
                            } else {
                                QT3DS_ASSERT(false);
                            }
                        } else if (iter->m_Type == ExecutableContentTypes::Content) {
                            SContent &theContent = static_cast<SContent &>(*iter);
                            if (!isTrivial(theContent.m_Expr)) {
                                bool success = theNewEvent->SetDataExpr(theContent.m_Expr);
                                if (!success)
                                    SignalError();
                            } else if (!isTrivial(theContent.m_ContentValue)) {
                                bool success = theNewEvent->SetDataStr(theContent.m_ContentValue);
                                if (!success)
                                    SignalError();
                            }
                        } else {
                            QT3DS_ASSERT(false);
                        }
                    }
                }
                if (m_Error == false) {
                    bool isExternal = true;
                    if (AreEqual("#_internal", theSend.m_Target))
                        isExternal = false;
                    if (theEventPtr)
                        m_Interpreter->QueueEvent(theEventPtr, theDelay, theSend.m_Id, isExternal);
                    else
                        m_Interpreter->QueueEvent(theEvent, theDelay, theSend.m_Id, isExternal);
                }
            }
        } break;
        case ExecutableContentTypes::Cancel: {
            SCancel &theCancel = *content.CastTo<SCancel>();
            if (theCancel.m_Send) {
                m_Interpreter->CancelEvent(theCancel.m_Send->m_Id);
            } else if (!isTrivial(theCancel.m_IdExpression)) {
                SScriptExecutionResult theStr =
                    m_ScriptContext->ExecuteExpressionToString(theCancel.m_IdExpression);
                if (theStr.Valid()) {
                    const char8_t *theStrVal(theStr.Result());
                    if (!isTrivial(theStrVal))
                        m_Interpreter->CancelEvent(m_StringTable->RegisterStr(theStrVal));
                }
            }
        } break;
        case ExecutableContentTypes::Raise: {
            SRaise &theRaise = *content.CastTo<SRaise>();
            m_Interpreter->QueueEvent(theRaise.m_Event, false);
        } break;
        case ExecutableContentTypes::Log: {
            SLog *theLog = content.CastTo<SLog>();
            SScriptExecutionResult str =
                m_ScriptContext->ExecuteExpressionToString(theLog->m_Expression);
            if (str.Valid()) {
                m_Logger->Log(theLog->m_Label, str.Result());
                if (m_DebugLogger)
                    m_DebugLogger->Log(theLog->m_Label, str.Result());
            } else
                SignalError();
        } break;
        case ExecutableContentTypes::If: {
            SIf &theIf = *content.CastTo<SIf>();
            Option<bool> theCondResult = m_ScriptContext->ExecuteCondition(theIf.m_Cond);
            if (theCondResult.hasValue()) {
                bool currentConditionResult = *theCondResult;
                for (TExecutableContentList::iterator ifIter = theIf.m_Children.begin(),
                                                      ifEnd = theIf.m_Children.end();
                     ifIter != ifEnd && m_Error == false; ++ifIter) {
                    if (currentConditionResult) {
                        switch (ifIter->m_Type) {
                        case ExecutableContentTypes::Else:
                        case ExecutableContentTypes::ElseIf:
                            return;
                        default:
                            ExecuteContent(*ifIter);
                            break;
                        }
                    } else {
                        switch (ifIter->m_Type) {
                        case ExecutableContentTypes::ElseIf: {
                            SElseIf &theElseIf = *ifIter->CastTo<SElseIf>();
                            theCondResult = m_ScriptContext->ExecuteCondition(theElseIf.m_Cond);
                            if (theCondResult.hasValue())
                                currentConditionResult = *theCondResult;
                            else {
                                SignalError();
                                return;
                            }
                        } break;
                        case ExecutableContentTypes::Else:
                            currentConditionResult = true;
                            break;
                        // Ignore all content that isn't if or else if we shouldn't be currently
                        // executing it.
                        default:
                            break;
                        }
                    }
                }
            } else
                SignalError();
        } break;
        case ExecutableContentTypes::Foreach: {
            SForeach &theItem = *content.CastTo<SForeach>();
            Option<bool> success;
            for (success = m_ScriptContext->BeginForeach(theItem.m_Array, theItem.m_Item,
                                                         theItem.m_Index);
                 success.hasValue() && *success && m_Error == false;
                 success = m_ScriptContext->NextForeach(theItem.m_Item, theItem.m_Index)) {
                ExecuteContent(theItem.m_Children);
            }
            if (m_Error) {

            } else if (success.hasValue() == false)
                SignalError();
        } break;
        // We shouldn't get top level else or else if statements, they can only be inside an if
        // statement.
        case ExecutableContentTypes::Else:
        case ExecutableContentTypes::ElseIf:
            QT3DS_ASSERT(false);
            break;

        case ExecutableContentTypes::Assign: {
            SAssign &theAssign = *content.CastTo<SAssign>();
            bool success = m_ScriptContext->Assign(theAssign.m_Location, theAssign.m_Expression);
            if (!success)
                SignalError();
        } break;
        case ExecutableContentTypes::Script: {
            SScript &theScript = *content.CastTo<SScript>();
            if (!isTrivial(theScript.m_Data))
                m_ScriptContext->ExecuteScript(theScript.m_Data);
        } break;
        default:
            qCCritical(INTERNAL_ERROR, "Unimplemented executable content %s",
                ExecutableContentTypes::ToString(content.m_Type));
        }
    }

    void ExecuteContent(TExecutableContentList &inContent)
    {
        for (TExecutableContentList::iterator iter = inContent.begin(), end = inContent.end();
             iter != end && m_Error == false; ++iter) {
            ExecuteContent(*iter);
        }
    }

    void Execute(STransition &inTransaction) override
    {
        m_Error = false;
        ExecuteContent(inTransaction.m_ExecutableContent);
    }

    // These functions take the node as well as the list so a context can cache a fast
    // execution path if necessary.
    void Execute(SStateNode & /*inNode*/, TOnEntryList &inList) override
    {
        for (TOnEntryList::iterator iter = inList.begin(), end = inList.end(); iter != end;
             ++iter) {
            m_Error = false;
            ExecuteContent(iter->m_ExecutableContent);
        }
    }

    void Execute(SStateNode & /*inNode*/, TOnExitList &inList) override
    {
        for (TOnExitList::iterator iter = inList.begin(), end = inList.end(); iter != end; ++iter) {
            m_Error = false;
            ExecuteContent(iter->m_ExecutableContent);
        }
    }
};
}

QT3DSU64 IExecutionContext::ParseTimeStrToMilliseconds(const char8_t *timeStr)
{
    if (isTrivial(timeStr))
        return 0;

    char *endPtr;
    double theData = strtod(timeStr, &endPtr);
    if (!isTrivial(endPtr)) {
        if (AreEqual(endPtr, "s")) {
            theData *= 1000;
        } else if (AreEqual(endPtr, "ms")) {
            // empty intentional
        } else
            theData = 0;
    } else
        theData = 0;
    if (theData < 0)
        theData = 0.0;
    return static_cast<QT3DSU64>(theData);
}

IExecutionContext &IExecutionContext::Create(NVFoundationBase &inFoundation,
                                             IStringTable &inStringTable, IStateLogger &inLogger,
                                             IScriptContext &inScriptContext)
{
    return *QT3DS_NEW(inFoundation.getAllocator(), SExecContext)(inFoundation, inStringTable, inLogger,
                                                              inScriptContext);
}

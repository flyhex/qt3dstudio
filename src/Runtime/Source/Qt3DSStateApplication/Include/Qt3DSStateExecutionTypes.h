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
#ifndef UIC_STATE_EXECUTION_TYPES_H
#define UIC_STATE_EXECUTION_TYPES_H
#pragma once
#include "Qt3DSStateTypes.h"

namespace qt3ds {
namespace state {

    struct ExecutableContentTypes
    {
        enum Enum {
            NoType = 0,
            Raise,
            If,
            ElseIf,
            Else,
            Foreach,
            Log,
            Assign,
            Script,
            Send,
            Cancel,
            Param,
            Content,
        };
        static const char8_t *ToString(Enum inVal)
        {
            switch (inVal) {
            case Raise:
                return "Raise";
            case If:
                return "If";
            case ElseIf:
                return "ElseIf";
            case Else:
                return "Else";
            case Foreach:
                return "Foreach";
            case Log:
                return "Log";
            case Assign:
                return "Assign";
            case Script:
                return "Script";
            case Send:
                return "Send";
            case Cancel:
                return "Cancel";
            case Param:
                return "Param";
            case Content:
                return "Content";
            default:
                return "Unknown execution type";
            }
        }
    };

    struct SRaise;
    struct SIf;
    struct SElseIf;
    struct SElse;
    struct SForeach;
    struct SLog;
    struct SAssign;
    struct SScript;
    struct SSend;
    struct SCancel;
    struct SParam;
    struct SContent;

    template <typename TDataType>
    struct SExecutableContentTypeMap
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::NoType;
        }
    };

    template <>
    struct SExecutableContentTypeMap<SRaise>
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::Raise;
        }
    };
    template <>
    struct SExecutableContentTypeMap<SIf>
    {
        static ExecutableContentTypes::Enum GetContentType() { return ExecutableContentTypes::If; }
    };
    template <>
    struct SExecutableContentTypeMap<SElseIf>
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::ElseIf;
        }
    };
    template <>
    struct SExecutableContentTypeMap<SElse>
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::Else;
        }
    };
    template <>
    struct SExecutableContentTypeMap<SForeach>
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::Foreach;
        }
    };
    template <>
    struct SExecutableContentTypeMap<SLog>
    {
        static ExecutableContentTypes::Enum GetContentType() { return ExecutableContentTypes::Log; }
    };
    template <>
    struct SExecutableContentTypeMap<SAssign>
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::Assign;
        }
    };
    template <>
    struct SExecutableContentTypeMap<SScript>
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::Script;
        }
    };
    template <>
    struct SExecutableContentTypeMap<SSend>
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::Send;
        }
    };
    template <>
    struct SExecutableContentTypeMap<SCancel>
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::Cancel;
        }
    };
    template <>
    struct SExecutableContentTypeMap<SParam>
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::Param;
        }
    };
    template <>
    struct SExecutableContentTypeMap<SContent>
    {
        static ExecutableContentTypes::Enum GetContentType()
        {
            return ExecutableContentTypes::Content;
        }
    };

    // Defined by the execution context to speed up evaluation of executable data.
    struct SExecutionData;

    struct SExecutableContent
    {
        const ExecutableContentTypes::Enum m_Type;
        SStateNode *m_StateNodeParent;
        SExecutableContent *m_Parent;
        SExecutableContent *m_NextSibling;
        SExecutableContent *m_PreviousSibling;
        TExecutableContentList m_Children;

        SExecutableContent(ExecutableContentTypes::Enum inType)
            : m_Type(inType)
            , m_StateNodeParent(NULL)
            , m_Parent(NULL)
            , m_NextSibling(NULL)
            , m_PreviousSibling(NULL)
        {
        }

        template <typename TDataType>
        TDataType *CastTo()
        {
            if (m_Type == SExecutableContentTypeMap<TDataType>::GetContentType())
                return static_cast<TDataType *>(this);
            return NULL;
        }

        template <typename TDataType>
        const TDataType *CastTo() const
        {
            if (m_Type == SExecutableContentTypeMap<TDataType>::GetContentType())
                return static_cast<const TDataType *>(this);
            return NULL;
        }
    };

    IMPLEMENT_INVASIVE_LIST(ExecutableContent, m_PreviousSibling, m_NextSibling);

    struct SRaise : public SExecutableContent
    {
        CRegisteredString m_Event;
        SRaise()
            : SExecutableContent(ExecutableContentTypes::Raise)
        {
        }
    };

    struct SIf : public SExecutableContent
    {
        const char8_t *m_Cond;
        SIf()
            : SExecutableContent(ExecutableContentTypes::If)
            , m_Cond(NULL)
        {
        }
    };

    struct SElseIf : public SExecutableContent
    {
        const char8_t *m_Cond;
        SElseIf()
            : SExecutableContent(ExecutableContentTypes::ElseIf)
            , m_Cond(NULL)
        {
        }
    };

    struct SElse : public SExecutableContent
    {
        SElse()
            : SExecutableContent(ExecutableContentTypes::Else)
        {
        }
    };

    struct SForeach : public SExecutableContent
    {
        CRegisteredString m_Array;
        CRegisteredString m_Item;
        CRegisteredString m_Index;
        SForeach()
            : SExecutableContent(ExecutableContentTypes::Foreach)
        {
        }
    };

    struct SLog : public SExecutableContent
    {
        CRegisteredString m_Label;
        const char8_t *m_Expression;
        SLog()
            : SExecutableContent(ExecutableContentTypes::Log)
            , m_Expression(NULL)
        {
        }
    };

    struct SAssign : public SExecutableContent
    {
        const char8_t *m_Location;
        const char8_t *m_Expression;
        SAssign()
            : SExecutableContent(ExecutableContentTypes::Assign)
            , m_Location(NULL)
            , m_Expression(NULL)
        {
        }
    };

    struct SScript : public SExecutableContent
    {
        const char8_t *m_URL;
        const char8_t *m_Data;
        SScript()
            : SExecutableContent(ExecutableContentTypes::Script)
            , m_URL(NULL)
            , m_Data(NULL)
        {
        }
    };

    struct SSend : public SExecutableContent
    {
        CRegisteredString m_Event;
        const char8_t *m_EventExpr;
        const char8_t *m_Target;
        const char8_t *m_TargetExpr;
        const char8_t *m_Type;
        const char8_t *m_TypeExpr;
        CRegisteredString m_Id;
        const char8_t *m_IdLocation;
        const char8_t *m_Delay;
        const char8_t *m_DelayExpr;
        const char8_t *m_NameList;

        SSend()
            : SExecutableContent(ExecutableContentTypes::Send)
            , m_EventExpr(NULL)
            , m_Target(NULL)
            , m_TargetExpr(NULL)
            , m_Type(NULL)
            , m_TypeExpr(NULL)
            , m_IdLocation(NULL)
            , m_Delay(NULL)
            , m_DelayExpr(NULL)
            , m_NameList(NULL)
        {
        }
    };
    struct SCancel : public SExecutableContent
    {
        // If we have an id.
        SSend *m_Send;
        const char8_t *m_IdExpression;

        SCancel()
            : SExecutableContent(ExecutableContentTypes::Cancel)
            , m_Send(NULL)
            , m_IdExpression(NULL)
        {
        }
    };

    struct SParam : public SExecutableContent
    {
        CRegisteredString m_Name;
        const char8_t *m_Expr;
        CRegisteredString m_Location;
        SParam()
            : SExecutableContent(ExecutableContentTypes::Param)
            , m_Expr(NULL)
        {
        }
    };

    struct SContent : public SExecutableContent
    {
        const char8_t *m_Expr;
        const char8_t *m_ContentValue;
        SContent()
            : SExecutableContent(ExecutableContentTypes::Content)
            , m_Expr(NULL)
            , m_ContentValue(NULL)
        {
        }
    };
}
}

#endif

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
#ifndef UICDMERRORSH
#define UICDMERRORSH
#include "Qt3DSDMWindowsCompatibility.h"

namespace qt3dsdm {
class UICDMError : public std::exception
{
public:
    UICDMError(const wchar_t *inMessage)
        : std::exception()
    {
        wcscpy_s(m_Message, inMessage);
    }
    wchar_t m_Message[1024];
};

class HandleExists : public UICDMError
{
public:
    HandleExists(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class PropertyNotFound : public UICDMError
{
public:
    PropertyNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class PropertyExists : public UICDMError
{
public:
    PropertyExists(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class DuplicateInstanceName : public UICDMError
{
public:
    DuplicateInstanceName(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class InstanceNotFound : public UICDMError
{
public:
    InstanceNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class ValueTypeError : public UICDMError
{
public:
    ValueTypeError(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class SerializationError : public UICDMError
{
public:
    SerializationError(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class SlideNotFound : public UICDMError
{
public:
    SlideNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class SlideExists : public UICDMError
{
public:
    SlideExists(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class SlideDerivationError : public UICDMError
{
public:
    SlideDerivationError(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class SlideChildNotFoundError : public UICDMError
{
public:
    SlideChildNotFoundError(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class SlideGraphNotFound : public UICDMError
{
public:
    SlideGraphNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class SlideGraphExists : public UICDMError
{
public:
    SlideGraphExists(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class PropertyLinkError : public UICDMError
{
public:
    PropertyLinkError(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class RearrangeSlideArgumentsMustNotBeZero : public UICDMError
{
public:
    RearrangeSlideArgumentsMustNotBeZero(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class AnimationNotFound : public UICDMError
{
public:
    AnimationNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class AnimationExists : public UICDMError
{
public:
    AnimationExists(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class AnimationKeyframeTypeError : public UICDMError
{
public:
    AnimationKeyframeTypeError(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class AnimationKeyframeNotFound : public UICDMError
{
public:
    AnimationKeyframeNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class AnimationEvaluationError : public UICDMError
{
public:
    AnimationEvaluationError(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class ActionNotFound : public UICDMError
{
public:
    ActionNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class ActionExists : public UICDMError
{
public:
    ActionExists(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class HandlerArgumentNotFound : public UICDMError
{
public:
    HandlerArgumentNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class HandlerArgumentExists : public UICDMError
{
public:
    HandlerArgumentExists(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class CustomPropertyNotFound : public UICDMError
{
public:
    CustomPropertyNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class CustomEventNotFound : public UICDMError
{
public:
    CustomEventNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class CustomHandlerNotFound : public UICDMError
{
public:
    CustomHandlerNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class CustomHandlerParamNotFound : public UICDMError
{
public:
    CustomHandlerParamNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};

class AttributeTypeNotFound : public UICDMError
{
public:
    AttributeTypeNotFound(const wchar_t *inMessage)
        : UICDMError(inMessage)
    {
    }
};
}

#endif
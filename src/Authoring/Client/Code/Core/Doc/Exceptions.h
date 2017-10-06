/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#ifndef INCLUDED_EXCEPTIONS_H
#define INCLUDED_EXCEPTIONS_H 1

#pragma once

#include "StudioException.h"
#include "UICString.h"

class CGeneralException : public CStudioException
{
public:
    CGeneralException(const Q3DStudio::CString &inDescription)
        : m_Description(inDescription)
    {
    }
    virtual ~CGeneralException() {}

    const wchar_t *GetDescription() const override
    {
        return static_cast<const wchar_t *>(m_Description);
    }

protected:
    Q3DStudio::CString m_Description;
};

class CNoClientException : public CStudioException
{
public:
    CNoClientException() {}
    virtual ~CNoClientException() {}
    const wchar_t *GetDescription() const override
    {
        return Q3DStudio::CString(_UIC("Client is not installed"));
    }
};

class CClientCreationException : public CStudioException
{
public:
    CClientCreationException() {}
    virtual ~CClientCreationException() {}
    const wchar_t *GetDescription() const override
    {
        return Q3DStudio::CString(_UIC("Unable to create Client"));
    }
};

class CInvalidFileFormatException : public CStudioException
{
public:
    CInvalidFileFormatException() {}
    virtual ~CInvalidFileFormatException() {}
    const wchar_t *GetDescription() const override
    {
        return Q3DStudio::CString(_UIC("Invalid file format"));
    }
};

class CUnsupportedFileFormatException : public CStudioException
{
public:
    CUnsupportedFileFormatException() {}
    virtual ~CUnsupportedFileFormatException() {}
    const wchar_t *GetDescription() const override
    {
        return Q3DStudio::CString(_UIC("Unsupported file format"));
    }
};

class CENotImplException : public CStudioException
{
public:
    CENotImplException() {}
    virtual ~CENotImplException() {}
    const wchar_t *GetDescription() const override { return Q3DStudio::CString(_UIC("ENOTIMPL")); }
};

class CInvalidArgumentsException : public CStudioException
{
public:
    CInvalidArgumentsException() {}
    virtual ~CInvalidArgumentsException() {}
    const wchar_t *GetDescription() const override
    {
        return Q3DStudio::CString(_UIC("Invalid Argument"));
    }
};

class CNullPointerException : public CStudioException
{
public:
    CNullPointerException() {}
    virtual ~CNullPointerException() {}
    const wchar_t *GetDescription() const override
    {
        return Q3DStudio::CString(_UIC("Null Pointer Error"));
    }
};

class CUnexpectedResultException : public CStudioException
{
public:
    CUnexpectedResultException() {}
    virtual ~CUnexpectedResultException() {}
    const wchar_t *GetDescription() const override
    {
        return Q3DStudio::CString(_UIC("Unexpected Result"));
    }
};

class CClientFailException : public CStudioException
{
public:
    CClientFailException() {}
    virtual ~CClientFailException() {}
    const wchar_t *GetDescription() const override
    {
        return Q3DStudio::CString(_UIC("Client Failure"));
    }
};

// Any referenced (e.g. Data File) load errors
class CLoadReferencedFileException : public CGeneralException
{
public:
    CLoadReferencedFileException(const wchar_t *inFilePath, const Q3DStudio::CString &inDescription)
        : CGeneralException(inDescription)
    {
        if (inFilePath)
            m_FilePath = inFilePath;
    }
    virtual ~CLoadReferencedFileException() {}
    const wchar_t *GetFilePath() const { return static_cast<const wchar_t *>(m_FilePath); }

protected:
    Q3DStudio::CString m_FilePath;
};

#endif // INCLUDED_EXCEPTIONS_H

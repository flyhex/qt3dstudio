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
#include <QtCore/qstring.h>
#include <QtCore/qobject.h>

class CGeneralException : public CStudioException
{
public:
    CGeneralException(const QString &inDescription)
        : m_Description(inDescription)
    {
    }
    virtual ~CGeneralException() {}

    QString GetDescription() const override
    {
        return m_Description;
    }

protected:
    QString m_Description;
};

class CNoClientException : public CStudioException
{
public:
    CNoClientException() {}
    virtual ~CNoClientException() {}
    QString GetDescription() const override
    {
        return QObject::tr("Client is not installed");
    }
};

class CClientCreationException : public CStudioException
{
public:
    CClientCreationException() {}
    virtual ~CClientCreationException() {}
    QString GetDescription() const override
    {
        return QObject::tr("Unable to create Client");
    }
};

class CInvalidFileFormatException : public CStudioException
{
public:
    CInvalidFileFormatException() {}
    virtual ~CInvalidFileFormatException() {}
    QString GetDescription() const override
    {
        return QObject::tr("Invalid file format");
    }
};

class CUnsupportedFileFormatException : public CStudioException
{
public:
    CUnsupportedFileFormatException() {}
    virtual ~CUnsupportedFileFormatException() {}
    QString GetDescription() const override
    {
        return QObject::tr("Unsupported file format");
    }
};

class ProjectFileNotFoundException : public CStudioException
{
public:
    ProjectFileNotFoundException() {}
    virtual ~ProjectFileNotFoundException() {}
    QString GetDescription() const override
    {
        return QObject::tr("Project file was not found");
    }
};

class CENotImplException : public CStudioException
{
public:
    CENotImplException() {}
    virtual ~CENotImplException() {}
    QString GetDescription() const override { return QObject::tr("ENOTIMPL"); }
};

class CInvalidArgumentsException : public CStudioException
{
public:
    CInvalidArgumentsException() {}
    virtual ~CInvalidArgumentsException() {}
    QString GetDescription() const override
    {
        return QObject::tr("Invalid Argument");
    }
};

class CNullPointerException : public CStudioException
{
public:
    CNullPointerException() {}
    virtual ~CNullPointerException() {}
    QString GetDescription() const override
    {
        return QObject::tr("Null Pointer Error");
    }
};

class CUnexpectedResultException : public CStudioException
{
public:
    CUnexpectedResultException() {}
    virtual ~CUnexpectedResultException() {}
    QString GetDescription() const override
    {
        return QObject::tr("Unexpected Result");
    }
};

class CClientFailException : public CStudioException
{
public:
    CClientFailException() {}
    virtual ~CClientFailException() {}
    QString GetDescription() const override
    {
        return QObject::tr("Client Failure");
    }
};

// Any referenced (e.g. Data File) load errors
class CLoadReferencedFileException : public CGeneralException
{
public:
    CLoadReferencedFileException(const QString &inFilePath, const QString &inDescription)
        : CGeneralException(inDescription)
    {
        if (!inFilePath.isEmpty())
            m_FilePath = inFilePath;
    }
    virtual ~CLoadReferencedFileException() {}
    QString GetFilePath() const { return m_FilePath; }

protected:
    QString m_FilePath;
};

#endif // INCLUDED_EXCEPTIONS_H

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
#ifndef UICIMPORTERRORCODESH
#define UICIMPORTERRORCODESH

namespace UICIMP {

#define NVIMP_ITERATE_ERROR_CODES                                                                  \
    NVIMP_HANDLE_ERROR_CODE(ResourceNotWriteable)                                                  \
    NVIMP_HANDLE_ERROR_CODE(SourceFileNotReadable)                                                 \
    NVIMP_HANDLE_ERROR_CODE(UnableToCreateDirectory)                                               \
    NVIMP_HANDLE_ERROR_CODE(SourceFileDoesNotExist)                                                \
    NVIMP_HANDLE_ERROR_CODE(TranslationToImportFailed)

struct ImportErrorCodes
{
    enum Enum {
        NoError = 0,
#define NVIMP_HANDLE_ERROR_CODE(x) x,
        NVIMP_ITERATE_ERROR_CODES
#undef NVIMP_HANDLE_ERROR_CODE
    };
    static const wchar_t *GetEnglishFormatString(Enum value)
    {
        switch (value) {
        case ResourceNotWriteable:
            return L"Refresh failed because could not update Resource:\n %s\nPlease check out "
                   L"resource and try again";
        case SourceFileNotReadable:
            return L"Could not open %s for read\n";
        case UnableToCreateDirectory:
            return L"Unable to create directory %s\n";
        default: ;
        }
        return L"No error code";
    }
};
}

#endif

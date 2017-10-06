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
#ifndef INCLUDED_DLL_VERSION_H
#define INCLUDED_DLL_VERSION_H 1

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
/*  For some odd reason, Microsoft published a sample code that uses shlwapi.h
    (and shlwapi.lib)
    to tinker file versions.

    This header file could not be found anywhere !!!
    Not in Visual C++ 4.2, Visual C++ 5.0 or MSDN versions up to July 97`.

    So, I just took out the interesting structures from scraps I found
    and re-defined them here.
*/

//  Remember: You must link version.lib to the project for this class to work !!

#include <string>
/*
#ifndef DLLVERSIONINFO
typedef struct _DllVersionInfo
{
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
}DLLVERSIONINFO;

#endif
*/
/*
#ifndef DLLGETVERSIONPROC
typedef int (FAR WINAPI *DLLGETVERSIONPROC) (DLLVERSIONINFO *);
#endif
*/
class CDLLVersion
{
    typedef enum {
        WIN_DIR, // Windows directory (e.g.: "C:\Windows\")
        SYS_DIR, // Windows system directory (e.g.: "C:\Windows\System")
        CUR_DIR, // Current directory (e.g.: ".")
        NO_DIR
    } // No directory (path in file name)
    FileLocationType; // Possible ways to add a path prefix to a file

public:
    CDLLVersion(LPCSTR szDLLFileName)
        : m_dwMajor(0)
        , m_dwMinor(0)
        , m_dwIteration(0)
        , m_dwBuild(0)
    {
        m_bValid = GetDLLVersion(szDLLFileName, m_dwMajor, m_dwMinor, m_dwIteration, m_dwBuild);
    }

    virtual ~CDLLVersion(){}

    DWORD GetMajorVersion() { return m_dwMajor; }

    DWORD GetMinorVersion() { return m_dwMinor; }

    DWORD GetBuildNumber() { return m_dwBuild; }

    DWORD GetIterationNumber() { return m_dwIteration; }

    BOOL IsValid() { return m_bValid; }

    std::string GetFullVersion() { return m_stFullVersion; }

private:
    char *getVersion(char *fileName);

    BOOL GetDLLVersion(LPCSTR szDLLFileName, DWORD &dwMajor, DWORD &dwMinor, DWORD &outIteration,
                       DWORD &dwBuildNumber);
    BOOL ParseVersionString(LPSTR lpVersion, DWORD &dwMajor, DWORD &dwMinor, DWORD &outIteration,
                            DWORD &dwBuildNumber);
    BOOL ParseVersionString1(LPSTR lpVersion, DWORD &dwMajor, DWORD &dwMinor, DWORD &outIteration,
                             DWORD &dwBuildNumber);
    BOOL CheckFileVersion(LPSTR szFileName, int FileLoc, DWORD &dwMajor, DWORD &dwMinor,
                          DWORD &outIteration, DWORD &dwBuildNumber);

    BOOL FixFilePath(char *szFileName, int FileLoc);

    DWORD m_dwMajor; // Major version number
    DWORD m_dwMinor; // Minor version number
    DWORD m_dwIteration;
    DWORD m_dwBuild; // Build number
    BOOL m_bValid; // Is the DLL version information valid ?
    std::string m_stFullVersion;
};

#endif // INCLUDED_DLL_VERSION_H

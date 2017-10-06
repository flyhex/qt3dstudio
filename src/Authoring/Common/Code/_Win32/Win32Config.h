/****************************************************************************
**
** Copyright (C) 1999-2002 NVIDIA Corporation.
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
//==============================================================================
//	Prefix
//==============================================================================

#ifndef INCLUDED_WIN32_CONFIG_H
#define INCLUDED_WIN32_CONFIG_H 1

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//==============================================================================
//	Includes
//==============================================================================

#include <string>
#include <vector>
#ifdef KDAB_TEMPORARILY_REMOVED
#include <Wbemidl.h>
#endif
#include "CompConfig.h"

//==============================================================================
//	Constants
//==============================================================================

//==============================================================================
//	Class
//==============================================================================
//==============================================================================
/**
 *	@class	CWin32Config
 *
 */
//==============================================================================
class CWin32Config : public CCompConfig
{
    //==============================================================================
    //	Structures
    //==============================================================================

protected:
    struct SOperatingSystem
    {
        std::string CountryCode;
        std::string Locale;
        std::string Name;
        std::string ServicePacks;
        std::string TotalVisibleMemorySize;
        std::string Version;
    };

    struct SProcessor
    {
        std::string CurrentClockSpeed;
        std::string DeviceID;
        std::string Manufacturer;
        std::string MaxClockSpeed;
        std::string Name;
        std::string Version;
    };

    struct SVideoController
    {
        std::string AdapterRAM;
        std::string CurrentBitsPerPixel;
        std::string CurrentHorizontalResolution;
        std::string CurrentRefreshRate;
        std::string CurrentVerticalResolution;
        std::string DeviceID;
        std::string DriverDate;
        std::string DriverVersion;
        std::string InstalledDisplayDrivers;
        std::string MaxRefreshRate;
        std::string MinRefreshRate;
        std::string Name;
        std::string VideoProcessor;
        std::string DX7CapsKey;
    };

    struct SApplications
    {
        std::string DXVersion;
        std::string IEVersion;
    };

    //==============================================================================
    //	Fields
    //==============================================================================

protected:
    std::string m_ConfigName; ///< The name of this specific configuration

    SOperatingSystem m_OS; ///< Contains inforamtion about the operating system
    std::vector<SProcessor> m_Processors; ///< Contains information about the system processor
    SVideoController m_VideoController; ///< Contains information about the video controller
    SApplications m_Applications; ///< Contians information about various applications installed

    //==============================================================================
    //	Methods
    //==============================================================================

    // Construction

public:
    CWin32Config();
    virtual ~CWin32Config();

    // Access

public:
    void DetermineConfig() override;
    void WriteXML(std::ostream &inStream) override;

    // Implementation

protected:
#ifdef KDAB_TEMPORARILY_REMOVED
    void DetermineOS(IWbemServices *inServices);
    void DetermineProcessor(IWbemServices *inServices);
    void DetermineVideoController(IWbemServices *inServices);
    void DetermineApps(IWbemServices *inServices);
    void GetConfigName(IWbemServices *inServices);
    void GetObjValue(IWbemClassObject *inObject, BSTR inPropName, std::string &ioValue);
    void GetDXVersion();
    void GetIEVersion();
    void GetVidGuid();
    bool GetComponentVersion(LPTSTR inPath, std::string &outVersion);
#endif
};

#endif // INCLUDED_WIN32_CONFIG_H

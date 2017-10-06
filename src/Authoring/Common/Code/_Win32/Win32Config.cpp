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
//	Includes
//==============================================================================

#include "stdafx.h"

#ifdef KDAB_TEMPORARILY_REMOVED
#include <objbase.h>
#include <atlbase.h>
#endif

#ifndef INCLUDED_WIN32_CONFIG_H
#include "Win32Config.h"
#endif
#ifdef _WIN32
#pragma comment (lib,"version.lib")
#endif

//==============================================================================
/**
 *	Constructor:
 */
//==============================================================================
CWin32Config::CWin32Config()
{
}

//==============================================================================
/**
 *	Descructor: Destroys the object
 */
//==============================================================================
CWin32Config::~CWin32Config()
{
}

//==============================================================================
/**
 *	DetermineConfig:	Obtains the overall computer information.
 *
 *	Operating system, hardware configuration, etc...
 *
 *	@return	NONE
 */
//==============================================================================
void CWin32Config::DetermineConfig()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    HRESULT theResult;
    CComPtr<IWbemLocator> theLocator;
    CComPtr<IWbemServices> theServices;
    CComBSTR theResource = "\\\\.\\root\\cimv2";

    // Initialize COM
    theResult = ::CoInitialize(NULL);
    if (SUCCEEDED(theResult)) {
        // Create WBEM Locator
        theResult = ::CoCreateInstance(CLSID_WbemLocator, NULL, CLSCTX_ALL, IID_IWbemLocator,
                                       (void **)&theLocator);
        if (theResult == WBEM_S_NO_ERROR && theLocator) {
            theResult =
                theLocator->ConnectServer(theResource, NULL, NULL, NULL,
                                          WBEM_FLAG_CONNECT_USE_MAX_WAIT, NULL, NULL, &theServices);
            if (theResult == WBEM_S_NO_ERROR && theServices) {
                // Switch the security level to IMPERSONATE so that provider(s)
                // will grant access to system-level objects, and so that
                // CALL authorization will be used.
                ::CoSetProxyBlanket(theServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                                    RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                    EOAC_NONE);

                // Get the configuration name
                GetConfigName(theServices);

                // Get OS info
                DetermineOS(theServices);

                // Get processor info
                DetermineProcessor(theServices);

                // Get video controller info
                DetermineVideoController(theServices);

                // Get application info
                DetermineApps(theServices);
            }
        }
    }
#endif
}

//==============================================================================
/**
 *	WriteXML:  method used for printing out in XML format.
 *
 *	@param	<std::ostream>	inStream		The stream we are writing to
 */
//==============================================================================
void CWin32Config::WriteXML(std::ostream &inStream)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    unsigned long theTabCounter;

// Helper macros for writing XML to stream

// Indent
#define XML_INDENT                                                                                 \
    for (theTabCounter = 0; theTabCounter < m_TabCount; ++theTabCounter) {                         \
        inStream << "\t";                                                                          \
    }

// Newline
#define XML_NEW_LINE (inStream << std::endl)
// Beginning tag
#define XML_BEGIN_TAG(inTagName) (inStream << "<" << inTagName)
// Name and value
#define XML_ATTRIBUTE(inTagName, inValue) (inStream << " " << inTagName << "=\"" << inValue << "\"")
// End tag
#define XML_END_TAG(inTagName) (inStream << "</" << inTagName << ">")
// Close tag
#define XML_CLOSE_TAG (inStream << ">")

    // Begin config tab
    XML_NEW_LINE;
    XML_BEGIN_TAG("Config2");
    XML_ATTRIBUTE("name", m_ConfigName.c_str());
    XML_ATTRIBUTE("VisibleMemory", m_OS.TotalVisibleMemorySize.c_str());
    XML_CLOSE_TAG;
    XML_NEW_LINE;
    XML_INDENT;

    // OS information
    XML_BEGIN_TAG("OS2");
    XML_ATTRIBUTE("CountryCode", m_OS.CountryCode.c_str());
    XML_ATTRIBUTE("Locale", m_OS.Locale.c_str());
    XML_ATTRIBUTE("name", m_OS.Name.c_str());
    XML_ATTRIBUTE("ServicePacks", m_OS.ServicePacks.c_str());
    XML_ATTRIBUTE("Version", m_OS.Version.c_str());
    XML_CLOSE_TAG;
    XML_END_TAG("OS2");
    XML_NEW_LINE;

    XML_INDENT;

    char theNumProcs[32];
    _itoa((int)m_Processors.size(), theNumProcs, 32);
    XML_BEGIN_TAG("ProcessorConfig");
    XML_ATTRIBUTE("NumProcs", theNumProcs);
    XML_CLOSE_TAG;
    XML_NEW_LINE;
    XML_INDENT;
    unsigned long theI;
    // Processor information
    for (theI = 0; theI < m_Processors.size(); theI++) {
        XML_BEGIN_TAG("Processor2");
        XML_ATTRIBUTE("CurrentClockSpeed", m_Processors[theI].CurrentClockSpeed.c_str());
        XML_ATTRIBUTE("Manufacturer", m_Processors[theI].Manufacturer.c_str());
        XML_ATTRIBUTE("MaxClockSpeed", m_Processors[theI].MaxClockSpeed.c_str());
        XML_ATTRIBUTE("name", m_Processors[theI].Name.c_str());
        XML_ATTRIBUTE("Version", m_Processors[theI].Version.c_str());
        XML_CLOSE_TAG;
        XML_END_TAG("Processor2");
        XML_NEW_LINE;

        XML_INDENT;
    }
    XML_END_TAG("ProcessorConfig");
    XML_NEW_LINE;
    XML_INDENT;

    // Video Controller information
    XML_BEGIN_TAG("VideoConfig");
    XML_CLOSE_TAG;
    XML_NEW_LINE;
    XML_INDENT;
    XML_BEGIN_TAG("DisplayConfig");

    XML_ATTRIBUTE("ColorDepth", m_VideoController.CurrentBitsPerPixel.c_str());
    XML_ATTRIBUTE("HResolution", m_VideoController.CurrentHorizontalResolution.c_str());
    XML_ATTRIBUTE("RefreshRate", m_VideoController.CurrentRefreshRate.c_str());
    XML_ATTRIBUTE("VResolution", m_VideoController.CurrentVerticalResolution.c_str());
    XML_CLOSE_TAG;
    XML_END_TAG("DisplayConfig");
    XML_NEW_LINE;
    XML_BEGIN_TAG("VideoAdapter");
    XML_ATTRIBUTE("AdapterRAM", m_VideoController.AdapterRAM.c_str());
    XML_ATTRIBUTE("MaxRefreshRate", m_VideoController.MaxRefreshRate.c_str());
    XML_ATTRIBUTE("MinRefreshRate", m_VideoController.MinRefreshRate.c_str());
    XML_CLOSE_TAG;
    XML_BEGIN_TAG("VideoChipsets");
    XML_ATTRIBUTE("DriverVersion", m_VideoController.DriverVersion.c_str());
    XML_ATTRIBUTE("VideoProcessor", m_VideoController.VideoProcessor.c_str());
    XML_ATTRIBUTE("DX7CapsKey", m_VideoController.DX7CapsKey.c_str());
    XML_CLOSE_TAG;
    XML_END_TAG("VideoChipsets");
    XML_NEW_LINE;
    XML_END_TAG("VideoAdapter");
    XML_NEW_LINE;

    XML_END_TAG("VideoConfig");
    XML_NEW_LINE;

    XML_INDENT;

    // Applications information
    XML_BEGIN_TAG("Applications");
    XML_ATTRIBUTE("DXVersion", m_Applications.DXVersion.c_str());
    XML_ATTRIBUTE("IEVersion", m_Applications.IEVersion.c_str());
    XML_CLOSE_TAG;
    XML_END_TAG("Applications");

    // End config tag
    XML_END_TAG("Config2");
    XML_NEW_LINE;
#endif
}

#ifdef KDAB_TEMPORARILY_REMOVED
//==============================================================================
/**
 *	DetermineOS:	Obtains information about the operating system.
 *
 *	@param	<IWbemServices>		inServices		Used to access WMI services
 *
 *	@return	NONE
 */
//==============================================================================
void CWin32Config::DetermineOS(IWbemServices *inServices)
{
    HRESULT theResult;
    CComPtr<IEnumWbemClassObject> theObjEnum;
    CComPtr<IWbemClassObject> theObjInstance;
    CComVariant theValue;
    CComBSTR theObjectName = "Win32_OperatingSystem";
    unsigned long theNumObjects;

    if (::IsBadReadPtr(inServices, sizeof(IWbemServices)) == FALSE) {
        // Create an Instance Enumerator
        theResult =
            inServices->CreateInstanceEnum(theObjectName, WBEM_FLAG_DEEP, NULL, &theObjEnum);
        if (SUCCEEDED(theResult) && theObjEnum) {
            while (WBEM_S_NO_ERROR == (theResult = theObjEnum->Next(INFINITE, 1, &theObjInstance,
                                                                    &theNumObjects))) {
                std::string theSPMajor;
                std::string theSPMinor;

                // Obtain neede values
                GetObjValue(theObjInstance, CComBSTR("name"), m_OS.Name);
                GetObjValue(theObjInstance, CComBSTR("CountryCode"), m_OS.CountryCode);
                GetObjValue(theObjInstance, CComBSTR("Locale"), m_OS.Locale);
                GetObjValue(theObjInstance, CComBSTR("Version"), m_OS.Version);
                GetObjValue(theObjInstance, CComBSTR("TotalVisibleMemorySize"),
                            m_OS.TotalVisibleMemorySize);

                // Obtain service pack major and minor version and put into string format
                GetObjValue(theObjInstance, CComBSTR("ServicePackMajorVersion"), theSPMajor);
                GetObjValue(theObjInstance, CComBSTR("ServicePackMinorVersion"), theSPMinor);
                m_OS.ServicePacks = theSPMajor;
                m_OS.ServicePacks += ".";
                m_OS.ServicePacks += theSPMinor;

                // Release the instance
                theObjInstance = NULL;
            }
        }
    }
}

//==============================================================================
/**
 *	DetermineProcessor:	Obtains information about the processor.
 *
 *	@param	<IWbemServices>		inServices		Used to access WMI services
 *
 *	@return	NONE
 */
//==============================================================================
void CWin32Config::DetermineProcessor(IWbemServices *inServices)
{
    HRESULT theResult;
    CComPtr<IEnumWbemClassObject> theObjEnum;
    CComPtr<IWbemClassObject> theObjInstance;
    CComVariant theValue;
    CComBSTR theObjectName = "Win32_Processor";
    unsigned long theNumObjects;

    if (::IsBadReadPtr(inServices, sizeof(IWbemServices)) == FALSE) {
        // Create an Instance Enumerator
        theResult =
            inServices->CreateInstanceEnum(theObjectName, WBEM_FLAG_DEEP, NULL, &theObjEnum);
        if (SUCCEEDED(theResult) && theObjEnum) {
            while (WBEM_S_NO_ERROR == (theResult = theObjEnum->Next(INFINITE, 1, &theObjInstance,
                                                                    &theNumObjects))) {
                // Obtain needed values
                SProcessor theProcessor;
                GetObjValue(theObjInstance, CComBSTR("CurrentClockSpeed"),
                            theProcessor.CurrentClockSpeed);
                GetObjValue(theObjInstance, CComBSTR("DeviceID"), theProcessor.DeviceID);
                GetObjValue(theObjInstance, CComBSTR("Manufacturer"), theProcessor.Manufacturer);
                GetObjValue(theObjInstance, CComBSTR("MaxClockSpeed"), theProcessor.MaxClockSpeed);
                GetObjValue(theObjInstance, CComBSTR("name"), theProcessor.Name);
                GetObjValue(theObjInstance, CComBSTR("Version"), theProcessor.Version);
                m_Processors.push_back(theProcessor);

                // Release the instance
                theObjInstance = NULL;
            }
        }
    }
}

//==============================================================================
/**
 *	DetermineVideoController:	Obtains information about the video controller.
 *
 *	@param	<IWbemServices>		inServices		Used to access WMI services
 *
 *	@return	NONE
 */
//==============================================================================
void CWin32Config::DetermineVideoController(IWbemServices *inServices)
{
    HRESULT theResult;
    CComPtr<IEnumWbemClassObject> theObjEnum;
    CComPtr<IWbemClassObject> theObjInstance;
    CComVariant theValue;
    CComBSTR theObjectName = "Win32_VideoController";
    unsigned long theNumObjects;

    if (::IsBadReadPtr(inServices, sizeof(IWbemServices)) == FALSE) {
        // Create an Instance Enumerator
        theResult =
            inServices->CreateInstanceEnum(theObjectName, WBEM_FLAG_DEEP, NULL, &theObjEnum);
        if (SUCCEEDED(theResult) && theObjEnum) {
            while (WBEM_S_NO_ERROR == (theResult = theObjEnum->Next(INFINITE, 1, &theObjInstance,
                                                                    &theNumObjects))) {
                // Obtain needed values
                GetObjValue(theObjInstance, CComBSTR("AdapterRAM"), m_VideoController.AdapterRAM);
                GetObjValue(theObjInstance, CComBSTR("CurrentBitsPerPixel"),
                            m_VideoController.CurrentBitsPerPixel);
                GetObjValue(theObjInstance, CComBSTR("CurrentHorizontalResolution"),
                            m_VideoController.CurrentHorizontalResolution);
                GetObjValue(theObjInstance, CComBSTR("CurrentRefreshRate"),
                            m_VideoController.CurrentRefreshRate);
                GetObjValue(theObjInstance, CComBSTR("CurrentVerticalResolution"),
                            m_VideoController.CurrentVerticalResolution);
                GetObjValue(theObjInstance, CComBSTR("DeviceID"), m_VideoController.DeviceID);
                GetObjValue(theObjInstance, CComBSTR("DriverDate"), m_VideoController.DriverDate);
                GetObjValue(theObjInstance, CComBSTR("DriverVersion"),
                            m_VideoController.DriverVersion);
                GetObjValue(theObjInstance, CComBSTR("InstalledDisplayDrivers"),
                            m_VideoController.InstalledDisplayDrivers);
                GetObjValue(theObjInstance, CComBSTR("MaxRefreshRate"),
                            m_VideoController.MaxRefreshRate);
                GetObjValue(theObjInstance, CComBSTR("MinRefreshRate"),
                            m_VideoController.MinRefreshRate);
                GetObjValue(theObjInstance, CComBSTR("name"), m_VideoController.Name);
                GetObjValue(theObjInstance, CComBSTR("VideoProcessor"),
                            m_VideoController.VideoProcessor);

                // Determine video guid
                GetVidGuid();

                // Release the instance
                theObjInstance = NULL;
            }
        }
    }
}

//==============================================================================
/**
 *	DetermineVideoController:	Obtains information about the video controller.
 *
 *	@param	<IWbemServices>		Not used yet...
 *
 *	@return	NONE
 */
//==============================================================================
void CWin32Config::DetermineApps(IWbemServices *)
{
    // Determine DirectX version
    GetDXVersion();

    // Obtain the Internet Explorer version
    GetIEVersion();
}

//==============================================================================
/**
 *	GetObjValue:	Obtains the value of a certain property from a WMI object
 *
 *	@param	<IWbemClassObject>		inObject		Instance of object
 *
 *	@param	<BSTR>					inPropName		Name of property
 *
 *	@param	<std::string&>			ioValue			Reference to value
 *
 *	@return	NONE
 */
//==============================================================================
void CWin32Config::GetObjValue(IWbemClassObject *inObject, BSTR inPropName, std::string &ioValue)
{
    if (::IsBadReadPtr(inObject, sizeof(IWbemClassObject)) == FALSE) {
        CComVariant theValue;
        HRESULT theResult;

        // Obtain property value
        theResult = inObject->Get(inPropName, 0, &theValue, NULL, NULL);
        if (SUCCEEDED(theResult)) {
            // Make sure the variant is valid
            if (theValue.vt != VT_NULL) {
                // Change type to BSTR and copy over
                USES_CONVERSION;
                theValue.ChangeType(VT_BSTR);

                if (theValue.vt == VT_BSTR) {
                    ioValue = OLE2A(theValue.bstrVal);
                }
            }
        }
    }
}

//==============================================================================
/**
 *	GetConfigName:	Sets the configuration name.
 *
 *	Queries WMI for the name of the computer and sets this to be the config name.
 *
 *	@param	<IWbemServices>		inServices		Used to access WMI services
 *
 *	@return	NONE
 */
//==============================================================================
void CWin32Config::GetConfigName(IWbemServices *inServices)
{
    HRESULT theResult;
    CComPtr<IEnumWbemClassObject> theObjEnum;
    CComPtr<IWbemClassObject> theObjInstance;
    CComVariant theValue;
    CComBSTR theObjectName = "Win32_ComputerSystem";
    unsigned long theNumObjects;

    if (::IsBadReadPtr(inServices, sizeof(IWbemServices)) == FALSE) {
        // Create an Instance Enumerator
        theResult =
            inServices->CreateInstanceEnum(theObjectName, WBEM_FLAG_DEEP, NULL, &theObjEnum);
        if (SUCCEEDED(theResult) && theObjEnum) {
            while (WBEM_S_NO_ERROR == (theResult = theObjEnum->Next(INFINITE, 1, &theObjInstance,
                                                                    &theNumObjects))) {
                // Obtain the name of this computer system
                GetObjValue(theObjInstance, CComBSTR("name"), m_ConfigName);

                // Release the instance
                theObjInstance = NULL;
            }
        }
    }
}

//==============================================================================
/**
 *		GetDXVersion:	Obtains the DirectX version.
 *
 *		@return	NONE
 */
//==============================================================================
void CWin32Config::GetDXVersion()
{
    CRegKey theRegistryKey;

    m_Applications.DXVersion.erase();

    // If there is an error opening the key
    if (ERROR_SUCCESS
        == theRegistryKey.Open(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectX"),
                               KEY_QUERY_VALUE)) {
        TCHAR theValueString[MAX_PATH];
        DWORD theStringSize = sizeof(theValueString);

// NOTE: TEMPORARY COMPILE FIX
// Are we in VC6 (1200) or VC7 (1300)?
#if _MSC_VER >= 1300
        if (ERROR_SUCCESS
            == theRegistryKey.QueryStringValue(_TEXT("Version"), theValueString, &theStringSize))
#else
        if (ERROR_SUCCESS
            == theRegistryKey.QueryValue(theValueString, _TEXT("Version"), &theStringSize))
#endif // _MSC_VER >= 1300
        {
            m_Applications.DXVersion = Q3DStudio::CString(theValueString).GetCharStar();
        }

        theRegistryKey.Close();
    }
}

//==============================================================================
/**
 *		GetIEVersion:	Obtains the Internet Explorer version.
 *
 *		@return	NONE
 */
//==============================================================================
void CWin32Config::GetIEVersion()
{
    CRegKey theRegistryKey;

    m_Applications.IEVersion.erase();

    // If there is an error opening the key
    if (ERROR_SUCCESS
        == theRegistryKey.Open(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\Internet Explorer"),
                               KEY_QUERY_VALUE)) {
        TCHAR theValueString[MAX_PATH];
        DWORD theStringSize = sizeof(theValueString);

// NOTE: TEMPORARY COMPILE FIX
// Are we in VC6 (1200) or VC7 (1300)?
#if _MSC_VER >= 1300
        if (ERROR_SUCCESS
            == theRegistryKey.QueryStringValue(_TEXT("Version"), theValueString, &theStringSize))
#else
        if (ERROR_SUCCESS
            == theRegistryKey.QueryValue(theValueString, _TEXT("Version"), &theStringSize))
#endif // _MSC_VER >= 1300
        {
            m_Applications.IEVersion = Q3DStudio::CString(theValueString).GetCharStar();
        }

        theRegistryKey.Close();
    }
}

//==============================================================================
/**
 *		GetVidGuid:	Obtains the Internet Explorer version.
 *
 *		@return	NONE
 */
//==============================================================================
void CWin32Config::GetVidGuid()
{
    // throw; // really?
    // typedef HRESULT (WINAPI * DIRECTDRAWCREATEEX) (GUID FAR*, LPVOID *, REFIID, IUnknown FAR *);

    // DIRECTDRAWCREATEEX		theDDrawProcAddr;
    // LPDIRECTDRAW7			theDDraw7Ptr;
    // HRESULT					theResult;
    // HINSTANCE				theDDrawDLL			= NULL;
    //

    //   // See if DDRAW.DLL even exists.
    //   theDDrawDLL = ::LoadLibrary( "DDRAW.DLL" );

    //// Get the address of the exported DLL function, DirectDrawCreateEx
    // theDDrawProcAddr = (DIRECTDRAWCREATEEX)::GetProcAddress( theDDrawDLL, "DirectDrawCreateEx" );

    //// Try calling the exported DLL function, DirectDrawCreateEx
    // theResult = (*theDDrawProcAddr) ( NULL, reinterpret_cast<void**>(&theDDraw7Ptr),
    // IID_IDirectDraw7, NULL );
    // if ( SUCCEEDED( theResult ) && theDDraw7Ptr )
    //{
    //	WCHAR					theDeviceGUID[ _MAX_PATH ];
    //	DDDEVICEIDENTIFIER2		theDevID;
    //	std::string				theDriverVersion;
    //
    //	// Get the video driver info
    //	theDDraw7Ptr->GetDeviceIdentifier( &theDevID,  DDGDI_GETHOSTIDENTIFIER );

    //	// Convert guid to string
    //	if ( ::StringFromGUID2( theDevID.guidDeviceIdentifier, theDeviceGUID, _MAX_PATH ) )
    //	{
    //		USES_CONVERSION;
    //		m_VideoController.DX7CapsKey = OLE2A( theDeviceGUID );

    //		// Grab the version of the driver
    //		if ( GetComponentVersion( theDevID.szDriver, theDriverVersion ) )
    //		{
    //			m_VideoController.DX7CapsKey += ".";
    //			m_VideoController.DX7CapsKey += theDriverVersion;
    //		}
    //	}
    //	else
    //	{
    //		m_VideoController.DX7CapsKey.erase();
    //	}
    //}

    //// DDraw7 was created successfully. We must be at least DX7.0
    // theDDraw7Ptr->Release();

    //// Release the DDRAW.DLL
    //::FreeLibrary( theDDrawDLL );
}

//==============================================================================
/**
 *		GetComponentVersion:  This method grabs the File Version given the path.
 *
 *		Given a path, this method will construct a version in the following
 *		format:  MajorVersion.MinorVersion.BuildNumber.PlatformID.
 *
 *		@param	inPath		The path of the file to retrieve version information on.
 *		@param	ioVersion	The version of the given file in the format explained
 *							above.
 *
 *		@return true if the version was determined, otherwise false
 */
//==============================================================================
bool CWin32Config::GetComponentVersion(LPTSTR inPath, std::string &ioVersion)
{
    DWORD theHandle;
    DWORD theFileVersionSize;

    // Get the file version information
    if ((theFileVersionSize = ::GetFileVersionInfoSize(inPath, &theHandle)) == 0) {
        ioVersion.erase();
    }

    else {
        // Create a buffer to hold the file version information
        TCHAR *theFileVersionBuffer = new TCHAR[theFileVersionSize];

        // Get the file version information
        if (::GetFileVersionInfo(inPath, theHandle, theFileVersionSize, theFileVersionBuffer)
            == 0) {
            // we failed getting the version information for some reason
            ioVersion.erase();
        }

        else {
            // Build a version structure
            VS_FIXEDFILEINFO *theFileVersionStructure;
            UINT theStructureSize;
            LPTSTR theSearchString = TEXT("\\\0");

            // Fille the version structure with the basic verison information
            if (::VerQueryValue(theFileVersionBuffer, theSearchString,
                                reinterpret_cast<void **>(&theFileVersionStructure),
                                &theStructureSize)
                == 0) {
                // we failed on something
                ioVersion.erase();
            }

            else {
                // Get the major file version
                WORD theMajorVer = HIWORD(theFileVersionStructure->dwFileVersionMS);
                TCHAR theMajorString[_MAX_PATH];
                ::_ltot(theMajorVer, theMajorString, 10);

                // Get the minor file version
                WORD theMinorVer = LOWORD(theFileVersionStructure->dwFileVersionMS);
                TCHAR theMinorString[_MAX_PATH];
                ::_ltot(theMinorVer, theMinorString, 10);

                // Get the build number
                WORD theBuildNum = HIWORD(theFileVersionStructure->dwFileVersionLS);
                TCHAR theBuildNumString[_MAX_PATH];
                ::_ltot(theBuildNum, theBuildNumString, 10);

                // Get the platform ID
                WORD thePlatformID = LOWORD(theFileVersionStructure->dwFileVersionLS);
                TCHAR thePlatformIDString[_MAX_PATH];
                ::_ltot(thePlatformID, thePlatformIDString, 10);

                // Construct a string to represent the version number
                Q3DStudio::CString theVersionString;
                theVersionString = theMajorString;
                theVersionString += _TEXT(".");
                theVersionString += theMinorString;
                theVersionString += _TEXT(".");
                theVersionString += theBuildNumString;
                theVersionString += _TEXT(".");
                theVersionString += thePlatformIDString;

                // We are golden.. copy the version out.
                ioVersion = theVersionString.GetCharStar();
                //_tcscpy( ioVersion, theVersionString );
            }
        }

        // Delete our newed memory
        delete[] theFileVersionBuffer;
    }

    return (!ioVersion.empty());
}
#endif

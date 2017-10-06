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
#include <atlbase.h>
#include <objbase.h>
#endif

#ifndef INCLUDED_SYSTEM_INFO_H
#include "SystemInfo.h"
#endif

// const GUID IID_IDirectDraw7 = { 0x15e65ec0, 0x3b9c, 0x11d2, { 0xb9, 0x2f, 0x00, 0x60, 0x97, 0x97,
// 0xea, 0x5b } };

extern const GUID IID_IDirectDraw7;
//     static const GUID GUID_SCENE2 = { 0xFFFFFFFF, 0xFFFF, 0xFFFF, { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
//     0xFF, 0xFF, 0xFF } };

//==============================================================================
/**
 *	Constructor:
 */
//==============================================================================
CSystemInfo::CSystemInfo()
{
    m_TabCount = 0;
}

//==============================================================================
/**
 *	Descructor: Destroys the object
 */
//==============================================================================
CSystemInfo::~CSystemInfo()
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
void CSystemInfo::DetermineConfig()
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
            theResult = theLocator->ConnectServer(theResource, NULL, NULL, NULL, 0, NULL, NULL,
                                                  &theServices);
            if (theResult == WBEM_S_NO_ERROR && theServices) {
                // Switch the security level to IMPERSONATE so that provider(s)
                // will grant access to system-level objects, and so that
                // CALL authorization will be used.
                ::CoSetProxyBlanket(theServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                                    RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                    EOAC_NONE);

                // Get the configuration name
                this->GetConfigName(theServices);

                // Get OS info
                this->DetermineOS(theServices);

                // Get processor info
                this->DetermineProcessor(theServices);

                // Get video controller info
                // this->DetermineVideoController( theServices );

                // Get application info
                this->DetermineApps(theServices);
            }
        }
    }
#endif
}

//==============================================================================
/**
 *	SetTabCount:	Sets the tab count for printing in XML format.
 *
 *	@param	<unsigned long>		inTabCount		The tab count
 *
 *	@return	NONE
 */
//==============================================================================
void CSystemInfo::SetTabCount(unsigned long inTabCount)
{
    m_TabCount = inTabCount;
}

//==============================================================================
/**
 *	GetProcessorString:	Formats the processor string. i.e. If more than 1 processor
 *						is present, it is separated by a ,
 *
 *	@return	the formatted string
 */
//==============================================================================
std::string CSystemInfo::GetProcessorString()
{
    std::string theReturn;

    unsigned long theI;

    // Processor information
    for (theI = 0; theI < m_Processors.size(); theI++) {
        if (theI != 0)
            theReturn += ", ";

        theReturn += m_Processors[theI].Name + " " + m_Processors[theI].CurrentClockSpeed + "Mhz";
    }

    theReturn.erase(0, theReturn.find_first_not_of(" \n\r\t"));
    theReturn.erase(theReturn.find_last_not_of(" \n\r\t") + 1);

    return theReturn;
}

//==============================================================================
/**
 *	operator <<:  ostream operator used for printing out in XML format.
 *
 *	@param	<std::ostream>	inStream		The stream
 *
 *	@param	<CSystemInfo*>	inConfig		Pointer to a CSystemInfo class.
 *
 *	@return	<std::ostream&>					Returns the modified stream
 */
//==============================================================================
std::ostream &operator<<(std::ostream &inStream, CSystemInfo *inConfig)
{
#ifdef KDAB_TEMPORARILY_REMOVED
    unsigned long theTabCounter;

    if (::IsBadReadPtr(inConfig, sizeof(CSystemInfo)) == FALSE) {
// Helper macros for writing XML to stream

// Indent
#define XML_INDENT                                                                                 \
    for (theTabCounter = 0; theTabCounter < inConfig->m_TabCount; ++theTabCounter) {               \
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

        XML_INDENT;
        XML_BEGIN_TAG("SystemInfo");
        XML_ATTRIBUTE("MachineName", inConfig->GetMachineName().c_str());
        XML_ATTRIBUTE("OS", inConfig->m_OS.Name.c_str());
        std::string theProcessorString = inConfig->GetProcessorString();
        XML_ATTRIBUTE("Processor", theProcessorString.c_str());
        XML_ATTRIBUTE("Video", inConfig->m_VideoController.VideoProcessor.c_str());
        XML_ATTRIBUTE("VideoDriverVersion", inConfig->m_VideoController.DriverVersion.c_str());
        XML_ATTRIBUTE("DX", inConfig->m_Applications.DXVersion.c_str());
        XML_CLOSE_TAG;
        XML_END_TAG("SystemInfo");
        XML_NEW_LINE;
    }

    /*
    unsigned long	theTabCounter;

    if ( ::IsBadReadPtr( inConfig, sizeof (CSystemInfo) ) == FALSE )
    {
            // Helper macros for writing XML to stream

            // Indent
            #define XML_INDENT	\
                    for ( theTabCounter = 0; theTabCounter < inConfig->m_TabCount; ++theTabCounter )
    \
                    {
    \
                            inStream << "\t";
    \
                    }

            // Newline
            #define XML_NEW_LINE						( inStream << std::endl
    )
            // Beginning tag
            #define	XML_BEGIN_TAG(inTagName)			( inStream << "<"	<< inTagName
    )
            // Name and value
            #define	XML_ATTRIBUTE(inTagName,inValue)	( inStream << " "	<< inTagName	<<
    "=\""	<< inValue	<< "\"" )
            // End tag
            #define XML_END_TAG(inTagName)				( inStream << "</"	<< inTagName	<<
    ">" )
            // Close tag
            #define XML_CLOSE_TAG						( inStream << ">" )


            // Begin config tab
            XML_NEW_LINE;
            XML_BEGIN_TAG( "Config2" );
            XML_ATTRIBUTE( "name", inConfig->m_ConfigName.c_str() );
            XML_ATTRIBUTE( "VisibleMemory", inConfig->m_OS.TotalVisibleMemorySize.c_str() );
            XML_CLOSE_TAG;
            XML_NEW_LINE;
            XML_INDENT;

            // OS information
            XML_BEGIN_TAG( "OS2" );
            XML_ATTRIBUTE( "CountryCode", inConfig->m_OS.CountryCode.c_str() );
            XML_ATTRIBUTE( "Locale", inConfig->m_OS.Locale.c_str() );
            XML_ATTRIBUTE( "name", inConfig->m_OS.Name.c_str() );
            XML_ATTRIBUTE( "ServicePacks", inConfig->m_OS.ServicePacks.c_str() );
            XML_ATTRIBUTE( "Version", inConfig->m_OS.Version.c_str() );
            XML_CLOSE_TAG;
            XML_END_TAG( "OS2" );
            XML_NEW_LINE;

            XML_INDENT;


            char theNumProcs[32];
            itoa( inConfig->m_Processors.size(), theNumProcs, 32 );
            XML_BEGIN_TAG( "ProcessorConfig" );
            XML_ATTRIBUTE( "NumProcs", theNumProcs );
            XML_CLOSE_TAG;
            XML_NEW_LINE;
            XML_INDENT;
            unsigned long theI;
            // Processor information
            for( theI = 0; theI < inConfig->m_Processors.size(); theI++ )
            {
                    XML_BEGIN_TAG( "Processor2" );
                    XML_ATTRIBUTE( "CurrentClockSpeed",
    inConfig->m_Processors[theI].CurrentClockSpeed.c_str() );
                    XML_ATTRIBUTE( "Manufacturer", inConfig->m_Processors[theI].Manufacturer.c_str()
    );
                    XML_ATTRIBUTE( "MaxClockSpeed",
    inConfig->m_Processors[theI].MaxClockSpeed.c_str() );
                    XML_ATTRIBUTE( "name", inConfig->m_Processors[theI].Name.c_str() );
                    XML_ATTRIBUTE( "Version", inConfig->m_Processors[theI].Version.c_str() );
                    XML_CLOSE_TAG;
                    XML_END_TAG( "Processor2" );
                    XML_NEW_LINE;

                    XML_INDENT;
            }
            XML_END_TAG( "ProcessorConfig" );
            XML_NEW_LINE;
            XML_INDENT;

            // Video Controller information
            XML_BEGIN_TAG( "VideoConfig" );
            XML_CLOSE_TAG;
            XML_NEW_LINE;
            XML_INDENT;
            XML_BEGIN_TAG( "DisplayConfig" );

            XML_ATTRIBUTE( "ColorDepth", inConfig->m_VideoController.CurrentBitsPerPixel.c_str() );
            XML_ATTRIBUTE( "HResolution",
    inConfig->m_VideoController.CurrentHorizontalResolution.c_str() );
            XML_ATTRIBUTE( "RefreshRate", inConfig->m_VideoController.CurrentRefreshRate.c_str() );
            XML_ATTRIBUTE( "VResolution",
    inConfig->m_VideoController.CurrentVerticalResolution.c_str() );
            XML_CLOSE_TAG;
            XML_END_TAG( "DisplayConfig" );
            XML_NEW_LINE;
            XML_BEGIN_TAG( "VideoAdapter" );
            XML_ATTRIBUTE( "AdapterRAM", inConfig->m_VideoController.AdapterRAM.c_str() );
            XML_ATTRIBUTE( "MaxRefreshRate", inConfig->m_VideoController.MaxRefreshRate.c_str() );
            XML_ATTRIBUTE( "MinRefreshRate", inConfig->m_VideoController.MinRefreshRate.c_str() );
            XML_CLOSE_TAG;
            XML_BEGIN_TAG( "VideoChipsets" );
            XML_ATTRIBUTE( "DriverVersion", inConfig->m_VideoController.DriverVersion.c_str() );
            XML_ATTRIBUTE( "VideoProcessor", inConfig->m_VideoController.VideoProcessor.c_str() );
            XML_ATTRIBUTE( "DX7CapsKey", inConfig->m_VideoController.DX7CapsKey.c_str() );
            XML_CLOSE_TAG;
            XML_END_TAG( "VideoChipsets" );
            XML_NEW_LINE;
            XML_END_TAG( "VideoAdapter" );
            XML_NEW_LINE;

            XML_END_TAG( "VideoConfig" );
            XML_NEW_LINE;

            XML_INDENT;

            // Applications information
            XML_BEGIN_TAG( "Applications" );
            XML_ATTRIBUTE( "DXVersion", inConfig->m_Applications.DXVersion.c_str() );
            XML_ATTRIBUTE( "IEVersion", inConfig->m_Applications.IEVersion.c_str() );
            XML_CLOSE_TAG;
            XML_END_TAG( "Applications" );

            // End config tag
            XML_END_TAG( "Config2" );
            XML_NEW_LINE;

            // Begin config tab
            XML_BEGIN_TAG( "Renderer" );
            XML_ATTRIBUTE( "RendererID", inConfig->m_Renderer.c_str() );
            XML_CLOSE_TAG;
            XML_END_TAG( "Renderer" );
            XML_NEW_LINE;
    }
    */
#endif
    return inStream;
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
void CSystemInfo::DetermineOS(IWbemServices *inServices)
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
                this->GetObjValue(theObjInstance, CComBSTR("Caption"), m_OS.Name);
                // this->GetObjValue( theObjInstance, CComBSTR( "name" ), m_OS.Name );
                this->GetObjValue(theObjInstance, CComBSTR("CountryCode"), m_OS.CountryCode);
                this->GetObjValue(theObjInstance, CComBSTR("Locale"), m_OS.Locale);
                this->GetObjValue(theObjInstance, CComBSTR("Version"), m_OS.Version);
                this->GetObjValue(theObjInstance, CComBSTR("TotalVisibleMemorySize"),
                                  m_OS.TotalVisibleMemorySize);

                // Obtain service pack major and minor version and put into string format
                this->GetObjValue(theObjInstance, CComBSTR("ServicePackMajorVersion"), theSPMajor);
                this->GetObjValue(theObjInstance, CComBSTR("ServicePackMinorVersion"), theSPMinor);
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
void CSystemInfo::DetermineProcessor(IWbemServices *inServices)
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
                this->GetObjValue(theObjInstance, CComBSTR("CurrentClockSpeed"),
                                  theProcessor.CurrentClockSpeed);
                this->GetObjValue(theObjInstance, CComBSTR("DeviceID"), theProcessor.DeviceID);
                this->GetObjValue(theObjInstance, CComBSTR("Manufacturer"),
                                  theProcessor.Manufacturer);
                this->GetObjValue(theObjInstance, CComBSTR("MaxClockSpeed"),
                                  theProcessor.MaxClockSpeed);
                this->GetObjValue(theObjInstance, CComBSTR("name"), theProcessor.Name);
                this->GetObjValue(theObjInstance, CComBSTR("Version"), theProcessor.Version);
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
void CSystemInfo::DetermineVideoController(IWbemServices *inServices)
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
                this->GetObjValue(theObjInstance, CComBSTR("AdapterRAM"),
                                  m_VideoController.AdapterRAM);
                this->GetObjValue(theObjInstance, CComBSTR("CurrentBitsPerPixel"),
                                  m_VideoController.CurrentBitsPerPixel);
                this->GetObjValue(theObjInstance, CComBSTR("CurrentHorizontalResolution"),
                                  m_VideoController.CurrentHorizontalResolution);
                this->GetObjValue(theObjInstance, CComBSTR("CurrentRefreshRate"),
                                  m_VideoController.CurrentRefreshRate);
                this->GetObjValue(theObjInstance, CComBSTR("CurrentVerticalResolution"),
                                  m_VideoController.CurrentVerticalResolution);
                this->GetObjValue(theObjInstance, CComBSTR("DeviceID"), m_VideoController.DeviceID);
                this->GetObjValue(theObjInstance, CComBSTR("DriverDate"),
                                  m_VideoController.DriverDate);
                this->GetObjValue(theObjInstance, CComBSTR("DriverVersion"),
                                  m_VideoController.DriverVersion);
                this->GetObjValue(theObjInstance, CComBSTR("InstalledDisplayDrivers"),
                                  m_VideoController.InstalledDisplayDrivers);
                this->GetObjValue(theObjInstance, CComBSTR("MaxRefreshRate"),
                                  m_VideoController.MaxRefreshRate);
                this->GetObjValue(theObjInstance, CComBSTR("MinRefreshRate"),
                                  m_VideoController.MinRefreshRate);
                this->GetObjValue(theObjInstance, CComBSTR("name"), m_VideoController.Name);
                this->GetObjValue(theObjInstance, CComBSTR("VideoProcessor"),
                                  m_VideoController.VideoProcessor);

                // Determine video guid
                this->GetVidGuid();

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
void CSystemInfo::DetermineApps(IWbemServices *)
{
    // Determine DirectX version
    this->GetDXVersion();

    // Obtain the Internet Explorer version
    this->GetIEVersion();
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
void CSystemInfo::GetObjValue(IWbemClassObject *inObject, BSTR inPropName, std::string &ioValue)
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
void CSystemInfo::GetConfigName(IWbemServices *inServices)
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
                this->GetObjValue(theObjInstance, CComBSTR("name"), m_ConfigName);
                this->GetObjValue(theObjInstance, CComBSTR("UserName"), m_UserName);

                std::string::size_type thePos = m_UserName.rfind("\\");
                m_UserName.erase(0, thePos + 1);

                // Release the instance
                theObjInstance = NULL;
            }
        }
    }
}
#endif

std::string CSystemInfo::GetMachineName()
{
    return m_ConfigName;
}

std::string CSystemInfo::GetUserName()
{
    return m_UserName;
}

#ifdef KDAB_TEMPORARILY_REMOVED
//==============================================================================
/**
 *		GetDXVersion:	Obtains the DirectX version.
 *
 *		@return	NONE
 */
//==============================================================================
void CSystemInfo::GetDXVersion()
{
    CRegKey theRegistryKey;

    m_Applications.DXVersion.erase();

    // If there is an error opening the key
    if (ERROR_SUCCESS
        == theRegistryKey.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\DirectX",
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
void CSystemInfo::GetIEVersion()
{
    CRegKey theRegistryKey;

    m_Applications.IEVersion.erase();

    // If there is an error opening the key
    if (ERROR_SUCCESS
        == theRegistryKey.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Internet Explorer",
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
void CSystemInfo::GetVidGuid()
{
    throw; // draw
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
bool CSystemInfo::GetComponentVersion(LPTSTR inPath, std::string &ioVersion)
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

// Get renderer info
void CSystemInfo::DetermineRenderer(std::string inrenderer)
{
    m_Renderer = inrenderer;
}

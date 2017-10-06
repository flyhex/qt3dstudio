/****************************************************************************
**
** Copyright (C) 1999-2001 NVIDIA Corporation.
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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "UICString.h"
#include "UICExceptions.h"
#include "UICExceptionClass.h"

bool CUICExceptionClass::s_HandlingTypes = true;

//==============================================================================
/**
 *	Constructs a CUICExceptionClass object.
 *
 *	This method creates a CUICExceptionClass object.
 *
 *	@param	<long>			inlong			The long to check for failure
 *	@param	<const char*>	inFileName		The name of the file of the calling function
 *	@param	<unsigned long>	inLineNumber	The line number of the calling function
 *	@param	<long>			inErrorCode		The error code to handle if the
 *failure occurs.
 *	@param	<unsigned long>	inType			The type of error
 *	@param	<va_list>		inParameters	A pointer to the optional arugments for
 *string formatting purposes
 */
CUICExceptionClass::CUICExceptionClass(const wchar_t *inFileName, const unsigned long inLineNumber,
                                       const long inErrorCode, const long inlong,
                                       va_list inParameters)
#ifdef KDAB_TEMPORARILY_REMOVED
    : m_ParameterList(inParameters)
    , m_LineNumber(inLineNumber)
    , m_long(inlong)
    , m_ErrorCode(inErrorCode)
    , m_Type(0)
#endif
{
    // Clear strings
    *m_Location = 0;
    *m_Description = 0;
    *m_StackTrace = 0;

#ifdef KDAB_TEMPORARILY_REMOVED
    // Copy the filename
    _aswprintf(m_FileName, _arraysize(m_FileName) - 1, L"%ls", inFileName);

    // TODO get stacktrace somehow

    // Format the error string and handle the exception
    FormatDescription();
    HandleException();
#endif
}

//==============================================================================
/**
 *	Handles the exception
 */
void CUICExceptionClass::HandleException()
{
    // Check registry for changes
    CUICExceptionClass::Initialize();

    // Are we handling this type of exception?
    //
    // SDJ 9/19/01: m_Type does not appear to be initialized anywhere, so I removed this test
    //
    // if ( s_HandlingTypes & m_Type )

    if (s_HandlingTypes) {
        // Display the exception in the debugger output window?
        Output2Debugger();

        // Display the exception in a message box?
        BreakInDebugger();

        // Log the exception?
        // Log2File();

        // Display the exception in a message box?
        DisplayMessageBox();
    }
}

//==============================================================================
/**
 *	Displays the exception description in a windows message box.
 *
 *	@return bool	TRUE	the description was formatted correctly.
 */
bool CUICExceptionClass::FormatDescription()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    bool theResult = false;
    //	wchar_t	theErrorString[MAX_ENTRY_LENGTH] = { 0 };
    wchar_t theDescription[MAX_ENTRY_LENGTH] = { 0 };

    // Format location and clear description
    _aswprintf(m_Location, _arraysize(m_Location) - 1, L"%ls(%ld)", m_FileName, m_LineNumber);
    m_Description[0] = 0;

    // Is it our ErrorCode?
    if (HRESULT_FACILITY(m_ErrorCode) == FACILITY_ITF) {
        // Tack on a failing long?
        if (FAILED(m_long)) {
// Look up the error code in the string table
// GetStringFromID( long_CODE( m_ErrorCode ), theErrorString );
// theResult = _avswprintf( theDescription, MAX_ENTRY_LENGTH - 1, theErrorString, m_ParameterList )
// > 0;

#ifdef WIN32

            // Add the long at the end
            if (theResult) {
                LPTSTR theMsgBuffer;
                FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                              m_long, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPTSTR)&theMsgBuffer, 0, NULL);

                // Strip off any CR/LF
                if (theMsgBuffer != NULL) {
                    size_t nLen = ::wcslen(theMsgBuffer);
                    if (nLen > 1 && theMsgBuffer[nLen - 1] == '\n') {
                        theMsgBuffer[nLen - 1] = 0;
                        if (theMsgBuffer[nLen - 2] == '\r') {
                            theMsgBuffer[nLen - 2] = 0;
                        }
                    }
                }

                // else NULL buffer
                else {
                    const int MSG_MAXSIZE = 32;
                    theMsgBuffer = (LPTSTR)LocalAlloc(0, MSG_MAXSIZE * sizeof(wchar_t));
                    if (theMsgBuffer != NULL) {
                        _snwprintf(theMsgBuffer, MSG_MAXSIZE, TEXT("0x%0lX"), m_long);
                    }
                }

                theResult = _aswprintf(m_Description, _arraysize(m_Description) - 1, L"%ls (%ls)",
                                       theDescription, theMsgBuffer)
                    > 0;

                LocalFree(theMsgBuffer);
            }

#else
// NOT IMPLEMENTED ON THE MAC
#endif
        } else {
            // Look up the error code in the string table
            // GetStringFromID( long_CODE( m_ErrorCode ), theErrorString );
            // theResult = _avswprintf( m_Description, _arraysize(m_Description) - 1,
            // theErrorString, m_ParameterList ) > 0;
        }
    }

    return theResult;
#endif
    return {};
}

//==============================================================================
/**
 *	Breaks immediately in the debugger.
 */
void CUICExceptionClass::BreakInDebugger()
{
#ifdef _DEBUG
#ifdef WIN32
//		::DebugBreak( );
#else
#endif
#endif
}

//==============================================================================
/**
 *	Displays the exception description in a windows message box.
 */
void CUICExceptionClass::DisplayMessageBox()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    // Display the exception in a message box?
    wchar_t theBufferString[MAX_ENTRY_LENGTH];
    long theErrorCode = HRESULT_CODE(m_ErrorCode);

    _aswprintf(theBufferString, MAX_ENTRY_LENGTH - 1,
               L"A program exception was caught.\n%s\nFile: %s\nCode: 0x%lX\n\nAbort quits, Retry "
               L"launches debugger and Ignore continues program execution.\n",
               m_Description, m_Location, theErrorCode);
    ::wcsncat(theBufferString, L"\r\n\r\n", MAX_ENTRY_LENGTH - 1);

#ifdef _DEBUG
    long thePressedButton;

    // Display the dialog
    thePressedButton = ::MessageBoxW(NULL, theBufferString, L"Exception",
                                     MB_ABORTRETRYIGNORE | MB_ICONERROR | MB_DEFBUTTON3);
    switch (thePressedButton) {
    // Exit the application?
    case IDABORT:
        ::exit(1);
        break;

    // Launch the IDE debugger?
    case IDRETRY:
        ::DebugBreak();
        break;

    // If the button pressed is ignore, then just continue
    default:
        break;
    }

#endif // _DEBUG
#endif
}

//==============================================================================
/**
 *	Output the exception description to the development environment debug window.
 */
void CUICExceptionClass::Output2Debugger()
{
#ifdef KDAB_TEMPORARILY_REMOVED
#ifdef _DEBUG
    wchar_t theBufferString[MAX_ENTRY_LENGTH];
    long theErrorCode = HRESULT_CODE(m_ErrorCode);

    // Format and output
    _aswprintf(theBufferString, MAX_ENTRY_LENGTH - 1, L"%ls - %ls - 0x%lX\n", m_Location,
               m_Description, theErrorCode);
#endif

#ifdef WIN32
    ::OutputDebugStringW(theBufferString);
#else
// NOT IMPLEMENTED ON THE MAC
#endif
#endif
}

//==============================================================================
/**
 *	Initialize all static members from preferences in registry.
 *
 *	Method opens up the key at HKEY_CURRENT_USER/m_RegistryPreferencePath.
 *	The three DWORD values looked up are: TrackErrorTypes, ReportingLevel and ErrorAction
 */
void CUICExceptionClass::Initialize()
{
    /*
    #ifdef WIN32
            long	theReturn;
            HKEY	theKey			= NULL;
            DWORD	theValueType;
            DWORD	theDataSize;

            // Open Key
            theReturn = ::RegOpenKeyEx( HKEY_CURRENT_USER, UIC_CLIENT_REGISTRY_KEY, 0, KEY_READ,
    &theKey );

            if ( ERROR_SUCCESS == theReturn && theKey )
            {
                    // Query the registry for the TrackErrorTypes value
                    theValueType = REG_DWORD;
                    theDataSize = sizeof(DWORD);
                    theReturn = ::RegQueryValueEx( theKey, UIC_CLIENT_TRACKERRORTYPES_VALUE, NULL,
    &theValueType, reinterpret_cast<BYTE*>(&s_HandlingTypes), &theDataSize );

                    // Query the registry for the ReportingLevel value
                    theValueType = REG_DWORD;
                    theDataSize = sizeof(DWORD);
                    theReturn = ::RegQueryValueEx( theKey, UIC_CLIENT_REPORTINGLEVEL_VALUE, NULL,
    &theValueType,  reinterpret_cast<BYTE*>(&s_ReportingLevel), &theDataSize );

                    // Query the registry for the ErrorAction value
                    theValueType = REG_DWORD;
                    theDataSize = sizeof(DWORD);
                    theReturn = ::RegQueryValueEx( theKey, UIC_CLIENT_ERRORACTION_VALUE, NULL,
    &theValueType,  reinterpret_cast<BYTE*>(&s_Actions), &theDataSize );

                    // Close reg file
                    theReturn =  ::RegCloseKey( theKey );
            }
    #else
            // NOT IMPLEMENTED ON THE MAC
    #endif
    */
}

//==============================================================================
/**
 *	Throws an exception
 *
 *	This static method throws an exception.
 *	It may also just create an instance of the exception class if the
 *	the program is currently running in God Mode.
 *
 *	@param	<long>		inlong		The long to check for failure
 *	@param	<const wchar_t*>	inFileName		The name of the file of the calling
 *function
 *	@param	<unsigned long>	inLineNumber	The line number of the calling function
 *	@param	<long>		inErrorCode		The error code to handle if the failure
 *occurs.
 *	@param	<unsigned long>	inType			The type of error
 *	@param	<...>							Optional arugments for string
 *formatting purposes
 */
void CUICExceptionClass::Throw(const wchar_t *inFileName, const unsigned long inLineNumber,
                               const long inErrorCode, ...)
{
    // Define a parameter list
    va_list theParameters;

    // Initalize the list to the parameter before the optional ones start
    va_start(theParameters, inErrorCode);

#ifdef KDAB_TEMPORARILY_REMOVED
    // Throw an exception
    throw CUICExceptionClass(inFileName, inLineNumber, inErrorCode, S_FALSE, theParameters);
#endif
}

//==============================================================================
/**
 *	Throws an exception if the inlong fails.
 *
 *	This static method throws an exception if the inlong is not S_OK.
 *	It may also just create an instance of the exception class if the
 *	the program is currently running in God Mode.
 *
 *	@param	<long>				inlong			The long to check for
 *failure
 *	@param	<const wchar_t*>	inFileName		The name of the file of the calling
 *function
 *	@param	<unsigned long>		inLineNumber	The line number of the calling function
 *	@param	<long>				inErrorCode		The error code to handle if
 *the failure occurs.
 *	@param	<unsigned long>		inType			The type of error
 *	@param	<...>								Optional arugments for
 *string formatting purposes
 */
void CUICExceptionClass::ThrowFail(const long inlong, const wchar_t *inFileName,
                                   const unsigned long inLineNumber, const long inErrorCode, ...)
{
    // Define a parameter list
    va_list theParameters;

    // Initalize the list to the parameter before the optional ones start
    va_start(theParameters, inErrorCode);

    // If the the result is not good
#ifdef KDAB_TEMPORARILY_REMOVED
    if (FAILED(inlong)) {
        // Throw an exception
        throw CUICExceptionClass(inFileName, inLineNumber, inErrorCode, inlong, theParameters);
    }
#endif
}

//==============================================================================
/**
 *	Throws an exception if the inCondition is FALSE
 *
 *	This static method throws an exception if the inCondition is FALSE.
 *	It may also just create an instance of the exception class if the
 *	the program is currently running in God Mode.
 *
 *	@param	<long>		inlong		The long to check for falsity :)
 *	@param	<const wchar_t*>	inFileName		The name of the file of the calling
 *function
 *	@param	<unsigned long>	inLineNumber	The line number of the calling function
 *	@param	<long>		inErrorCode		The error code to handle if the failure
 *occurs.
 *	@param	<unsigned long>	inType			The type of error
 *	@param	<...>							Optional arugments for string
 *formatting purposes
 */
void CUICExceptionClass::ThrowFalse(bool inCondition, const wchar_t *inFileName,
                                    const unsigned long inLineNumber, const long inErrorCode, ...)
{
    // Define a parameter list
    va_list theParameters;

    // Initalize the list to the parameter before the optional ones start
    va_start(theParameters, inErrorCode);

    // If the condition fails
    if (!inCondition) {
        // Throw an exception
#ifdef KDAB_TEMPORARILY_REMOVED
        throw CUICExceptionClass(inFileName, inLineNumber, inErrorCode, S_FALSE, theParameters);
#endif
    }
}

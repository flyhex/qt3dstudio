/****************************************************************************
**
** Copyright (C) 2005 NVIDIA Corporation.
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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "StudioProjectVariables.h"
#include "Core.h"
#include "Dispatch.h"

//==============================================================================
/**
 *	Constructor
 */
//==============================================================================
CStudioProjectVariables::CStudioProjectVariables(CCore *inCore)
    : m_Core(inCore)
{
}

//==============================================================================
/**
 *	Destructor: Releases the object.
 */
//==============================================================================
CStudioProjectVariables::~CStudioProjectVariables()
{
}

//==============================================================================
/**
 *	Cleanup all the existing environment variables. It's the responsibility of the
 *	listen for the change event to remove themselves. Calling Clear will not remove them
 */
//==============================================================================
void CStudioProjectVariables::Clear()
{
    m_VariablesMap.clear();
}

//==============================================================================
/**
 *	Forms all the environment into a CString. Each enviroment variable is seperated
 *	by a carriage return and a line feed. This is the format expected by the Project
 *	Settings UI
 */
//==============================================================================
Q3DStudio::CString CStudioProjectVariables::GetProjectVariables() const
{
    // Formulate the string
    Q3DStudio::CString theString;
    TVariableMap::const_iterator theBegin = m_VariablesMap.begin();
    TVariableMap::const_iterator theEnd = m_VariablesMap.end();
    for (TVariableMap::const_iterator theIterator = theBegin; theIterator != theEnd;
         ++theIterator) {
        if (theIterator != theBegin)
            theString += "\r\n";

        theString += theIterator->first;
        theString += " = ";
        theString += theIterator->second;
    }

    return theString;
}

void CStudioProjectVariables::GetProjectVariables(std::list<Q3DStudio::CString> &ioVariables)
{
    Q3DStudio::CString theString;
    TVariableMap::const_iterator theBegin = m_VariablesMap.begin();
    TVariableMap::const_iterator theEnd = m_VariablesMap.end();
    for (TVariableMap::const_iterator theIterator = theBegin; theIterator != theEnd;
         ++theIterator) {
        theString = theIterator->first;
        theString += " = ";
        theString += theIterator->second;
        ioVariables.push_back(theString);
    }
}

//==============================================================================
/**
 *	Reset all the enviroment variables with this new set. Each entry in the list
 *	is one variable in the format {Variable} = Value.
 */
//==============================================================================
bool CStudioProjectVariables::SetProjectVariables(std::list<Q3DStudio::CString> &inVariables)
{
    // copy the environment variables out
    TVariableMap theOrigMap = m_VariablesMap;

    // prepare m_VariablesMap for new settings
    m_VariablesMap.clear();

    bool theChangeFlag(false);
    bool theVariableError(false);
    Q3DStudio::CString theErrorMessage;
    std::list<Q3DStudio::CString>::iterator theEnd = inVariables.end();
    std::list<Q3DStudio::CString>::iterator theIterator = inVariables.begin();
    for (; theIterator != theEnd; ++theIterator) {
        // Decode the string into key and value
        long theIndex = theIterator->Find('=');
        if (theIndex != Q3DStudio::CString::ENDOFSTRING) {
            Q3DStudio::CString theKey = theIterator->Left(theIndex);
            theKey.TrimLeft();
            theKey.TrimRight();
            Q3DStudio::CString theValue = theIterator->Extract(theIndex + 1);
            theValue.TrimLeft();
            theValue.TrimRight();

            // Strip the key of '{' and '}' if any and store key only as uppercase
            long theBeginIndex = theKey.Find('{');
            if (theBeginIndex != Q3DStudio::CString::ENDOFSTRING) {
                long theEndIndex = theKey.Find('}');
                if (theEndIndex != Q3DStudio::CString::ENDOFSTRING) {
                    theKey = theKey.Extract(theBeginIndex + 1, theEndIndex - theBeginIndex - 1);
                }
            }
            theKey.ToUpper();

            // find whether it exists
            TVariableMap::iterator theMapIterator = theOrigMap.find(theKey);
            if (theMapIterator == theOrigMap.end()) {
                theChangeFlag = true;
            } else {
                if (theMapIterator->second != theValue) {
                    theChangeFlag = true;
                }
                theOrigMap.erase(theMapIterator);
            }
            m_VariablesMap.insert(std::make_pair(theKey, theValue));
        } else {
            Q3DStudio::CString theString = *theIterator;
            theString.TrimLeft();
            if (!theString.IsEmpty()) {
                // Error encountered. Form the error message to be displayed
                if (!theVariableError) {
                    theVariableError = true;
                } else
                    theErrorMessage += "\n";
                // Format error in this parameters
                theErrorMessage += "\t";
                theErrorMessage += *theIterator;
            }
        }
    }

    if (theVariableError) {
        m_Core->GetDispatch()->FireOnProjectVariableFail(theErrorMessage);
    }

    if (theChangeFlag || 0 != theOrigMap.size()) // something was deleted
        FireChangeEvent();

    return theChangeFlag;
}

//==============================================================================
/**
 *	Formulate a new string by parsing the input string and replace every instance
 *	of environment variable with it's corresponding value. If the environment is not
 *	found, it is replace with an empty string. The environement variable is identified
 *	by '{' and '}'
 */
//==============================================================================
Q3DStudio::CString CStudioProjectVariables::ResolveString(const Q3DStudio::CString &inString)
{
    Q3DStudio::CString theReturnString;
    long theStart = 0; // start index of string
    long theBeginIndex = 0; // index of '{'
    long theEndIndex = 0; // index of '}'
    while (Q3DStudio::CString::ENDOFSTRING != theEndIndex) {
        theBeginIndex = inString.Find('{', theStart);
        if (Q3DStudio::CString::ENDOFSTRING != theBeginIndex) {
            theReturnString += inString.Extract(theStart, theBeginIndex - theStart);
            // find the corresponding '}'
            theEndIndex = inString.Find('}', theBeginIndex + 1);
            if (Q3DStudio::CString::ENDOFSTRING != theEndIndex) {
                // resolve the string
                Q3DStudio::CString theVariable =
                    inString.Extract(theBeginIndex + 1, theEndIndex - theBeginIndex - 1);
                theVariable.ToUpper();
                TVariableMap::const_iterator theMapIter = m_VariablesMap.find(theVariable);

                if (m_VariablesMap.end() != theMapIter) {
                    theReturnString += theMapIter->second;
                }
                theStart = theEndIndex + 1;
            } else
                theReturnString += inString.Extract(theBeginIndex);
        } else {
            theEndIndex = theBeginIndex;
            theReturnString += inString.Extract(theStart);
        }
    }

    return theReturnString;
}

//==============================================================================
/**
 *	Resolves the passed in variable and write out the resolved value if it exists.
 *	@param inVariable	the environment to be resolved
 *	@param outValue		the string to receive the resolved value
 *	@return true if the variable exists, else false
 */
//==============================================================================
bool CStudioProjectVariables::ResolveVariable(const Q3DStudio::CString &inVariable,
                                              Q3DStudio::CString &outValue,
                                              bool inCaseInsensitive /* = true*/)
{
    // environment variable is wrapped by '{' and '}'
    // so form the inVariable in this syntax before searching
    // Q3DStudio::CString theVariable = "{" + inVariable + "}";
    TVariableMap::const_iterator theMapIter;
    if (inCaseInsensitive) {
        Q3DStudio::CString theUpperInVariable = inVariable;
        theUpperInVariable.ToUpper();
        theMapIter = m_VariablesMap.find(theUpperInVariable);
    } else {
        theMapIter = m_VariablesMap.find(inVariable);
    }
    if (m_VariablesMap.end() != theMapIter) {
        outValue = theMapIter->second;
        return true;
    } else
        return false;
}

//=============================================================================
/**
 * Add a listener to the list of objects to be notified on a change.
 * The listener gets told when the any of the environment changes.
 * @param inChangeListener	listener who intereste for changes.
 */
void CStudioProjectVariables::AddChangeListener(IVariableChangeListener *inChangeListener)
{
    m_ChangeListeners.AddListener(inChangeListener);
}

//=============================================================================
/**
 * Remove a listener from the list of objects to be notified on a change.
 * @param inChangeListener	the change listener to be removed.
 */
void CStudioProjectVariables::RemoveChangeListener(IVariableChangeListener *inChangeListener)
{
    m_ChangeListeners.RemoveListener(inChangeListener);
}

//=============================================================================
/**
 * Fire a change event for this object.
 */
void CStudioProjectVariables::FireChangeEvent()
{
    m_ChangeListeners.FireEvent(&IVariableChangeListener::OnVariableChanged);
}

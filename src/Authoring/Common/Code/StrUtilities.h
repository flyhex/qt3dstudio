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
//==============================================================================
//	Prefixes
//==============================================================================
#ifndef STRUTILITIES_H
#define STRUTILITIES_H

//==============================================================================
//	Includes
//==============================================================================
#include <string>
#include <ctype.h>
#include <stddef.h>

//==============================================================================
//	String Conversion Functions
//==============================================================================
//==============================================================================
/**
 *	@functions bunch of stream ins to facilitate putting various different
 *	types into std::string and std::wstring
 *
 */
std::wstring &operator<<(std::wstring &inStr, bool inBool);
std::wstring &operator<<(std::wstring &inStr, long inLong);
std::wstring &operator<<(std::wstring &inStr, double inDouble);
std::wstring &operator<<(std::wstring &inStr, const char *inStr02);
std::wstring &operator<<(std::wstring &inStr, const wchar_t *inStr02);
std::wstring &operator<<(std::wstring &inStr, const std::string &inStr02);
std::wstring &operator<<(std::wstring &inStr, const std::wstring &inStr02);

std::string &operator<<(std::string &inStr, bool inBool);
std::string &operator<<(std::string &inStr, long inLong);
std::string &operator<<(std::string &inStr, double inDouble);
std::string &operator<<(std::string &inStr, const char *inStr02);
std::string &operator<<(std::string &inStr, const wchar_t *inStr02);
std::string &operator<<(std::string &inStr, const std::string &inStr02);
std::string &operator<<(std::string &inStr, const std::wstring &inStr02);

//==============================================================================
/**
 *	Compares 2 strings in a case-insensitive way.
 *	operator== should be reserved for case sensitive compare,
 *	so this leaves us with this ugly function name to use.
 *
 *	@param		inString1		The first string to be compared
 *	@param		inString2		The second string to be compared
 *
 *	@return		Returns true if both strings are equal, false otherwise
 */
template <class T, class U>
bool CaseInsensitiveEqual(const std::basic_string<T> &inString1,
                          const std::basic_string<U> &inString2)
{
    bool theResult = true;

    typename std::basic_string<T>::const_iterator theIterator1 = inString1.begin();
    typename std::basic_string<U>::const_iterator theIterator2 = inString2.begin();

    while (theIterator1 != inString1.end() && theIterator2 != inString2.end()) {
        if (toupper(*theIterator1++) != toupper(*theIterator2++)) {
            theResult = false;
            theIterator1 = inString1.end(); // break
        }
    }

    if (theIterator1 != inString1.end() || theIterator2 != inString2.end()) {
        theResult = false;
    }

    return theResult;
}

//==============================================================================
/**
 *	Converts a string to upper-case.
 *
 *	@param		ioString		The string
 *
 */
template <class T>
void ToUpperCase(std::basic_string<T> &ioString)
{
    typename std::basic_string<T>::iterator theIterator;
    for (theIterator = ioString.begin(); theIterator != ioString.end(); ++theIterator) {
        *theIterator = static_cast<T>(toupper(*theIterator));
    }
}

//==============================================================================
/**
 *	Finds a string in a case-insensitive way.
 *
 *	@param		inString1		The source string
 *	@param		inString2		The string to find
 *
 *	@return		Returns the pos in inString1 where inString2 is found, npos if not found.
 */
template <class T, class U>
size_t CaseInsensitiveFind(const std::basic_string<T> &inString1,
                           const std::basic_string<U> &inString2)
{
    size_t theResult = std::basic_string<T>::npos;

    std::basic_string<T> theString1(inString1);
    std::basic_string<T> theString2;
    theString2 << inString2;

    // convert both to upper-case
    ToUpperCase<T>(theString1);
    ToUpperCase<T>(theString2);

    theResult = theString1.find(theString2);

    return theResult;
}

//==============================================================================
/**
 * Returns the actual path of the string.
 *
 *	@param	inString		the string to extract the path
 *	@param	outString		the string to gett the path
 *
 *	@return	the path, if there is one, '.' otherwise.
 */
template <class T, class U>
void GetActualPath(const std::basic_string<T> &inString, std::basic_string<U> &outString)
{
    outString.clear();

    size_t thePos = inString.find_last_of("/\\");
    if (thePos != std::string::npos) {
        for (std::string::const_iterator theIter = inString.begin();
             theIter != inString.begin() + thePos; ++theIter) {
            outString.push_back(*theIter);
        }
    } else {
        outString.push_back('.');
    }
}

//==============================================================================
/**
 * Search for tokens and replace them.
 *
 *	@param	inToBeReplaced		the tokens to be replaced
 *	@param	inToBeReplacedBy	the tokens to be replaced with
 *	@param	outString			the string to be operated on
 *
 */
template <class T>
void ReplaceString(const std::basic_string<T> &inToBeReplaced,
                   const std::basic_string<T> &inToBeReplacedBy, std::basic_string<T> &outString)
{
    size_t thePos = outString.find_first_of(inToBeReplaced);
    while (thePos != std::basic_string<T>::npos) {
        outString.replace(outString.begin() + thePos, outString.begin() + thePos + 1,
                          inToBeReplacedBy.begin(), inToBeReplacedBy.end());
        thePos = outString.find_first_of(inToBeReplaced, thePos);
    }
}

#endif

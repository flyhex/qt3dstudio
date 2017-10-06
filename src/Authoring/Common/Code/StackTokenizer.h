/****************************************************************************
**
** Copyright (C) 1999-2004 NVIDIA Corporation.
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
#ifndef INCLUDED_STACKTOKENIZER_H_
#define INCLUDED_STACKTOKENIZER_H_ 1

//==============================================================================
//	Includes
//==============================================================================
#include "UICString.h"

//==============================================================================
//	Class
//==============================================================================
//==============================================================================
/**
 *	@class CStackTokenizer seperates a string out using a delimiter; Difference
 *						   between it and CStringTokenizer is that the
 *delimiter
 *						   is a single UICChar and there is an escape char as
 *well.
 */
class CStackTokenizer
{
public:
    CStackTokenizer(const Q3DStudio::CString &inString, Q3DStudio::UICChar inDelimiter,
                    Q3DStudio::UICChar inEscapeChar);
    virtual ~CStackTokenizer();

    bool HasNextPartition();
    Q3DStudio::CString GetCurrentPartition();
    void operator++();

protected:
    Q3DStudio::CString m_String; ///< contains the string to tokenize
    Q3DStudio::UICChar m_Delimiter; ///< single char delimiter
    Q3DStudio::UICChar m_EscapeChar; ///< single char escape char
    long m_Index; ///< index to begin tokenizing the string from
    long m_LastIndex; ///< index of the string since last tokenize operation
    const long m_StringLength; ///< length of the string to tokenize
};

#endif // INCLUDED_STACKTOKENIZER_H_
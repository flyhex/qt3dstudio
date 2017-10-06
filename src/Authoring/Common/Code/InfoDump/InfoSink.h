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
#ifndef INFOSINK_H
#define INFOSINK_H

#include <string>

//==============================================================================
//	Class
//==============================================================================
//==============================================================================
/**
 *	@class	CInfoSink
 *
 *	This base class that defines the destination of all output to the CInfoDump
 *	It is the responsibility of the the InfoSink class to insert a "\r\n" to the
 *	incoming string
 */
class CInfoSink
{
    //==============================================================================
    //	Enum
    //==============================================================================
public:
    enum EPriorityBits { ELOG = 1, EERR = 2, EDONT_CARE = 3 };

protected:
    long m_PriorityBits;

    //==============================================================================
    //	Ctor & Dtor
    //==============================================================================
public:
    CInfoSink(const long &inPriorityBits);
    virtual ~CInfoSink() {}

    //==============================================================================
    //	Interface
    //==============================================================================
public:
    virtual void Sink(const wchar_t *inStr) = 0;
    void SetPriorityBits(const long &inPBits);
    bool MatchPriority(const long &inPBits);
};

#endif
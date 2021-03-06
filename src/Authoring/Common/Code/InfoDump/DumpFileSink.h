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
#ifndef _DUMPFILESINK_H_
#define _DUMPFILESINK_H_
#pragma once

//==============================================================================
//	Include
//==============================================================================
#include <string>
#include "InfoSink.h"

//==============================================================================
//	Class
//==============================================================================
//==============================================================================
/**
 *	@class	CDumpFileSink
 *	This class inherits a CInfoSink, it can/will be called by CInfoDump to
 *	dump wchar_t passed to it into a designated file
  */
class CDumpFileSink : public CInfoSink
{
    //==============================================================================
    //	Internal Data
    //==============================================================================
private:
    std::wstring m_Filename;

    //==============================================================================
    //	Ctor Dtor
    //==============================================================================
public:
    CDumpFileSink(const wchar_t *inFilename, const long &inPriority = CInfoSink::EDONT_CARE);
    ~CDumpFileSink();

    //==============================================================================
    //	Interface
    //==============================================================================
public:
    void Sink(const wchar_t *inStr) override;
};

#endif

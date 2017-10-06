/****************************************************************************
**
** Copyright (C) 2002 NVIDIA Corporation.
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
#ifndef INCLUDED_DATADRIVENCHANGE_LISTENER_H
#define INCLUDED_DATADRIVENCHANGE_LISTENER_H 1

#pragma once

//==============================================================================
/**
 *	@class	IDataDrivenChangeListener
 *	@brief	Provides an interface for notifying listeners that the data driven property (not
 *the value) has changed.
 */
class IDataDrivenChangeListener
{
public:
    virtual ~IDataDrivenChangeListener() {}
    //==============================================================================
    /**
    * @param inIsDataDriven will be true if an item was set to data driven and false otherwise.
    * @param inPropName property that is affected
    */
    virtual void OnDataDrivenChange(bool /*inIsDataDriven*/,
                                    const Q3DStudio::CString & /*inPropName*/) = 0;

    //==============================================================================
    /**
    * Called when if the data drive link is changed
    * @param inPropName property that is affected
    */
    virtual void OnDataDrivenLinkValueChange(const Q3DStudio::CString & /*inPropName*/) = 0;
};

#endif // INCLUDED_DATADRIVENCHANGE_LISTENER_H

/****************************************************************************
**
** Copyright (C) 1999-2009 NVIDIA Corporation.
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

#include "UICImportTranslationCommon.h"

namespace UICIMP {

void GetFloatsToFloatArray(const domListOfFloats &inFloats, float *outArray)
{
    long theCount = (long)inFloats.getCount();
    for (long theIndex = 0; theIndex < theCount; ++theIndex)
        outArray[theIndex] = (float)inFloats[theIndex];
}

template <typename T>
void ProcessTransformElement(daeElement *inElement, float *outArray)
{
    const T *theDomElement = daeSafeCast<T>(inElement);
    assert(theDomElement != NULL);
    if (theDomElement)
        GetFloatsToFloatArray(theDomElement->getValue(), outArray);
}

void ProcessMatrix(daeElement *inMatrixElement, float *outMatrix)
{
    ProcessTransformElement<domMatrix>(inMatrixElement, outMatrix);
}

void ProcessTranslation(daeElement *inTranslateElement, float *outTranslate)
{
    ProcessTransformElement<domTranslate>(inTranslateElement, outTranslate);
}

void ProcessRotation(daeElement *inRotationElement, float *outRotate)
{
    ProcessTransformElement<domRotate>(inRotationElement, outRotate);
}

void ProcessScale(daeElement *inScaleElement, float *outScale)
{
    ProcessTransformElement<domScale>(inScaleElement, outScale);
}
}
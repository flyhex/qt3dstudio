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

//==============================================================================
// Includes
//==============================================================================
#pragma once
#include "UICImportTranslationCommon.h"
#include "dom/domTranslate.h"
#include "dom/domCommon_color_or_texture_type.h"
#include "dom/domCg_param_type.h"
#include "dom/domCommon_newparam_type.h"
#include "dom/domGles_basic_type_common.h"
#include "dom/domGlsl_param_type.h"
#include "dom/domFx_sampler1D_common.h"
#include "dom/domFx_sampler2D_common.h"
#include "dom/domFx_sampler3D_common.h"
#include "dom/domFx_samplerCUBE_common.h"
#include "dom/domFx_samplerDEPTH_common.h"
#include "dom/domFx_samplerRECT_common.h"
#include "dom/domSource.h"

namespace UICIMP {

typedef std::vector<std::string> TStringList;

//==============================================================================
/**
 * Converts a Collada <rotate> tag into a x, y or z rotation.
 * This only works when one axis is being animated.
 */
const char *FindRotationAxis(daeDoubleArray *inRotationArray)
{
    daeDoubleArray &theArray = *inRotationArray;

    if (theArray[0] == 1 && theArray[1] == 0 && theArray[2] == 0)
        return "rotate.x";
    else if (theArray[0] == 0 && theArray[1] == 1 && theArray[2] == 0)
        return "rotate.y";
    else if (theArray[0] == 0 && theArray[1] == 0 && theArray[2] == 1)
        return "rotate.z";

    return NULL;
}

inline int GetElementType(daeElement &inElement)
{
#define MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domType, colldataType)                                    \
    if (dynamic_cast<ColladaDOM141::domType *>(&inElement))                                        \
        return COLLADA_TYPE::colldataType;

    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domRotate, ROTATE);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domTranslate, TRANSLATE);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domScale, SCALE);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domMatrix, MATRIX);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domCommon_color_or_texture_type_complexType::domColor, COLOR);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domCg_param_type::domFloat, FLOAT);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domCommon_float_or_param_type_complexType::domFloat, FLOAT);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domCommon_newparam_type_complexType::domFloat, FLOAT);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domFx_annotate_type_common::domFloat, FLOAT);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domFx_basic_type_common::domFloat, FLOAT);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domGles_basic_type_common::domFloat, FLOAT);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domGlsl_param_type::domFloat, FLOAT);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domSource, SOURCE);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domFx_samplerRECT_common_complexType::domSource, SOURCE);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domFx_samplerDEPTH_common_complexType::domSource, SOURCE);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domFx_samplerCUBE_common_complexType::domSource, SOURCE);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domFx_sampler3D_common_complexType::domSource, SOURCE);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domFx_sampler2D_common_complexType::domSource, SOURCE);
    MAP_DYNAMIC_TYPE_TO_COLLADA_TYPE(domFx_sampler1D_common_complexType::domSource, SOURCE);
    return 0;
}

//==============================================================================
/**
 * Retrieves information about the animated element.
 * Finds out the animated property name (using the element's name or it's parent's name).
 * Determines the sub components of the property (e.g. xyz or rgb)
 * Looks for the element that is the target of the animation.
 */
const char *GetAnimatedPropertyInfoFromElement(const char *inTarget, domCOLLADA *inColladaRoot,
                                               daeElement *&outContainerElement,
                                               TStringList &outIdentifiers)
{
    daeSidRef::resolveData theData = daeSidRef(inTarget, inColladaRoot).resolve();

    // Push any identifiers after the '.' into the identifier list
    std::string theIdentifier(inTarget);
    std::string::size_type theOffset = theIdentifier.rfind('.');
    if (theOffset != std::string::npos)
        outIdentifiers.push_back(theIdentifier.substr(theOffset + 1));

    if (theData.elt) {
        outContainerElement = theData.elt;

        // HACK: Special case for animation on the angle component of <rotate>
        if (GetElementType(*outContainerElement) == COLLADA_TYPE::ROTATE && theData.scalar
            && (std::find(outIdentifiers.begin(), outIdentifiers.end(), "ANGLE")
                    != outIdentifiers.end()
                || std::find(outIdentifiers.begin(), outIdentifiers.end(), "W")
                    != outIdentifiers.end())) {
            return FindRotationAxis(theData.array);
        } else if (outIdentifiers.empty()) // If no identifiers found at the end of inTarget, obtain
                                           // identifiers based on the element that is being
                                           // animated.
        {
            switch (GetElementType(*outContainerElement)) {
            case COLLADA_TYPE::TRANSLATE:
            case COLLADA_TYPE::SCALE:
                outIdentifiers = g_XYZIdentifiers;
                break;
            case COLLADA_TYPE::ROTATE:
                // outIdentifiers = g_XYZANGLEIdentifiers; // TODO: We don't support x,y,z,angle
                // animation currently
                break;
            case COLLADA_TYPE::COLOR:
                outIdentifiers = g_RGBAIdentifiers;
                break;
            case COLLADA_TYPE::FLOAT:
                outIdentifiers = g_FLOATIdentifiers; // Actually, there wouldn't be any name in the
                                                     // accessor param for single float animations.
                                                     // This is just a placeholder.
                break;
            default:
                break;
            }
        }

        // The element's or parent's name will be used as the name of the base property to animate.
        switch (GetElementType(*outContainerElement)) {
        case COLLADA_TYPE::TRANSLATE:
        case COLLADA_TYPE::SCALE:
        case COLLADA_TYPE::ROTATE:
            return theData.elt->getElementName();
            break;
        case COLLADA_TYPE::COLOR:
        case COLLADA_TYPE::FLOAT:
            return outContainerElement->getParentElement()->getElementName();
            break;
        default:
            break;
        }
    }

    return "";
}
}
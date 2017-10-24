/****************************************************************************
**
** Copyright (C) 2008-2016 NVIDIA Corporation.
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt 3D Studio.
**
** $QT_BEGIN_LICENSE:GPL$
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
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "Qt3DSRenderPrefilterTexture.h"
#include "render/Qt3DSRenderContext.h"
#include "render/Qt3DSRenderShaderProgram.h"

#include <string>

using namespace qt3ds;
using namespace qt3ds::render;
using namespace qt3ds::foundation;

Qt3DSRenderPrefilterTexture::Qt3DSRenderPrefilterTexture(NVRenderContext *inNVRenderContext,
                                                     QT3DSI32 inWidth, QT3DSI32 inHeight,
                                                     NVRenderTexture2D &inTexture2D,
                                                     NVRenderTextureFormats::Enum inDestFormat,
                                                     NVFoundationBase &inFnd)
    : m_Foundation(inFnd)
    , mRefCount(0)
    , m_Texture2D(inTexture2D)
    , m_DestinationFormat(inDestFormat)
    , m_Width(inWidth)
    , m_Height(inHeight)
    , m_NVRenderContext(inNVRenderContext)
{
    // Calculate mip level
    int maxDim = inWidth >= inHeight ? inWidth : inHeight;

    m_MaxMipMapLevel = static_cast<int>(logf((float)maxDim) / logf(2.0f));
    // no concept of sizeOfFormat just does'nt make sense
    m_SizeOfFormat = NVRenderTextureFormats::getSizeofFormat(m_DestinationFormat);
    m_NoOfComponent = NVRenderTextureFormats::getNumberOfComponent(m_DestinationFormat);
}

Qt3DSRenderPrefilterTexture *
Qt3DSRenderPrefilterTexture::Create(NVRenderContext *inNVRenderContext, QT3DSI32 inWidth, QT3DSI32 inHeight,
                                  NVRenderTexture2D &inTexture2D,
                                  NVRenderTextureFormats::Enum inDestFormat,
                                  qt3ds::NVFoundationBase &inFnd)
{
    Qt3DSRenderPrefilterTexture *theBSDFMipMap = NULL;

    if (inNVRenderContext->IsComputeSupported()) {
        theBSDFMipMap = QT3DS_NEW(inFnd.getAllocator(), Qt3DSRenderPrefilterTextureCompute)(
            inNVRenderContext, inWidth, inHeight, inTexture2D, inDestFormat, inFnd);
    }

    if (!theBSDFMipMap) {
        theBSDFMipMap = QT3DS_NEW(inFnd.getAllocator(), Qt3DSRenderPrefilterTextureCPU)(
            inNVRenderContext, inWidth, inHeight, inTexture2D, inDestFormat, inFnd);
    }

    if (theBSDFMipMap)
        theBSDFMipMap->addRef();

    return theBSDFMipMap;
}

Qt3DSRenderPrefilterTexture::~Qt3DSRenderPrefilterTexture()
{
}

//------------------------------------------------------------------------------------
// CPU based filtering
//------------------------------------------------------------------------------------

Qt3DSRenderPrefilterTextureCPU::Qt3DSRenderPrefilterTextureCPU(
    NVRenderContext *inNVRenderContext, int inWidth, int inHeight, NVRenderTexture2D &inTexture2D,
    NVRenderTextureFormats::Enum inDestFormat, NVFoundationBase &inFnd)
    : Qt3DSRenderPrefilterTexture(inNVRenderContext, inWidth, inHeight, inTexture2D, inDestFormat,
                                inFnd)
{
}

inline int Qt3DSRenderPrefilterTextureCPU::wrapMod(int a, int base)
{
    return (a >= 0) ? a % base : (a % base) + base;
}

inline void Qt3DSRenderPrefilterTextureCPU::getWrappedCoords(int &sX, int &sY, int width, int height)
{
    if (sY < 0) {
        sX -= width >> 1;
        sY = -sY;
    }
    if (sY >= height) {
        sX += width >> 1;
        sY = height - sY;
    }
    sX = wrapMod(sX, width);
}

STextureData
Qt3DSRenderPrefilterTextureCPU::CreateBsdfMipLevel(STextureData &inCurMipLevel,
                                                 STextureData &inPrevMipLevel, int width,
                                                 int height) //, IPerfTimer& inPerfTimer )
{
    STextureData retval;
    int newWidth = width >> 1;
    int newHeight = height >> 1;
    newWidth = newWidth >= 1 ? newWidth : 1;
    newHeight = newHeight >= 1 ? newHeight : 1;

    if (inCurMipLevel.data) {
        retval = inCurMipLevel;
        retval.dataSizeInBytes =
            newWidth * newHeight * NVRenderTextureFormats::getSizeofFormat(inPrevMipLevel.format);
    } else {
        retval.dataSizeInBytes =
            newWidth * newHeight * NVRenderTextureFormats::getSizeofFormat(inPrevMipLevel.format);
        retval.format = inPrevMipLevel.format; // inLoadedImage.format;
        retval.data = m_Foundation.getAllocator().allocate(
            retval.dataSizeInBytes, "Bsdf Scaled Image Data", __FILE__, __LINE__);
    }

    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            float accumVal[4];
            accumVal[0] = 0;
            accumVal[1] = 0;
            accumVal[2] = 0;
            accumVal[3] = 0;
            for (int sy = -2; sy <= 2; ++sy) {
                for (int sx = -2; sx <= 2; ++sx) {
                    int sampleX = sx + (x << 1);
                    int sampleY = sy + (y << 1);
                    getWrappedCoords(sampleX, sampleY, width, height);

                    // Cauchy filter (this is simply because it's the easiest to evaluate, and
                    // requires no complex
                    // functions).
                    float filterPdf = 1.f / (1.f + float(sx * sx + sy * sy) * 2.f);
                    // With FP HDR formats, we're not worried about intensity loss so much as
                    // unnecessary energy gain,
                    // whereas with LDR formats, the fear with a continuous normalization factor is
                    // that we'd lose
                    // intensity and saturation as well.
                    filterPdf /= (NVRenderTextureFormats::getSizeofFormat(retval.format) >= 8)
                        ? 4.71238898f
                        : 4.5403446f;
                    // filterPdf /= 4.5403446f;		// Discrete normalization factor
                    // filterPdf /= 4.71238898f;		// Continuous normalization factor
                    float curPix[4];
                    QT3DSI32 byteOffset = (sampleY * width + sampleX)
                        * NVRenderTextureFormats::getSizeofFormat(retval.format);
                    if (byteOffset < 0) {
                        sampleY = height + sampleY;
                        byteOffset = (sampleY * width + sampleX)
                            * NVRenderTextureFormats::getSizeofFormat(retval.format);
                    }

                    NVRenderTextureFormats::decodeToFloat(inPrevMipLevel.data, byteOffset, curPix,
                                                          retval.format);

                    accumVal[0] += filterPdf * curPix[0];
                    accumVal[1] += filterPdf * curPix[1];
                    accumVal[2] += filterPdf * curPix[2];
                    accumVal[3] += filterPdf * curPix[3];
                }
            }

            QT3DSU32 newIdx =
                (y * newWidth + x) * NVRenderTextureFormats::getSizeofFormat(retval.format);

            NVRenderTextureFormats::encodeToPixel(accumVal, retval.data, newIdx, retval.format);
        }
    }

    return retval;
}

void Qt3DSRenderPrefilterTextureCPU::Build(void *inTextureData, QT3DSI32 inTextureDataSize,
                                         NVRenderTextureFormats::Enum inFormat)
{

    m_InternalFormat = inFormat;
    m_SizeOfInternalFormat = NVRenderTextureFormats::getSizeofFormat(m_InternalFormat);
    m_InternalNoOfComponent = NVRenderTextureFormats::getNumberOfComponent(m_InternalFormat);

    m_Texture2D.SetTextureData(NVDataRef<QT3DSU8>((QT3DSU8 *)inTextureData, inTextureDataSize), 0,
                               m_Width, m_Height, inFormat, m_DestinationFormat);

    STextureData theMipImage;
    STextureData prevImage;
    prevImage.data = inTextureData;
    prevImage.dataSizeInBytes = inTextureDataSize;
    prevImage.format = inFormat;
    int curWidth = m_Width;
    int curHeight = m_Height;
    int size = NVRenderTextureFormats::getSizeofFormat(m_InternalFormat);
    for (int idx = 1; idx <= m_MaxMipMapLevel; ++idx) {
        theMipImage =
            CreateBsdfMipLevel(theMipImage, prevImage, curWidth, curHeight); //, m_PerfTimer );
        curWidth = curWidth >> 1;
        curHeight = curHeight >> 1;
        curWidth = curWidth >= 1 ? curWidth : 1;
        curHeight = curHeight >= 1 ? curHeight : 1;
        inTextureDataSize = curWidth * curHeight * size;

        m_Texture2D.SetTextureData(toU8DataRef((char *)theMipImage.data, (QT3DSU32)inTextureDataSize),
                                   (QT3DSU8)idx, (QT3DSU32)curWidth, (QT3DSU32)curHeight, theMipImage.format,
                                   m_DestinationFormat);

        if (prevImage.data == inTextureData)
            prevImage = STextureData();

        STextureData temp = prevImage;
        prevImage = theMipImage;
        theMipImage = temp;
    }
    QT3DS_FREE(m_Foundation.getAllocator(), theMipImage.data);
    QT3DS_FREE(m_Foundation.getAllocator(), prevImage.data);
}

//------------------------------------------------------------------------------------
// GL compute based filtering
//------------------------------------------------------------------------------------

static const char *computeUploadShader(std::string &prog, NVRenderTextureFormats::Enum inFormat,
                                       bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_ARB_compute_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n"
                "precision mediump image2D;\n";
    } else {
        prog += "#version 430\n"
                "#extension GL_ARB_compute_shader : enable\n";
    }

    if (inFormat == NVRenderTextureFormats::RGBA8) {
        prog += "// Set workgroup layout;\n"
                "layout (local_size_x = 16, local_size_y = 16) in;\n\n"
                "layout (rgba8, binding = 1) readonly uniform image2D inputImage;\n\n"
                "layout (rgba16f, binding = 2) writeonly uniform image2D outputImage;\n\n"
                "void main()\n"
                "{\n"
                "  if ( gl_GlobalInvocationID.x >= gl_NumWorkGroups.x || gl_GlobalInvocationID.y "
                ">= gl_NumWorkGroups.y )\n"
                "    return;\n"
                "  vec4 value = imageLoad(inputImage, ivec2(gl_GlobalInvocationID.xy));\n"
                "  imageStore( outputImage, ivec2(gl_GlobalInvocationID.xy), value );\n"
                "}\n";
    } else {
        prog += "float convertToFloat( in uint inValue )\n"
                "{\n"
                "  uint v = inValue & uint(0xFF);\n"
                "  float f = float(v)/256.0;\n"
                "  return f;\n"
                "}\n";

        prog += "int getMod( in int inValue, in int mod )\n"
                "{\n"
                "  int v = mod * (inValue/mod);\n"
                "  return inValue - v;\n"
                "}\n";

        prog += "vec4 getRGBValue( in int byteNo, vec4 inVal, vec4 inVal1 )\n"
                "{\n"
                "  vec4 result= vec4(0.0);\n"
                "  if( byteNo == 0) {\n"
                "    result.r = inVal.r;\n"
                "    result.g = inVal.g;\n"
                "    result.b = inVal.b;\n"
                "  }\n"
                "  else if( byteNo == 1) {\n"
                "    result.r = inVal.g;\n"
                "    result.g = inVal.b;\n"
                "    result.b = inVal.a;\n"
                "  }\n"
                "  else if( byteNo == 2) {\n"
                "    result.r = inVal.b;\n"
                "    result.g = inVal.a;\n"
                "    result.b = inVal1.r;\n"
                "  }\n"
                "  else if( byteNo == 3) {\n"
                "    result.r = inVal.a;\n"
                "    result.g = inVal1.r;\n"
                "    result.b = inVal1.g;\n"
                "  }\n"
                "  return result;\n"
                "}\n";

        prog += "// Set workgroup layout;\n"
                "layout (local_size_x = 16, local_size_y = 16) in;\n\n"
                "layout (rgba8, binding = 1) readonly uniform image2D inputImage;\n\n"
                "layout (rgba16f, binding = 2) writeonly uniform image2D outputImage;\n\n"
                "void main()\n"
                "{\n"
                "  vec4 result = vec4(0.0);\n"
                "  if ( gl_GlobalInvocationID.x >= gl_NumWorkGroups.x || gl_GlobalInvocationID.y "
                ">= gl_NumWorkGroups.y )\n"
                "    return;\n"
                "  int xpos = (int(gl_GlobalInvocationID.x)*3)/4;\n"
                "  int xmod = getMod(int(gl_GlobalInvocationID.x)*3, 4);\n"
                "  ivec2 readPos = ivec2(xpos, gl_GlobalInvocationID.y);\n"
                "  vec4 value = imageLoad(inputImage, readPos);\n"
                "  vec4 value1 = imageLoad(inputImage, ivec2(readPos.x + 1, readPos.y));\n"
                "  result = getRGBValue( xmod, value, value1);\n"
                "  imageStore( outputImage, ivec2(gl_GlobalInvocationID.xy), result );\n"
                "}\n";
    }
    return prog.c_str();
}

static const char *computeWorkShader(std::string &prog, bool binESContext)
{
    if (binESContext) {
        prog += "#version 310 es\n"
                "#extension GL_ARB_compute_shader : enable\n"
                "precision highp float;\n"
                "precision highp int;\n"
                "precision mediump image2D;\n";
    } else {
        prog += "#version 430\n"
                "#extension GL_ARB_compute_shader : enable\n";
    }

    prog += "int wrapMod( in int a, in int base )\n"
            "{\n"
            "  return ( a >= 0 ) ? a % base : -(a % base) + base;\n"
            "}\n";

    prog += "void getWrappedCoords( inout int sX, inout int sY, in int width, in int height )\n"
            "{\n"
            "  if (sY < 0) { sX -= width >> 1; sY = -sY; }\n"
            "  if (sY >= height) { sX += width >> 1; sY = height - sY; }\n"
            "  sX = wrapMod( sX, width );\n"
            "}\n";

    prog += "// Set workgroup layout;\n"
            "layout (local_size_x = 16, local_size_y = 16) in;\n\n"
            "layout (rgba16f, binding = 1) readonly uniform image2D inputImage;\n\n"
            "layout (rgba16f, binding = 2) writeonly uniform image2D outputImage;\n\n"
            "void main()\n"
            "{\n"
            "  int prevWidth = int(gl_NumWorkGroups.x) << 1;\n"
            "  int prevHeight = int(gl_NumWorkGroups.y) << 1;\n"
            "  if ( gl_GlobalInvocationID.x >= gl_NumWorkGroups.x || gl_GlobalInvocationID.y >= "
            "gl_NumWorkGroups.y )\n"
            "    return;\n"
            "  vec4 accumVal = vec4(0.0);\n"
            "  for ( int sy = -2; sy <= 2; ++sy )\n"
            "  {\n"
            "    for ( int sx = -2; sx <= 2; ++sx )\n"
            "    {\n"
            "      int sampleX = sx + (int(gl_GlobalInvocationID.x) << 1);\n"
            "      int sampleY = sy + (int(gl_GlobalInvocationID.y) << 1);\n"
            "      getWrappedCoords(sampleX, sampleY, prevWidth, prevHeight);\n"
            "	   if ((sampleY * prevWidth + sampleX) < 0 )\n"
            "        sampleY = prevHeight + sampleY;\n"
            "      ivec2 pos = ivec2(sampleX, sampleY);\n"
            "      vec4 value = imageLoad(inputImage, pos);\n"
            "      float filterPdf = 1.0 / ( 1.0 + float(sx*sx + sy*sy)*2.0 );\n"
            "      filterPdf /= 4.71238898;\n"
            "      accumVal[0] += filterPdf * value.r;\n"
            "	   accumVal[1] += filterPdf * value.g;\n"
            "	   accumVal[2] += filterPdf * value.b;\n"
            "	   accumVal[3] += filterPdf * value.a;\n"
            "    }\n"
            "  }\n"
            "  imageStore( outputImage, ivec2(gl_GlobalInvocationID.xy), accumVal );\n"
            "}\n";

    return prog.c_str();
}

inline NVConstDataRef<QT3DSI8> toRef(const char *data)
{
    size_t len = strlen(data) + 1;
    return NVConstDataRef<QT3DSI8>((const QT3DSI8 *)data, (QT3DSU32)len);
}

static bool isGLESContext(NVRenderContext *context)
{
    NVRenderContextType ctxType = context->GetRenderContextType();

    // Need minimum of GL3 or GLES3
    if (ctxType == NVRenderContextValues::GLES2 || ctxType == NVRenderContextValues::GLES3
        || ctxType == NVRenderContextValues::GLES3PLUS) {
        return true;
    }

    return false;
}

#define WORKGROUP_SIZE 16

Qt3DSRenderPrefilterTextureCompute::Qt3DSRenderPrefilterTextureCompute(
    NVRenderContext *inNVRenderContext, QT3DSI32 inWidth, QT3DSI32 inHeight,
    NVRenderTexture2D &inTexture2D, NVRenderTextureFormats::Enum inDestFormat,
    NVFoundationBase &inFnd)
    : Qt3DSRenderPrefilterTexture(inNVRenderContext, inWidth, inHeight, inTexture2D, inDestFormat,
                                inFnd)
    , m_BSDFProgram(NULL)
    , m_UploadProgram_RGBA8(NULL)
    , m_UploadProgram_RGB8(NULL)
    , m_Level0Tex(NULL)
    , m_TextureCreated(false)
{
}

Qt3DSRenderPrefilterTextureCompute::~Qt3DSRenderPrefilterTextureCompute()
{
    m_UploadProgram_RGB8 = NULL;
    m_UploadProgram_RGBA8 = NULL;
    m_BSDFProgram = NULL;
    m_Level0Tex = NULL;
}

void Qt3DSRenderPrefilterTextureCompute::createComputeProgram(NVRenderContext *context)
{
    std::string computeProg;

    if (!m_BSDFProgram) {
        m_BSDFProgram = context
                            ->CompileComputeSource(
                                "Compute BSDF mipmap shader",
                                toRef(computeWorkShader(computeProg, isGLESContext(context))))
                            .mShader;
    }
}

NVRenderShaderProgram *Qt3DSRenderPrefilterTextureCompute::getOrCreateUploadComputeProgram(
    NVRenderContext *context, NVRenderTextureFormats::Enum inFormat)
{
    std::string computeProg;

    if (inFormat == NVRenderTextureFormats::RGB8) {
        if (!m_UploadProgram_RGB8) {
            m_UploadProgram_RGB8 =
                context
                    ->CompileComputeSource(
                        "Compute BSDF mipmap level 0 RGB8 shader",
                        toRef(computeUploadShader(computeProg, inFormat, isGLESContext(context))))
                    .mShader;
        }

        return m_UploadProgram_RGB8;
    } else {
        if (!m_UploadProgram_RGBA8) {
            m_UploadProgram_RGBA8 =
                context
                    ->CompileComputeSource(
                        "Compute BSDF mipmap level 0 RGBA8 shader",
                        toRef(computeUploadShader(computeProg, inFormat, isGLESContext(context))))
                    .mShader;
        }

        return m_UploadProgram_RGBA8;
    }
}

void Qt3DSRenderPrefilterTextureCompute::CreateLevel0Tex(void *inTextureData, QT3DSI32 inTextureDataSize,
                                                       NVRenderTextureFormats::Enum inFormat)
{
    NVRenderTextureFormats::Enum theFormat = inFormat;
    QT3DSI32 theWidth = m_Width;

    // Since we cannot use RGB format in GL compute
    // we treat it as a RGBA component format
    if (inFormat == NVRenderTextureFormats::RGB8) {
        // This works only with 4 byte aligned data
        QT3DS_ASSERT(m_Width % 4 == 0);
        theFormat = NVRenderTextureFormats::RGBA8;
        theWidth = (m_Width * 3) / 4;
    }

    if (m_Level0Tex == NULL) {
        m_Level0Tex = m_NVRenderContext->CreateTexture2D();
        m_Level0Tex->SetTextureStorage(1, theWidth, m_Height, theFormat, theFormat,
                                       NVDataRef<QT3DSU8>((QT3DSU8 *)inTextureData, inTextureDataSize));
    } else {
        m_Level0Tex->SetTextureSubData(NVDataRef<QT3DSU8>((QT3DSU8 *)inTextureData, inTextureDataSize), 0,
                                       0, 0, theWidth, m_Height, theFormat);
    }
}

void Qt3DSRenderPrefilterTextureCompute::Build(void *inTextureData, QT3DSI32 inTextureDataSize,
                                             NVRenderTextureFormats::Enum inFormat)
{
    bool needMipUpload = (inFormat != m_DestinationFormat);
    // re-upload data
    if (!m_TextureCreated) {
        m_Texture2D.SetTextureStorage(
            m_MaxMipMapLevel + 1, m_Width, m_Height, m_DestinationFormat, inFormat, (needMipUpload)
                ? NVDataRef<QT3DSU8>()
                : NVDataRef<QT3DSU8>((QT3DSU8 *)inTextureData, inTextureDataSize));
        m_Texture2D.addRef();
        // create a compute shader (if not aloread done) which computes the BSDF mipmaps for this
        // texture
        createComputeProgram(m_NVRenderContext);

        if (!m_BSDFProgram) {
            QT3DS_ASSERT(false);
            return;
        }

        m_TextureCreated = true;
    } else if (!needMipUpload) {
        m_Texture2D.SetTextureSubData(NVDataRef<QT3DSU8>((QT3DSU8 *)inTextureData, inTextureDataSize), 0,
                                      0, 0, m_Width, m_Height, inFormat);
    }

    if (needMipUpload) {
        CreateLevel0Tex(inTextureData, inTextureDataSize, inFormat);
    }

    NVScopedRefCounted<NVRenderImage2D> theInputImage;
    NVScopedRefCounted<NVRenderImage2D> theOutputImage;
    theInputImage =
        m_NVRenderContext->CreateImage2D(&m_Texture2D, NVRenderImageAccessType::ReadWrite);
    theOutputImage =
        m_NVRenderContext->CreateImage2D(&m_Texture2D, NVRenderImageAccessType::ReadWrite);

    if (needMipUpload && m_Level0Tex) {
        NVRenderShaderProgram *uploadProg =
            getOrCreateUploadComputeProgram(m_NVRenderContext, inFormat);
        if (!uploadProg)
            return;

        m_NVRenderContext->SetActiveShader(uploadProg);

        NVScopedRefCounted<NVRenderImage2D> theInputImage0;
        theInputImage0 =
            m_NVRenderContext->CreateImage2D(m_Level0Tex, NVRenderImageAccessType::ReadWrite);

        theInputImage0->SetTextureLevel(0);
        NVRenderCachedShaderProperty<NVRenderImage2D *> theCachedinputImage0("inputImage",
                                                                             *uploadProg);
        theCachedinputImage0.Set(theInputImage0);

        theOutputImage->SetTextureLevel(0);
        NVRenderCachedShaderProperty<NVRenderImage2D *> theCachedOutputImage("outputImage",
                                                                             *uploadProg);
        theCachedOutputImage.Set(theOutputImage);

        m_NVRenderContext->DispatchCompute(uploadProg, m_Width, m_Height, 1);

        // sync
        NVRenderBufferBarrierFlags flags(NVRenderBufferBarrierValues::ShaderImageAccess);
        m_NVRenderContext->SetMemoryBarrier(flags);
    }

    int width = m_Width >> 1;
    int height = m_Height >> 1;

    m_NVRenderContext->SetActiveShader(m_BSDFProgram);

    for (int i = 1; i <= m_MaxMipMapLevel; ++i) {
        theOutputImage->SetTextureLevel(i);
        NVRenderCachedShaderProperty<NVRenderImage2D *> theCachedOutputImage("outputImage",
                                                                             *m_BSDFProgram);
        theCachedOutputImage.Set(theOutputImage);
        theInputImage->SetTextureLevel(i - 1);
        NVRenderCachedShaderProperty<NVRenderImage2D *> theCachedinputImage("inputImage",
                                                                            *m_BSDFProgram);
        theCachedinputImage.Set(theInputImage);

        m_NVRenderContext->DispatchCompute(m_BSDFProgram, width, height, 1);

        width = width > 2 ? width >> 1 : 1;
        height = height > 2 ? height >> 1 : 1;

        // sync
        NVRenderBufferBarrierFlags flags(NVRenderBufferBarrierValues::ShaderImageAccess);
        m_NVRenderContext->SetMemoryBarrier(flags);
    }
}

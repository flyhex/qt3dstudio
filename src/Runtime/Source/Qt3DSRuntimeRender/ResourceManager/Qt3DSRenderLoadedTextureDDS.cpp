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

#include "Qt3DSRenderLoadedTextureDDS.h"
#include "Qt3DSRenderLoadedTextureFreeImageCompat.h"

using namespace qt3ds::render;

namespace qt3ds {
namespace render {

    static int s_exception_string;

    //================================================================================
    // DXT data-layout structure definitions.
    typedef struct
    {
        QT3DSU16 col0; // 16-bit 565 interpolant endpoints
        QT3DSU16 col1;
        QT3DSU8 row[4]; // 4x4 * 2bpp color-index == 4 bytes.
    } DXTColBlock;

    typedef struct
    {
        QT3DSU16 row[4]; // 4x4 * 4bpp alpha == 8 bytes.  (pure 4-bit alpha values)
    } DXT3AlphaBlock;

    typedef struct
    {
        QT3DSU8 alpha0; // 8-bit alpha interpolant endpoints
        QT3DSU8 alpha1;
        QT3DSU8 row[6]; // 4x4 * 3bpp alpha-index == 48bits == 6 bytes.
    } DXT5AlphaBlock;

    typedef struct
    {
        QT3DSU8 red;
        QT3DSU8 green;
        QT3DSU8 blue;
        QT3DSU8 alpha;
    } Color8888;

//================================================================================
// Various DDS file defines

#define DDSD_CAPS 0x00000001l
#define DDSD_HEIGHT 0x00000002l
#define DDSD_WIDTH 0x00000004l
#define DDSD_PIXELFORMAT 0x00001000l
#define DDS_ALPHAPIXELS 0x00000001l
#define DDS_FOURCC 0x00000004l
#define DDS_PITCH 0x00000008l
#define DDS_COMPLEX 0x00000008l
#define DDS_RGB 0x00000040l
#define DDS_TEXTURE 0x00001000l
#define DDS_MIPMAPCOUNT 0x00020000l
#define DDS_LINEARSIZE 0x00080000l
#define DDS_VOLUME 0x00200000l
#define DDS_MIPMAP 0x00400000l
#define DDS_DEPTH 0x00800000l

#define DDS_CUBEMAP 0x00000200L
#define DDS_CUBEMAP_POSITIVEX 0x00000400L
#define DDS_CUBEMAP_NEGATIVEX 0x00000800L
#define DDS_CUBEMAP_POSITIVEY 0x00001000L
#define DDS_CUBEMAP_NEGATIVEY 0x00002000L
#define DDS_CUBEMAP_POSITIVEZ 0x00004000L
#define DDS_CUBEMAP_NEGATIVEZ 0x00008000L

#define FOURCC_DXT1 0x31545844 //(MAKEFOURCC('D','X','T','1'))
#define FOURCC_DXT3 0x33545844 //(MAKEFOURCC('D','X','T','3'))
#define FOURCC_DXT5 0x35545844 //(MAKEFOURCC('D','X','T','5'))

#define DDS_MAGIC_FLIPPED 0x0F7166ED

    //================================================================================
    // DDS file format structures.
    typedef struct _DDS_PIXELFORMAT
    {
        QT3DSU32 dwSize;
        QT3DSU32 dwFlags;
        QT3DSU32 dwFourCC;
        QT3DSU32 dwRGBBitCount;
        QT3DSU32 dwRBitMask;
        QT3DSU32 dwGBitMask;
        QT3DSU32 dwBBitMask;
        QT3DSU32 dwABitMask;
    } DDS_PIXELFORMAT;

    typedef struct _DDS_HEADER
    {
        QT3DSU32 dwSize;
        QT3DSU32 dwFlags;
        QT3DSU32 dwHeight;
        QT3DSU32 dwWidth;
        QT3DSU32 dwPitchOrLinearSize;
        QT3DSU32 dwDepth;
        QT3DSU32 dwMipMapCount;
        QT3DSU32 dwReserved1[11];
        DDS_PIXELFORMAT ddspf;
        QT3DSU32 dwCaps1;
        QT3DSU32 dwCaps2;
        QT3DSU32 dwReserved2[3];
    } DDS_HEADER;

    //================================================================================
    // helper functions
    //================================================================================

    // helper macros.
    static inline void NvSwapChar(QT3DSU8 &a, QT3DSU8 &b)
    {
        QT3DSU8 tmp;
        tmp = a;
        a = b;
        b = tmp;
    }

    static inline void NvSwapShort(QT3DSU16 &a, QT3DSU16 &b)
    {
        QT3DSU16 tmp;
        tmp = a;
        a = b;
        b = tmp;
    }

    //================================================================================
    //================================================================================
    static void flip_blocks_dxtc1(DXTColBlock *line, QT3DSI32 numBlocks)
    {
        DXTColBlock *curblock = line;
        QT3DSI32 i;

        for (i = 0; i < numBlocks; i++) {
            NvSwapChar(curblock->row[0], curblock->row[3]);
            NvSwapChar(curblock->row[1], curblock->row[2]);
            curblock++;
        }
    }

    //================================================================================
    //================================================================================
    static void flip_blocks_dxtc3(DXTColBlock *line, QT3DSI32 numBlocks)
    {
        DXTColBlock *curblock = line;
        DXT3AlphaBlock *alphablock;
        QT3DSI32 i;

        for (i = 0; i < numBlocks; i++) {
            alphablock = (DXT3AlphaBlock *)curblock;

            NvSwapShort(alphablock->row[0], alphablock->row[3]);
            NvSwapShort(alphablock->row[1], alphablock->row[2]);
            curblock++;

            NvSwapChar(curblock->row[0], curblock->row[3]);
            NvSwapChar(curblock->row[1], curblock->row[2]);
            curblock++;
        }
    }

    static void flip_dxt5_alpha(DXT5AlphaBlock *block)
    {
        QT3DSI8 gBits[4][4];

        const QT3DSU32 mask = 0x00000007; // bits = 00 00 01 11
        QT3DSU32 bits = 0;
        memcpy(&bits, &block->row[0], sizeof(QT3DSI8) * 3);

        gBits[0][0] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[0][1] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[0][2] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[0][3] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[1][0] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[1][1] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[1][2] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[1][3] = (QT3DSI8)(bits & mask);

        bits = 0;
        memcpy(&bits, &block->row[3], sizeof(QT3DSI8) * 3);

        gBits[2][0] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[2][1] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[2][2] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[2][3] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[3][0] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[3][1] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[3][2] = (QT3DSI8)(bits & mask);
        bits >>= 3;
        gBits[3][3] = (QT3DSI8)(bits & mask);

        bits = (gBits[3][0] << 0) | (gBits[3][1] << 3) | (gBits[3][2] << 6) | (gBits[3][3] << 9)
            | (gBits[2][0] << 12) | (gBits[2][1] << 15) | (gBits[2][2] << 18) | (gBits[2][3] << 21);
        memcpy(&block->row[0], &bits, 3);

        bits = (gBits[1][0] << 0) | (gBits[1][1] << 3) | (gBits[1][2] << 6) | (gBits[1][3] << 9)
            | (gBits[0][0] << 12) | (gBits[0][1] << 15) | (gBits[0][2] << 18) | (gBits[0][3] << 21);
        memcpy(&block->row[3], &bits, 3);
    }

    static void flip_blocks_dxtc5(DXTColBlock *line, QT3DSI32 numBlocks)
    {
        DXTColBlock *curblock = line;
        DXT5AlphaBlock *alphablock;
        QT3DSI32 i;

        for (i = 0; i < numBlocks; i++) {
            alphablock = (DXT5AlphaBlock *)curblock;

            flip_dxt5_alpha(alphablock);
            curblock++;

            NvSwapChar(curblock->row[0], curblock->row[3]);
            NvSwapChar(curblock->row[1], curblock->row[2]);
            curblock++;
        }
    }

    static void flip_data_vertical(FreeImageIO *io, QT3DSI8 *image, QT3DSI32 width, QT3DSI32 height,
                                   Qt3DSDDSImage *info)
    {
        if (info->compressed) {
            QT3DSI32 linesize, j;
            DXTColBlock *top;
            DXTColBlock *bottom;
            QT3DSI8 *tmp;
            void (*flipblocks)(DXTColBlock *, QT3DSI32) = NULL;
            QT3DSI32 xblocks = width / 4;
            QT3DSI32 yblocks = height / 4;
            QT3DSI32 blocksize;

            switch (info->format) {
            case qt3ds::render::NVRenderTextureFormats::RGBA_DXT1:
                blocksize = 8;
                flipblocks = &flip_blocks_dxtc1;
                break;
            case qt3ds::render::NVRenderTextureFormats::RGBA_DXT3:
                blocksize = 16;
                flipblocks = &flip_blocks_dxtc3;
                break;
            case qt3ds::render::NVRenderTextureFormats::RGBA_DXT5:
                blocksize = 16;
                flipblocks = &flip_blocks_dxtc5;
                break;
            default:
                return;
            }

            linesize = xblocks * blocksize;
            tmp = (QT3DSI8 *)QT3DS_ALLOC(io->m_Allocator, linesize, "flip_data_vertical compressed");

            for (j = 0; j < (yblocks >> 1); j++) {
                top = (DXTColBlock *)(void *)(image + j * linesize);
                bottom = (DXTColBlock *)(void *)(image + (((yblocks - j) - 1) * linesize));

                (*flipblocks)(top, xblocks);
                (*flipblocks)(bottom, xblocks);

                memcpy(tmp, bottom, linesize);
                memcpy(bottom, top, linesize);
                memcpy(top, tmp, linesize);
            }

            // Catch the middle row of blocks if there is one
            // The loop above will skip the middle row
            if (yblocks & 0x01) {
                DXTColBlock *middle = (DXTColBlock *)(void *)(image + (yblocks >> 1) * linesize);
                (*flipblocks)(middle, xblocks);
            }

            QT3DS_FREE(io->m_Allocator, tmp);
        } else {
            QT3DSI32 linesize = width * info->bytesPerPixel;
            QT3DSI32 j;
            QT3DSI8 *top;
            QT3DSI8 *bottom;
            QT3DSI8 *tmp;

            // much simpler - just compute the line length and swap each row
            tmp = (QT3DSI8 *)QT3DS_ALLOC(io->m_Allocator, linesize, "flip_data_vertical");
            ;

            for (j = 0; j < (height >> 1); j++) {
                top = (QT3DSI8 *)(image + j * linesize);
                bottom = (QT3DSI8 *)(image + (((height - j) - 1) * linesize));

                memcpy(tmp, bottom, linesize);
                memcpy(bottom, top, linesize);
                memcpy(top, tmp, linesize);
            }

            QT3DS_FREE(io->m_Allocator, tmp);
        }
    }

    static QT3DSI32 size_image(QT3DSI32 width, QT3DSI32 height, const Qt3DSDDSImage *image)
    {
        if (image->compressed) {
            return ((width + 3) / 4) * ((height + 3) / 4)
                * (image->format == qt3ds::render::NVRenderTextureFormats::RGBA_DXT1 ? 8 : 16);
        } else {
            return width * height * image->bytesPerPixel;
        }
    }

    static QT3DSI32 total_image_data_size(Qt3DSDDSImage *image)
    {
        QT3DSI32 i, j, index = 0, size = 0, w, h;
        QT3DSI32 cubeCount = image->cubemap ? 6 : 1;

        for (j = 0; j < cubeCount; j++) {
            w = image->width;
            h = image->height;

            for (i = 0; i < image->numMipmaps; i++) // account for base plus each mip
            {
                image->size[index] = size_image(w, h, image);
                image->mipwidth[index] = w;
                image->mipheight[index] = h;
                size += image->size[index];
                if (w != 1) {
                    w >>= 1;
                }
                if (h != 1) {
                    h >>= 1;
                }

                index++;
            }
        }

        return (size);
    }

    void *Qt3DSDDSAllocDataBlock(FreeImageIO *io, Qt3DSDDSImage *image)
    {
        if (image) {
            QT3DSI32 i;
            QT3DSI32 size = total_image_data_size(image);
            image->dataBlock =
                QT3DS_ALLOC(io->m_Allocator, size,
                         "Qt3DSDDSAllocDataBlock"); // no need to calloc, as we fill every bit...
            if (image->dataBlock == NULL) {
                return NULL;
            }

            image->data[0] = image->dataBlock;

            QT3DSI32 planes = image->numMipmaps * (image->cubemap ? 6 : 1);

            for (i = 1; i < planes; i++) // account for base plus each mip
            {
                image->data[i] =
                    (void *)(((size_t)(image->data[i - 1])) + (size_t)image->size[i - 1]);
            }

            return (image->dataBlock); // in case caller wants to sanity check...
        }
        return (NULL);
    }

    static FIBITMAP *DoLoadDDS(FreeImageIO *io, IInStream &inStream, QT3DSI32 flipVertical)
    {
        FIBITMAP *dib = NULL;
        DDS_HEADER ddsh;
        QT3DSI8 filecode[4];
        Qt3DSDDSImage *image = NULL;
        bool needsBGRASwap = false;
        ;
        bool isAllreadyFlipped = false;

        try {
            // check file code
            inStream.Read(filecode, 4);
            if (memcmp(filecode, "DDS ", 4)) {
                throw "Invalid DDS file";
            }

            image = (Qt3DSDDSImage *)QT3DS_ALLOC(io->m_Allocator, sizeof(Qt3DSDDSImage), "DoLoadDDS");
            if (image == NULL) {
                throw "Qt3DSDDSImage allocation failed";
            }
            memset(image, 0, sizeof(Qt3DSDDSImage));

            // read in DDS header
            inStream.Read(&ddsh, 1);

            // check if image is a cubempap
            if (ddsh.dwCaps2 & DDS_CUBEMAP) {
                const QT3DSI32 allFaces = DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_POSITIVEY
                    | DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEX | DDS_CUBEMAP_NEGATIVEY
                    | DDS_CUBEMAP_NEGATIVEZ;

                if ((ddsh.dwCaps2 & allFaces) != allFaces) {
                    throw "Not all cubemap faces defined - not supported";
                }

                image->cubemap = 1;
            } else {
                image->cubemap = 0;
            }

            // check if image is a volume texture
            if ((ddsh.dwCaps2 & DDS_VOLUME) && (ddsh.dwDepth > 0)) {
                throw "Volume textures not supported";
            }

            // allocated the memory for the structure we return
            dib = QT3DS_NEW(io->m_Allocator, SLoadedTexture)(io->m_Allocator);
            if (dib == NULL) {
                throw "DIB allocation failed";
            }

            // figure out what the image format is
            if (ddsh.ddspf.dwFlags & DDS_FOURCC) {
                switch (ddsh.ddspf.dwFourCC) {
                case FOURCC_DXT1:
                    image->format = qt3ds::render::NVRenderTextureFormats::RGBA_DXT1;
                    image->components = 3;
                    image->compressed = 1;
                    image->alpha = 0; // Ugh - for backwards compatibility
                    dib->format = qt3ds::render::NVRenderTextureFormats::RGB_DXT1;
                    break;
                case FOURCC_DXT3:
                    image->format = qt3ds::render::NVRenderTextureFormats::RGBA_DXT3;
                    image->components = 4;
                    image->compressed = 1;
                    image->alpha = 1;
                    dib->format = qt3ds::render::NVRenderTextureFormats::RGBA_DXT3;
                    break;
                case FOURCC_DXT5:
                    image->format = qt3ds::render::NVRenderTextureFormats::RGBA_DXT5;
                    image->components = 4;
                    image->compressed = 1;
                    image->alpha = 1;
                    dib->format = qt3ds::render::NVRenderTextureFormats::RGBA_DXT5;
                    break;
                default:
                    throw "Unsupported FOURCC code";
                }
            } else {
                // Check for a supported pixel format
                if ((ddsh.ddspf.dwRGBBitCount == 32) && (ddsh.ddspf.dwRBitMask == 0x000000FF)
                    && (ddsh.ddspf.dwGBitMask == 0x0000FF00)
                    && (ddsh.ddspf.dwBBitMask == 0x00FF0000)
                    && (ddsh.ddspf.dwABitMask == 0xFF000000)) {
                    // We support D3D's A8B8G8R8, which is actually RGBA in linear
                    // memory, equivalent to GL's RGBA
                    image->format = qt3ds::render::NVRenderTextureFormats::RGBA8;
                    image->components = 4;
                    image->componentFormat = qt3ds::render::NVRenderComponentTypes::QT3DSU8;
                    image->bytesPerPixel = 4;
                    image->alpha = 1;
                    image->compressed = 0;
                    dib->format = qt3ds::render::NVRenderTextureFormats::RGBA8;
                } else if ((ddsh.ddspf.dwRGBBitCount == 32) && (ddsh.ddspf.dwRBitMask == 0x00FF0000)
                           && (ddsh.ddspf.dwGBitMask == 0x0000FF00)
                           && (ddsh.ddspf.dwBBitMask == 0x000000FF)
                           && (ddsh.ddspf.dwABitMask == 0xFF000000)) {
                    // We support D3D's A8R8G8B8, which is actually BGRA in linear
                    // memory, need to be
                    image->format = qt3ds::render::NVRenderTextureFormats::RGBA8;
                    image->components = 4;
                    image->componentFormat = qt3ds::render::NVRenderComponentTypes::QT3DSU8;
                    image->bytesPerPixel = 4;
                    image->alpha = 1;
                    image->compressed = 0;
                    needsBGRASwap = true;
                    dib->format = qt3ds::render::NVRenderTextureFormats::RGBA8;
                } else if ((ddsh.ddspf.dwRGBBitCount == 16) && (ddsh.ddspf.dwRBitMask == 0x0000F800)
                           && (ddsh.ddspf.dwGBitMask == 0x000007E0)
                           && (ddsh.ddspf.dwBBitMask == 0x0000001F)
                           && (ddsh.ddspf.dwABitMask == 0x00000000)) {
                    // We support D3D's R5G6B5, which is actually RGB in linear
                    // memory.  It is equivalent to GL's GL_UNSIGNED_SHORT_5_6_5
                    image->format = qt3ds::render::NVRenderTextureFormats::RGB8;
                    image->components = 3;
                    image->alpha = 0;
                    image->componentFormat = qt3ds::render::NVRenderComponentTypes::QT3DSU16;
                    image->bytesPerPixel = 2;
                    image->compressed = 0;
                    dib->format = qt3ds::render::NVRenderTextureFormats::RGB8;
                } else if ((ddsh.ddspf.dwRGBBitCount == 8) && (ddsh.ddspf.dwRBitMask == 0x00000000)
                           && (ddsh.ddspf.dwGBitMask == 0x00000000)
                           && (ddsh.ddspf.dwBBitMask == 0x00000000)
                           && (ddsh.ddspf.dwABitMask == 0x000000FF)) {
                    // We support D3D's A8
                    image->format = qt3ds::render::NVRenderTextureFormats::Alpha8;
                    image->components = 1;
                    image->alpha = 1;
                    image->componentFormat = qt3ds::render::NVRenderComponentTypes::QT3DSU8;
                    image->bytesPerPixel = 1;
                    image->compressed = 0;
                    dib->format = qt3ds::render::NVRenderTextureFormats::Alpha8;
                } else if ((ddsh.ddspf.dwRGBBitCount == 8) && (ddsh.ddspf.dwRBitMask == 0x000000FF)
                           && (ddsh.ddspf.dwGBitMask == 0x00000000)
                           && (ddsh.ddspf.dwBBitMask == 0x00000000)
                           && (ddsh.ddspf.dwABitMask == 0x00000000)) {
                    // We support D3D's L8 (flagged as 8 bits of red only)
                    image->format = qt3ds::render::NVRenderTextureFormats::Luminance8;
                    image->components = 1;
                    image->alpha = 0;
                    image->componentFormat = qt3ds::render::NVRenderComponentTypes::QT3DSU8;
                    image->bytesPerPixel = 1;
                    image->compressed = 0;
                    dib->format = qt3ds::render::NVRenderTextureFormats::Luminance8;
                } else if ((ddsh.ddspf.dwRGBBitCount == 16) && (ddsh.ddspf.dwRBitMask == 0x000000FF)
                           && (ddsh.ddspf.dwGBitMask == 0x00000000)
                           && (ddsh.ddspf.dwBBitMask == 0x00000000)
                           && (ddsh.ddspf.dwABitMask == 0x0000FF00)) {
                    // We support D3D's A8L8 (flagged as 8 bits of red and 8 bits of alpha)
                    image->format = qt3ds::render::NVRenderTextureFormats::LuminanceAlpha8;
                    image->components = 2;
                    image->alpha = 1;
                    image->componentFormat = qt3ds::render::NVRenderComponentTypes::QT3DSU8;
                    image->bytesPerPixel = 2;
                    image->compressed = 0;
                    dib->format = qt3ds::render::NVRenderTextureFormats::LuminanceAlpha8;
                } else {
                    throw "not a DXTC or supported RGB(A) format image";
                }
            }

            // detect flagging to indicate this texture was stored in a y-inverted fashion
            if (!(ddsh.dwFlags & DDS_LINEARSIZE)) {
                if (ddsh.dwPitchOrLinearSize == DDS_MAGIC_FLIPPED) {
                    isAllreadyFlipped = true;
                }
            }

            flipVertical = (isAllreadyFlipped != (flipVertical ? true : false)) ? 1 : 0;

            // store primary surface width/height/numMipmaps
            image->width = ddsh.dwWidth;
            image->height = ddsh.dwHeight;
            image->numMipmaps = ddsh.dwFlags & DDS_MIPMAPCOUNT ? ddsh.dwMipMapCount : 1;

            if (image->numMipmaps > QT3DS_DDS_MAX_MIPMAPS) {
                throw "Too many mipmaps: max 16";
            }

            // allocate the meta datablock for all mip storage.
            Qt3DSDDSAllocDataBlock(io, image);
            if (image->dataBlock == NULL) {
                throw "Failed to allocate memory for image data storage";
            }

            dib->width = image->width;
            dib->height = image->height;
            dib->dds = image;

            QT3DSI32 faces = image->cubemap ? 6 : 1;

            QT3DSI32 index = 0;
            for (QT3DSI32 j = 0; j < faces; j++) {
                // load all surfaces for the image
                QT3DSI32 width = image->width;
                QT3DSI32 height = image->height;

                for (QT3DSI32 i = 0; i < image->numMipmaps; i++) {
                    // Get the size, read in the data.
                    inStream.Read(
                        NVDataRef<QT3DSU8>((QT3DSU8 *)image->data[index], (QT3DSU32)image->size[index]));

                    // Flip in Y for OpenGL if needed
                    if (flipVertical)
                        flip_data_vertical(io, (QT3DSI8 *)image->data[index], width, height, image);

                    // shrink to next power of 2
                    width >>= 1;
                    height >>= 1;

                    if (!width)
                        width = 1;

                    if (!height)
                        height = 1;

                    // make sure DXT isn't <4 on a side...
                    if (image->compressed) {
                        if (width < 4)
                            width = 4;
                        if (height < 4)
                            height = 4;
                    }

                    index++;
                }
            }

            if (needsBGRASwap) {
                QT3DSI32 index = 0;
                QT3DSI32 k;

                for (k = 0; k < faces; k++) {
                    QT3DSI32 width = image->width;
                    QT3DSI32 height = image->height;

                    for (QT3DSI32 i = 0; i < image->numMipmaps; i++) {
                        QT3DSI8 *data = (QT3DSI8 *)(image->data[index]);
                        QT3DSI32 pixels = width * height;
                        QT3DSI32 j;

                        for (j = 0; j < pixels; j++) {
                            QT3DSI8 temp = data[0];
                            data[0] = data[2];
                            data[2] = temp;

                            data += 4;
                        }

                        // shrink to next power of 2
                        width >>= 1;
                        height >>= 1;

                        if (!width)
                            width = 1;

                        if (!height)
                            height = 1;

                        index++;
                    }
                }
            }
        } catch (const char *message) {
            if (image) {
                if (image->dataBlock)
                    QT3DS_FREE(io->m_Allocator, image->dataBlock);

                QT3DS_FREE(io->m_Allocator, image);
            }
            if (dib) {
                FreeImage_Unload(dib);
            }
            if (message) {
                FreeImage_OutputMessageProc(s_exception_string, message, io);
            }
        }

        return dib;
    }

    SLoadedTexture *SLoadedTexture::LoadDDS(IInStream &inStream, QT3DSI32 flipVertical,
                                            NVFoundationBase &inFnd)
    {
        FreeImageIO theIO(inFnd.getAllocator(), inFnd);
        SLoadedTexture *retval = DoLoadDDS(&theIO, inStream, flipVertical);

        return retval;
    }
}
}

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
#ifndef ENDIANH
#define ENDIANH
#include "ByteSwap.h"

#include <QtEndian>

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#define UIC_BIG_ENDIAN
#else
#define UIC_LITTLE_ENDIAN
#endif

#ifdef UIC_LITTLE_ENDIAN
#define LTOH2(in, out) out = in;
#define HTOL2(in, out) out = in;

#define LTOH4(in, out) out = in;
#define HTOL4(in, out) out = in;

#define LTOH8(in, out) out = in;
#define HTOL8(in, out) out = in;

#define BTOH2(in, out) SWAP2BYTES(in, out)
#define HTOB2(in, out) SWAP2BYTES(in, out)

#define BTOH4(in, out) SWAP4BYTES(in, out)
#define HTOB4(in, out) SWAP4BYTES(in, out)

#define BTOH8(in, out) SWAP8BYTES(in, out)
#define HTOB8(in, out) SWAP8BYTES(in, out)

#endif
#ifdef UIC_BIG_ENDIAN
#undef LTOH2
#define LTOH2(in, out) SWAP2BYTES(in, out)
#undef HTOL2
#define HTOL2(in, out) SWAP2BYTES(in, out)

#undef LTOH4
#define LTOH4(in, out) SWAP4BYTES(in, out)
#undef HTOL4
#define HTOL4(in, out) SWAP4BYTES(in, out)

#undef LTOH8
#define LTOH8(in, out) SWAP8BYTES(in, out)
#undef HTOL8
#define HTOL8(in, out) SWAP8BYTES(in, out)

#undef BTOH2
#define BTOH2(in, out) out = in;
#undef HTOB2
#define HTOB2(in, out) out = in;

#undef BTOH4
#define BTOH4(in, out) out = in;
#undef HTOB4
#define HTOB4(in, out) out = in;

#undef BTOH8
#define BTOH8(in, out) out = in;
#undef HTOB8
#define HTOB8(in, out) out = in;
#endif // ENDIANH

#if !defined(UIC_LITTLE_ENDIAN) && !defined(UIC_BIG_ENDIAN)
#error You must define either UIC_LITTLE_ENDIAN or UIC_BIG_ENDIAN
#endif

#endif
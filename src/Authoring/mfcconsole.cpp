/****************************************************************************
**
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

#include "stdafx.h"
#if 0
#ifdef _DEBUG

#pragma comment(linker, "/SUBSYSTEM:CONSOLE")

#include <string.h>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>

extern "C"
{
	int PASCAL WinMain(HINSTANCE inst,HINSTANCE dumb,LPSTR param,int show);
};

int main(int ac,char *av[])
{
	char buf[256];
	int i;
	HINSTANCE inst;

	inst = (HINSTANCE)GetModuleHandle(NULL);

	buf[0] = 0;
	for(i = 1; i < ac; i++)
	{
		strcat_s(buf, 256, av[i]);
		strcat_s(buf, 256, " ");
		//strcat(buf,av[i]);
		//strcat(buf," ");
	}

	return WinMain(inst, NULL, buf, SW_SHOWNORMAL);
}

#endif _DEBUG
#endif

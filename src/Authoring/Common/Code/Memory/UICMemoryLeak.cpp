/****************************************************************************
**
** Copyright (C) 1999-2005 NVIDIA Corporation.
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
#include "stdafx.h"

//==============================================================================
//	Includes
//==============================================================================
#include "UICMemoryLeak.h"

//==============================================================================
//	Namespace
//==============================================================================
using namespace Q3DStudio;

//==============================================================================
//	From C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\crt\src\dbgint.h
//==============================================================================

#define nNoMansLandSize 4
typedef struct _CrtMemBlockHeader
{
    struct _CrtMemBlockHeader *pBlockHeaderNext;
    struct _CrtMemBlockHeader *pBlockHeaderPrev;
    char *szFileName;
    int nLine;
#ifdef _WIN64
    /* These items are reversed on Win64 to eliminate gaps in the struct
     * and ensure that sizeof(struct)%16 == 0, so 16-byte alignment is
     * maintained in the debug heap.
     */
    int nBlockUse;
    size_t nDataSize;
#else /* _WIN64 */
    size_t nDataSize;
    int nBlockUse;
#endif /* _WIN64 */
    long lRequest;
    unsigned char gap[nNoMansLandSize];
    /* followed by:
     *  unsigned char           data[nDataSize];
     *  unsigned char           anotherGap[nNoMansLandSize];
     */
} _CrtMemBlockHeader;

#define pbData(pblock) ((unsigned char *)((_CrtMemBlockHeader *)pblock + 1))

//==============================================================================
//	Global Variables
//==============================================================================

bool CMemoryLeak::s_ShowDialogOnExit = false;
bool CMemoryLeak::s_DumpXMLOnExit = false;
char CMemoryLeak::s_XMLFileName[_MAX_PATH] = "UICMemoryLeaks.xml";
size_t CMemoryLeak::s_MaxDumpSize = 20;

// The one and only object instance. This is placed in the "compiler"
// initialization area, so that it gets constructed during C runtime
// initialization and before any user global objects are constructed.
// Also, disable the warning about us using the "compiler"
// initialization area. Otherwise, this object doesn't get constructed
// early enough to get a good spot in the "onexit" list and doesn't
// accurately dump memeory leaks.
#ifdef _DEBUG
#ifdef WIN32
#pragma warning(disable : 4074) // initializers put in compiler reserved initialization area
#pragma init_seg(compiler)
#endif
static CMemoryLeak g_MemoryLeakGlobalVar;
#endif

//==============================================================================
//	Static Methods
//==============================================================================

/**
 *	Called on exit.
 *
 *	NOTE: Be very careful with what you add to this method, it may be called
 *	AFTER facilities in the C runtime have been destroyed. This is close to the
 *	metal, be careful you don't get shocked!
 */
int CMemoryLeak::MemoryDumpCallback()
{
#ifdef _DEBUG

    // Only dump memory leaks if there are in fact leaks
#ifdef KDAB_TEMPORARILY_REMOVED
    _CrtMemState msNow;
    _CrtMemCheckpoint(&msNow);

    // This is the only way I could figure out how to get hold of
    // _pFirstBlock. This could change with other implementations
    // of the C Runtime.
    _CrtMemBlockHeader *_pFirstBlock = msNow.pBlockHeader;

    size_t theMemoryLeakTotal = 0;
    size_t theMemoryBytesTotal = 0;

    // We're only interested in NORMAL blocks
    if (msNow.lCounts[_NORMAL_BLOCK] != 0) {
        FILE *theFilePtr(NULL);
        XMLDumpInitialize(theFilePtr);

        // Loop through all the blocks
        for (_CrtMemBlockHeader *pHead = _pFirstBlock; pHead != NULL;
             pHead = pHead->pBlockHeaderNext) {
            if (pHead->nBlockUse == _NORMAL_BLOCK) {
                ++theMemoryLeakTotal;
                theMemoryBytesTotal += pHead->nDataSize;

                XMLDumpLeak(theFilePtr, pHead);
            }
        }

        XMLDumpClose(theFilePtr, theMemoryLeakTotal, theMemoryBytesTotal);

        // Format the string
        char theFormatString[_MAX_PATH];
        _snprintf(theFormatString, sizeof(theFormatString),
                  "Detected %ld memory leak(s) for a total of %ld bytes leaked.\n",
                  theMemoryLeakTotal, theMemoryBytesTotal);

        // Output to debugger
        ::OutputDebugStringA("*** ");
        ::OutputDebugStringA(theFormatString);

        // Display dialog
        if (s_ShowDialogOnExit)
            ::MessageBoxA(NULL, theFormatString, "Memory Leak Warning", MB_OK);
    }
#endif

#endif

    return 0;
}

/**
 *	Set the flag that controls whether or not we display a dialog box
 *	when exiting the application with memory leaks.
 */
void CMemoryLeak::SetShowDialogOnExit(bool inValue)
{
    s_ShowDialogOnExit = inValue;
}

/**
 *	Set the flag that controls whether or not we dump an XML file
 *	when exiting the application with memory leaks.
 */
void CMemoryLeak::SetDumpXMLOnExit(bool inValue)
{
    s_DumpXMLOnExit = inValue;
}

/**
 *	Set the flag that controls whether or not we dump an XML file
 *	when exiting the application with memory leaks.
 */
void CMemoryLeak::SetXMLFileName(const char *inValue)
{
    strncpy(s_XMLFileName, inValue, sizeof(s_XMLFileName));
    s_XMLFileName[sizeof(s_XMLFileName) - 1] = 0;
}

//==============================================================================
//	Internal Methods
//==============================================================================

/**
 *	Constructor
 */
CMemoryLeak::CMemoryLeak()
{
#ifdef KDAB_TEMPORARILY_REMOVED
    ::_onexit(CMemoryLeak::MemoryDumpCallback);
#endif
}

/**
 *	Opens and sets up the file ptr.
 *	Write some tags.
 */
void CMemoryLeak::XMLDumpInitialize(FILE *&outFilePtr)
{
    if (s_DumpXMLOnExit) {
        outFilePtr = ::fopen(s_XMLFileName, "w");

        if (outFilePtr) {
            ::fprintf(outFilePtr, "<?xml version=\"1.0\" encoding=\"iso-8859-1\"?>\n");
            ::fprintf(outFilePtr, "<MemoryLeaks>\n");
            ::fprintf(outFilePtr, "\t<Leaks>\n");
        }
    }
}

/**
 *	Dumps out first X bytes of the leak as a CDATA tag.
 */
#ifdef KDAB_TEMPORARILY_REMOVED
void CMemoryLeak::XMLDumpLeak(FILE *inFilePtr, _CrtMemBlockHeader *inBlockHdr)
{
    if (s_DumpXMLOnExit && inFilePtr) {
        size_t theDumpSize(::MIN(s_MaxDumpSize, inBlockHdr->nDataSize));

        ::fprintf(inFilePtr, "\t\t<Leak LeakSize = \"%d\" DumpSize = \"%d\">\n\t\t\t<![CDATA[",
                  inBlockHdr->nDataSize, theDumpSize);

        ::fwrite(pbData(inBlockHdr), 1, theDumpSize, inFilePtr);

        ::fprintf(inFilePtr, "]]>\n\t\t</Leak>\n");
    }
}
#endif

/**
 *	Write some tags. Flushes and closes the file.
 */
void CMemoryLeak::XMLDumpClose(FILE *inFilePtr, size_t inMemoryLeakTotal, size_t inMemoryBytesTotal)
{
    if (s_DumpXMLOnExit && inFilePtr) {
        ::fprintf(inFilePtr, "\t</Leaks>\n\t<Summary\n ");
        ::fprintf(inFilePtr, "\t\tTotalNumberOfLeaks = \"%lu\"\n", inMemoryLeakTotal);
        ::fprintf(inFilePtr, "\t\tTotalBytesLeaked = \"%lu\"\n", inMemoryBytesTotal);
        ::fprintf(inFilePtr, "\t/>\n</MemoryLeaks>");

        ::fflush(inFilePtr);
        ::fclose(inFilePtr);
    }
}

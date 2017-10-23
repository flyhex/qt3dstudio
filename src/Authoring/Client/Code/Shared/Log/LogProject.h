/****************************************************************************
**
** Copyright (C) 2016 NVIDIA Corporation.
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

#pragma warning(disable : 4049) /* more than 64k source lines */

/* this ALWAYS GENERATED file contains the definitions for the interfaces */

/* File created by MIDL compiler version 5.03.0280 */
/* at Thu Nov 01 17:29:05 2001
 */
//@@MIDL_FILE_HEADING(  )

/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 440
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include <winsock2.h>
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __LogProject_h__
#define __LogProject_h__

/* Forward Declarations */

#ifndef __IQt3DSLog_FWD_DEFINED__
#define __IQt3DSLog_FWD_DEFINED__
typedef interface IQt3DSLog IQt3DSLog;
#endif /* __IQt3DSLog_FWD_DEFINED__ */

#ifndef __IQt3DSLog2_FWD_DEFINED__
#define __IQt3DSLog2_FWD_DEFINED__
typedef interface IQt3DSLog2 IQt3DSLog2;
#endif /* __IQt3DSLog2_FWD_DEFINED__ */

#ifndef __QT3DS_Log_FWD_DEFINED__
#define __QT3DS_Log_FWD_DEFINED__

#ifdef __cplusplus
typedef class Qt3DSLog Qt3DSLog;
#else
typedef struct Qt3DSLog Qt3DSLog;
#endif /* __cplusplus */

#endif /* __QT3DS_Log_FWD_DEFINED__ */

#ifndef __QT3DS_Log2_FWD_DEFINED__
#define __QT3DS_Log2_FWD_DEFINED__

#ifdef __cplusplus
typedef class Qt3DSLog2 Qt3DSLog2;
#else
typedef struct Qt3DSLog2 Qt3DSLog2;
#endif /* __cplusplus */

#endif /* __QT3DS_Log2_FWD_DEFINED__ */

/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C" {
#endif

void __RPC_FAR *__RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free(void __RPC_FAR *);

#ifndef __IQt3DSLog_INTERFACE_DEFINED__
#define __IQt3DSLog_INTERFACE_DEFINED__

/* interface IQt3DSLog */
/* [unique][helpstring][dual][uuid][object] */

EXTERN_C const IID IID_IQt3DSLog;

#if defined(__cplusplus) && !defined(CINTERFACE)

MIDL_INTERFACE("B6F844EB-07AF-4CF1-9350-E4A2A6576E0E")
IQt3DSLog : public IDispatch
{
public:
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddEntry(
        /* [in] */ DWORD inLogType,
        /* [in] */ BYTE __RPC_FAR * inLogEntry) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE ReloadPreferences(void) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Terminate(void) = 0;
};

#else /* C style interface */

typedef struct IQt3DSLogVtbl
{
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *QueryInterface)
    (IQt3DSLog __RPC_FAR *This,
     /* [in] */ REFIID riid,
     /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

    ULONG(STDMETHODCALLTYPE __RPC_FAR *AddRef)(IQt3DSLog __RPC_FAR *This);

    ULONG(STDMETHODCALLTYPE __RPC_FAR *Release)(IQt3DSLog __RPC_FAR *This);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount)
    (IQt3DSLog __RPC_FAR *This,
     /* [out] */ UINT __RPC_FAR *pctinfo);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo)
    (IQt3DSLog __RPC_FAR *This,
     /* [in] */ UINT iTInfo,
     /* [in] */ LCID lcid,
     /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames)
    (IQt3DSLog __RPC_FAR *This,
     /* [in] */ REFIID riid,
     /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
     /* [in] */ UINT cNames,
     /* [in] */ LCID lcid,
     /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

    /* [local] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *Invoke)(
        IQt3DSLog __RPC_FAR *This,
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *AddEntry)(
        IQt3DSLog __RPC_FAR *This,
        /* [in] */ DWORD inLogType,
        /* [in] */ BYTE __RPC_FAR *inLogEntry);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *ReloadPreferences)(IQt3DSLog __RPC_FAR *This);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *Terminate)(IQt3DSLog __RPC_FAR *This);

    END_INTERFACE
} IQt3DSLogVtbl;

interface IQt3DSLog
{
    CONST_VTBL struct IQt3DSLogVtbl __RPC_FAR *lpVtbl;
};

#ifdef COBJMACROS

#define IQt3DSLog_QueryInterface(This, riid, ppvObject)                                              \
    (This)->lpVtbl->QueryInterface(This, riid, ppvObject)

#define IQt3DSLog_AddRef(This) (This)->lpVtbl->AddRef(This)

#define IQt3DSLog_Release(This) (This)->lpVtbl->Release(This)

#define IQt3DSLog_GetTypeInfoCount(This, pctinfo) (This)->lpVtbl->GetTypeInfoCount(This, pctinfo)

#define IQt3DSLog_GetTypeInfo(This, iTInfo, lcid, ppTInfo)                                           \
    (This)->lpVtbl->GetTypeInfo(This, iTInfo, lcid, ppTInfo)

#define IQt3DSLog_GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)                       \
    (This)->lpVtbl->GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)

#define IQt3DSLog_Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult,            \
                       pExcepInfo, puArgErr)                                                       \
    (This)->lpVtbl->Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult,        \
                           pExcepInfo, puArgErr)

#define IQt3DSLog_AddEntry(This, inLogType, inLogEntry)                                              \
    (This)->lpVtbl->AddEntry(This, inLogType, inLogEntry)

#define IQt3DSLog_ReloadPreferences(This) (This)->lpVtbl->ReloadPreferences(This)

#define IQt3DSLog_Terminate(This) (This)->lpVtbl->Terminate(This)

#endif /* COBJMACROS */

#endif /* C style interface */

/* [id] */ HRESULT STDMETHODCALLTYPE IQt3DSLog_AddEntry_Proxy(IQt3DSLog __RPC_FAR *This,
                                                            /* [in] */ DWORD inLogType,
                                                            /* [in] */ BYTE __RPC_FAR *inLogEntry);

void __RPC_STUB IQt3DSLog_AddEntry_Stub(IRpcStubBuffer *This, IRpcChannelBuffer *_pRpcChannelBuffer,
                                      PRPC_MESSAGE _pRpcMessage, DWORD *_pdwStubPhase);

/* [id] */ HRESULT STDMETHODCALLTYPE IQt3DSLog_ReloadPreferences_Proxy(IQt3DSLog __RPC_FAR *This);

void __RPC_STUB IQt3DSLog_ReloadPreferences_Stub(IRpcStubBuffer *This,
                                               IRpcChannelBuffer *_pRpcChannelBuffer,
                                               PRPC_MESSAGE _pRpcMessage, DWORD *_pdwStubPhase);

/* [id] */ HRESULT STDMETHODCALLTYPE IQt3DSLog_Terminate_Proxy(IQt3DSLog __RPC_FAR *This);

void __RPC_STUB IQt3DSLog_Terminate_Stub(IRpcStubBuffer *This, IRpcChannelBuffer *_pRpcChannelBuffer,
                                       PRPC_MESSAGE _pRpcMessage, DWORD *_pdwStubPhase);

#endif /* __IQt3DSLog_INTERFACE_DEFINED__ */

#ifndef __IQt3DSLog2_INTERFACE_DEFINED__
#define __IQt3DSLog2_INTERFACE_DEFINED__

/* interface IQt3DSLog2 */
/* [unique][helpstring][dual][uuid][object] */

EXTERN_C const IID IID_IQt3DSLog2;

#if defined(__cplusplus) && !defined(CINTERFACE)

MIDL_INTERFACE("18725949-89C4-460c-A092-75520BFFB542")
IQt3DSLog2 : public IQt3DSLog
{
public:
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddEntryIndirect(
        /* [in] */ LONG inStructPtr) = 0;
};

#else /* C style interface */

typedef struct IQt3DSLog2Vtbl
{
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *QueryInterface)
    (IQt3DSLog2 __RPC_FAR *This,
     /* [in] */ REFIID riid,
     /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

    ULONG(STDMETHODCALLTYPE __RPC_FAR *AddRef)(IQt3DSLog2 __RPC_FAR *This);

    ULONG(STDMETHODCALLTYPE __RPC_FAR *Release)(IQt3DSLog2 __RPC_FAR *This);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount)
    (IQt3DSLog2 __RPC_FAR *This,
     /* [out] */ UINT __RPC_FAR *pctinfo);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo)
    (IQt3DSLog2 __RPC_FAR *This,
     /* [in] */ UINT iTInfo,
     /* [in] */ LCID lcid,
     /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames)
    (IQt3DSLog2 __RPC_FAR *This,
     /* [in] */ REFIID riid,
     /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
     /* [in] */ UINT cNames,
     /* [in] */ LCID lcid,
     /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

    /* [local] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *Invoke)(
        IQt3DSLog2 __RPC_FAR *This,
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *AddEntry)(
        IQt3DSLog2 __RPC_FAR *This,
        /* [in] */ DWORD inLogType,
        /* [in] */ BYTE __RPC_FAR *inLogEntry);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *ReloadPreferences)(IQt3DSLog2 __RPC_FAR *This);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *Terminate)(IQt3DSLog2 __RPC_FAR *This);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *AddEntryIndirect)(IQt3DSLog2 __RPC_FAR *This,
                                                                      /* [in] */ LONG inStructPtr);

    END_INTERFACE
} IQt3DSLog2Vtbl;

interface IQt3DSLog2
{
    CONST_VTBL struct IQt3DSLog2Vtbl __RPC_FAR *lpVtbl;
};

#ifdef COBJMACROS

#define IQt3DSLog2_QueryInterface(This, riid, ppvObject)                                             \
    (This)->lpVtbl->QueryInterface(This, riid, ppvObject)

#define IQt3DSLog2_AddRef(This) (This)->lpVtbl->AddRef(This)

#define IQt3DSLog2_Release(This) (This)->lpVtbl->Release(This)

#define IQt3DSLog2_GetTypeInfoCount(This, pctinfo) (This)->lpVtbl->GetTypeInfoCount(This, pctinfo)

#define IQt3DSLog2_GetTypeInfo(This, iTInfo, lcid, ppTInfo)                                          \
    (This)->lpVtbl->GetTypeInfo(This, iTInfo, lcid, ppTInfo)

#define IQt3DSLog2_GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)                      \
    (This)->lpVtbl->GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)

#define IQt3DSLog2_Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult,           \
                        pExcepInfo, puArgErr)                                                      \
    (This)->lpVtbl->Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult,        \
                           pExcepInfo, puArgErr)

#define IQt3DSLog2_AddEntry(This, inLogType, inLogEntry)                                             \
    (This)->lpVtbl->AddEntry(This, inLogType, inLogEntry)

#define IQt3DSLog2_ReloadPreferences(This) (This)->lpVtbl->ReloadPreferences(This)

#define IQt3DSLog2_Terminate(This) (This)->lpVtbl->Terminate(This)

#define IQt3DSLog2_AddEntryIndirect(This, inStructPtr)                                               \
    (This)->lpVtbl->AddEntryIndirect(This, inStructPtr)

#endif /* COBJMACROS */

#endif /* C style interface */

/* [id] */ HRESULT STDMETHODCALLTYPE IQt3DSLog2_AddEntryIndirect_Proxy(IQt3DSLog2 __RPC_FAR *This,
                                                                     /* [in] */ LONG inStructPtr);

void __RPC_STUB IQt3DSLog2_AddEntryIndirect_Stub(IRpcStubBuffer *This,
                                               IRpcChannelBuffer *_pRpcChannelBuffer,
                                               PRPC_MESSAGE _pRpcMessage, DWORD *_pdwStubPhase);

#endif /* __IQt3DSLog2_INTERFACE_DEFINED__ */

#ifndef __LOGPROJECTLib_LIBRARY_DEFINED__
#define __LOGPROJECTLib_LIBRARY_DEFINED__

/* library LOGPROJECTLib */
/* [helpstring][version][uuid] */

EXTERN_C const IID LIBID_LOGPROJECTLib;

EXTERN_C const CLSID CLSID_Qt3DSLog;

#ifdef __cplusplus

class DECLSPEC_UUID("DE411D6F-2BDA-444C-AC87-9171BAFD9E99") Qt3DSLog;
#endif

EXTERN_C const CLSID CLSID_Qt3DSLog2;

#ifdef __cplusplus

class DECLSPEC_UUID("4691F58A-9856-43c6-9269-7A8987AFC715") Qt3DSLog2;
#endif
#endif /* __LOGPROJECTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif

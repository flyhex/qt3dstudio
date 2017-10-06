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

#ifndef __IUICLog_FWD_DEFINED__
#define __IUICLog_FWD_DEFINED__
typedef interface IUICLog IUICLog;
#endif /* __IUICLog_FWD_DEFINED__ */

#ifndef __IUICLog2_FWD_DEFINED__
#define __IUICLog2_FWD_DEFINED__
typedef interface IUICLog2 IUICLog2;
#endif /* __IUICLog2_FWD_DEFINED__ */

#ifndef __UICLog_FWD_DEFINED__
#define __UICLog_FWD_DEFINED__

#ifdef __cplusplus
typedef class UICLog UICLog;
#else
typedef struct UICLog UICLog;
#endif /* __cplusplus */

#endif /* __UICLog_FWD_DEFINED__ */

#ifndef __UICLog2_FWD_DEFINED__
#define __UICLog2_FWD_DEFINED__

#ifdef __cplusplus
typedef class UICLog2 UICLog2;
#else
typedef struct UICLog2 UICLog2;
#endif /* __cplusplus */

#endif /* __UICLog2_FWD_DEFINED__ */

/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C" {
#endif

void __RPC_FAR *__RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free(void __RPC_FAR *);

#ifndef __IUICLog_INTERFACE_DEFINED__
#define __IUICLog_INTERFACE_DEFINED__

/* interface IUICLog */
/* [unique][helpstring][dual][uuid][object] */

EXTERN_C const IID IID_IUICLog;

#if defined(__cplusplus) && !defined(CINTERFACE)

MIDL_INTERFACE("B6F844EB-07AF-4CF1-9350-E4A2A6576E0E")
IUICLog : public IDispatch
{
public:
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddEntry(
        /* [in] */ DWORD inLogType,
        /* [in] */ BYTE __RPC_FAR * inLogEntry) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE ReloadPreferences(void) = 0;

    virtual /* [id] */ HRESULT STDMETHODCALLTYPE Terminate(void) = 0;
};

#else /* C style interface */

typedef struct IUICLogVtbl
{
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *QueryInterface)
    (IUICLog __RPC_FAR *This,
     /* [in] */ REFIID riid,
     /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

    ULONG(STDMETHODCALLTYPE __RPC_FAR *AddRef)(IUICLog __RPC_FAR *This);

    ULONG(STDMETHODCALLTYPE __RPC_FAR *Release)(IUICLog __RPC_FAR *This);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount)
    (IUICLog __RPC_FAR *This,
     /* [out] */ UINT __RPC_FAR *pctinfo);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo)
    (IUICLog __RPC_FAR *This,
     /* [in] */ UINT iTInfo,
     /* [in] */ LCID lcid,
     /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames)
    (IUICLog __RPC_FAR *This,
     /* [in] */ REFIID riid,
     /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
     /* [in] */ UINT cNames,
     /* [in] */ LCID lcid,
     /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

    /* [local] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *Invoke)(
        IUICLog __RPC_FAR *This,
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *AddEntry)(
        IUICLog __RPC_FAR *This,
        /* [in] */ DWORD inLogType,
        /* [in] */ BYTE __RPC_FAR *inLogEntry);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *ReloadPreferences)(IUICLog __RPC_FAR *This);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *Terminate)(IUICLog __RPC_FAR *This);

    END_INTERFACE
} IUICLogVtbl;

interface IUICLog
{
    CONST_VTBL struct IUICLogVtbl __RPC_FAR *lpVtbl;
};

#ifdef COBJMACROS

#define IUICLog_QueryInterface(This, riid, ppvObject)                                              \
    (This)->lpVtbl->QueryInterface(This, riid, ppvObject)

#define IUICLog_AddRef(This) (This)->lpVtbl->AddRef(This)

#define IUICLog_Release(This) (This)->lpVtbl->Release(This)

#define IUICLog_GetTypeInfoCount(This, pctinfo) (This)->lpVtbl->GetTypeInfoCount(This, pctinfo)

#define IUICLog_GetTypeInfo(This, iTInfo, lcid, ppTInfo)                                           \
    (This)->lpVtbl->GetTypeInfo(This, iTInfo, lcid, ppTInfo)

#define IUICLog_GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)                       \
    (This)->lpVtbl->GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)

#define IUICLog_Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult,            \
                       pExcepInfo, puArgErr)                                                       \
    (This)->lpVtbl->Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult,        \
                           pExcepInfo, puArgErr)

#define IUICLog_AddEntry(This, inLogType, inLogEntry)                                              \
    (This)->lpVtbl->AddEntry(This, inLogType, inLogEntry)

#define IUICLog_ReloadPreferences(This) (This)->lpVtbl->ReloadPreferences(This)

#define IUICLog_Terminate(This) (This)->lpVtbl->Terminate(This)

#endif /* COBJMACROS */

#endif /* C style interface */

/* [id] */ HRESULT STDMETHODCALLTYPE IUICLog_AddEntry_Proxy(IUICLog __RPC_FAR *This,
                                                            /* [in] */ DWORD inLogType,
                                                            /* [in] */ BYTE __RPC_FAR *inLogEntry);

void __RPC_STUB IUICLog_AddEntry_Stub(IRpcStubBuffer *This, IRpcChannelBuffer *_pRpcChannelBuffer,
                                      PRPC_MESSAGE _pRpcMessage, DWORD *_pdwStubPhase);

/* [id] */ HRESULT STDMETHODCALLTYPE IUICLog_ReloadPreferences_Proxy(IUICLog __RPC_FAR *This);

void __RPC_STUB IUICLog_ReloadPreferences_Stub(IRpcStubBuffer *This,
                                               IRpcChannelBuffer *_pRpcChannelBuffer,
                                               PRPC_MESSAGE _pRpcMessage, DWORD *_pdwStubPhase);

/* [id] */ HRESULT STDMETHODCALLTYPE IUICLog_Terminate_Proxy(IUICLog __RPC_FAR *This);

void __RPC_STUB IUICLog_Terminate_Stub(IRpcStubBuffer *This, IRpcChannelBuffer *_pRpcChannelBuffer,
                                       PRPC_MESSAGE _pRpcMessage, DWORD *_pdwStubPhase);

#endif /* __IUICLog_INTERFACE_DEFINED__ */

#ifndef __IUICLog2_INTERFACE_DEFINED__
#define __IUICLog2_INTERFACE_DEFINED__

/* interface IUICLog2 */
/* [unique][helpstring][dual][uuid][object] */

EXTERN_C const IID IID_IUICLog2;

#if defined(__cplusplus) && !defined(CINTERFACE)

MIDL_INTERFACE("18725949-89C4-460c-A092-75520BFFB542")
IUICLog2 : public IUICLog
{
public:
    virtual /* [id] */ HRESULT STDMETHODCALLTYPE AddEntryIndirect(
        /* [in] */ LONG inStructPtr) = 0;
};

#else /* C style interface */

typedef struct IUICLog2Vtbl
{
    BEGIN_INTERFACE

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *QueryInterface)
    (IUICLog2 __RPC_FAR *This,
     /* [in] */ REFIID riid,
     /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);

    ULONG(STDMETHODCALLTYPE __RPC_FAR *AddRef)(IUICLog2 __RPC_FAR *This);

    ULONG(STDMETHODCALLTYPE __RPC_FAR *Release)(IUICLog2 __RPC_FAR *This);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetTypeInfoCount)
    (IUICLog2 __RPC_FAR *This,
     /* [out] */ UINT __RPC_FAR *pctinfo);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetTypeInfo)
    (IUICLog2 __RPC_FAR *This,
     /* [in] */ UINT iTInfo,
     /* [in] */ LCID lcid,
     /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTInfo);

    HRESULT(STDMETHODCALLTYPE __RPC_FAR *GetIDsOfNames)
    (IUICLog2 __RPC_FAR *This,
     /* [in] */ REFIID riid,
     /* [size_is][in] */ LPOLESTR __RPC_FAR *rgszNames,
     /* [in] */ UINT cNames,
     /* [in] */ LCID lcid,
     /* [size_is][out] */ DISPID __RPC_FAR *rgDispId);

    /* [local] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *Invoke)(
        IUICLog2 __RPC_FAR *This,
        /* [in] */ DISPID dispIdMember,
        /* [in] */ REFIID riid,
        /* [in] */ LCID lcid,
        /* [in] */ WORD wFlags,
        /* [out][in] */ DISPPARAMS __RPC_FAR *pDispParams,
        /* [out] */ VARIANT __RPC_FAR *pVarResult,
        /* [out] */ EXCEPINFO __RPC_FAR *pExcepInfo,
        /* [out] */ UINT __RPC_FAR *puArgErr);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *AddEntry)(
        IUICLog2 __RPC_FAR *This,
        /* [in] */ DWORD inLogType,
        /* [in] */ BYTE __RPC_FAR *inLogEntry);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *ReloadPreferences)(IUICLog2 __RPC_FAR *This);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *Terminate)(IUICLog2 __RPC_FAR *This);

    /* [id] */ HRESULT(STDMETHODCALLTYPE __RPC_FAR *AddEntryIndirect)(IUICLog2 __RPC_FAR *This,
                                                                      /* [in] */ LONG inStructPtr);

    END_INTERFACE
} IUICLog2Vtbl;

interface IUICLog2
{
    CONST_VTBL struct IUICLog2Vtbl __RPC_FAR *lpVtbl;
};

#ifdef COBJMACROS

#define IUICLog2_QueryInterface(This, riid, ppvObject)                                             \
    (This)->lpVtbl->QueryInterface(This, riid, ppvObject)

#define IUICLog2_AddRef(This) (This)->lpVtbl->AddRef(This)

#define IUICLog2_Release(This) (This)->lpVtbl->Release(This)

#define IUICLog2_GetTypeInfoCount(This, pctinfo) (This)->lpVtbl->GetTypeInfoCount(This, pctinfo)

#define IUICLog2_GetTypeInfo(This, iTInfo, lcid, ppTInfo)                                          \
    (This)->lpVtbl->GetTypeInfo(This, iTInfo, lcid, ppTInfo)

#define IUICLog2_GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)                      \
    (This)->lpVtbl->GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)

#define IUICLog2_Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult,           \
                        pExcepInfo, puArgErr)                                                      \
    (This)->lpVtbl->Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult,        \
                           pExcepInfo, puArgErr)

#define IUICLog2_AddEntry(This, inLogType, inLogEntry)                                             \
    (This)->lpVtbl->AddEntry(This, inLogType, inLogEntry)

#define IUICLog2_ReloadPreferences(This) (This)->lpVtbl->ReloadPreferences(This)

#define IUICLog2_Terminate(This) (This)->lpVtbl->Terminate(This)

#define IUICLog2_AddEntryIndirect(This, inStructPtr)                                               \
    (This)->lpVtbl->AddEntryIndirect(This, inStructPtr)

#endif /* COBJMACROS */

#endif /* C style interface */

/* [id] */ HRESULT STDMETHODCALLTYPE IUICLog2_AddEntryIndirect_Proxy(IUICLog2 __RPC_FAR *This,
                                                                     /* [in] */ LONG inStructPtr);

void __RPC_STUB IUICLog2_AddEntryIndirect_Stub(IRpcStubBuffer *This,
                                               IRpcChannelBuffer *_pRpcChannelBuffer,
                                               PRPC_MESSAGE _pRpcMessage, DWORD *_pdwStubPhase);

#endif /* __IUICLog2_INTERFACE_DEFINED__ */

#ifndef __LOGPROJECTLib_LIBRARY_DEFINED__
#define __LOGPROJECTLib_LIBRARY_DEFINED__

/* library LOGPROJECTLib */
/* [helpstring][version][uuid] */

EXTERN_C const IID LIBID_LOGPROJECTLib;

EXTERN_C const CLSID CLSID_UICLog;

#ifdef __cplusplus

class DECLSPEC_UUID("DE411D6F-2BDA-444C-AC87-9171BAFD9E99") UICLog;
#endif

EXTERN_C const CLSID CLSID_UICLog2;

#ifdef __cplusplus

class DECLSPEC_UUID("4691F58A-9856-43c6-9269-7A8987AFC715") UICLog2;
#endif
#endif /* __LOGPROJECTLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif

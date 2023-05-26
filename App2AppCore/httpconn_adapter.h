#pragma once


struct httpconn_adapter : winrt::implements<httpconn_adapter, IDispatch>
{
    httpconn_adapter(winrt::App2App::IApp2AppHttpConnection conn) : m_conn(conn) {};

    HRESULT STDMETHODCALLTYPE GetTypeInfoCount(
        /* [out] */ __RPC__out UINT* pctinfo) noexcept override;

    HRESULT STDMETHODCALLTYPE GetTypeInfo(
        /* [in] */ UINT,
        /* [in] */ LCID,
        /* [out] */ __RPC__deref_out_opt ITypeInfo** ppTInfo) noexcept override;

    HRESULT STDMETHODCALLTYPE GetIDsOfNames(
        /* [in] */ __RPC__in REFIID riid,
        /* [size_is][in] */ __RPC__in_ecount_full(cNames) LPOLESTR* rgszNames,
        /* [range][in] */ __RPC__in_range(0, 16384) UINT cNames,
        /* [in] */ LCID,
        /* [size_is][out] */ __RPC__out_ecount_full(cNames) DISPID* rgDispId) noexcept override;

    HRESULT STDMETHODCALLTYPE Invoke(
        _In_  DISPID dispIdMember,
        _In_  REFIID riid,
        _In_  LCID,
        _In_  WORD wFlags,
        _In_  DISPPARAMS* pDispParams,
        _Out_opt_  VARIANT* pVarResult,
        _Out_opt_  EXCEPINFO* pExcepInfo,
        _Out_opt_  UINT* puArgErr) noexcept override;

    winrt::App2App::IApp2AppHttpConnection m_conn;
};

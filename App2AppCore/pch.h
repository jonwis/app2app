#pragma once
#include <unknwn.h>
#include <string_view>
#include <wil/cppwinrt.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.AppExtensions.h>
#include <winrt/Windows.Management.Deployment.h>

using namespace std::literals;

template<typename To, typename From> To to_winrt(From* ptr)
{
    To result{ nullptr };
    winrt::check_hresult(ptr->QueryInterface(winrt::guid_of<To>(), winrt::put_abi(result)));
    return result;
}

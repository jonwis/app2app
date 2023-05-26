// SimpleAbiCaller.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <Unknwn.h>
#include <winrt/base.h>
#include <winrt/Windows.ApplicationModel.AppExtensions.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt;
using namespace Windows::ApplicationModel::AppExtensions;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

/*
* This demo program shows that there's nothing fancy or special about the App2AppCore project - it's
* all just built on primitive APIs in Windows. This project does not reference the App2AppCore project
* and its support code, it "just knows" how to interact with app2app services by reading the
* documentation.
*/
int main()
{
    winrt::init_apartment();

    auto catalog = AppExtensionCatalog::Open(L"com.microsoft.windows.app2app");
    for (auto const& c : catalog.FindAllAsync().get())
    {
        if (c.Id() == L"geolocation")
        {
            auto guid = winrt::guid{ c.GetExtensionPropertiesAsync().get()
                .Lookup(L"Activation").as<IPropertySet>()
                .Lookup(L"ClassId").as<IPropertySet>()
                .Lookup(L"#text").as<IPropertyValue>()
                .GetString() };

            auto serviceDispatch = winrt::create_instance<::IDispatch>(guid, CLSCTX_LOCAL_SERVER);

            // Find the name of the "invoke" method and "close" methods
            WCHAR invokeName[] = L"invoke";
            WCHAR closeName[] = L"close";
            LPOLESTR names[] = { invokeName, closeName };
            DISPID ids[_countof(names)];
            winrt::check_hresult(serviceDispatch->GetIDsOfNames(IID_NULL, names, _countof(names), LOCALE_USER_DEFAULT, ids));

            // This method takes an empty property set, so construct one and send it over
            PropertySet ps{};
            VARIANT vtArg{};
            vtArg.vt = VT_UNKNOWN;
            vtArg.punkVal = reinterpret_cast<::IUnknown*>(winrt::get_abi(ps));
            DISPPARAMS params{};
            params.cArgs = 1;
            params.rgvarg = &vtArg;
            EXCEPINFO excepInfo{};
            VARIANT vtResult{};
            UINT argErr{};
            winrt::check_hresult(serviceDispatch->Invoke(ids[0], IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, &vtResult, &excepInfo, &argErr));

            // The result is a variant with a property bag in there too, pull it and look at what it sent
            com_ptr<::IUnknown> unkArgs;
            winrt::attach_abi(unkArgs, vtResult.punkVal);
            for (auto&& [k, v] : unkArgs.as<IPropertySet>())
            {
                std::wcout << k.c_str() << L" = " << winrt::unbox_value<double>(v) << std::endl;
            }

            params = {};
            winrt::check_hresult(serviceDispatch->Invoke(ids[1], IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, nullptr));
        }
    }
}
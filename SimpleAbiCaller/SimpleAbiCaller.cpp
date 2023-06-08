// SimpleAbiCaller.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <filesystem>
#include <windows.h>
#include <Unknwn.h>
#include <string_view>
#include <wil/cppwinrt.h>
#include <winrt/Windows.ApplicationModel.AppExtensions.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Json.h>
#include <wil/resource.h>

#include "process_stdio_pipe.h"

using namespace std::literals;
using namespace std;
using namespace winrt;
using namespace Windows::ApplicationModel::AppExtensions;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Data::Json;

void get_geolocation()
{
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

            // Find the name of the "call" method
            WCHAR invokeName[] = L"call";
            WCHAR argsName[] = L"args";
            LPOLESTR callNames[] = { invokeName, argsName };
            DISPID ids[_countof(callNames)]{ -1,-1 };
            winrt::check_hresult(serviceDispatch->GetIDsOfNames(IID_NULL, callNames, _countof(callNames), LOCALE_USER_DEFAULT, ids));

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

            // Close the connection
            WCHAR closeName[] = L"close";
            LPOLESTR closeNames[] = { closeName };
            DISPID closeId{ -1 };
            if (SUCCEEDED(serviceDispatch->GetIDsOfNames(IID_NULL, closeNames, _countof(closeNames), LOCALE_USER_DEFAULT, &closeId)))
            {
                params = {};
                winrt::check_hresult(serviceDispatch->Invoke(closeId, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, nullptr));
            }
        }
    }
}


void get_localinfo(std::wstring const& command)
{
    auto catalog = AppExtensionCatalog::Open(L"com.microsoft.windows.app2app");
    for (auto const& c : catalog.FindAllAsync().get())
    {
        if (c.Id() == L"localinfo")
        {
            auto appExecAlias = c.GetExtensionPropertiesAsync().get()
                .Lookup(L"Activation").as<IPropertySet>()
                .Lookup(L"AppExecAlias").as<IPropertySet>();

            // Form the filesystem path to the executable
            auto exeAttribute = appExecAlias.Lookup(L"@Executable").as<hstring>();
            auto exePath = std::filesystem::path(static_cast<std::wstring_view>(c.Package().InstalledPath())) /
                static_cast<std::wstring_view>(exeAttribute);

            // Form the process' arguments list
            auto argsAttribute = appExecAlias.Lookup(L"@Arguments").as<hstring>();
            auto processArgs = std::wstring(exeAttribute + L" " + argsAttribute);

            // Construct the json body payload to squirt at stdin
            JsonObject argPayload{};
            argPayload.Insert(L"command", JsonValue::CreateStringValue(command));
            auto argPayloadString = winrt::to_string(argPayload.Stringify());

            // Send it over
            auto response = launch_and_get_one_response(exePath, processArgs, argPayloadString);
            if (std::holds_alternative<std::string>(response))
            {
                std::cout << std::get<std::string>(response) << std::endl;
            }
            else if (std::holds_alternative<winrt::hresult>(response))
            {
                std::cout << "Error in call " << std::hex << std::get<winrt::hresult>(response) << std::endl;
            }
            else
            {
                std::cout << "No response" << std::endl;
            }
        }
    }
}

/*
* This demo program shows that there's nothing fancy or special about the App2AppCore project - it's
* all just built on primitive APIs in Windows. This project does not reference the App2AppCore project
* and its support code, it "just knows" how to interact with app2app services by reading the
* documentation.
*/
int wmain(int argc, wchar_t const* const* argv)
{
    winrt::init_apartment();

    if ((argc == 2) && (argv[1] == L"geolocation"sv))
    {
        get_geolocation();
    }
    else if ((argc == 3) && (argv[1] == L"localinfo"sv))
    {
        get_localinfo(argv[2]);
    }
}


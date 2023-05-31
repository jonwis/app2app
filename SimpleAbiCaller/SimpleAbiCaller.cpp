// SimpleAbiCaller.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <windows.h>
#include <Unknwn.h>
#include <string_view>
#include <winrt/Windows.ApplicationModel.AppExtensions.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Json.h>
#include <wil/resource.h>

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

            // Find the executable path from the app extension information
            auto executable = std::wstring(appExecAlias.Lookup(L"@Executable").as<hstring>());
            auto args = executable + L" " + std::wstring(appExecAlias.TryLookup(L"@Arguments").as<hstring>());

            // Construct the json body payload to squirt at stdin
            JsonObject argPayload{};
            argPayload.Insert(L"command", JsonValue::CreateStringValue(command));
            auto argPayloadString = to_string(argPayload.Stringify());
            auto newlineString = to_string(L"\r\n"sv);

            // Windows uses pipes to communicate over stdin/stdout. Create them, marked so they can
            // be inherited by a child process. Put the "right" half of the pipe into the startup
            // info for the new process to use.
            const int pipe_read = 0;
            const int pipe_write = 1;
            wil::unique_handle stdIn[2];
            wil::unique_handle stdOut[2];
            SECURITY_ATTRIBUTES sa{};
            sa.nLength = sizeof(sa);
            sa.bInheritHandle = TRUE;
            sa.lpSecurityDescriptor = nullptr;
            THROW_IF_WIN32_BOOL_FALSE(::CreatePipe(&stdOut[pipe_read], &stdOut[pipe_write], &sa, 0));
            THROW_IF_WIN32_BOOL_FALSE(::SetHandleInformation(stdOut[pipe_read].get(), HANDLE_FLAG_INHERIT, 0));
            THROW_IF_WIN32_BOOL_FALSE(::CreatePipe(&stdIn[pipe_read], &stdIn[pipe_write], &sa, 0));
            THROW_IF_WIN32_BOOL_FALSE(::SetHandleInformation(stdIn[pipe_write].get(), HANDLE_FLAG_INHERIT, 0));

            STARTUPINFOW startupInfo = { sizeof(startupInfo) };
            startupInfo.hStdError = stdOut[pipe_write].get();
            startupInfo.hStdOutput = stdOut[pipe_write].get();
            startupInfo.hStdInput = stdIn[pipe_read].get();
            startupInfo.dwFlags |= STARTF_USESTDHANDLES;

            // Note the 'dangerous' const cast here; CreateProcess may write to this buffer, but
            // only ever inserting and removing null-character markers within its body. The buffer
            // is de-modified before it returns.
            wil::unique_process_information processInfo;
            THROW_IF_WIN32_BOOL_FALSE(::CreateProcessW(nullptr, const_cast<wchar_t*>(args.c_str()), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &startupInfo, &processInfo));

            // Simple mode - write the json string to the 'write' part of stdin, then wait for a
            // response string. A more complicated system is needed for arbitrary payload lengths,
            // as Windows pipes have a default buffer size. As this is a one-shot API, close the
            // pipe after the write is done so the caller sees EOF and terminates.
            DWORD written;
            THROW_IF_WIN32_BOOL_FALSE(WriteFile(stdIn[pipe_write].get(), argPayloadString.data(), argPayloadString.size(), &written, nullptr));
            THROW_IF_WIN32_BOOL_FALSE(WriteFile(stdIn[pipe_write].get(), newlineString.data(), newlineString.size(), &written, nullptr));
            stdIn[pipe_write].reset();
            stdIn[pipe_read].reset();

            std::string ss;
            // Reading from the input buffer needs to be done asynchonrously. I'm sure we could
            // figure out some overlapped IO here, but for now just spin a thread to pump the
            // stdout from the other side.
            auto read_thread = std::thread([&]
                {
                    const auto chunk_size = 120;
                    auto buffer = std::make_unique<char[]>(chunk_size);
                    DWORD readBytes;
                    while (ReadFile(stdOut[pipe_read].get(), buffer.get(), 120, &readBytes, nullptr) && (readBytes != 0))
                    {
                        ss.insert(ss.end(), buffer.get(), buffer.get() + readBytes);
                    }
                });

            // Wait for the other process to terminate, then close all our pipes. This could
            // get a timeout operation, 
            WaitForSingleObject(processInfo.hProcess, INFINITE);
            stdOut[pipe_write].reset();

            // Wait for the read-thread to terminate due to the pipe breaking
            read_thread.join();
            stdOut[pipe_read].reset();

            std::cout << ss << std::endl;
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
#include "pch.h"

#include <winrt/App2App.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace App2App;

int main()
{
    init_apartment();
    Uri uri(L"http://aka.ms/cppwinrt");
    printf("Hello, %ls!\n", uri.AbsoluteUri().c_str());

    for (auto&& p : App2AppConnection::GetPackagesWithService(L"geolocation"))
    {
        printf("Package %ls has it\n", p.Id().FullName().c_str());
    }
}
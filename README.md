# App2App Communication

This solution shows a basic app-to-app communication protocol similar to the system used in the
[https://learn.microsoft.com/en-us/uwp/api/Windows.ApplicationModel.AppService.AppServiceConnection?view=winrt-22621](AppServices) system.
Specifically, there is:

* A registry of apps by name and by "service"
* A way for apps to register themselves as "hosts"
* A way for apps to invoke "hosts" over a long-running connection

In this system, the app implementing the system is called a "host" or "service". The app invoking
the system is called a "client" or "caller".

Layered on top of that is a "web request like" model on both ends, generating a request on one end
and producing a response on the other.

# Layers

## Registration

Initially, apps register as COM objects using an `AppExtension`.  That is, packaged win32 apps add
markup to their manifest based on the [windows.appExtension](https://learn.microsoft.com/en-us/windows/uwp/launch-resume/how-to-create-an-extension)
system. An example is below:

```xml
<uap3:Extension Category="windows.appExtension">
    <uap3:AppExtension Name="com.microsoft.windows.launchforsession"
                        Id="app"
                        DisplayName="anything"
                        Description="anything"
                        PublicFolder="Public">
        <uap3:Properties>
            <Activation>
                <ClassId>587fc84c-xxxx-xxxx-xxxx-ff693f176f95</ClassId>
            </Activation>
        </uap3:Properties>
    </uap3:AppExtension>
</uap3:Extension>
```

In this chunk the `/Properties/Activation/ClassId` indicates an out-of-process COM object that
implements the `IApp2AppConnection` interface.

Discovery uses the `Open` method of [AppExtensionCatalog](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.appextensions.appextensioncatalog?view=winrt-22621)
to find and connect to a target app.

## Activation

When the 

## 

# Appendix / Notes

This project uses https://learn.microsoft.com/en-us/windows/uwp/winrt-components/raising-events-in-windows-runtime-components
to generate custom proxy stub DLLs between the processes. See also [this sample](https://github.com/microsoft/Windows-universal-samples/blob/ad9a0c4def222aaf044e51f8ee0939911cb58471/Samples/ProxyStubsForWinRTComponents/cpp/Server/ProxyStubsForWinRTComponents_server.vcxproj) for a complete description

See also this - https://github.com/microsoft/cppwinrt/pull/1290/files
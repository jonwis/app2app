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
<Application ...>
    <Extensions>
        <uap3:Extension Category="windows.appExtension">
            <uap3:AppExtension Name="com.microsoft.windows.app2app"
                                Id="app"
                                DisplayName="anything"
                                Description="anything"
                                PublicFolder="Public">
                <uap3:Properties>
                    <ServiceDefinition>something.yaml</ServiceDefinition>
                    <Activation>
                        <ClassId>587fc84c-xxxx-xxxx-xxxx-ff693f176f95</ClassId>
                    </Activation>
                </uap3:Properties>
            </uap3:AppExtension>
        </uap3:Extension>
	    <com:Extension Category="windows.comServer">
		    <com:ComServer>
                <!-- This is the CLSID that matches the above -->
			    <com:ExeServer Executable="PluginApp1.exe" DisplayName="PluginApp1" Arguments="-App2AppProvider">
				    <com:Class Id="587fc84c-xxxx-xxxx-xxxx-ff693f176f95"/>
			    </com:ExeServer>
		    </com:ComServer>
	    </com:Extension>
```

In this markup:

* `/AppExtension/Properties/Activation/ClassId` indicates an out-of-process COM object that implements the `IDispatch`
interface.
* `/AppExtension/@Id` is the "service name" used to identify sub-app plugins. Apps that don't have more than one
app-to-app entrypoint can just use the id string "any"
* `/AppExtension/Properties/ServiceDefinition` is a file name relative to the extension's public folder containing a
textual interface definition. General-purpose API mappers might use this information to figure out how to call an
app2app service.

## Discovery

Discovery uses the `Open` method of [AppExtensionCatalog](https://learn.microsoft.com/en-us/uwp/api/windows.applicationmodel.appextensions.appextensioncatalog?view=winrt-22621)
to find the mapping package and app2app entry from above. It'll find any registered application.

## Activation

Callers typically use `App2App.App2AppConnection.Connect` to bring up a new App2App connection. The system
uses the discover mode above to bind a package family name to the CLSID for the server, then calls CoCreateInstance
on that. The `Connect` method wraps the raw `IDispatch` in a convenience layer, details described below. The
caller then just calls `InvokeAsync` with a property set and gets a response result object back containing
another property set.

## Implementing a Host

Host class objects are expected to implement IDispatch. The `IApp2AppConnection` is a convenience wrapper around
their object.

The `App2AppCore` component provides multiple simplifications that bind `IDispatch` to certain types.


# Appendix / Notes

This project uses https://learn.microsoft.com/en-us/windows/uwp/winrt-components/raising-events-in-windows-runtime-components
to generate custom proxy stub DLLs between the processes. See also [this sample](https://github.com/microsoft/Windows-universal-samples/blob/ad9a0c4def222aaf044e51f8ee0939911cb58471/Samples/ProxyStubsForWinRTComponents/cpp/Server/ProxyStubsForWinRTComponents_server.vcxproj) for a complete description

See also this - https://github.com/microsoft/cppwinrt/pull/1290/files

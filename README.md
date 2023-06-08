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

# Components

This system is built in three parts:

* Discovery of plugins via AppExtension in packaged manifests
* Activation of plugins via COM's `CoCreate` and cross-process mechanisms
* Invocation of plugin via IDispatch with a single parameter in and single parameter out

On top of this, the `App2AppCore.dll` provides some semantic sugar:

* An `IApp2AppConnection` channel that sends/recieves a `IPropertySet` asynchronously
* An `IApp2AppHttpConnection` channel that sends `Windows.Web.Http.HttpRequestMessage`
  and recieves `Windows.Web.Http.HttpResponseMessage`

Note that you don't actually _need_ `app2appcore.dll` - all it provides is a nicer wrapper over the
`AppExtension` type to identify services, and adapters to make working with `IDispatch` nicer for
modern code.  The `SimpleAbiCaller` and `SimpleNetHostApp` project shows how to do this all-in-one
without any code from `App2AppCore.dll`.

# How to Experiment

You can build this solution yourself and see how it works.  You'll need VS2022 installed, along with the
VC++ tools for Windows Desktop development.

1. Build the solution for your favorite architecture
2. Right-click and "deploy" the `PluginApp1` and `PluginCaller2` projects
3. Launch `PluginCaller2`
4. Click one of the buttons and observe the output in the text box

The PluginApp1 (the "host") will launch and interact with PluginCaller. Currently, that interaction is
invisible, but can be observed by debugging PluginCaller2 and its `myButton_OnClick` handler.

> **Note:** If you get a "Failed to deploy" error with no other information, you're probably hitting
> a known issue in the single-project packaging model. Open the `.vcxproj` using "Open With > XML Editor"
> on the project that failed to deploy and modify it. Add a blank line somewhere in the middle of
> the XML content.  Save and close it, reload the project, then rebuild the solution.
> Deploy should just work.

## Projects

| Project | Purpose |
|:--|:--|
| [App2AppCore](./App2AppCore/App2AppCore.vcxproj) | A WinRT DLL that makes it easy to call into and implement a plugin for either IPropertySet or HTTP-style calls |
| [PluginApp1](./PluginApp1/PluginApp1.vcxproj) | Provides "geolocation" (property-set) and "desktopinfo" (http style) plugins for use |
| [PluginCaller2](./PluginCaller2/PluginCaller2.vcxproj) | Simple GUI around invoking geolocation and desktopinfo plugins, showing their results |
| [SimpleAbiCaller](./SimpleAbiCaller/SimpleAbiCaller.vcxproj) | Command line tool showing a very simple no-appcore-require invocation of the geolocation plugin(s) |
| [SimpleNetHostApp](./SimpleNetHostApp/SimpleNetHostApp.csproj) | Command line .NET executable providing a plugin |

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

See also the `App2App.App2AppConnection.ConnectHttp` which provides a channel that takes in `Windows.Web.Http.HttpRequestMessage`
and produces `Windows.Web.Http.HttpResponseMessage`.  This lets an app "be like" a web connection,
processing messages using whatever normal HTTP client code they have.

## Implementing a Host

Host class objects are expected to implement `IDispatch`.  Host classes generally provide two named methods,
one usually called `invoke` and one called `close`.  The `invoke` method takes some information and returns
a result value.  The `close` method lets the caller positively disconnect from the host, and the host is
free to drop any state or information. (Calls to `invoke` after `close` should return an error.)

The `IApp2AppConnection` is a convenience wrapper around
their object.

The `App2AppCore` component provides multiple simplifications that bind `IDispatch` to certain types.

# Simple Command Line Activation

Apps can also provide a simplified single-command-line-style invocation method.

For registration purposes, apps still use the AppExtension registration system, but indicate which
appexecutionalias provides the command-line operation. See [AppExecutionAlias](https://github.com/AdamBraden/AppExecutionAlias)
for information, or the SimpleNetHostApp project here for a working sample.

One style uses simple stdin/stdout communication - the caller sends data to the host's stdin and
the host processes it.  The host responds by writing to its stdout. The demo project expects to
exchange data on stdin/stdout via UTF-8 encoded JSON objects, with \r\n delimination between
command requests. JSON allows newlines in string values, but they must be encoded like "foo\r\nbar"
with the literal byte characters `f o o \ r \ n b a r`. JSON _allows_ newlines in "interstitial"
whitespace (like pretty-printing in an editor) - be sure your JSON-to-text conversion tool
eliminates those.

```xml
<Application ...>
    <Extensions>
        <uap3:Extension Category="windows.appExtension">
            <uap3:AppExtension Name="com.microsoft.windows.app2app"
                                Id="muffins"
                                DisplayName="Generates a list of preferred muffins based on a size"
                                Description="anything"
                                PublicFolder="PublicMuffins">
                <uap3:Properties>
                    <Activation>
                        <!-- Points to the application execution alias for this package  -->
                        <AppAlias Executable="ContosoMuffinMaker.exe" Arguments="-jsonrequest"/>
                        <!-- This app expects to use stdin/stdout communication -->
                        <UseStdInOut/>
                    </Activation>
                </uap3:Properties>
            </uap3:AppExtension>
        </uap3:Extension>
        <uap3:Extension Category="windows.appExecutionAlias" Executable="ContosoMuffinMaker.exe" EntryPoint="Windows.FullTrustApplication">
          <uap3:AppExecutionAlias>
            <desktop:ExecutionAlias Alias="ContosoMuffinMaker.exe" />
          </uap3:AppExecutionAlias>
        </uap3:Extension>
```

The [SimpleNetHostApp](./SimpleNetHostApp/SimpleNetHostApp.csproj) provides an example host. The host
displays a prompt to the user in a dialog, then reports the results back in a JSON format. See the
code in [PluginCaller2](./PluginCaller2/PluginCaller.vcxproj) for the invocation example.

# Coming Soon

A short list of "to do" items

**Enumerate available hosts** - Currently, the only supported operation is to identify packages that
provide an app2app connection. Some apps will want to enumerate them and dig through other configuration
data about them.

**Mapping between endpoint types** - If a caller talks PropertySet but the host talks HttpRequestMessage,
should there be a mapping betweent the two?

**Simplified registration** - Since the manifest has all the registration information in it, a single
method can register all the types. The connection manager can use "regular" WinRT type activation to
bring up an object.

**Method-calls** - Some app2app hosts might have a flat DLL with a single call export. Make it easier
to provide that level of binding.

**Sync or not?** - WinRT API design has moved away from "`-Async` all the things". Does the connection
pipe have to be async, or should it be synchronous? Apps already know how to deal with blocking background
calls on their main threads.  Hosts already know how to deal with routing calls from an apartment to
their main thread.

**GUI debugger** - Upgrade PluginCaller2 to be better; more tools to enumerate then call then visualize
the bodies of calls; more ability to take parameters to pass along, etc.

**Helpers for hosts** - APIs in App2AppCore that let hosts strongly identify who their callers are,
along with other systems to ask questions of the caller during the connect phase.

# Appendix / Notes

This project uses https://learn.microsoft.com/en-us/windows/uwp/winrt-components/raising-events-in-windows-runtime-components
to generate custom proxy stub DLLs between the processes. See also [this sample](https://github.com/microsoft/Windows-universal-samples/blob/ad9a0c4def222aaf044e51f8ee0939911cb58471/Samples/ProxyStubsForWinRTComponents/cpp/Server/ProxyStubsForWinRTComponents_server.vcxproj) for a complete description

See also this - https://github.com/microsoft/cppwinrt/pull/1290/files

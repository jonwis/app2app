using System;
using System.Runtime.InteropServices;
using System.Text.Json.Nodes;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Web.Http;

namespace SimpleNetHostApp
{
    /*
     * This interface describes the methods in the scheme for "property set" invocation, in terms of
     * a COM IDispatch method. Note that it has two methods, "call" and "close", which this plugin
     * style uses to handle operations.
     */
    [ComVisible(true)]
    [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
    public interface IPropertySetCallChannel
    {
        [return:MarshalAs(UnmanagedType.IUnknown)] [DispId(1)] IPropertySet call(IPropertySet args);
        [DispId(2)] void close();
    }

    /*
     * Implements the IPropertySetCallChannel interface for a "fake geolocation" service.
     */
    [Guid("fc9a6ed4-9c4a-42b7-a98d-b7b75f529bcd")]
    [ComVisible(true)]
    [ClassInterface(ClassInterfaceType.None)]
    public class FakeGeolocation : IPropertySetCallChannel
    {
        // Ignores the property set incoming, returns a faked-up output property set containing
        // frivolous values. Note that this call is synchronous here - the invocation channel is
        // by definition synchronous "on the wire" - it's OK for this method to invoke other
        // asynchronous operations, but it needs to wait before returning.
        public IPropertySet call(IPropertySet args)
        {
            PropertySet ps = new();
            ps.Add("kitten", 6.2);
            return ps;
        }

        // Handles the "close" call but does nothing.
        public void close()
        {
            // ok
        }
    }

    public class MainClass
    {
        static void Main(string[] args)
        {
            // Registers the FakeGeoLocation type with COM on launch.
            PluginClassFactory<FakeGeolocation>.Register();

            Console.WriteLine("Listening...");

            Console.ReadLine();
        }
    }
}
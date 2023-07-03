using System;
using System.Collections;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json.Nodes;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Storage;
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
            if ((args.Length == 1) && (args[0] == "-AppToAppProvider"))
            {
                // Registers the FakeGeoLocation type with COM on launch.
                PluginClassFactory<FakeGeolocation>.Register();

                Console.WriteLine("Listening...");

                Console.ReadLine();
            }
            else if ((args.Length == 1) && (args[0] == "-request"))
            {
                RunJsonRequestThing();
            }
        }

        static void RunJsonRequestThing()
        {
            // App2app console IO is UTF-8 encoded
            Console.InputEncoding = Encoding.UTF8;
            Console.OutputEncoding = Encoding.UTF8;

            string line;
            while ((line = Console.ReadLine()) != null)
            {
                try
                {
                    var obj = JsonObject.Parse(line);
                    var command = obj["command"].ToString();
                    if (command == "time")
                    {
                        JsonObject resp = new();
                        var n = DateTime.UtcNow;
                        resp["now"] = n;
                        resp["now_text"] = n.ToString("o");
                        Console.WriteLine(resp.ToJsonString());
                    }
                    else if (command == "environment")
                    {
                        JsonObject env = new();
                        foreach (DictionaryEntry e in Environment.GetEnvironmentVariables())
                        {
                            env[e.Key.ToString()] = e.Value.ToString();
                        }
                        JsonObject resp = new();
                        resp["environment"] = env;
                        Console.WriteLine(resp.ToJsonString());
                    }
                    else
                    {
                        JsonObject err = new();
                        err["error"] = "unknown command";
                        err["note"] = command;
                        JsonObject resp = new();
                        resp["error"] = err;
                        Console.WriteLine(resp.ToJsonString());
                    }
                }
                catch (Exception e)
                {
                    JsonObject ex = new();
                    ex["exception"] = e.ToString();
                    JsonObject resp = new();
                    resp["error"] = ex;
                    Console.WriteLine(resp.ToJsonString());
                }
            }
        }
    }
}
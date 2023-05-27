using System;
using System.Runtime.InteropServices;
using System.Text.Json.Nodes;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Web.Http;

namespace SimpleNetHostApp
{
    [ComVisible(true)]
    [InterfaceType(ComInterfaceType.InterfaceIsIDispatch)]
    public interface IPropertySetCallChannel
    {
        [return:MarshalAs(UnmanagedType.IUnknown)] [DispId(1)] IPropertySet call(IPropertySet args);
        [DispId(2)] void close();
    }

    [Guid("fc9a6ed4-9c4a-42b7-a98d-b7b75f529bcd")]
    [ComVisible(true)]
    [ClassInterface(ClassInterfaceType.None)]
    public class FakeGeolocation : IPropertySetCallChannel
    {
        public IPropertySet call(IPropertySet args)
        {
            PropertySet ps = new();
            ps.Add("kitten", PropertyValue.CreateDouble(6.2));
            return ps;
        }

        public void close()
        {
            // ok
        }
    }

    public class MainClass
    {
        static void Main(string[] args)
        {
            PluginClassFactory<FakeGeolocation>.Register();

            Console.WriteLine("Listening...");

            Console.ReadLine();
        }
    }
}
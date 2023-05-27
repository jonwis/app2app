using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using WinRT.Interop;

namespace SimpleNetHostApp
{
    // Snowcloned from  https://github.com/dotnet/samples/blob/main/core/extensions/OutOfProcCOM/COMRegistration/BasicClassFactory.cs
    [ComImport]
    [ComVisible(false)]
    [Guid("00000001-0000-0000-C000-000000000046")]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    internal interface IClassFactory
    {
        void CreateInstance([MarshalAs(UnmanagedType.Interface)] object outer, ref Guid riid, out IntPtr ppvObject);
        void LockServer([MarshalAs(UnmanagedType.Bool)] bool bLockServer);
    }

    static class Interop
    {
        // https://docs.microsoft.com/windows/win32/api/wtypesbase/ne-wtypesbase-clsctx
        public const int CLSCTX_LOCAL_SERVER = 0x4;

        // https://docs.microsoft.com/windows/win32/api/combaseapi/ne-combaseapi-regcls
        public const int REGCLS_MULTIPLEUSE = 1;
        public const int REGCLS_SUSPENDED = 4;

        // https://docs.microsoft.com/windows/win32/api/combaseapi/nf-combaseapi-coregisterclassobject
        [DllImport("ole32.dll")]
        public static extern int CoRegisterClassObject(ref Guid guid, [MarshalAs(UnmanagedType.IUnknown)] object obj, int context, int flags, out uint register);

        // https://docs.microsoft.com/windows/win32/api/combaseapi/nf-combaseapi-coresumeclassobjects
        [DllImport("ole32.dll")]
        public static extern int CoResumeClassObjects();

        // https://docs.microsoft.com/windows/win32/api/combaseapi/nf-combaseapi-corevokeclassobject
        [DllImport("ole32.dll")]
        public static extern int CoRevokeClassObject(uint register);

        public readonly static Guid IID_IUnknown = new Guid("00000000-0000-0000-C000-000000000046");
        public readonly static Guid IID_IDispatch = new Guid("00020400-0000-0000-C000-000000000046");
    }


    [ComVisible(true)]
    internal class PluginClassFactory<T> : IClassFactory where T : new()
    {
        public void CreateInstance([MarshalAs(UnmanagedType.Interface)] object outer, ref Guid riid, out IntPtr ppvObject)
        {
            if (outer != null)
            {
                throw new COMException(string.Empty, unchecked((int)0x80040110));
            }

            if (riid == Interop.IID_IUnknown)
            {
                ppvObject = Marshal.GetIUnknownForObject(new T());
            }
            else if (riid == Interop.IID_IDispatch)
            {
                ppvObject = Marshal.GetIDispatchForObject(new T());
            }
            else
            {
                foreach (Type i in typeof(T).GetInterfaces())
                {
                    if (i.GUID == riid)
                    {
                        ppvObject = Marshal.GetComInterfaceForObject(new T(), i);
                        return;
                    }
                }

                throw new InvalidCastException();
            }
        }

        public void LockServer([MarshalAs(UnmanagedType.Bool)] bool bLockServer)
        {
        }

        public static void Register()
        {
            var self = new PluginClassFactory<T>();
            Guid rclsid = typeof(T).GUID;

            if (s_regCookie != 0)
            {
                throw new InvalidOperationException();
            }

            var result = Interop.CoRegisterClassObject(ref rclsid, new PluginClassFactory<T>(), Interop.CLSCTX_LOCAL_SERVER, Interop.REGCLS_MULTIPLEUSE, out s_regCookie);
            if (result < 0)
            {
                s_regCookie = 0;
                Marshal.ThrowExceptionForHR(result);
            }
        }

        public static void Unregister()
        {
            if (s_regCookie != 0)
            {
                Interop.CoRevokeClassObject(s_regCookie);
            }
        }

        private static uint s_regCookie = 0;
    }
}

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace FileMonitor
{
    class FilterDriver
    {
        private const string PORT_NAME = "\\Mini-filter";
        SafePortHandle port;

        public void StartDriver()
        {
            var process = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    WindowStyle = ProcessWindowStyle.Hidden,
                    FileName = "cmd.exe",
                    Arguments = $"/C net start FsFilter1"
                }
            };

            process.Start();
        }

        public bool Connected
        {
            get => port != null && !port.IsClosed && !port.IsInvalid;
        }

        public void Connect()
        {
            if (!Connected)
            {
                var res = NativeMethods.FilterConnectCommunicationPort(PORT_NAME, 0, IntPtr.Zero, 0, IntPtr.Zero, out port);
                Marshal.ThrowExceptionForHR(res);
            }
        }

        public void Disconnect()
        {
            ThrowIfNotConnected();
            port.Dispose();
            port = null;
        }

        public void Send(string message)
        {
            ThrowIfNotConnected();

            var size = Marshal.SizeOf(message);
            var ptr = Marshal.AllocHGlobal(size);

            try
            {
                Marshal.StructureToPtr(message, ptr, false);
                var res = NativeMethods.FilterSendMessage(port, ptr, size, IntPtr.Zero, 0, out IntPtr resultSize);
                Marshal.ThrowExceptionForHR(res);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }

        public void Send(string message, string path)
        {
            Send(message, Encoding.Unicode.GetBytes(path + '\0'));
        }

        public void Send(string message, byte[] data)
        {
            ThrowIfNotConnected();

            var size = Marshal.SizeOf(message) + data.Length;
            IntPtr ptr = Marshal.AllocHGlobal(size);

            try
            {
                Marshal.StructureToPtr(message, ptr, false);
                IntPtr address = IntPtr.Add(ptr, Marshal.SizeOf(typeof(string)));
                Marshal.Copy(data, 0, address, data.Length);
                var res = NativeMethods.FilterSendMessage(port, ptr, size, IntPtr.Zero, 0, out IntPtr resultSize);
                Marshal.ThrowExceptionForHR(res);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }
      
        private void ThrowIfNotConnected()
        {
            if (!Connected)
                throw new Exception("Not Connected!!");
        }
    }
}

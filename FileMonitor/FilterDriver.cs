using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
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

        public void StopDriver()
        {
            var process = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    WindowStyle = ProcessWindowStyle.Hidden,
                    FileName = "cmd.exe",
                    Arguments = $"/C net stop FsFilter1"
                }
            };

            process.Start();
        }

        public bool Connected
        {
            get => port != null && !port.IsClosed && !port.IsInvalid;
        }

        private CancellationTokenSource cancellationTokenSource = new CancellationTokenSource();
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
            if (!Connected)
                return;

            port.Dispose();
            port = null;
        }

        public void Send(string path, long op, long pid, long tid)
        {
            Send(Encoding.Unicode.GetBytes(path + '\0'), op, pid, tid);
        }


        public void Send(byte[] data, long op, long pid, long tid)
        {
            ThrowIfNotConnected();

            //Send(data);

            ThrowIfNotConnected();

            var size = sizeof(long) * 3 + data.Length;
            IntPtr ptr = Marshal.AllocHGlobal(size);

            try
            {
                IntPtr ptr2 = ptr;
                Marshal.WriteInt64(ptr2, 0, op);

                ptr2 = IntPtr.Add(ptr2, sizeof(long));
                Marshal.WriteInt64(ptr2, 0, pid);

                ptr2 = IntPtr.Add(ptr2, sizeof(long));
                Marshal.WriteInt64(ptr2, 0, tid);

                ptr2 = IntPtr.Add(ptr2, sizeof(long));
                Marshal.Copy(data, 0, ptr2, data.Length);
                
                var res = NativeMethods.FilterSendMessage(port, ptr, size, IntPtr.Zero, 0, out IntPtr resultSize);
                Marshal.ThrowExceptionForHR(res);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }

        }

        public void Send(byte[] data)
        {
            ThrowIfNotConnected();

            var size = data.Length;
            IntPtr ptr = Marshal.AllocHGlobal(size);

            try
            {
                Marshal.Copy(data, 0, ptr, data.Length);
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


        public string GetLogs()
        {
            return SendAndRead(1);
        }

        public string SendAndRead(long op)
        {
            ThrowIfNotConnected();

            const int bufferSize = 4096;
            var buffer = Marshal.AllocHGlobal(bufferSize);

            var size = sizeof(long);
            IntPtr ptr = Marshal.AllocHGlobal(size);

            try
            {
                Marshal.WriteInt64(ptr, 0, op);

                var res = NativeMethods.FilterSendMessage(port, ptr, size, buffer, bufferSize, out IntPtr resultSize);
                Marshal.ThrowExceptionForHR(res);
                return Marshal.PtrToStringUni(buffer);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
        }



    }
}
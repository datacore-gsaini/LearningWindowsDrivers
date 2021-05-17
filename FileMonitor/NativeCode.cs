using Microsoft.Win32.SafeHandles;
using System;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Text;

namespace FileMonitor
{
    [SecurityPermission(SecurityAction.InheritanceDemand, UnmanagedCode = true)]
    [SecurityPermission(SecurityAction.Demand, UnmanagedCode = true)]
    internal class SafePortHandle : SafeHandleZeroOrMinusOneIsInvalid
    {
        private SafePortHandle() : base(true)
        {
        }

        [ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
        override protected bool ReleaseHandle()
        {
            return NativeMethods.CloseHandle(handle);
        }
    }
    public class NativeMethods
    {
        [DllImport("FltLib.dll", SetLastError = true)]
        internal extern static int FilterConnectCommunicationPort([MarshalAs(UnmanagedType.LPWStr)] string lpPortName,
            uint dwOptions,
            IntPtr lpContext,
            uint dwSizeOfContext,
            IntPtr lpSecurityAttributes,
            out SafePortHandle hPort
        );

        [DllImport("FltLib.dll", SetLastError = true)]
        internal extern static int FilterSendMessage(SafePortHandle port,
           IntPtr lpInBuffer,
           int dwInBufferSize,
           IntPtr lpOutBuffer,
           int dwOutBufferSize,
           out IntPtr lpBytesReturned);

        [DllImport("Kernel32")]
        internal extern static bool CloseHandle(IntPtr handle);

        [DllImport("kernel32.dll")]
        internal static extern int QueryDosDevice(string lpDeviceName, StringBuilder lpTargetPath, int ucchMax);

        [DllImport("kernel32.dll")]
        internal static extern uint GetCurrentThreadId();

    }

}

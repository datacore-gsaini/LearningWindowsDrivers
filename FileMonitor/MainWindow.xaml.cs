using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Cursors = System.Windows.Input.Cursors;
using Path = System.IO.Path;

namespace FileMonitor
{
    class PathConverter
    {
        private static Dictionary<string, string> driveToDevice = new Dictionary<string, string>();

        static PathConverter()
        {
            foreach (DriveInfo driveInfo in DriveInfo.GetDrives())
            {
                var drive = driveInfo.Name.TrimEnd(Path.DirectorySeparatorChar);
                var device = GetDevicePath(drive);

                driveToDevice[drive] = device;
            }
        }

        public static string ReplaceDriveLetter(string path)
        {
            foreach (var entry in driveToDevice)
            {
                if (path.StartsWith(entry.Key))
                {
                    return path.Replace(entry.Key, entry.Value);
                }
            }

            return path;
        }

        public static string GetDevicePath(string label)
        {
            var builder = new StringBuilder(64);
            Marshal.ThrowExceptionForHR(NativeMethods.QueryDosDevice(label, builder, builder.Capacity));
            return builder.ToString();
        }
    }


    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void btnBrowseFile_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new OpenFileDialog();
            var result = dialog.ShowDialog();

            if (result == System.Windows.Forms.DialogResult.OK)
            {
                txtSelectedPath.Text = dialog.FileName;
            }
        }

        private void btnBrowseDir_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new FolderBrowserDialog();
            var result = dialog.ShowDialog();

            if (result == System.Windows.Forms.DialogResult.OK)
            {
                txtSelectedPath.Text = dialog.SelectedPath;
            }
        }

        const string PlayButton = "\uE102";
        const string StopButton = "\uE15B";

        FilterDriver driver = new FilterDriver();

        private void btnStartStop_Click(object sender, RoutedEventArgs e)
        {
            if (btnStartStop.Content.ToString() == PlayButton)
            {
                try
                {
                    driver.StartDriver();
                    this.Cursor = Cursors.Wait;
                    Thread.Sleep(1000);
                    this.Cursor = Cursors.Arrow;

                    Setfilter();
                    btnUpdateFilter.IsEnabled = true;
                }
                catch (Exception ex)
                {
                    System.Windows.MessageBox.Show(ex.Message);
                    return;
                }
                btnStartStop.Content = StopButton;
            }
            else
            {
                btnStartStop.Content = PlayButton;
                driver.Disconnect();
                driver.StopDriver();
                btnUpdateFilter.IsEnabled = false;
            }
        }


        void Setfilter()
        {
            driver.Connect();
            
            var path = txtSelectedPath.Text;
            long op = 0;
            long.TryParse(txtProcessId.Text, out long pid);
            long.TryParse(txtThreadId.Text, out long tid);

            driver.Send(PathConverter.ReplaceDriveLetter(path), op, pid, tid);
        }

        private void BtnUpdateFilter_Click(object sender, RoutedEventArgs e)
        {
            Setfilter();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            driver.Disconnect();
            driver.StopDriver();
        }

        private void chkEnableTracing_Checked(object sender, RoutedEventArgs e)
        {
            this.Cursor = Cursors.Wait;

            string batFileName = Guid.NewGuid() + ".bat";

            using (StreamWriter batFile = new StreamWriter(batFileName))
            {
                batFile.WriteLine(@"logman create trace FsFilter -p {cea490b7 - b23b - 4bc5-986e-f79949bea6a9} 0xFFFFFFFF 5 -nb 1 1 -bs 1 -max 100 -o C:\PerfLogs\FsFilter.etl");
                batFile.WriteLine("logman start FsFilter");
            }

            ProcessStartInfo processStartInfo = new ProcessStartInfo("cmd.exe", "/c " + batFileName);
            processStartInfo.UseShellExecute = true;
            processStartInfo.CreateNoWindow = true;
            processStartInfo.WindowStyle = ProcessWindowStyle.Hidden;

            Process p = new Process();
            p.StartInfo = processStartInfo;
            p.Start();
            p.WaitForExit();
            this.Cursor = Cursors.Arrow;
        }

        private void chkEnableTracing_Unchecked(object sender, RoutedEventArgs e)
        {
            this.Cursor = Cursors.Wait;

            string batFileName = Guid.NewGuid() + ".bat";
            using (StreamWriter batFile = new StreamWriter(batFileName))
            {
                batFile.WriteLine("logman stop FsFilter");
                batFile.WriteLine("logman delete FsFilter");
            }

            ProcessStartInfo processStartInfo = new ProcessStartInfo("cmd.exe", "/c " + batFileName);
            processStartInfo.UseShellExecute = true;
            processStartInfo.CreateNoWindow = false;
            processStartInfo.WindowStyle = ProcessWindowStyle.Hidden;

            Process p = new Process();
            p.StartInfo = processStartInfo;
            p.Start();
            //p.WaitForExit();

            this.Cursor = Cursors.Arrow;
        }
    }
}

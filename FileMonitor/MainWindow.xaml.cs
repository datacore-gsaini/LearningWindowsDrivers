using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
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

namespace FileMonitor
{

    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            connector.StartDriver();
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
        const string PauseButton = "\uE103";

        FilterDriver connector = new FilterDriver();
        private void btnStartStop_Click(object sender, RoutedEventArgs e)
        {
            if (btnStartStop.Content.ToString() == PlayButton)
            {
                try
                {
                    connector.Connect();
                    //connector.Send("", txtSelectedPath.Text);
                }
                catch (Exception ex)
                {
                    System.Windows.MessageBox.Show(ex.Message);
                    return;
                }
                btnStartStop.Content = PauseButton;
            }
            else
            {
                btnStartStop.Content = PlayButton;
                connector.Disconnect();
            }

          
        }
    }
}

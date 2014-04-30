using EMServer;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace EMUtils
{
    public partial class MainForm : Form
    {
        public MainForm()
        {
            InitializeComponent();
            Reporting.OnChanged += Reporting_OnChanged;
        }

        void Reporting_OnChanged(object sender, ReportingEvent e)
        {
            MessagesListBox.Items.Add(e.Text);
            MessagesListBox.SetSelected(MessagesListBox.Items.Count - 1, true);
        }

        private void testWaveformVisualiserToolStripMenuItem_Click(object sender, EventArgs e)
        {
            emWaveFormVisualizerTests.Go();
        }

        private void sequenceVisualiserToolStripMenuItem_Click(object sender, EventArgs e)
        {
            emSequenceVisualiserTest.Go();            
        }

        private void simple1ToolStripMenuItem_Click(object sender, EventArgs e)
        {
            Test_Simple1.Go();
        }

        private void MainForm_Load(object sender, EventArgs e)
        {

        }

        private void outputsToolStripMenuItem_Click(object sender, EventArgs e)
        {
            OutputTester OT = new OutputTester();
            OT.Show();
        }

        private void toneDiscriminatorToolStripMenuItem_Click(object sender, EventArgs e)
        {
            emUtilities.Disconnect();            
            Experiment_ToneDiscrimination.Go();
        }

        private void logicGatesToolStripMenuItem_Click(object sender, EventArgs e)
        {            
            Experiment_Gates.Go();
            Experiment_ToneDiscrimination.Go();
        }

        private void exhaustiveSearchToolStripMenuItem_Click(object sender, EventArgs e)
        {
            ExhaustiveSearch.Go();
        }
    }
}

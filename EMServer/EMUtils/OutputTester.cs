using emInterfaces;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace EMUtils
{
    public partial class OutputTester : Form
    {

        emEvolvableMotherboard.Client Motherboard = null;
        private bool Busy = false;
        public OutputTester()
        {
            InitializeComponent();
        }

        private void OutputTester_Load(object sender, EventArgs e)
        {
            this.Busy = true;
            Motherboard = emUtilities.Connect();
            if (Motherboard == null)
            {
                MessageBox.Show("Failed to connect to the motherboard. Check the output log for details");
                this.Close();
                return;
            }            
            UpdateOutputFrequency();
            this.Busy = false;
            timer1.Enabled = true;
        }

        public void UpdateOutputFrequency()
        {
            int Pin = 0;
            int Pin2 = 0;
            int Freq = 0;
            try
            {
                Pin = int.Parse(this.OutputPinTextBox.Text);
                Pin2 = int.Parse(this.textBox1.Text);
                Freq = int.Parse(this.FrequencyTextBox.Text);
            }
            catch (Exception err)
            {
                MessageBox.Show("Values for pin or frequency are invalid", "Error",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            try
            {
                Motherboard.clearSequences();
            }
            catch (emException emErr)
            {
                MessageBox.Show(
                    emErr.ExceptionType.ToString() + " " + emErr.Reason + ". Error code=" + emErr.ErrorCode,
                    "Error from EvolvableMotherboard",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            emSequenceItem Action0 = new emSequenceItem();
            Action0.Frequency = Freq;
            Action0.WaveFormType = emWaveFormType.PWM;
            Action0.OperationType = emSequenceOperationType.PREDEFINED;
            Action0.CycleTime = 50;
            Action0.StartTime = 0;
            Action0.EndTime = 1024;
            Action0.Pin = new List<int>(); Action0.Pin.Add( Pin);
            Motherboard.appendSequenceAction(Action0);

            emSequenceItem ActionRecord = new emSequenceItem();
            ActionRecord.Frequency = Freq * 4;
            ActionRecord.StartTime = 0;
            ActionRecord.EndTime = 1024;           
            ActionRecord.Pin = new List<int>(); ActionRecord.Pin.Add(Pin2);
            ActionRecord.OperationType = emSequenceOperationType.RECORD;
            Motherboard.appendSequenceAction(ActionRecord);

            try
            {
                Motherboard.runSequences();
            }
            catch (emException emErr)
            {                
                MessageBox.Show( 
                    emErr.ExceptionType.ToString() + " " + emErr.Reason+ ". Error code="+emErr.ErrorCode,
                    "Error from EvolvableMotherboard",
                    MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }
            Motherboard.clearSequences();
        }

        private void SendButton_Click(object sender, EventArgs e)
        {
            SendButton.Enabled = false;
            while (this.Busy) Thread.Sleep(0);
            this.Busy = true;
            UpdateOutputFrequency();
            this.Busy = false;
            SendButton.Enabled = true;
        }

        public string DoStats(emWaveForm WaveForm)
        {
            string Report = "";

            List<int> IndicesOfLowToHigh = new List<int>();
            for (int i = 0; i < WaveForm.Samples.Count - 1; i++)
            {
                if (WaveForm.Samples[i] == 0 && WaveForm.Samples[i + 1] != 0)
                    IndicesOfLowToHigh.Add(i);
            }

            Report += "Hi->Low Count = " + IndicesOfLowToHigh.Count;

            List<int> LowToHighGap = new List<int>();
            for (int i = 0; i < IndicesOfLowToHigh.Count - 1; i++)
            {
                LowToHighGap.Add(IndicesOfLowToHigh[i + 1] - IndicesOfLowToHigh[i]);
            }

            Report += " Avg " + LowToHighGap.Average();
            Report += " Min " + LowToHighGap.Min();
            Report += " Max " + LowToHighGap.Max();
            Report += " Count " + WaveForm.Samples.Count;
            
            return Report;
        }

        private void GetReadings()
        {
            emWaveForm WaveForm = Motherboard.getRecording(int.Parse(this.textBox1.Text));
            chart1.Series.Clear();
            chart1.ChartAreas[0].CursorX.IsUserSelectionEnabled = true;
            //chart1.ChartAreas[0].CursorX.IsUserSelectionEnabled = true;
            Series S = new Series();
            S.IsVisibleInLegend = false;
            S.ChartType = SeriesChartType.FastLine;

            int ZeroCounter = 0;
            if (WaveForm.Samples != null)
            {
                for (int i = 0; i < WaveForm.SampleCount; i++)
                {
                    S.Points.AddXY(i, WaveForm.Samples[i]);
                    if (WaveForm.Samples[i] == 0) ZeroCounter++;
                }
                InputInfoLabel.Text = WaveForm.SampleCount + " samples, received at " + DateTime.Now.ToLongTimeString()+". "+ZeroCounter+" values are 0";
            } 
            chart1.Series.Add(S);
            statsListBox.Items.Add(DoStats(WaveForm));
            statsListBox.SelectedIndex = statsListBox.Items.Count - 1;
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (Busy) return;
            Busy = true;
            UpdateOutputFrequency();
            GetReadings();
            Busy = false;


        }

        private void OutputTester_FormClosing(object sender, FormClosingEventArgs e)
        {
            timer1.Enabled = false;
        }
    }
}

using emInterfaces;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;


namespace EMUtils
{
    public partial class emWaveFormVisualizer : Form
    {

        
        public emWaveFormVisualizer()
        {
            InitializeComponent();
 
            //this.ilPanel1.Scene.Add(PlotCube);
        }

        public void Show(params emWaveForm[] WaveForms)
        {
            base.Show();

            foreach (emWaveForm WaveForm in WaveForms)
                this.AddLine(WaveForm);
        }

     
        public void ShowDialog(params emWaveForm[] WaveForms)
        {
            

            foreach (emWaveForm WaveForm in WaveForms)
                this.AddLine(WaveForm);

            base.ShowDialog();
        }

        public double MinInputVoltage = -12;
        public double MaxInputVoltage = 12;
        public double MinInputValue = 0;
        public double MaxInputValue = int.MaxValue;

        public void Clear()
        {
            this.chart1.Series.Clear();
        }


        public void AddLines(params emWaveForm[] WaveForm)
        {
            foreach (emWaveForm W in WaveForm) AddLine(W);
        }
        public void AddLine(emWaveForm WaveForm)
        {
            double[,] RawValues = new double[WaveForm.SampleCount, 2];
            for (int i = 0; i < WaveForm.SampleCount; i++)
            {
                RawValues[i,0] = i;
                RawValues[i, 1] = WaveForm.Samples[i];// ScaleNumber(WaveForm.Samples[i], MinInputValue, MaxInputValue, MinInputVoltage, MaxInputVoltage);
            }
            Series S = new Series();
            for (int i = 0; i < WaveForm.SampleCount; i++)
            {
                S.Points.AddXY(i, RawValues[i, 1]);
            }
            S.ChartType = SeriesChartType.FastLine;
            
            chart1.ChartAreas[0].CursorX.IsUserSelectionEnabled = true;
            this.chart1.Series.Add(S);
        }

        private void emWaveFormVisualizer_Load(object sender, EventArgs e)
        {
            

        }

        private void ilPanel1_Load(object sender, EventArgs e)
        {

        }

    
        public static double ScaleNumber(double Value, double MinValue, double MaxValue, double MinScaledValue, double MaxScaledValue)
        {
            double P = (Value - MinValue) / (MaxValue - MinValue);
            return (P * (MaxScaledValue - MinScaledValue)) + MinScaledValue;
        }

        private void emWaveFormVisualizer_Load_1(object sender, EventArgs e)
        {

        }

        private void chart1_Click(object sender, EventArgs e)
        {

        }

    }

    public class emWaveFormVisualizerTests
    {
        public static void Go()
        {
            emWaveForm Wave0 = new emWaveForm();
            Wave0.SampleCount = 1024;
            Wave0.Samples = new List<int>();
            for (int i = 0; i < Wave0.SampleCount; i++)
                Wave0.Samples.Add((int)emWaveFormVisualizer.ScaleNumber(Math.Sin(0.1 * i) * 6, -12, 12, 0d, int.MaxValue));
            emWaveFormVisualizer WFV1 = new emWaveFormVisualizer();
            WFV1.Show(Wave0);
        }
    }
}

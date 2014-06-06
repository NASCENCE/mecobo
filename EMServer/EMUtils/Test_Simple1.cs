using emInterfaces;
using EMServer;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using Thrift.Protocol;
using Thrift.Transport;

namespace EMUtils
{
    class Test_Simple1
    {
        public static void Go()
        {
            
            emEvolvableMotherboard.Client Motherboard = emUtilities.Connect();

            emSequenceItem Action0 = new emSequenceItem();
            Action0.Frequency = 1000000;
            Action0.WaveFormType = emWaveFormType.PWM;
            Action0.OperationType = emSequenceOperationType.PREDEFINED;

            emSequenceItem ActionRecord = new emSequenceItem();
            ActionRecord.Frequency = 100000;
            ActionRecord.OperationType = emSequenceOperationType.RECORD;

            Motherboard.appendSequenceAction(Action0);
            Motherboard.appendSequenceAction(ActionRecord);

            Motherboard.runSequences();
            Thread.Sleep(1000);

            //em.joinSequence();

            emWaveForm WaveForm = Motherboard.getRecording(0);            
            Console.WriteLine("WaveForm contains " + WaveForm.SampleCount + " samples");
            emWaveFormUtilities.SaveWaveForm(WaveForm, "waveform1.csv");

            emWaveFormVisualizer ReturnedWave = new emWaveFormVisualizer();
            ReturnedWave.MinInputValue = 0;
            ReturnedWave.MaxInputValue = 1;
            ReturnedWave.MinInputVoltage = 0;
            ReturnedWave.MaxInputVoltage = 1;
            ReturnedWave.ShowDialog(WaveForm);
        }
    }
}

using emInterfaces;
using PdfFileWriter;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace EMUtils
{
    //[STAThread] 
    public partial class emSequenceVisualiser : Form
    {
        private delegate void UpdatePDFView(string FileName);

        private UpdatePDFView UpdatePDFViewDelegate;

        public emSequenceVisualiser()
        {
            InitializeComponent();
            this.UpdatePDFViewDelegate = new UpdatePDFView(UpdateViz);
        }

        private void emSequenceVisualiser_Load(object sender, EventArgs e)
        {

        }
        private string FileName = "";

        public string UpdateViz(List<emSequenceItem> Items)
        {            
            FileName = Environment.CurrentDirectory + "/" + Guid.NewGuid().ToString() + ".pdf";
            Application.DoEvents();
            emSequenceRenderer.Render(FileName, Items);
            Application.DoEvents();
            UpdatePDFViewDelegate(FileName);
            return FileName;
        }

        public void UpdateViz(string FileName)
        {
            this.axAcroPDF1.LoadFile(FileName);
            this.axAcroPDF1.setZoom(80);
            Application.DoEvents();
        }
        public void Show(List<emSequenceItem> Items)
        {
            base.Show();
            UpdateViz(Items);            
        }

        public void ShowDialog(List<emSequenceItem> Items)
        {
            UpdateViz(Items);
            
            base.ShowDialog();
        }

        private void emSequenceVisualiser_FormClosing(object sender, FormClosingEventArgs e)
        {
            //if (File.Exists(this.FileName))
              //  File.Delete(this.FileName);
        }

        private void emSequenceVisualiser_Shown(object sender, EventArgs e)
        {

        
        }

        private void axFoxitCtl1_Enter(object sender, EventArgs e)
        {

        }
    }

    public class emSequenceRenderer
    {
        private static PdfFont ArialNormal;
        private static PdfFont ArialBold;
        private static PdfFont ArialItalic;
        private static PdfFont ArialBoldItalic;
        private static PdfFont TimesNormal;
        private static PdfFont Comic;

        // Define Font Resources
        private static void DefineFontResources(PdfDocument Document)
        {
            // Define font resources
            // Arguments: PdfDocument class, font family name, font style, embed flag
            // Font style (must be: Regular, Bold, Italic or Bold | Italic) All other styles are invalid.
            // Embed font. If true, the font file will be embedded in the PDF file.
            // If false, the font will not be embedded.
            ArialNormal = new PdfFont(Document, "Arial", FontStyle.Regular, true);
            ArialBold = new PdfFont(Document, "Arial", FontStyle.Bold, true);
            ArialItalic = new PdfFont(Document, "Arial", FontStyle.Italic, true);
            ArialBoldItalic = new PdfFont(Document, "Arial", FontStyle.Bold | FontStyle.Italic, true);
            TimesNormal = new PdfFont(Document, "Times New Roman", FontStyle.Regular, true);
            Comic = new PdfFont(Document, "Comic Sans MS", FontStyle.Bold, true);

            // substitute one character for another
            // this program supports characters 32 to 126 and 160 to 255
            // if a font has a character outside these ranges that is required by the application,
            // you can replace an unused character with this character.
            ArialNormal.CharSubstitution(9679, 9679, 164);
            return;
        }

        private static void Rectangle(PdfContents Contents, double X, double Y, double Width, double Height, double LineWidth, Color Fill, Color Stroke, 
            string Text = null)
        {
            Contents.SaveGraphicsState();

            // Set frame’s line width to 0.02"
            Contents.SetLineWidth(LineWidth);

            // set frame’s color to dark blue
            Contents.SetColorStroking(Stroke);
            Contents.SetColorNonStroking(Fill);
            
            // Draw the frame
            // Rectangle position: x=1.0", y=1.0", width=6.5", height=9.0"
            Contents.DrawRectangle(X,Y,Width,Height, PaintOp.CloseFillStroke);

            // restore graphics sate
            Contents.RestoreGraphicsState();

            
        }

        public static double MMToPoints(double MM)
        {
            return (MM / 25.4) * 72;
        }
        private static void DrawText(PdfContents Contents, double X, double Y, double MaxWidth, double FontSizeMM, string Text)
        {
            Contents.SaveGraphicsState();

            PdfFileWriter.TextBox Box = new PdfFileWriter.TextBox(MaxWidth);
            double FontSize = MMToPoints(FontSizeMM);
            Box.AddText(ArialNormal, FontSize, Text);

            Contents.DrawText(X, ref Y, 0, 0, Box);
            Contents.RestoreGraphicsState();
        }

        private static Color OperationTypeToColor(emSequenceOperationType OpType)
        {
            switch (OpType)
            {
                case(emSequenceOperationType.emNULL):
                    return Color.White;
                    break;
                case(emSequenceOperationType.ARBITRARY):
                    return Color.LightBlue;
                    break;                
                case (emSequenceOperationType.RECORD):
                    return Color.LightPink;
                    break;
                case (emSequenceOperationType.PREDEFINED):
                    return Color.LightGreen;
                    break;
                case (emSequenceOperationType.WAIT):
                    return Color.LightYellow;
                    break;
                case (emSequenceOperationType.CONSTANT):
                    return Color.LightSkyBlue;
                    break;

            }

            throw new Exception("Unknown operation type!");
        }

        public static void Render(string OutputFileName, List<emSequenceItem> Items)        
        { 
            long MaxTime =0;
            int MaxPin = 0;

            List<int> PinsUsed = new List<int>();
            foreach (emSequenceItem Item in Items)
            {
                if (Item.EndTime > MaxTime) MaxTime = Item.EndTime;
                if (Item.Pin.Max() > MaxPin) MaxPin = Item.Pin.Max();
                foreach(int p in Item.Pin)
                if (!PinsUsed.Contains(p))
                    PinsUsed.Add(p);
            }
            PinsUsed.Sort(); PinsUsed.Reverse();
            double OrignX = 10; double OriginY = 10;

            double SequenceRowWidth = 100;
            double SequenceRowHeight = 10;
            double SequenceRowGap = 2;
            double MaxLabelWidth = 20;
            
            
            PdfDocument Document = new PdfDocument(120, (PinsUsed.Count +2) * (SequenceRowHeight + SequenceRowGap), UnitOfMeasure.mm);
            DefineFontResources(Document);
            // Step 3: Add new empty page
            PdfPage Page = new PdfPage(Document);
            // Step 4: Add contents object to the page object
            PdfContents Contents = new PdfContents(Page);

         
            for (int i = 0; i <PinsUsed.Count; i++)
            {                
                Rectangle(Contents, OrignX, OriginY + (i * (SequenceRowGap+SequenceRowHeight)), SequenceRowWidth, SequenceRowHeight, 0.1, Color.White, Color.Black);
                Rectangle(Contents, OrignX + MaxLabelWidth, OriginY + (i * (SequenceRowGap + SequenceRowHeight)), SequenceRowWidth - MaxLabelWidth, SequenceRowHeight, 0.1, Color.White, Color.Black);
                DrawText(Contents, OrignX + 1, OriginY + (i * (SequenceRowGap + SequenceRowHeight)) + (SequenceRowHeight/2) + (4/2), MaxLabelWidth, 4, String.Format("PIN{0:00}", PinsUsed[i]));
            }

            double MaxTimeWidth = SequenceRowWidth - MaxLabelWidth;
            double MMPerTime = MaxTimeWidth / MaxTime;
            for (int i = 0; i < Items.Count; i++)
            {
                if (Items[i].OperationType == emSequenceOperationType.emNULL) continue;
                foreach (int PinIndex in PinsUsed)
                {
                    //int PinIndex = PinsUsed.IndexOf(Items[i].Pin);
                    double Y = OriginY + (PinIndex * (SequenceRowGap + SequenceRowHeight));
                    double X_Start = OrignX + MaxLabelWidth + (MMPerTime * Items[i].StartTime);
                    double X_End = OrignX + MaxLabelWidth + (MMPerTime * Items[i].EndTime);
                    Rectangle(Contents, X_Start, Y, X_End - X_Start, SequenceRowHeight, 0.2, OperationTypeToColor(Items[i].OperationType), Color.Black);
                    string Label = Items[i].OperationType.ToString();
                    if (Items[i].OperationType == emSequenceOperationType.PREDEFINED)
                    {
                        Label = Items[i].Frequency + "Hz, " + Items[i].CycleTime + "% ±" + Items[i].Amplitude + "V, " + Items[i].WaveFormType.ToString();
                    }
                    else if (Items[i].OperationType == emSequenceOperationType.CONSTANT)
                    {
                        Label += " " + Items[i].Amplitude + "V ";
                    }
                    else if (Items[i].OperationType == emSequenceOperationType.WAIT)
                        Label = "";

                    DrawText(Contents, X_Start + 1, Y + (SequenceRowHeight / 2) + 4.5, X_End - X_Start - 1, 2.5, Label);
                }
            }

            Document.CreateFile(OutputFileName);            
        }
    }


    public class emSequenceVisualiserTest
    {
        public static List<emSequenceItem> MakeTestSequence()
        {
             List<emSequenceItem> Items = new List<emSequenceItem>();
            emSequenceItem RecordItem = new emSequenceItem();
            RecordItem.OperationType = emSequenceOperationType.RECORD;
            RecordItem.Pin = new List<int>(); RecordItem.Pin.Add(0);
            RecordItem.StartTime = 0;
            RecordItem.EndTime = 60;
            Items.Add(RecordItem);

            emSequenceItem Item1 = new emSequenceItem();
            Item1.OperationType = emSequenceOperationType.PREDEFINED;
            Item1.StartTime = 0;
            Item1.EndTime = 60;
            Item1.Pin = new List<int>(); Item1.Pin.Add( 1);
            Item1.Frequency = 100;
            Item1.Amplitude = 5;
            Item1.WaveFormType = emWaveFormType.PWM;
            Items.Add(Item1);

            emSequenceItem Item2 = new emSequenceItem();
            Item2.OperationType = emSequenceOperationType.PREDEFINED;
            Item2.StartTime = 10;
            Item2.EndTime = 45;
            Item2.Pin = new List<int>(); Item2.Pin.Add(2);
            Item2.Frequency = 10;
            Item2.Amplitude = 2;
            Item2.WaveFormType = emWaveFormType.SINE;
            Items.Add(Item2);

            emSequenceItem Item3 = new emSequenceItem();
            Item3.OperationType = emSequenceOperationType.PREDEFINED;
            Item3.StartTime = 45;
            Item3.EndTime = 60;
            Item3.Pin = new List<int>(); Item3.Pin.Add(3);
            Item3.Frequency = 100;
            Item3.Amplitude = 1;
            Item3.WaveFormType = emWaveFormType.SAW;
            Items.Add(Item3);
            
            emSequenceItem Item4 = new emSequenceItem();            
            Item4.OperationType = emSequenceOperationType.ARBITRARY;
            Item4.StartTime = 15;
            Item4.EndTime = 35;
            Item4.Pin = new List<int>(); Item4.Pin.Add(3);
            Items.Add(Item4);
            emSequenceItem Item5 = new emSequenceItem();
            Item5.OperationType = emSequenceOperationType.WAIT;
            Item5.StartTime = 35;
            Item5.EndTime = 40;
            Item5.Pin = new List<int>(); Item5.Pin.Add(3); ;
            Items.Add(Item5);
            emSequenceItem Item6 = new emSequenceItem();
            Item6.OperationType = emSequenceOperationType.ARBITRARY;
            Item6.StartTime = 40;
            Item6.EndTime = 60;
            Item6.Pin = new List<int>(); Item6.Pin.Add(3); ;
            Items.Add(Item6);

            emSequenceItem Item7 = new emSequenceItem();
            Item7.OperationType = emSequenceOperationType.PREDEFINED;
            Item7.StartTime = 10;
            Item7.EndTime = 30;
            Item7.Pin = new List<int>(); Item7.Pin.Add(4); ;
            Items.Add(Item7);

            return Items;

        }

        public static void Go()
        {
            List<emSequenceItem> Items = MakeTestSequence();
            //emSequenceRenderer.Render("test.pdf", Items);
            emSequenceVisualiser Frm = new emSequenceVisualiser();
            Frm.ShowDialog(Items);
        }
    }
}

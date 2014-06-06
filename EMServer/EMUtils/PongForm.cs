#define WITHoutGOO
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Drawing.Imaging;
using System.Linq;
using System.Text;
using System.Threading;
using System.Windows.Forms;

namespace EMUtils
{
    public partial class PongForm : Form
    {
        public PongForm()
        {
            InitializeComponent();
            this.TopMost = false;
        }
        PongGame Game = null;
        Graphics Canvas;

        Thread GooThread = null;


        private void PongForm_Load(object sender, EventArgs e)
        {
            Game = new PongGame();
            
            this.Canvas = this.panel2.CreateGraphics();
            this.timer1.Enabled = true;
        }
        public bool Busy = false;
        public Experiment_Pong.Individual Ind;
        private void timer1_Tick(object sender, EventArgs e)
        {
            if (Busy) return;
            Busy = true;
            this.Game.Render();
            this.Canvas.DrawImage(this.Game.OutputBuffer, new Rectangle(0, 0, this.panel2.Width, this.panel2.Height));
            this.Text = this.Game.CurrentState.PaddleAPosition + "," + this.Game.CurrentState.PaddleASpeed;
            this.Game.UpdateStates();
            this.PlayerAScoreLabel.Text = this.Game.PlayerAScore.ToString();
            this.PlayerBScoreLabel.Text = this.Game.PlayerBScore.ToString();
            this.label1.Text = this.UpdateCounter.ToString()+"_"+this.UpdateText;
            Busy = false;
        }

        private void PongForm_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == 'q')
            {
                this.timer1.Enabled = false;
                this.Visible = false;
                this.Close();
            }
            if (e.KeyChar == 'a')
            {
                if (this.Game.CurrentState.PaddleASpeed > 0) this.Game.CurrentState.PaddleASpeed = 0;
                this.Game.CurrentState.PaddleASpeed += -5;
            }
            if (e.KeyChar == 'z')
            {
                if (this.Game.CurrentState.PaddleASpeed < 0) this.Game.CurrentState.PaddleASpeed = 0;
                this.Game.CurrentState.PaddleASpeed += 5;
            }
            if (e.KeyChar == 'k')
            {
                if (this.Game.CurrentState.PaddleBSpeed > 0) this.Game.CurrentState.PaddleBSpeed = 0;
                this.Game.CurrentState.PaddleBSpeed += -5;
            }
            if (e.KeyChar == 'm')
            {
                if (this.Game.CurrentState.PaddleBSpeed < 0) this.Game.CurrentState.PaddleBSpeed = 0;
                this.Game.CurrentState.PaddleBSpeed += 5;
            }
        }

        private void panel2_Paint(object sender, PaintEventArgs e)
        {

        }

        private void panel2_Resize(object sender, EventArgs e)
        {
            while (Busy) Application.DoEvents();
            Busy = true;
            this.Canvas.Dispose();
            this.Canvas = this.panel2.CreateGraphics();
            Busy = false;
        }

        public Experiment_Pong.PongFitnessFunction FitnessFunction { get; set; }

        public bool EmBusy = false;
        private void timer2_Tick(object sender, EventArgs e)
        {
          
        }
        int UpdateCounter = 0;
        private string UpdateText;
        private void GooComputeThread()
        {

           if (EmBusy) return;
           while (this.Visible)
           {
               UpdateCounter++;
               EmBusy = true;
               double AvgOutputVoltage = 0;
               double US = this.FitnessFunction.UpdateStep(this.Ind, this.Game.CurrentState.PaddleBPosition, this.Game.CurrentState.BallY,out AvgOutputVoltage);
               EmBusy = false;

               if (AvgOutputVoltage < this.Ind.Threshold)
               {
                   if (this.Game.CurrentState.PaddleBSpeed > 0) this.Game.CurrentState.PaddleBSpeed = 0;
                   this.Game.CurrentState.PaddleBSpeed = -10;
                   this.UpdateText = "M";
               }
               if (AvgOutputVoltage >=this.Ind.Threshold)
               {
                   if (this.Game.CurrentState.PaddleBSpeed < 0) this.Game.CurrentState.PaddleBSpeed = 0;
                   this.Game.CurrentState.PaddleBSpeed = 10;
                   this.UpdateText = "K";
               }
               Application.DoEvents();
               Thread.Sleep(0);

           }
        }

        private void PongForm_Shown(object sender, EventArgs e)
        {
#if WITHGOO
            this.FitnessFunction.Motherboard.reset();
            this.FitnessFunction.ApplyConfigFromIndividual(this.Ind);

            this.GooThread = new Thread(GooComputeThread);
            this.GooThread.Start();
#endif
        }
    }

    public class PongState
    {
        public float PaddleAPosition = 0.5f;
        public float PaddleBPosition = 0.5f;
        public float PaddleASpeed = 0;
        public float PaddleBSpeed = 0;
        public float BallX = 0.5f;
        public float BallY = 0.5f;
        public float BalldX = 5;
        public float BalldY = 5;
    }

    public class PongGame
    {
        public PongState CurrentState;

        public int DisplayWidth = 640;
        public int DisplayHeight = 480;

        public Bitmap OutputBuffer = null;
        public Graphics OutputCanvas = null;

        public int PaddleHeight = 100;
        public int PaddleWidth = 10;
        public int BallSize = 20;
        public int BorderWidth = 10;
        public int PlayerAScore = 0;
        public int PlayerBScore = 0;

        public PongGame()
        {
            OutputBuffer = new Bitmap(this.DisplayWidth, this.DisplayHeight);
            this.OutputCanvas = Graphics.FromImage(this.OutputBuffer);
            this.OutputCanvas.Clear(Color.Black);
            this.CurrentState = new PongState();
            this.CurrentState.PaddleAPosition = this.DisplayHeight / 2;
            this.CurrentState.PaddleBPosition = this.DisplayHeight / 2;
            this.CurrentState.BallY = this.DisplayHeight / 2;
            this.CurrentState.BallX = this.DisplayWidth / 2; 
        }
        public int MaxPaddleSpeed = 20;
        public void UpdateStates()
        {
            if (this.CurrentState.PaddleASpeed < -MaxPaddleSpeed) this.CurrentState.PaddleASpeed = -MaxPaddleSpeed;
            if (this.CurrentState.PaddleASpeed > MaxPaddleSpeed) this.CurrentState.PaddleASpeed = MaxPaddleSpeed;

            this.CurrentState.PaddleAPosition += this.CurrentState.PaddleASpeed;
            this.CurrentState.PaddleAPosition += this.CurrentState.PaddleASpeed;

            if (this.CurrentState.PaddleAPosition < this.BorderWidth + (this.PaddleHeight / 2)) this.CurrentState.PaddleAPosition = (this.PaddleHeight / 2) + this.BorderWidth;
            if (this.CurrentState.PaddleAPosition > this.DisplayHeight - (this.PaddleHeight / 2) - this.BorderWidth) this.CurrentState.PaddleAPosition = this.DisplayHeight - (this.PaddleHeight / 2) - this.BorderWidth;

            if (this.CurrentState.PaddleBSpeed < -MaxPaddleSpeed) this.CurrentState.PaddleBSpeed = -MaxPaddleSpeed;
            if (this.CurrentState.PaddleBSpeed > MaxPaddleSpeed) this.CurrentState.PaddleBSpeed = MaxPaddleSpeed;
            this.CurrentState.PaddleBPosition += this.CurrentState.PaddleBSpeed;
            this.CurrentState.PaddleBPosition += this.CurrentState.PaddleBSpeed;
            if (this.CurrentState.PaddleBPosition < this.BorderWidth+(this.PaddleHeight / 2)) this.CurrentState.PaddleBPosition = (this.PaddleHeight / 2)+this.BorderWidth;
            if (this.CurrentState.PaddleBPosition > this.DisplayHeight - (this.PaddleHeight / 2) - this.BorderWidth) this.CurrentState.PaddleBPosition = this.DisplayHeight - (this.PaddleHeight / 2)-this.BorderWidth;


            this.CurrentState.BallX += this.CurrentState.BalldX;
            this.CurrentState.BallY += this.CurrentState.BalldY;
            if (this.CurrentState.BallX <= this.PaddleWidth)
            {
                this.CurrentState.BallX = this.PaddleWidth;
                this.CurrentState.BalldX *= -1;
                if (this.CurrentState.BallY >= this.CurrentState.PaddleAPosition - (PaddleHeight / 2) &&
                    this.CurrentState.BallY <= this.CurrentState.PaddleAPosition + (PaddleHeight / 2))
                {
                    var relativeIntersectY = (this.CurrentState.PaddleAPosition + (PaddleHeight / 2)) - this.CurrentState.BallY;
                    var normalizedRelativeIntersectionY = (relativeIntersectY / (PaddleHeight / 2));
                    var bounceAngle = normalizedRelativeIntersectionY * 2;
                    float BALLSPEED = 10;
                    this.CurrentState.BalldX = (float)(BALLSPEED * Math.Cos(bounceAngle));
                    this.CurrentState.BalldY = (float)(BALLSPEED * -Math.Sin(bounceAngle));
                    if (this.CurrentState.BalldX>=0 && this.CurrentState.BalldX < 5) this.CurrentState.BalldX = 5;

                }
                else
                    PlayerBScore++;
            }
            if (this.CurrentState.BallY <= this.BorderWidth)
            {
                this.CurrentState.BallY = this.BorderWidth;
                this.CurrentState.BalldY *= -1;
            }
            if (this.CurrentState.BallX >= this.DisplayWidth - this.PaddleWidth)
            {
                this.CurrentState.BallX = this.DisplayWidth - this.PaddleWidth;
                this.CurrentState.BalldX *= -1;
                if (this.CurrentState.BallY >= this.CurrentState.PaddleBPosition - (PaddleHeight / 2) &&
                    this.CurrentState.BallY <= this.CurrentState.PaddleBPosition + (PaddleHeight / 2))
                {
                    var relativeIntersectY = (this.CurrentState.PaddleBPosition + (PaddleHeight / 2)) - this.CurrentState.BallY;
                    var normalizedRelativeIntersectionY = (relativeIntersectY / (PaddleHeight / 2));
                    var bounceAngle = normalizedRelativeIntersectionY *2;
                    float BALLSPEED = 10;
                    this.CurrentState.BalldX = -1 * (float)(BALLSPEED * Math.Cos(bounceAngle));
                    this.CurrentState.BalldY = (float)(BALLSPEED * -Math.Sin(bounceAngle));
                    if (this.CurrentState.BalldX <= 0 && this.CurrentState.BalldX > -5) this.CurrentState.BalldX = -5;
                }
                else
                    PlayerAScore++;
            }
            if (this.CurrentState.BallY >= this.DisplayHeight - this.BorderWidth)
            {
                this.CurrentState.BallY = this.DisplayHeight - this.BorderWidth;
                this.CurrentState.BalldY *= -1;
            }

            if (this.CurrentState.BalldX < 0 && this.CurrentState.BalldX > -5) this.CurrentState.BalldX = -5;
            if (this.CurrentState.BalldX >= 0 && this.CurrentState.BalldX < 5) this.CurrentState.BalldX = 5;
        }

        public void Render()
        {
            Bitmap Temp = new Bitmap(this.DisplayWidth, this.DisplayHeight);
            Graphics TempCanvas = Graphics.FromImage(Temp);
            TempCanvas.Clear(Color.Black);

            ColorMatrix cm = new ColorMatrix();
            cm.Matrix33 = 0.5f;
            ImageAttributes ia = new ImageAttributes();
            ia.SetColorMatrix(cm);

            TempCanvas.DrawImage(this.OutputBuffer,
                new Rectangle(0, 0, this.DisplayWidth, this.DisplayHeight),
                0, 0, this.DisplayWidth, this.DisplayHeight,
                GraphicsUnit.Pixel,
                ia);

            this.OutputBuffer.Dispose();
            this.OutputCanvas.Dispose();
            this.OutputCanvas = TempCanvas;
            this.OutputBuffer = Temp;
                

            Brush B = new SolidBrush(Color.White);


            this.OutputCanvas.FillRectangle(B, new RectangleF(0, 0, this.DisplayWidth, BorderWidth));
            this.OutputCanvas.FillRectangle(B, new RectangleF(0, this.DisplayHeight-BorderWidth, this.DisplayWidth, BorderWidth));
            
            this.OutputCanvas.FillRectangle(B, new RectangleF(0, this.CurrentState.PaddleAPosition - (this.PaddleHeight / 2), this.PaddleWidth, this.PaddleHeight));
            this.OutputCanvas.FillRectangle(B, new RectangleF(this.DisplayWidth-this.PaddleWidth, this.CurrentState.PaddleBPosition - (this.PaddleHeight / 2), this.PaddleWidth, this.PaddleHeight));

            this.OutputCanvas.FillRectangle(B, new RectangleF(this.CurrentState.BallX - (this.BallSize / 2), this.CurrentState.BallY - (this.BallSize / 2), this.BallSize, this.BallSize));

        }
    }
}

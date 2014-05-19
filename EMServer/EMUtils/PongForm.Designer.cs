namespace EMUtils
{
    partial class PongForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.panel1 = new System.Windows.Forms.Panel();
            this.PlayerBScoreLabel = new System.Windows.Forms.Label();
            this.PlayerAScoreLabel = new System.Windows.Forms.Label();
            this.panel2 = new System.Windows.Forms.PictureBox();
            this.panel1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.panel2)).BeginInit();
            this.SuspendLayout();
            // 
            // timer1
            // 
            this.timer1.Interval = 10;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // panel1
            // 
            this.panel1.BackColor = System.Drawing.Color.Black;
            this.panel1.Controls.Add(this.PlayerBScoreLabel);
            this.panel1.Controls.Add(this.PlayerAScoreLabel);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(0, 0);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(863, 230);
            this.panel1.TabIndex = 0;
            // 
            // PlayerBScoreLabel
            // 
            this.PlayerBScoreLabel.Dock = System.Windows.Forms.DockStyle.Right;
            this.PlayerBScoreLabel.Font = new System.Drawing.Font("ModeSeven", 72F);
            this.PlayerBScoreLabel.ForeColor = System.Drawing.Color.White;
            this.PlayerBScoreLabel.Location = new System.Drawing.Point(588, 0);
            this.PlayerBScoreLabel.Name = "PlayerBScoreLabel";
            this.PlayerBScoreLabel.Size = new System.Drawing.Size(275, 230);
            this.PlayerBScoreLabel.TabIndex = 1;
            this.PlayerBScoreLabel.Text = "0";
            this.PlayerBScoreLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // PlayerAScoreLabel
            // 
            this.PlayerAScoreLabel.Dock = System.Windows.Forms.DockStyle.Left;
            this.PlayerAScoreLabel.Font = new System.Drawing.Font("ModeSeven", 72F);
            this.PlayerAScoreLabel.ForeColor = System.Drawing.Color.White;
            this.PlayerAScoreLabel.Location = new System.Drawing.Point(0, 0);
            this.PlayerAScoreLabel.Name = "PlayerAScoreLabel";
            this.PlayerAScoreLabel.Size = new System.Drawing.Size(275, 230);
            this.PlayerAScoreLabel.TabIndex = 0;
            this.PlayerAScoreLabel.Text = "0";
            this.PlayerAScoreLabel.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
            // 
            // panel2
            // 
            this.panel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel2.Location = new System.Drawing.Point(0, 230);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(863, 310);
            this.panel2.TabIndex = 1;
            this.panel2.TabStop = false;
            // 
            // PongForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(863, 540);
            this.Controls.Add(this.panel2);
            this.Controls.Add(this.panel1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.None;
            this.Name = "PongForm";
            this.Text = "PongForm";
            this.TopMost = true;
            this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
            this.Load += new System.EventHandler(this.PongForm_Load);
            this.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.PongForm_KeyPress);
            this.panel1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.panel2)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.PictureBox panel2;
        private System.Windows.Forms.Label PlayerBScoreLabel;
        private System.Windows.Forms.Label PlayerAScoreLabel;
    }
}
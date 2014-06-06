namespace EMUtils
{
    partial class MainForm
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
            this.menuStrip1 = new System.Windows.Forms.MenuStrip();
            this.testsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.testWaveformVisualiserToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.sequenceVisualiserToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.simple1ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.controlToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.outputsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.MessagesListBox = new System.Windows.Forms.ListBox();
            this.experimentsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.toneDiscriminatorToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.logicGatesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.exhaustiveSearchToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.menuStrip1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.SuspendLayout();
            // 
            // menuStrip1
            // 
            this.menuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.testsToolStripMenuItem,
            this.controlToolStripMenuItem,
            this.experimentsToolStripMenuItem});
            this.menuStrip1.Location = new System.Drawing.Point(0, 0);
            this.menuStrip1.Name = "menuStrip1";
            this.menuStrip1.Size = new System.Drawing.Size(763, 24);
            this.menuStrip1.TabIndex = 0;
            this.menuStrip1.Text = "menuStrip1";
            // 
            // testsToolStripMenuItem
            // 
            this.testsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.testWaveformVisualiserToolStripMenuItem,
            this.sequenceVisualiserToolStripMenuItem,
            this.simple1ToolStripMenuItem});
            this.testsToolStripMenuItem.Name = "testsToolStripMenuItem";
            this.testsToolStripMenuItem.Size = new System.Drawing.Size(46, 20);
            this.testsToolStripMenuItem.Text = "Tests";
            // 
            // testWaveformVisualiserToolStripMenuItem
            // 
            this.testWaveformVisualiserToolStripMenuItem.Name = "testWaveformVisualiserToolStripMenuItem";
            this.testWaveformVisualiserToolStripMenuItem.Size = new System.Drawing.Size(203, 22);
            this.testWaveformVisualiserToolStripMenuItem.Text = "Test waveform visualiser";
            this.testWaveformVisualiserToolStripMenuItem.Click += new System.EventHandler(this.testWaveformVisualiserToolStripMenuItem_Click);
            // 
            // sequenceVisualiserToolStripMenuItem
            // 
            this.sequenceVisualiserToolStripMenuItem.Name = "sequenceVisualiserToolStripMenuItem";
            this.sequenceVisualiserToolStripMenuItem.Size = new System.Drawing.Size(203, 22);
            this.sequenceVisualiserToolStripMenuItem.Text = "Sequence visualiser";
            this.sequenceVisualiserToolStripMenuItem.Click += new System.EventHandler(this.sequenceVisualiserToolStripMenuItem_Click);
            // 
            // simple1ToolStripMenuItem
            // 
            this.simple1ToolStripMenuItem.Name = "simple1ToolStripMenuItem";
            this.simple1ToolStripMenuItem.Size = new System.Drawing.Size(203, 22);
            this.simple1ToolStripMenuItem.Text = "Simple 1";
            this.simple1ToolStripMenuItem.Click += new System.EventHandler(this.simple1ToolStripMenuItem_Click);
            // 
            // controlToolStripMenuItem
            // 
            this.controlToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.outputsToolStripMenuItem});
            this.controlToolStripMenuItem.Name = "controlToolStripMenuItem";
            this.controlToolStripMenuItem.Size = new System.Drawing.Size(59, 20);
            this.controlToolStripMenuItem.Text = "Control";
            // 
            // outputsToolStripMenuItem
            // 
            this.outputsToolStripMenuItem.Name = "outputsToolStripMenuItem";
            this.outputsToolStripMenuItem.Size = new System.Drawing.Size(152, 22);
            this.outputsToolStripMenuItem.Text = "Outputs";
            this.outputsToolStripMenuItem.Click += new System.EventHandler(this.outputsToolStripMenuItem_Click);
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(0, 24);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.MessagesListBox);
            this.splitContainer1.Size = new System.Drawing.Size(763, 392);
            this.splitContainer1.SplitterDistance = 254;
            this.splitContainer1.TabIndex = 1;
            // 
            // MessagesListBox
            // 
            this.MessagesListBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.MessagesListBox.FormattingEnabled = true;
            this.MessagesListBox.Location = new System.Drawing.Point(0, 0);
            this.MessagesListBox.Name = "MessagesListBox";
            this.MessagesListBox.Size = new System.Drawing.Size(763, 134);
            this.MessagesListBox.TabIndex = 0;
            // 
            // experimentsToolStripMenuItem
            // 
            this.experimentsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toneDiscriminatorToolStripMenuItem,
            this.logicGatesToolStripMenuItem,
            this.exhaustiveSearchToolStripMenuItem});
            this.experimentsToolStripMenuItem.Name = "experimentsToolStripMenuItem";
            this.experimentsToolStripMenuItem.Size = new System.Drawing.Size(83, 20);
            this.experimentsToolStripMenuItem.Text = "Experiments";
            // 
            // toneDiscriminatorToolStripMenuItem
            // 
            this.toneDiscriminatorToolStripMenuItem.Name = "toneDiscriminatorToolStripMenuItem";
            this.toneDiscriminatorToolStripMenuItem.Size = new System.Drawing.Size(174, 22);
            this.toneDiscriminatorToolStripMenuItem.Text = "Tone discriminator";
            this.toneDiscriminatorToolStripMenuItem.Click += new System.EventHandler(this.toneDiscriminatorToolStripMenuItem_Click);
            // 
            // logicGatesToolStripMenuItem
            // 
            this.logicGatesToolStripMenuItem.Name = "logicGatesToolStripMenuItem";
            this.logicGatesToolStripMenuItem.Size = new System.Drawing.Size(174, 22);
            this.logicGatesToolStripMenuItem.Text = "Logic gates";
            this.logicGatesToolStripMenuItem.Click += new System.EventHandler(this.logicGatesToolStripMenuItem_Click);
            // 
            // exhaustiveSearchToolStripMenuItem
            // 
            this.exhaustiveSearchToolStripMenuItem.Name = "exhaustiveSearchToolStripMenuItem";
            this.exhaustiveSearchToolStripMenuItem.Size = new System.Drawing.Size(174, 22);
            this.exhaustiveSearchToolStripMenuItem.Text = "Exhaustive search";
            this.exhaustiveSearchToolStripMenuItem.Click += new System.EventHandler(this.exhaustiveSearchToolStripMenuItem_Click);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(763, 416);
            this.Controls.Add(this.splitContainer1);
            this.Controls.Add(this.menuStrip1);
            this.MainMenuStrip = this.menuStrip1;
            this.Name = "MainForm";
            this.Text = "EM Utilities";
            this.WindowState = System.Windows.Forms.FormWindowState.Maximized;
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.menuStrip1.ResumeLayout(false);
            this.menuStrip1.PerformLayout();
            this.splitContainer1.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.MenuStrip menuStrip1;
        private System.Windows.Forms.ToolStripMenuItem testsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem testWaveformVisualiserToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem sequenceVisualiserToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem simple1ToolStripMenuItem;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.ListBox MessagesListBox;
        private System.Windows.Forms.ToolStripMenuItem controlToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem outputsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem experimentsToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem toneDiscriminatorToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem logicGatesToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem exhaustiveSearchToolStripMenuItem;
    }
}
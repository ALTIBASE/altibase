namespace WindowsApplication1
{
    partial class SqlEditor
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.tabControl1 = new System.Windows.Forms.TabControl();
            this.tabPage1 = new System.Windows.Forms.TabPage();
            this.dataGridView1 = new System.Windows.Forms.DataGridView();
            this.tabPage2 = new System.Windows.Forms.TabPage();
            this.listBox1 = new System.Windows.Forms.ListBox();
            this.tabPage3 = new System.Windows.Forms.TabPage();
            this.listBox2 = new System.Windows.Forms.ListBox();
            this.tabPage4 = new System.Windows.Forms.TabPage();
            this.listBox3 = new System.Windows.Forms.ListBox();
            this.dataSet1 = new System.Data.DataSet();
            this.conn = new Altibase.Data.AltibaseClient.AltibaseConnection();
            this.syntaxRichTextBox1 = new SyntaxHighlighter.SyntaxRichTextBox();
            this.tabControl1.SuspendLayout();
            this.tabPage1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView1)).BeginInit();
            this.tabPage2.SuspendLayout();
            this.tabPage3.SuspendLayout();
            this.tabPage4.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.dataSet1)).BeginInit();
            this.SuspendLayout();
            // 
            // tabControl1
            // 
            this.tabControl1.Controls.Add(this.tabPage1);
            this.tabControl1.Controls.Add(this.tabPage2);
            this.tabControl1.Controls.Add(this.tabPage3);
            this.tabControl1.Controls.Add(this.tabPage4);
            this.tabControl1.Location = new System.Drawing.Point(0, 248);
            this.tabControl1.Name = "tabControl1";
            this.tabControl1.SelectedIndex = 0;
            this.tabControl1.Size = new System.Drawing.Size(529, 305);
            this.tabControl1.TabIndex = 1;
            // 
            // tabPage1
            // 
            this.tabPage1.Controls.Add(this.dataGridView1);
            this.tabPage1.Location = new System.Drawing.Point(4, 21);
            this.tabPage1.Name = "tabPage1";
            this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage1.Size = new System.Drawing.Size(521, 280);
            this.tabPage1.TabIndex = 0;
            this.tabPage1.Text = "Data";
            this.tabPage1.UseVisualStyleBackColor = true;
            // 
            // dataGridView1
            // 
            this.dataGridView1.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.AutoSize;
            this.dataGridView1.Location = new System.Drawing.Point(0, 0);
            this.dataGridView1.Name = "dataGridView1";
            this.dataGridView1.RowTemplate.Height = 23;
            this.dataGridView1.Size = new System.Drawing.Size(523, 283);
            this.dataGridView1.TabIndex = 0;
            // 
            // tabPage2
            // 
            this.tabPage2.Controls.Add(this.listBox1);
            this.tabPage2.Location = new System.Drawing.Point(4, 21);
            this.tabPage2.Name = "tabPage2";
            this.tabPage2.Padding = new System.Windows.Forms.Padding(3);
            this.tabPage2.Size = new System.Drawing.Size(521, 280);
            this.tabPage2.TabIndex = 1;
            this.tabPage2.Text = "Message";
            this.tabPage2.UseVisualStyleBackColor = true;
            // 
            // listBox1
            // 
            this.listBox1.FormattingEnabled = true;
            this.listBox1.ItemHeight = 12;
            this.listBox1.Location = new System.Drawing.Point(0, 0);
            this.listBox1.Name = "listBox1";
            this.listBox1.Size = new System.Drawing.Size(523, 280);
            this.listBox1.TabIndex = 0;
            // 
            // tabPage3
            // 
            this.tabPage3.Controls.Add(this.listBox2);
            this.tabPage3.Location = new System.Drawing.Point(4, 21);
            this.tabPage3.Name = "tabPage3";
            this.tabPage3.Size = new System.Drawing.Size(521, 280);
            this.tabPage3.TabIndex = 2;
            this.tabPage3.Text = "Explain Plan";
            this.tabPage3.UseVisualStyleBackColor = true;
            // 
            // listBox2
            // 
            this.listBox2.FormattingEnabled = true;
            this.listBox2.ItemHeight = 12;
            this.listBox2.Location = new System.Drawing.Point(0, 0);
            this.listBox2.Name = "listBox2";
            this.listBox2.Size = new System.Drawing.Size(523, 280);
            this.listBox2.TabIndex = 0;
            // 
            // tabPage4
            // 
            this.tabPage4.Controls.Add(this.listBox3);
            this.tabPage4.Location = new System.Drawing.Point(4, 21);
            this.tabPage4.Name = "tabPage4";
            this.tabPage4.Size = new System.Drawing.Size(521, 280);
            this.tabPage4.TabIndex = 3;
            this.tabPage4.Text = "History";
            this.tabPage4.UseVisualStyleBackColor = true;
            // 
            // listBox3
            // 
            this.listBox3.FormattingEnabled = true;
            this.listBox3.ItemHeight = 12;
            this.listBox3.Location = new System.Drawing.Point(0, 0);
            this.listBox3.Name = "listBox3";
            this.listBox3.Size = new System.Drawing.Size(523, 280);
            this.listBox3.TabIndex = 0;
            // 
            // dataSet1
            // 
            this.dataSet1.DataSetName = "NewDataSet";
            // 
            // conn
            // 
            this.conn.ConnectionString = "Server=127.0.0.1;Port=20300;User Id=sys;Password=manager;Database=mydb";
            // 
            // syntaxRichTextBox1
            // 
            this.syntaxRichTextBox1.Location = new System.Drawing.Point(0, 0);
            this.syntaxRichTextBox1.Name = "syntaxRichTextBox1";
            this.syntaxRichTextBox1.Size = new System.Drawing.Size(531, 242);
            this.syntaxRichTextBox1.TabIndex = 2;
            this.syntaxRichTextBox1.Text = "";
            // 
            // SqlEditor
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.syntaxRichTextBox1);
            this.Controls.Add(this.tabControl1);
            this.Name = "SqlEditor";
            this.Size = new System.Drawing.Size(531, 555);
            this.tabControl1.ResumeLayout(false);
            this.tabPage1.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.dataGridView1)).EndInit();
            this.tabPage2.ResumeLayout(false);
            this.tabPage3.ResumeLayout(false);
            this.tabPage4.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.dataSet1)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        public System.Windows.Forms.TabControl tabControl1;
        public System.Windows.Forms.TabPage tabPage1;
        public System.Windows.Forms.TabPage tabPage2;
        public System.Windows.Forms.ListBox listBox1;
        public System.Windows.Forms.TabPage tabPage3;
        public System.Windows.Forms.TabPage tabPage4;
        public System.Windows.Forms.ListBox listBox2;
        public System.Windows.Forms.ListBox listBox3;
        public System.Windows.Forms.DataGridView dataGridView1;
        public System.Data.DataSet dataSet1;

        public Altibase.Data.AltibaseClient.AltibaseConnection conn;
        public SyntaxHighlighter.SyntaxRichTextBox syntaxRichTextBox1;
        //public Altibase.Data.AltibaseClient.AltibaseCommand cmd;
        //public Altibase.Data.AltibaseClient.AltibaseDataAdapter dataAdapter1;
    }
}

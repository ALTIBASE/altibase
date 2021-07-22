using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Data;

using Altibase.Data.AltibaseClient;

namespace WindowsApplication1
{
    public partial class Form1 : Form
    {
        Hashtable mainTabControlHT = new Hashtable();
        int mainTabPageID;
//        String del = "Altibase.Data.AltibaseClient.AltibaseException:Altibase ADO.NET ERROR: ";
//        String del = "Altibase.Data.AltibaseClient.AltibaseException:";
//        String split = "\r\n";

        public Form1()
        {
            InitializeComponent();

            mainTabPageID = 1;
        }

        // Connect
        private void toolStripButton1_Click(object sender, EventArgs e)
        {
            ConnNode c_node = new ConnNode();

            String mainTabPageName = c_node.AddConn(tabControl1, mainTabPageID++, textBox1.Text);
            mainTabControlHT.Add(mainTabPageName, c_node);

            EditorNode e_node = (EditorNode)c_node.subTabControlHT[c_node.sub_tabctl.SelectedTab.Name];

            e_node.s_editor.conn.Open();
        }

        // Add SqlEditor
        private void toolStripButton2_Click(object sender, EventArgs e)
        {
            EditorNode e_node = new EditorNode();

            ConnNode t_node = (ConnNode)mainTabControlHT[tabControl1.SelectedTab.Name];
            String subTabPageName = e_node.AddSqlEditor(t_node.sub_tabctl, t_node.subTabPageID++);
            t_node.subTabControlHT.Add(subTabPageName, e_node);
            textBox2.Text = subTabPageName;

            e_node.s_editor.conn.Open();
        }

        private void toolStripButton4_Click(object sender, EventArgs e)
        {
            ConnNode t_node = (ConnNode)mainTabControlHT[tabControl1.SelectedTab.Name];
            EditorNode e_node = (EditorNode)t_node.subTabControlHT[t_node.sub_tabctl.SelectedTab.Name];

            String query = e_node.s_editor.syntaxRichTextBox1.Text.Trim().ToLower();

            if (query.StartsWith("select"))
            {
                select();
            }
            else
            {
                nonSelect();
            }

            incSeq();
        }

        private void select()
        {
            ConnNode t_node = (ConnNode)mainTabControlHT[tabControl1.SelectedTab.Name];
            EditorNode e_node = (EditorNode)t_node.subTabControlHT[t_node.sub_tabctl.SelectedTab.Name];
            AltibaseConnection a_conn = e_node.s_editor.conn;
            DataSet a_dataset = e_node.s_editor.dataSet1;
            DataGridView a_dataGV = e_node.s_editor.dataGridView1;
            //AltibaseDataAdapter a_dataAdapter = e_node.s_editor.dataAdapter1;
            //AltibaseCommand a_cmd = e_node.s_editor.cmd;

            String query = e_node.s_editor.syntaxRichTextBox1.Text;
            AltibaseDataAdapter da = new AltibaseDataAdapter(a_conn.CreateCommand());
            da.SelectCommand = new AltibaseCommand(query, a_conn);

            a_dataset.Clear();

            try
            {
                int result = da.Fill(a_dataset);
                a_dataGV.DataSource = a_dataset.Tables[0];
                setMsg("select", result);
                setHistory();
            }
            catch (Exception ex)
            {
                setErrMsg(ex);
            }
        }

        private void nonSelect()
        {
            ConnNode t_node = (ConnNode)mainTabControlHT[tabControl1.SelectedTab.Name];
            EditorNode e_node = (EditorNode)t_node.subTabControlHT[t_node.sub_tabctl.SelectedTab.Name];
            AltibaseConnection a_conn = e_node.s_editor.conn;
            DataSet a_dataset = e_node.s_editor.dataSet1;

            String query = e_node.s_editor.syntaxRichTextBox1.Text;
            AltibaseCommand cmd = new AltibaseCommand(query, a_conn);
            String split = " ";

            a_dataset.Clear();

            try
            {
                int result = cmd.ExecuteNonQuery();

                string[] query_split = query.Split(split.ToCharArray());

                foreach (string s in query_split)
                {
                    if (s.Trim() != "")
                    {
                        setMsg((String)s, result);
                        break;
                    }
                }
                setHistory();
            }
            catch (Exception ex)
            {
                setErrMsg(ex);
            }
        }

        private void setErrMsg(Exception ex)
        {
            ConnNode t_node = (ConnNode)mainTabControlHT[tabControl1.SelectedTab.Name];
            EditorNode e_node = (EditorNode)t_node.subTabControlHT[t_node.sub_tabctl.SelectedTab.Name];
            String del = "Altibase.Data.AltibaseClient.AltibaseException:";
            String split = "\r\n";

            int i = 1;
//            String msg = ex.ToString();
            String msg = ex.ToString().TrimStart(del.ToCharArray()).Trim();

            string[] msg_split = msg.Split(split.ToCharArray());

            foreach (string s in msg_split)
            {
                if (s.Trim() != "" && s.Trim().StartsWith("at") == false)
                {
                    if (i == 1)
                    {
                        e_node.s_editor.listBox1.Items.Add(e_node.seq + ": " + s);
                    }
                    else
                    {
                        e_node.s_editor.listBox1.Items.Add("     " + s);
                    }
                    i++;
                }
            }

            toolStripStatusLabel2.Text = "Command execute fail.";
        }

        private void setMsg(String s, int cnt)
        {
            ConnNode t_node = (ConnNode)mainTabControlHT[tabControl1.SelectedTab.Name];
            EditorNode e_node = (EditorNode)t_node.subTabControlHT[t_node.sub_tabctl.SelectedTab.Name];
            String tmp = "";

            switch (s)
            {
                case "alter" :
                case "create" :
                case "drop" :
                case "grant" :
                case "revoke" :
                case "rename" :
                case "truncate" :
                case "commit" :
                case "rollback" :
                case "savepoint" :
                    e_node.s_editor.listBox1.Items.Add(e_node.seq + ": " + s.Replace(s.Substring(0,1), s.Substring(0,1).ToUpper()) + " success.");
                    toolStripStatusLabel2.Text = s.Replace(s.Substring(0,1), s.Substring(0,1).ToUpper()) + " success.";
                    break;
                case "select" :
                    tmp = "selected.";
                    break;
                case "insert" :
                    tmp = "inserted.";
                    break;
                case "update" :
                    tmp = "updated.";
                    break;
                case "delete" :
                    tmp = "deleted.";
                    break;
                case "move" :
                    tmp = "moved.";
                    break;
/*                case "ENQUEUE" :
                    break;
                case "DEQUEUE" :
                    break;
*/                default :
                    e_node.s_editor.listBox1.Items.Add(e_node.seq + ": " + " Command execute success.");
                    toolStripStatusLabel2.Text = "Command execute success.";
                    tmp = " ";
                    break;
            }

            if (tmp != "")
            {
                if (cnt == 0)
                {
                    e_node.s_editor.listBox1.Items.Add(e_node.seq + ": No rows " + tmp);
                    toolStripStatusLabel2.Text = "No rows " + tmp;
                }
                else if (cnt == 1)
                {
                    e_node.s_editor.listBox1.Items.Add(e_node.seq + ": 1 row " + tmp);
                    toolStripStatusLabel2.Text = "1 row " + tmp;
                }
                else
                {
                    e_node.s_editor.listBox1.Items.Add(e_node.seq + ": " + cnt + " rows " + tmp);
                    toolStripStatusLabel2.Text = cnt + " rows " + tmp;
                }
            }
        }

        private void setHistory()
        {
            ConnNode t_node = (ConnNode)mainTabControlHT[tabControl1.SelectedTab.Name];
            EditorNode e_node = (EditorNode)t_node.subTabControlHT[t_node.sub_tabctl.SelectedTab.Name];
            String query = e_node.s_editor.syntaxRichTextBox1.Text.Replace("\r\n", " ");

            e_node.s_editor.listBox3.Items.Add(e_node.seq + ": " + query);
        }

        private void incSeq()
        {
            ConnNode t_node = (ConnNode)mainTabControlHT[tabControl1.SelectedTab.Name];
            EditorNode e_node = (EditorNode)t_node.subTabControlHT[t_node.sub_tabctl.SelectedTab.Name];

            e_node.seq++;
        }

        private void connectToolStripMenuItem_Click(object sender, EventArgs e)
        {

        }
    }
}



using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Data;


namespace WindowsApplication1
{
    public class ConnNode
    {
        public TabPage conn_page = new TabPage();
        public TabControl sub_tabctl = new TabControl();
        public EditorNode editor_page = new EditorNode();
        public Hashtable subTabControlHT = new Hashtable();
        public int subTabPageID;

        public ConnNode()
        {
            subTabPageID = 1;
        }

        // Connect 
        public String AddConn(TabControl tabctl, int id, String dsn)
        {
            // 새로운 커넥션을 위해서 main tabcontol에 탭페이지 추가
            conn_page.Name = getMainTabPageName(id);
            conn_page.Text = dsn;
            tabctl.TabPages.Add(conn_page);
            tabctl.SelectedTab = conn_page;

            // 추가한 탭페이지에 sub tabcontrol 추가
            conn_page.Controls.Add(sub_tabctl);
            sub_tabctl.Size = new Size(531, 570);

            // 추가한 sub tabcontrol에 탭페이지와 sqleditor 추가
            String subTabPageName = editor_page.AddSqlEditor(sub_tabctl, subTabPageID++);
            subTabControlHT.Add(subTabPageName, editor_page);

            return conn_page.Name;
        }

        private String getMainTabPageName(int id)
        {
            return "MainTabPage_" + id;
        }
    }

    public class EditorNode
    {
        public TabPage s_editor_page = new TabPage();
        public SqlEditor s_editor = new SqlEditor();
        public int seq;

        public EditorNode()
        {
            seq = 1;
        }

        // SqlEditor 추가 
        public String AddSqlEditor(TabControl tabctl, int id)
        {
            // sub tabcontrol에 탭페이지 추가
            s_editor_page.Name = getSubTabPageName(id);
            s_editor_page.Text = "SqlEditor";
            tabctl.TabPages.Add(s_editor_page);
            tabctl.SelectedTab = s_editor_page;

            // 추가한 탭페이지에 sqleditor 추가
            s_editor_page.Controls.Add(s_editor);

            reg_keywords();

            return s_editor_page.Name;
        }

        private String getSubTabPageName(int id)
        {
            return "SubTabPage_" + id;
        }

        private void reg_keywords()
        {
            // Add the keywords to the list.
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ADD");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("AFTER");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("AGER");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ALL");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ALTER");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("AND");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ANY");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ARCHIVE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ARCHIVELOG");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("AS");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ASC");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("BACKUP");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("BEFORE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("BEGIN");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("BY");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CASCADE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CASE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CLOSE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("COLUMN");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("COMPILE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("COMMIT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CONNECT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CONSTRAINT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CONSTRAINTS");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CONTINUE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CREATE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CURSOR");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CYCLE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DECLARE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DEFAULT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DELETE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DEQUEUE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DESC");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DISABLE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DIRECTORY");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DISCONNECT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DISTINCT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DROP");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("EACH");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ELSE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ELSEIF");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ENABLE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("END");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ENQUEUE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ESCAPE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("EXCEPTION");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("EXEC");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("EXECUTE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("EXIT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("FALSE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("FETCH");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("FIFO");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("FLUSH");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("FOR");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("FOREIGN");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("FROM");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("FULL");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("FUNCTION");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("GOTO");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("GRANT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("GROUP");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("HAVING");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("IF");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("IN");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("INNER");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("INSERT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("INTERSECT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("INTO");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("IS");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ISOLATION");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("JOIN");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("KEY");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("LEFT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("LEVEL");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("LIFO");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("LIKE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("LIMIT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("LOGANCHOR");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("LOOP");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("MOVE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("NEW");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("NO");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("NOARCHIVELOG");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("NOCYCLE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("NOT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("NULL");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("OF");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("OFF");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("OLD");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ON");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("OPEN");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("OR");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ORDER");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("OUT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("OUTER");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("PRIMARY");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("PRIOR");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("PRIVILEGES");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("PROCEDURE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("PUBLIC");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("QUEUE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("READ");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("RECOVER");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("REFERENCES");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("REFERENCING");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("RESTRICT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("RETURN");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("REVOKE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("RIGHT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ROLLBACK");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ROW");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("SAVEPOINT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("SELECT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("SEQUENCE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("SESSION");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("SET");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("SOME");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("START");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("STATEMENT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("TABLE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("TEMPORARY");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("THEN");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("TO");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("TRUE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("TRIGGER");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("TYPE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("TYPESET");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("UNION");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("UNIQUE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("UNTIL");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("UPDATE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("USER");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("USING");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("VALUES");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("VARIABLE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("VIEW");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("WHEN");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("WHERE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("WHILE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("WITH");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("WORK");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("WRITE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("BETWEEN");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("EXISTS");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CONSTANT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("IDENTIFIED");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("INDEX");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("MINUS");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("MODE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("OTHERS");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("RAISE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("RENAME");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("REPLACE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ROWTYPE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("WAIT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DATABASE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ELSIF");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("EXTENTSIZE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("FIXED");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("LOCK");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("MAXROWS");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ONLINE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("OFFLINE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("REPLICATION");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("REVERSE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ROWCOUNT");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("SQLCODE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("SQLERRM");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("STEP");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("TABLESPACE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("TRUNCATE");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("SYNONYM");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("PARALLEL");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("NOPARALLEL");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ENABLE_RECPTR");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("DISABLE_RECPTR");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("CLEAR_RECPTRS");
            s_editor.syntaxRichTextBox1.Settings.Keywords.Add("ENABLEALL_RECPTRS");

            // Set the comment identifier. 
            // For Lua this is two minus-signs after each other (--).
            // For C++ code we would set this property to "//".
            s_editor.syntaxRichTextBox1.Settings.Comment = "--";
            s_editor.syntaxRichTextBox1.Settings.Comment = "//";

            // Set the colors that will be used.
            s_editor.syntaxRichTextBox1.Settings.KeywordColor = Color.Blue;
            s_editor.syntaxRichTextBox1.Settings.CommentColor = Color.Green;
            s_editor.syntaxRichTextBox1.Settings.StringColor = Color.Gray;
            s_editor.syntaxRichTextBox1.Settings.IntegerColor = Color.Red;

            // Let's not process strings and integers.
            s_editor.syntaxRichTextBox1.Settings.EnableStrings = true;
            s_editor.syntaxRichTextBox1.Settings.EnableIntegers = true;

            // Let's make the settings we just set valid by compiling
            // the keywords to a regular expression.
            s_editor.syntaxRichTextBox1.CompileKeywords();

            // Load a file and update the syntax highlighting.
            //syntaxRichTextBox1.LoadFile("script.txt", RichTextBoxStreamType.PlainText);
            s_editor.syntaxRichTextBox1.ProcessAllLines();
        }
    }
}

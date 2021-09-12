#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <locale.h>
#include <windows.h>

#undef __IUPDEF_H
#include <iup.h>
#include <iupcontrols.h>
#include <iupcbs.h>

#include "csv.h"

Ihandle * global_multitext = NULL;
Ihandle* mat = NULL;

static void fill(Ihandle* mat)
{
	struct CsvHandle_ handle = {0};
	handle.delim = ';';
	handle.quote = '"';
	handle.escape = '\\';
	CsvOpen(&handle, "../src/2030.csv");
	int r = 1;
	while (1)
	{
		int c = 2;
		char * row = CsvReadNextRow(&handle);
		if (row == NULL) {break;}
		while (1)
		{
			const char * col = CsvReadNextCol(row, &handle);
			if (col == NULL) {break;}
			IupSetStrAttributeId2(mat, "", r, c, col);
			c++;
		}
		{
			char buf[10];
			snprintf (buf, 10, "%i", r);
			IupSetStrAttributeId2(mat, "", r, 1, buf);
		}
		printf("Row %i: Number of cols %i\n", r, c);
		r++;
	}
}

static int click(Ihandle *self, int lin, int col)
{
	char * s = IupGetAttributeId2(self, "", lin, col);
	//printf("click_cb(%d, %d) %s\n", lin, col, s);
	IupSetAttribute(global_multitext, "VALUE", s);  /* clear all marks */
	IupSetInt(global_multitext, "r", lin);
	IupSetInt(global_multitext, "column", col);
	//IupSetAttribute(self,"MARKED", NULL);  /* clear all marks */
	//IupSetAttributeId2(self,"MARK", lin, 0, "1");
	//IupSetfAttribute(self,"REDRAW", "L%d", lin);
	return IUP_DEFAULT;
}

static Ihandle* create_matrix(void)
{
	mat = IupMatrix(NULL);
	IupSetAttribute(mat, "NUMLIN", "100");
	IupSetAttribute(mat, "NUMCOL", "10");
	IupSetCallback(mat,"ENTERITEM_CB",(Icallback)click);
	fill(mat);
	return mat;
}

static int action(Ihandle *self)
{
	int r = IupGetInt(self, "r");
	int c = IupGetInt(self, "column");
	char * s = IupGetAttribute(self, "VALUE");
	printf("action %i %i %s\n", r, c, s);
	IupSetStrAttributeId2(mat, "", r, c, s);
	IupRedraw(mat, 0);
}


void MatrixTest(void)
{
	Ihandle* dlg, *box, *mat;
	global_multitext = IupText(NULL);
	IupSetAttribute(global_multitext, "READONLY", "NO");
	IupSetAttribute(global_multitext, "MULTILINE", "YES");
	IupSetAttribute(global_multitext, "EXPAND", "YES");
	IupSetAttribute(global_multitext, "WORDWRAP", "YES");
	IupSetCallback(global_multitext, "VALUECHANGED_CB", (Icallback) action);
	//IupSetAttribute(mat, "SIZE", "200x");

	mat = create_matrix();
	box = IupVbox(mat, global_multitext, NULL);
	IupSetAttribute(box, "MARGIN", "20x20");
	dlg = IupDialog(box);
	IupSetAttribute(dlg, "TITLE", "IupMatrix Simple Test");
	IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
}





int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);
	//setlocale( LC_ALL, "en_US.UTF-8" );
	IupOpen(&argc, &argv);
	printf ("%s", IupGetGlobal("UTF8MODE"));
	IupSetGlobal("UTF8MODE", "YES");
	printf ("%s", IupGetGlobal("UTF8MODE"));
	IupControlsOpen();
	MatrixTest();
	IupMainLoop();
	IupClose();
	return EXIT_SUCCESS;
}

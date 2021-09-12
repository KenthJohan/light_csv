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
#include "test_img.h"
#include "csv.h"




/*
static void fill(Ihandle* mat)
{
	FILE * fp = fopen("../testiup/Utbildningar.Produktion2030.csv", "rb+");
	int r = 1;
	int c = 1;
	char line_buffer[1024] = {0};
	while(1)
	{
		int n = fread(line_buffer, 1024, 1, fp);
		if(n <= 0){break;}
		char * a = strstr(line_buffer, "\r\n");
		if (a)
		{
			IupSetStrAttributeId2(mat, "", r, c, line_buffer);
			a[0] = '\0';
			fflush(stdout);
			r++;
		}
		printf("%s",line_buffer);
		memset(line_buffer, 0, 1024);
		//replace(line_buffer, ';', 1024);
	}
	fclose(fp);
}
*/



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


static Ihandle* create_matrix(void)
{
	Ihandle* mat = IupMatrix(NULL);
	IupSetAttribute(mat, "NUMLIN", "100");
	IupSetAttribute(mat, "NUMCOL", "30");
	fill(mat);
	return mat;
}

void MatrixTest(void)
{
	Ihandle* dlg, *box, *mat, *multitext;
	multitext = IupText(NULL);
	IupSetAttribute(multitext, "MULTILINE", "YES");
	IupSetAttribute(multitext, "EXPAND", "YES");

	mat = create_matrix();
	box = IupVbox(mat, multitext, NULL);
	IupSetAttribute(box, "MARGIN", "20x20");
	dlg = IupDialog(box);
	IupSetAttribute(dlg, "TITLE", "IupMatrix Simple Test");
	IupShowXY(dlg, IUP_CENTER, IUP_CENTER);
}

#ifndef BIG_TEST
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
#endif

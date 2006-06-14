/* common.c
 * Copyright (C) 2006 Dale Whittaker <dayul@users.sf.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "core.h"

/*function to get the date and time*/
char *get_current_time(void)
{
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo= localtime(&rawtime);

	return(asctime(timeinfo));
}

/*run an error dialog reporting error*/
int run_error_dialog(char *errmsg, ...)
{
	GtkWidget *dialog;
	va_list list;
	char errstring[200];
	
	/*create a va_list and display it as a dialog*/
	va_start(list, errmsg); 
	g_vsnprintf(errstring, 200, errmsg, list);
	dialog= gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, errstring);
	
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	va_end(list);
	
	return 1;
}

static void error_and_log_func(char *errmsg, va_list args)
{
	char *ptimestring;
	
	/*output to stderr and logfile*/
	vfprintf(stderr, errmsg, args);
	fflush(stderr);

	ptimestring= get_current_time();
	if(ptimestring[strlen(ptimestring)- 1]== '\n')
		ptimestring[strlen(ptimestring)- 1]= '\0';
	
	if(config.logfile!= NULL)
	{
		fprintf(config.logfile, "%s: ", ptimestring);
		vfprintf(config.logfile, errmsg, args);
	
		fflush(config.logfile);
	}
}

/*function to report error, log it, and then exit*/
int error_and_log(char *errmsg, ...)
{
	/*create va_list of arguments*/
	va_list list;
	
	va_start(list, errmsg); 
	error_and_log_func(errmsg, list);
	va_end(list);

	exit(EXIT_FAILURE);

	return 0; /*shouldnt really ever happen*/
}

/*function to report error and log*/
int error_and_log_no_exit(char *errmsg, ...)
{
	/*create va_list of arguments*/
	va_list list;
	
	va_start(list, errmsg);
	error_and_log_func(errmsg, list);
	va_end(list);
	
	return 1;
}


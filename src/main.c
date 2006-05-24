/* main.c
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

/*function to print mailtc options on command line*/
static void print_usage_and_exit(void)
{
	error_and_log(S_MAIN_ERR_PRINT_USAGE, 
				 PACKAGE, VERSION, PACKAGE, PACKAGE, PACKAGE, PACKAGE, PACKAGE);
}

/*function to run the mailtc configuration dialog*/
static int config_dialog_start(int argc, char* argv[])
{
	/*initialise gtk and run the dialog*/
	GtkWidget *dialog= NULL;
	gtk_init(&argc, &argv);  
	run_config_dialog(dialog);
	gtk_main();
	
	return EXIT_SUCCESS;
}

/*function to read from the pid file*/
static int read_pid_file(int action)
{
	FILE *tmppidfile, *pidfile;
	char pidstring[PORT_LEN];
	char pidfilename[NAME_MAX], tmppidfilename[NAME_MAX];
	int retval= 1;
	
	/*get the full paths for the files*/
	get_account_file(pidfilename, PID_FILE, -1);
	get_account_file(tmppidfilename, PID_FILE, 0);
	
	/*rename the pidfile to a temp pid file*/
	if((access(pidfilename, F_OK)!= -1)&& (rename(pidfilename, tmppidfilename)== -1))
		error_and_log(S_MAIN_ERR_RENAME_PIDFILE, pidfilename, tmppidfilename);

	if(access(tmppidfilename, F_OK)!= -1)
	{	
		int instance_running= 0;
		
		/*open the pid file and the temp pid file*/
		if((tmppidfile= fopen(tmppidfilename, "r"))== NULL)
			error_and_log(S_MAIN_ERR_OPEN_PIDFILE_READ, tmppidfilename);
		
		if((pidfile= fopen(pidfilename, "w"))== NULL)
			error_and_log(S_MAIN_ERR_OPEN_PIDFILE_WRITE, pidfilename);
		
		memset(pidstring, '\0', PORT_LEN);
		
		/*read each value from temp file and write it to the pid file unless it is the current pid*/
		while(fgets(pidstring, PORT_LEN, tmppidfile)!= NULL)
		{
			pidstring[strlen(pidstring)- 1]= '\0';
			
			/*if it is a valid process*/
			if(kill(atoi(pidstring), 0)== 0)
			{
				int currentpid= (atoi(pidstring)== getpid());
				switch(action)
				{
					/*Load the app, if no other instances are already running*/
					case PID_APPLOAD:
						if(!currentpid)
						{
							instance_running= 1;
							fprintf(pidfile, "%s\n", pidstring);
						}
					break;

					/*Exit the app, output any other running processes to pidfile
					 *(theoretically no other process should be active, but if they are, leave them)*/
					case PID_APPEXIT:
						if(!currentpid)
							fprintf(pidfile, "%s\n", pidstring);
					break;
					
					/*Kill all mailtc processes (theoretically should only be max 1 running)*/
					case PID_APPKILL:
						if((!currentpid)&& (kill(atoi(pidstring), SIGHUP)!= 0))
						{
							error_and_log_no_exit(S_MAIN_ERR_CANNOT_KILL, currentpid);
							fprintf(pidfile, "%s\n", pidstring);
						}
					break;
				}
			}
		}
		
		/*check flag and run dialog to say instance is already running, and exit*/
		/*otherwise add the new process*/
		if(action== PID_APPLOAD)
		{
			if(instance_running)
				retval= 0;
			else
				fprintf(pidfile, "%d\n", getpid());
		}
		/*close the files and cleanup*/
		if(fclose(pidfile)== EOF)
			error_and_log(S_MAIN_ERR_CLOSE_PIDFILE);
							
		if(fclose(tmppidfile)== EOF)
			error_and_log(S_MAIN_ERR_CLOSE_PIDFILE);
			
		remove(tmppidfilename);
			 
	}
	
	return(retval);
}

/*function to cleanup in case the program is killed*/
void term_handler(int signal)
{
	/*remove the pid and report relevant message*/
	read_pid_file(PID_APPEXIT);

	if(signal== SIGSEGV)
		error_and_log(S_MAIN_ERR_SEGFAULT, PACKAGE);
	else
		error_and_log(S_MAIN_ERR_APP_KILLED, PACKAGE);
}

/*function to write the initial header to the log file when mailtc starts*/
static int init_files(void)
{
	char logfilename[NAME_MAX];
	
	paccounts= NULL;
	
	/*get the path for the program*/
	memset(&files, '\0', sizeof(mailtc_files));
	get_program_dir();

	/* here we need to create our dir*/
	mkdir(files.base_name, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);

	/*set the logfilename*/
	get_account_file(logfilename, LOG_FILE, -1);
	
	/*clear the structures*/
	memset(&config, '\0', sizeof(config_details));
	
	/*open the logfile for writing*/
	if((files.logfile= fopen(logfilename, "wt"))== NULL) /*open log file for appending*/
	{	
		perror(S_MAIN_ERR_OPEN_LOGFILE);
		exit(EXIT_FAILURE);
	}
	
	/*write the log header*/
	fprintf(files.logfile, "\n*******************************************\n");
	fprintf(files.logfile, S_MAIN_LOG_STARTED, PACKAGE, get_current_time());
	fprintf(files.logfile, "*******************************************\n"); 
	fflush(files.logfile);
	
	/*add the pid to the file and return*/
	return 1;
}

/*function called before program exits*/
static void cleanup(void)
{
	/*close the log file*/
	if(fclose(files.logfile)== EOF)
	{	
		perror(S_MAIN_ERR_CLOSE_LOGFILE);
		exit(EXIT_FAILURE);
	}
	
	/*remove the pid from the file*/
	read_pid_file(PID_APPEXIT);
}

/*function to run the warning dialog*/
static int run_warning_dlg(int argc, char* argv[], char *msg, int startconfig)
{
	/*init gtk to run the dialog*/
	GtkWidget *dialog;
	
	gtk_init(&argc, &argv);
	dialog= gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, msg, PACKAGE);
	gtk_dialog_run(GTK_DIALOG(dialog)); 
	gtk_widget_destroy(dialog);
	
	/*run the config dialog and return*/
	return((startconfig)? config_dialog_start(argc, argv): 1);
}


/*the main function*/
int main(int argc, char *argv[])
{
	gint sleeptime;
	gint func_ref;
	
#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif /*ENABLE_NLS*/

	paccounts= NULL;
	
	/*cleanup at the end*/
	if(atexit(free_accounts)!= 0)
	{
		perror(S_MAIN_ERR_ATEXIT_FUNC);
		exit(EXIT_FAILURE);
	}
	
	/*first step is to check our arguments are valid*/
	if((argc!= 2)&& (argc!=1))
		print_usage_and_exit();

	/*intitialise the stuctures*/
	init_files();

	/*check if instance is running*/
	if(((argc== 1)||
	   ((argc== 2)&&((strcmp(argv[1], "-d")== 0)|| strcmp(argv[1], "-c")== 0)))&& 
	   (!read_pid_file(PID_APPLOAD)))
	{
		error_and_log_no_exit(S_MAIN_ERR_INSTANCE_RUNNING, PACKAGE);
		cleanup();
		return(run_warning_dlg(argc, argv, S_MAIN_INSTANCE_RUNNING, 0));
	}
	
	/*setup to cleanup on exit*/
	/*atexit(cleanup);*/
	signal(SIGHUP, term_handler);
	signal(SIGQUIT, term_handler);
	signal(SIGTERM, term_handler);
	signal(SIGABRT, term_handler);
	signal(SIGINT, term_handler);
	signal(SIGSEGV, term_handler);
	
	/*if mailtc or mailtc -d*/
	if((argc== 1) || ((argc== 2)&& (strcmp(argv[1], "-d")== 0))) 
	{	
		/*set debug mode if -d*/
		config.net_debug= (argc== 1)? 0: 1;
	
		/*check mail details and run dialog if none found*/
		read_accounts();
		if((paccounts== NULL)|| !(read_config_file()))
		{	
			return(run_warning_dlg(argc, argv, S_MAIN_NO_CONFIG_FOUND, 1));
		}
		/*code to check that icon is valid (e.g for old mailtc versions*/
		else
		{
			if(paccounts->icon[0]!= '#')
				return(run_warning_dlg(argc, argv, S_MAIN_OLD_VERSION_FOUND, 1));
		}
	}
	/*if mailtc -c*/
	else if((argc== 2)&& strcmp(argv[1], "-c")== 0)
		return(config_dialog_start(argc, argv));
	/*if mailtc -k*/
	else if((argc== 2)&& strcmp(argv[1], "-k")== 0)
		return(!read_pid_file(PID_APPKILL));
	/*invalid option*/
	else
		print_usage_and_exit();

	/*MAIN LOOP*/
	/*initialise gtk stuff for docklet*/
	gtk_init(&argc, &argv);

	/*set the time interval for mail checks from the check_delay variable*/
	if(sscanf(config.check_delay, "%d", &sleeptime)!= 1)
		error_and_log(S_MAIN_ERR_DELAY_INFO);

	sleeptime= (sleeptime* 60* 1000);

	/*call the mail thread for the initial mail check
	 *(otherwise it will wait a full minute or more before initial check)*/
	mail_thread(NULL);
	
	/*call the mail thread to check every sleeptime milliseconds*/
	func_ref= g_timeout_add(sleeptime, mail_thread, NULL);
	gtk_main();
	g_source_remove(func_ref);
	
	cleanup();

	return EXIT_SUCCESS;

}




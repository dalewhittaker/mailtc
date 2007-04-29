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

enum mtc_mode { MODE_NONE= 0, MODE_NORMAL, MODE_DEBUG, MODE_CFG, MODE_KILL };
static gint func_ref= 0;

/*function to print mailtc options on command line*/
static void return_usage(void)
{
#ifdef MTC_USE_PIDFUNC
	g_fprintf(stderr, S_MAIN_ERR_PRINT_USAGE, PACKAGE, VERSION, PACKAGE, PACKAGE, PACKAGE, PACKAGE, PACKAGE);
#else
	g_fprintf(stderr, S_MAIN_ERR_PRINT_USAGE_NOKILL, PACKAGE, VERSION, PACKAGE, PACKAGE, PACKAGE, PACKAGE);
#endif /*MTC_USE_PIDFUNC*/	
	exit(EXIT_FAILURE);
}

/*function to run the mailtc configuration dialog*/
static gint cfgdlg_start()
{
	/*initialise gtk and run the dialog*/
	GtkWidget *dialog= NULL;
	cfgdlg_run(dialog);
	gtk_main();
	
	return(EXIT_SUCCESS);
}

/*function to read from the pid file*/
#ifdef MTC_USE_PIDFUNC
static gboolean pid_read(gint action)
{
	FILE *tmppidfile, *pidfile;
	gchar pidstring[PORT_LEN];
	gchar pidfilename[NAME_MAX], tmppidfilename[NAME_MAX];
	gboolean retval= TRUE;
	gboolean instance_running= FALSE;
	int pid= 0;

	/*get the full paths for the files*/
	mtc_file(pidfilename, PID_FILE, -1);
	mtc_file(tmppidfilename, PID_FILE, 0);
	
	/*rename the pidfile to a temp pid file*/
	if((IS_FILE(pidfilename))&& (g_rename(pidfilename, tmppidfilename)== -1))
		err_exit(S_MAIN_ERR_RENAME_PIDFILE, pidfilename, tmppidfilename);

    /*get the current pid*/
    pid= getpid();

	
		if((pidfile= g_fopen(pidfilename, "w"))== NULL)
			err_exit(S_MAIN_ERR_OPEN_PIDFILE_WRITE, pidfilename);
	
        if(IS_FILE(tmppidfilename))
	    {	
		
		    /*open the pid file and the temp pid file*/
		    if((tmppidfile= g_fopen(tmppidfilename, "r"))== NULL)
			    err_exit(S_MAIN_ERR_OPEN_PIDFILE_READ, tmppidfilename);
		
		    memset(pidstring, '\0', PORT_LEN);
		
		    /*read each value from temp file and write it to the pid file unless it is the current pid*/
		    while(fgets(pidstring, PORT_LEN, tmppidfile)!= NULL)
		    {
                g_strchomp(pidstring);
			
			    /*if it is a valid process*/
			    if(kill(atoi(pidstring), 0)== 0)
			    {
				    gint currentpid;
                
                    currentpid= (atoi(pidstring)== pid);
				    switch(action)
				    {
					    /*Load the app, if no other instances are already running*/
					    case PID_APPLOAD:
						    if(!currentpid)
						    {
							    instance_running= TRUE;
							    g_fprintf(pidfile, "%s\n", pidstring);
						    }
					    break;

					    /*Exit the app, output any other running processes to pidfile
					    *(theoretically no other process should be active, but if they are, leave them)*/
					    case PID_APPEXIT:
						    if(!currentpid)
							    g_fprintf(pidfile, "%s\n", pidstring);
					    break;
					
					    /*Kill all mailtc processes (theoretically should only be max 1 running)*/
					    case PID_APPKILL:
						    if((!currentpid)&& (kill(atoi(pidstring), SIGHUP)!= 0))
						    {
							    err_noexit(S_MAIN_ERR_CANNOT_KILL, currentpid);
							    g_fprintf(pidfile, "%s\n", pidstring);
						    }
					    break;
				    }
			    }
		    }
	
            if(fclose(tmppidfile)== EOF)
			err_exit(S_MAIN_ERR_CLOSE_PIDFILE);
			
		    g_remove(tmppidfilename);
		
	    }
		/*check flag and run dialog to say instance is already running, and exit*/
		/*otherwise add the new process*/
		if(action== PID_APPLOAD)
		{
			if(instance_running)
				retval= FALSE;
			else
				g_fprintf(pidfile, "%d\n", pid);
		}
		
        /*close the files and cleanup*/
		if(fclose(pidfile)== EOF)
			err_exit(S_MAIN_ERR_CLOSE_PIDFILE);
							 
	return(retval);
}
#endif /*MTC_USE_PIDFUNC*/

/*initialise the tray icon widget*/
static gboolean trayicon_init(void)
{
    mtc_trayicon *ptrayicon;
	
    ptrayicon= &config.trayicon;

#ifdef MTC_EGGTRAYICON
    /*create the trayicon event box, and tooltip*/
	ptrayicon->box= gtk_event_box_new();
	g_object_ref(G_OBJECT(ptrayicon->box));
    ptrayicon->tooltip= gtk_tooltips_new();
	
    g_signal_connect(G_OBJECT(ptrayicon->box), "button-press-event", G_CALLBACK(docklet_clicked), NULL);
#else
    ptrayicon->docklet= gtk_status_icon_new();
    if(gtk_status_icon_get_visible(ptrayicon->docklet))
        gtk_status_icon_set_visible(ptrayicon->docklet, FALSE);

    /*add left and right click handlers*/
    g_signal_connect(G_OBJECT(ptrayicon->docklet), "popup-menu", G_CALLBACK(docklet_rclicked), NULL);
    g_signal_connect(G_OBJECT(ptrayicon->docklet), "activate", G_CALLBACK(docklet_lclicked), NULL);

    ptrayicon->active= ACTIVE_ICON_NONE;

#endif /*MTC_EGGTRAYICON*/
	return(TRUE);
}

/*destroy the tray icon widget*/
static gboolean trayicon_destroy(void)
{
    mtc_trayicon *ptrayicon;
	
    ptrayicon= &config.trayicon;

#ifdef MTC_EGGTRAYICON
	/*destroy the tooltip and box first*/
	if(ptrayicon->box)
	{
		g_object_unref(G_OBJECT(ptrayicon->box));
		gtk_widget_destroy(GTK_WIDGET(ptrayicon->box));
		ptrayicon->box= NULL;
    }
    /*NOTE important that the tooltip is destroyed after the box*/
    if(ptrayicon->tooltip)
	{
		gtk_object_destroy(GTK_OBJECT(ptrayicon->tooltip));
		ptrayicon->tooltip= NULL;
	}
#endif /*MTC_EGGTRAYICON*/

	/*destroy the widget then unref the docklet*/
	if(ptrayicon->docklet)
	{
		g_object_unref(G_OBJECT(ptrayicon->docklet));
#ifdef MTC_EGGTRAYICON
        /*always disconnect prior to destroying the widget*/
        g_signal_handlers_disconnect_by_func(G_OBJECT(ptrayicon->docklet), G_CALLBACK(docklet_destroyed), NULL);
		gtk_widget_destroy(GTK_WIDGET(ptrayicon->docklet)); 
#endif /*MTC_EGGTRAYICON*/
		ptrayicon->docklet= NULL;
	}

    return(TRUE);
}

/*function called when the app exits*/
static void atexit_func(void)
{
    mtc_icon *picon= NULL;
    
	/*remove the source if it was active*/
	if(func_ref)
		g_source_remove(func_ref);

#ifdef MTC_EXPERIMENTAL
    /*destroy the summary dialog if it exists*/
    if(config.run_summary)
        sumdlg_destroy();
#endif

    /*destroy the trayicon widget*/
    trayicon_destroy();

	/*free the account list*/
	free_accounts();

    /*free the 'multi' icon*/
    picon= &config.icon;
    if(picon->image)
		g_object_unref(G_OBJECT(picon->image));

#ifndef MTC_EGGTRAYICON
    if(picon->pixbuf)
        g_object_unref(G_OBJECT(picon->pixbuf));
#endif /*MTC_EGGTRAYICON*/

	/*unload the plugins and free the plugin list*/
	plg_unload_all();	
	
	/*remove the pid from the file*/
#ifdef MTC_USE_PIDFUNC
	pid_read(PID_APPEXIT);
#endif /*MTC_USE_PIDFUNC*/

	/*finally, close the log file if it is open*/
	if((config.logfile!= NULL)&& fclose(config.logfile)== EOF)
		g_printerr("%s %s\n", S_MAIN_ERR_CLOSE_LOGFILE, g_strerror(errno));
}

/*function to cleanup in case the program is killed*/
void term_handler(gint signal)
{

	if(signal== SIGSEGV)
		err_exit(S_MAIN_ERR_SEGFAULT, PACKAGE);
	else
		err_exit(S_MAIN_ERR_APP_KILLED, PACKAGE);
}

/*function to write the initial header to the log file when mailtc starts*/
static gboolean mtc_init(void)
{
	gchar logfilename[NAME_MAX];
	
	/*initialise stuff*/
	acclist= NULL;
	config.logfile= NULL;

	/*clear the structures*/
	memset(&config, '\0', sizeof(mtc_cfg));
	
	/*get the path for the program*/
	mtc_dir();

	/* here we need to create our dir*/
	if(!IS_DIR(config.dir))
	{
		if(FILE_EXISTS(config.dir))
		{
			g_printerr(S_MAIN_ERR_NOT_DIRECTORY, config.dir, PACKAGE);
			exit(EXIT_FAILURE);
		}
		else
			g_mkdir(config.dir, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
	}
	/*set the logfilename*/
	mtc_file(logfilename, LOG_FILE, -1);

	/*open the logfile for writing*/
	if((config.logfile= g_fopen(logfilename, "wt"))== NULL) /*open log file for appending*/
	{	
		g_printerr("%s %s\n", S_MAIN_ERR_OPEN_LOGFILE, g_strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	/*write the log header*/
	g_fprintf(config.logfile, "\n*******************************************\n");
	g_fprintf(config.logfile, S_MAIN_LOG_STARTED, PACKAGE, str_time());
	g_fprintf(config.logfile, "*******************************************\n"); 
	fflush(config.logfile);
	
	return TRUE;
}

/*function to run the warning dialog*/
static gint warndlg_run(gchar *msg, gboolean startconfig)
{
	/*init gtk to run the dialog*/
	GtkWidget *dialog;
	
	dialog= gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, msg, PACKAGE);
	gtk_dialog_run(GTK_DIALOG(dialog)); 
	gtk_widget_destroy(dialog);
	
	/*run the config dialog and return*/
	return((startconfig)? cfgdlg_start(): EXIT_FAILURE);
}

/*the main function*/
gint main(gint argc, gchar *argv[])
{
	guint sleeptime;
	enum mtc_mode runmode= MODE_NONE;

	/*first step is to check our arguments are valid*/
	if((argc!= 2)&& (argc!= 1))
		return_usage();

/*now setup the gettext stuff if needed*/
#ifdef HAVE_LOCALE_H
    setlocale (LC_ALL, "");
#endif /*HAVE_LOCALE_H*/
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
#endif /*ENABLE_NLS*/

	acclist= NULL;

	/*cleanup at the end*/
	g_atexit(atexit_func);
	
	/*setup to cleanup on exit*/
	signal(SIGHUP, term_handler);
	signal(SIGQUIT, term_handler);
	signal(SIGTERM, term_handler);
	signal(SIGABRT, term_handler);
	signal(SIGINT, term_handler);
	signal(SIGSEGV, term_handler);

    /*determine the run mode*/
    if(argc== 1)
        runmode= MODE_NORMAL;
    else if(argc== 2)
    {
        if(g_ascii_strcasecmp(argv[1], "-d")== 0)
            runmode= MODE_DEBUG;
        else if(g_ascii_strcasecmp(argv[1], "-c")== 0)
            runmode= MODE_CFG;
#ifdef MTC_USE_PIDFUNC	
        else if(g_ascii_strcasecmp(argv[1], "-k")== 0)
            runmode= MODE_KILL;
#endif /*MTC_USE_PIDFUNC*/
    }

    if(runmode== MODE_NONE)
        return_usage();

	/*intitialise the stuctures*/
	mtc_init();

#ifdef MTC_USE_PIDFUNC	
	/*if mailtc -k*/
	if(runmode== MODE_KILL)
		return(!pid_read(PID_APPKILL));
#endif /*MTC_USE_PIDFUNC*/

    /*initialise gtk first*/
	gtk_init(&argc, &argv);
	
	/*check if instance is running*/
#ifdef MTC_USE_PIDFUNC	
    if(!pid_read(PID_APPLOAD))
	{
		err_noexit(S_MAIN_ERR_INSTANCE_RUNNING, PACKAGE);
		return(warndlg_run(S_MAIN_INSTANCE_RUNNING, FALSE));
	}
	/*load the network plugins*/
	else 
#endif /*MTC_USE_PIDFUNC*/
    	if(!plg_load_all())
		{
			err_noexit(S_MAIN_ERR_LOAD_PLUGINS);
			return(warndlg_run(S_MAIN_LOAD_PLUGINS, FALSE));
		}

    /*if mailtc -c*/
	if(runmode== MODE_CFG)
	{
        config.isdlg= TRUE;
        return(cfgdlg_start());
    }

	/*if mailtc or mailtc -d*/
	else if(runmode== MODE_NORMAL|| runmode== MODE_DEBUG)
	{	
        gboolean cfgfound= FALSE;

    	/*set debug mode if -d*/
	    config.net_debug= (runmode== MODE_DEBUG);
	    config.isdlg= FALSE;

        /*check mail details and run dialog if none found*/
	    cfgfound= cfg_read();
    	read_accounts();
		if(acclist== NULL|| !cfgfound)
		{	
			return(warndlg_run(S_MAIN_NO_CONFIG_FOUND, TRUE));
		}
		/*code to check that icon is valid (e.g for old mailtc versions*/
		else
		{
			mtc_account *pacclist;
            mtc_icon *picon;

			pacclist= (mtc_account *)acclist->data;
			picon= &pacclist->icon;

            if(picon->colour[0]!= '#')
				return(warndlg_run(S_MAIN_OLD_VERSION_FOUND, TRUE));

            /*initialise the tray icon widget*/
            trayicon_init();
		}
	
        /*MAIN LOOP*/
	    /*set the time interval for mail checks from the check_delay variable*/
	    sleeptime= (gint)config.check_delay* 60* 1000;

	    /*call the mail thread for the initial mail check
	    *(otherwise it will wait a full minute or more before initial check)*/
	    mail_thread(NULL);
	
        /*call the mail thread to check every sleeptime milliseconds*/
	    func_ref= g_timeout_add(sleeptime, mail_thread, NULL);
	    gtk_main();
	    /*g_source_remove(func_ref);*/

	}
	return(EXIT_SUCCESS);

}




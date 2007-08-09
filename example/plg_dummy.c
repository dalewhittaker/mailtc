/* plg_dummy.c
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

/*A dummy mailtc network plugin*/
/*This is a simple example of how to write a mailtc network plugin*/
#include <stdio.h> /*included for FILE *, and printf() etc*/
#include <stdlib.h> /*included for rand()*/
#include <time.h> /*included for rand() seed*/

#include "plugin.h" /*must be included in order to work*/

/*This MUST match the mailtc revision it is used with, if not, mailtc will report that it is an invalid plugin*/
#define PLUGIN_NAME "Dummy"
#define PLUGIN_AUTHOR "Dale Whittaker (dayul@users.sf.net)"
#define PLUGIN_DESC "An example network plugin."
#define DEFAULT_PORT 123

/*pointer used to write to mailtc log*/
static mtc_cfg *pcfg= NULL;

/*log file that will point to the mailtc log*/
G_MODULE_EXPORT mtc_plugin *init_plugin(void);

/*this is called every n minutes by mailtc to check for new messages*/
mtc_error dummy_get_messages(gpointer pdata)
{
	/*get our pointer to the mail_details structure*/
	/*see the file plugin.h for possible structure member values that can be used*/
	mtc_account *paccount= (mtc_account *)pdata;

	/*as an example we print the account name to stdout and stderr when the function is called*/
	printf("Get messages for account %s\n", paccount->name);
	
	/*generate a random number between 1 and 10 and return*/
	/*this returns a random number of new messages*/
    srand(time(NULL));
    paccount->msginfo.new_messages= rand()% 10;
    paccount->msginfo.num_messages= paccount->msginfo.new_messages;
	return(MTC_RETURN_TRUE);
}

/*this is called when the plugin is first loaded. it is here that we set globals passed from the core app*/
mtc_error dummy_load(gpointer pdata)
{
    pcfg= (mtc_cfg *)pdata;
	
    /*print the information when loaded*/
	fprintf(pcfg->logfile, PLUGIN_NAME " plugin loaded\n");
	return(MTC_RETURN_TRUE);
}

/*this is called when unloading, one use for this is to free memory if needed*/
mtc_error dummy_unload(void)
{
    /*when unloading a plugin, it can't be assumed that the log file exists, print to stdout if it doesn't*/
    if(pcfg!= NULL && pcfg->logfile!= NULL)
        fprintf(pcfg->logfile, PLUGIN_NAME " plugin unloaded\n");
    else
        printf(PLUGIN_NAME " plugin unloaded\n");
	return(MTC_RETURN_TRUE);
}

/*this is called when the docklet is clicked*/
mtc_error dummy_clicked(gpointer pdata)
{
	/*get the pointer to the mail_details structure*/
	mtc_account *paccount= (mtc_account *)pdata;

	/*report to stdout and log that icon has been clicked for the account*/
	printf("Clicked: read messages for account %s\n", paccount->name);
	
	return(MTC_RETURN_TRUE);
}

/*setup all our plugin stuff so mailtc knows what to do*/
static mtc_plugin dummy_pluginfo =
{
	NULL, /*pointer to handle, set to NULL*/
	VERSION,
	PLUGIN_NAME,
	PLUGIN_AUTHOR,
	PLUGIN_DESC,
	0/*MTC_ENABLE_FILTERS*/, /*this enables filter options in the dialog, set to MTC_ENABLE_FILTERS if you want filters*/
	DEFAULT_PORT,
	&dummy_load, /*function called when plugin is loaded*/
	&dummy_unload, /*function called when plugin is unloaded*/
	&dummy_get_messages, /*function called every n minutes by mailtc to get number of messages*/
	&dummy_clicked /*function called when the mail icon is clicked in the system tray*/
};

/*the initialisation function leave this as is*/
G_MODULE_EXPORT mtc_plugin *init_plugin(void)
{
	/*set the plugin pointer passed to point to the struct*/
	return(&dummy_pluginfo);
}


/* plg_dummy.c
 * Copyright (C) 2007 Dale Whittaker <dayul@users.sf.net>
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

#include <gtk/gtklabel.h> /*GtkLabel*/
#include <gtk/gtktable.h> /*GtkTable*/
#include <gtk/gtkentry.h> /*GtkEntry*/

#include "plugin.h" /*must be included in order to work*/

/*This MUST match the mailtc revision it is used with, if not, mailtc will report that it is an invalid plugin*/
#define PLUGIN_NAME "Dummy"
#define PLUGIN_AUTHOR "Dale Whittaker (dayul@users.sf.net)"
#define PLUGIN_DESC "An example network plugin."
#define DEFAULT_PORT 123
#define ELEMENT_DUMMYOPT "dummy_options"
#define ELEMENT_OPT1 "opt1"

/*define a structure that will hold the plugin option*/
typedef struct _dummy_opts
{
    gchar myopt1[NAME_MAX];

} dummy_opts;

/*pointer used to write to mailtc log*/
static mtc_cfg *pcfg= NULL;

/*create some Gtk widgets for the plugin option tab*/
static GtkWidget *dummy_table= NULL;
static GtkWidget *dummy_entry= NULL;

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

	/*report to stdout that icon has been clicked for the account*/
	printf("Clicked: read messages for account %s\n", paccount->name);
	
	return(MTC_RETURN_TRUE);
}

/*this is called when an account is removed*/
mtc_error dummy_remove(gpointer pdata, guint *naccounts)
{
	mtc_account *paccount= (mtc_account *)pdata;

    /*print to stdout when account is removed*/
	printf("Remove account %d of %d (%s)\n", paccount->id, *naccounts, paccount->name);
    
    return(MTC_RETURN_TRUE);
}

/*this is called when showing configuration options*/
gpointer dummy_get_config(gpointer pdata)
{
    /*TODO work here (e.g ref counts)*/
	mtc_account *paccount= (mtc_account *)pdata;
    GtkWidget *dummy_title;
    GtkWidget *dummy_label;

    /*create a table containing widgets and return it to be shown*/
    dummy_title= gtk_label_new(PLUGIN_NAME " plugin options:");
    dummy_label= gtk_label_new("example option:");
    dummy_entry= gtk_entry_new();
    dummy_table= gtk_table_new(2, 2, FALSE);

	gtk_table_set_col_spacings(GTK_TABLE(dummy_table), 10);
	gtk_table_set_row_spacings(GTK_TABLE(dummy_table), 20);
    gtk_container_set_border_width(GTK_CONTAINER(dummy_table), 10);
    gtk_table_attach(GTK_TABLE(dummy_table), dummy_title, 0, 2, 0, 1, GTK_FILL| GTK_EXPAND, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(dummy_table), dummy_label, 0, 1, 1, 2, GTK_FILL| GTK_EXPAND, GTK_SHRINK, 0, 0);
    gtk_table_attach(GTK_TABLE(dummy_table), dummy_entry, 1, 2, 1, 2, GTK_FILL| GTK_EXPAND, GTK_SHRINK, 0, 0);
    
    /*if there are plugin options, get them*/
    if((paccount!= NULL)&& paccount->plg_opts!= NULL)
    {
        dummy_opts *popts;
        popts= (dummy_opts *)paccount->plg_opts;
        
        /*set the value*/
        if(*popts->myopt1!= 0)
            gtk_entry_set_text(GTK_ENTRY(dummy_entry), popts->myopt1);
    }
    return((gpointer)dummy_table);
}

/*this is called when storing configuration options*/
mtc_error dummy_put_config(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    dummy_opts *popts;
    
    /*create an options struct if there isn't one already*/
    if(paccount->plg_opts== NULL)
        paccount->plg_opts= (gpointer)g_malloc0(sizeof(dummy_opts));

    /*copy the entry value to the option struct*/
    popts= (dummy_opts *)paccount->plg_opts;
    g_strlcpy(popts->myopt1, gtk_entry_get_text(GTK_ENTRY(dummy_entry)), sizeof(popts->myopt1));
    
    return(MTC_RETURN_TRUE);
}

/*this is called when reading options from the configuration file*/
mtc_error dummy_read_config(xmlDocPtr doc, xmlNodePtr node, gpointer pdata)
{
    xmlNodePtr child= NULL;

    /*get the child option*/
    child= node->children;
        
    while(child!= NULL)
    {
        if((child!= NULL)&& (xmlStrEqual(child->name, BAD_CAST ELEMENT_OPT1))&&
            (child->type== XML_ELEMENT_NODE)&& !(xmlIsBlankNode(child->children)))
        {
	        mtc_account *paccount= (mtc_account *)pdata;
            xmlChar *pcontent;
            dummy_opts *popts;

            /*valid node, get the content*/
            pcontent= xmlNodeListGetString(doc, child->children, 1);

            /*create an options struct if there isn't one already*/
            if(paccount->plg_opts== NULL)
                paccount->plg_opts= (gpointer)g_malloc0(sizeof(dummy_opts));

            /*copy the entry value to the option struct*/
            popts= (dummy_opts *)paccount->plg_opts;
            g_strlcpy(popts->myopt1, (gchar *)pcontent, sizeof(popts->myopt1));
 
            xmlFree(pcontent);
        }
        child= child->next;
    }
    return(MTC_RETURN_TRUE);
}

/*this is called when writing the configuration options to file*/
mtc_error dummy_write_config(xmlNodePtr node, gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    if(paccount!= NULL)
    {
        dummy_opts *popts= (dummy_opts *)paccount->plg_opts;
        
        if(popts!= NULL&& *popts->myopt1!= 0)
        {
            /*write the plugin option to the config file*/
            xmlNewChild(node, NULL, BAD_CAST ELEMENT_OPT1, BAD_CAST popts->myopt1);
        }
    }
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
	MTC_HAS_PLUGIN_OPTS, /*the flags that the plugin sets.  Add MTC_HAS_PLUGIN_OPTS if you wish to use custom plugin options*/
	DEFAULT_PORT,
	&dummy_load, /*function called when plugin is loaded*/
	&dummy_unload, /*function called when plugin is unloaded*/
	&dummy_get_messages, /*function called every n minutes by mailtc to get number of messages*/
	&dummy_clicked, /*function called when the mail icon is clicked in the system tray*/
    &dummy_remove, /*function called when an account is removed from the config dialog*/
    &dummy_get_config, /*function called when requesting plugin config options*/
    &dummy_put_config, /*function called when storing plugin config options prior to write*/
    &dummy_read_config, /*function called when reading plugin config from file*/
    &dummy_write_config /*function called when writing plugin config to file*/
};

/*the initialisation function leave this as is*/
G_MODULE_EXPORT mtc_plugin *init_plugin(void)
{
	/*set the plugin pointer passed to point to the struct*/
	return(&dummy_pluginfo);
}


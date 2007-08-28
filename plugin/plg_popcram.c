/* plg_popcram.c
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

/*The POP CRAM-MD5 plugin*/
#include "popfunc.h"

/*This MUST match the mailtc revision it is used with, if not, mailtc will report that it is an invalid plugin*/
#define PLUGIN_NAME "POP (CRAM-MD5)"
#define PLUGIN_AUTHOR "Dale Whittaker (dayul@users.sf.net)"
#define PLUGIN_DESC "POP3 network plugin with CRAM-MD5 authentication."
#define DEFAULT_PORT 110

G_MODULE_EXPORT mtc_plugin *init_plugin(void);

/*this is called every n minutes by mailtc to check for new messages*/
mtc_error popcram_get_messages(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
	return(check_crampop_mail(paccount, cfg_get()));
}

/*this is called when the plugin is first loaded. it is here that we set globals passed from the core app*/
mtc_error popcram_load(gpointer pdata)
{
	/*set the network debug flag and log file*/
    cfg_load((mtc_cfg *)pdata);
	g_fprintf(((mtc_cfg  *)pdata)->logfile, PLUGIN_NAME " plugin loaded\n");
	return(MTC_RETURN_TRUE);
}

/*this is called when unloading, one use for this is to free memory if needed*/
mtc_error popcram_unload(void)
{
	cfg_unload();
	return(MTC_RETURN_TRUE);
}

/*this is called when the docklet is clicked*/
mtc_error popcram_clicked(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
	return(pop_read_mail(paccount, cfg_get()));
}

/*this is called when an account is removed*/
mtc_error popcram_remove(gpointer pdata, guint *naccounts)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(rm_uidfile(paccount, *naccounts));
}

/*this is called when showing configuration options*/
gpointer popcram_get_config(gpointer pdata)
{
    /*TODO work here*/
    return(NULL);
}

/*this is called when storing configuration options*/
mtc_error popcram_put_config(gpointer pdata)
{
    /*TODO work here*/
    return(MTC_RETURN_TRUE);
}

/*this is called when writing the configuration options to file*/
mtc_error popcram_write_config(xmlNodePtr node, gpointer pdata)
{
    /*TODO work here*/
    return(MTC_RETURN_TRUE);
}

/*setup all our plugin stuff so mailtc knows what to do*/
static mtc_plugin popcram_pluginfo =
{
	NULL, /*pointer to handle, set to NULL*/
	VERSION,
	PLUGIN_NAME,
	PLUGIN_AUTHOR,
	PLUGIN_DESC,
	MTC_ENABLE_FILTERS,
	DEFAULT_PORT,
	&popcram_load,
	&popcram_unload, 
	&popcram_get_messages,
	&popcram_clicked,
    &popcram_remove,
    &popcram_get_config,
    &popcram_put_config,
    &popcram_write_config
};

/*the initialisation function*/
G_MODULE_EXPORT mtc_plugin *init_plugin(void)
{
	/*set the plugin pointer passed to point to the struct*/
	return(&popcram_pluginfo);
}


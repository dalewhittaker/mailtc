/* plg_apop.c
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

/*The APOP plugin*/
#include "popfunc.h"

#ifdef MTC_NOTMINIMAL
#include "filter.h"
#define PLUGIN_FLAGS MTC_HAS_PLUGIN_OPTS
#else
#define PLUGIN_FLAGS 0
#endif /*MTC_NOTMINIMAL*/

/*This MUST match the mailtc revision it is used with, if not, mailtc will report that it is an invalid plugin*/
#define PLUGIN_NAME "POP (APOP)"
#define PLUGIN_AUTHOR "Dale Whittaker (dayul@users.sf.net)"
#define PLUGIN_DESC "POP3 network plugin with APOP authentication."
#define DEFAULT_PORT 110

G_MODULE_EXPORT mtc_plugin *init_plugin(void);

/*this is called every n minutes by mailtc to check for new messages*/
mtc_error apop_get_messages(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
	return(check_apop_mail(paccount, cfg_get()));
}

/*this is called when the plugin is first loaded. it is here that we set globals passed from the core app*/
mtc_error apop_load(gpointer pdata)
{
	/*set the network debug flag and log file*/
    cfg_load((mtc_cfg *)pdata);
	g_fprintf(((mtc_cfg  *)pdata)->logfile, PLUGIN_NAME " plugin loaded\n");
	return(MTC_RETURN_TRUE);
}

/*this is called when unloading, one use for this is to free memory if needed*/
mtc_error apop_unload(void)
{
    filter_unload();
    cfg_unload();
	return(MTC_RETURN_TRUE);
}

/*this is called when the docklet is clicked*/
mtc_error apop_clicked(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
	return(pop_read_mail(paccount, cfg_get()));
}

/*this is called when an account is removed*/
mtc_error apop_remove(gpointer pdata, guint *naccounts)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(rm_uidfile(paccount, *naccounts));
}

#ifdef MTC_NOTMINIMAL
/*this is called when showing configuration options*/
gpointer apop_get_config(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return((gpointer)filter_table(paccount, PLUGIN_NAME));
}

/*this is called when storing configuration options*/
mtc_error apop_put_config(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(filter_enabled(paccount));
}

/*this is called when reading options from the configuration file*/
mtc_error apop_read_config(xmlDocPtr doc, xmlNodePtr node, gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(read_filters(doc, node, paccount));
}

/*this is called when writing the configuration options to file*/
mtc_error apop_write_config(xmlNodePtr node, gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(filter_write(node, paccount));
}

/*this is called when freeing an account*/
mtc_error apop_free(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(free_filters(paccount));
}
#endif /*MTC_NOTMINIMAL*/

/*setup all our plugin stuff so mailtc knows what to do*/
static mtc_plugin apop_pluginfo =
{
	NULL, /*pointer to handle, set to NULL*/
	VERSION,
	PLUGIN_NAME,
	PLUGIN_AUTHOR,
	PLUGIN_DESC,
	PLUGIN_FLAGS,
	DEFAULT_PORT,
	&apop_load,
	&apop_unload, 
	&apop_get_messages,
	&apop_clicked,
    &apop_remove,
#ifdef MTC_NOTMINIMAL
    &apop_get_config,
    &apop_put_config,
    &apop_read_config,
    &apop_write_config,
    &apop_free
#else
    NULL, NULL, NULL, NULL, NULL
#endif /*MTC_NOTMINIMAL*/
};

/*the initialisation function*/
G_MODULE_EXPORT mtc_plugin *init_plugin(void)
{
	/*set the plugin pointer passed to point to the struct*/
	return(&apop_pluginfo);
}


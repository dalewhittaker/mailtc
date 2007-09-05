/* plg_imap.c
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

/*The IMAP4 plugin*/
#include "imapfunc.h"

#ifdef MTC_NOTMINIMAL
#include "filter.h"
#define PLUGIN_FLAGS MTC_HAS_PLUGIN_OPTS
#else
#define PLUGIN_FLAGS 0
#endif /*MTC_NOTMINIMAL*/

/*This MUST match the mailtc revision it is used with, if not, mailtc will report that it is an invalid plugin*/
#define PLUGIN_NAME "IMAP"
#define PLUGIN_AUTHOR "Dale Whittaker (dayul@users.sf.net)"
#define PLUGIN_DESC "IMAP4 network plugin."
#define DEFAULT_PORT 143

G_MODULE_EXPORT mtc_plugin *init_plugin(void);

/*this is called every n minutes by mailtc to check for new messages*/
mtc_error imap_get_messages(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
	return(check_imap_mail(paccount, cfg_get()));
}

/*this is called when the plugin is first loaded. it is here that we set globals passed from the core app*/
mtc_error imap_load(gpointer pdata)
{
	/*set the network debug flag and log file*/
    cfg_load((mtc_cfg *)pdata);

	g_fprintf(((mtc_cfg  *)pdata)->logfile, PLUGIN_NAME " plugin loaded\n");
	return(MTC_RETURN_TRUE);
}

/*this is called when unloading, one use for this is to free memory if needed*/
mtc_error imap_unload(void)
{
	cfg_unload();
	return(MTC_RETURN_TRUE);
}

/*this is called when the docklet is clicked*/
mtc_error imap_clicked(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
	return(imap_read_mail(paccount, cfg_get()));
}

/*this is called when an account is removed*/
mtc_error imap_remove(gpointer pdata, guint *naccounts)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(rm_uidfile(paccount, *naccounts));
}

#ifdef MTC_NOTMINIMAL
/*this is called when showing configuration options*/
gpointer imap_get_config(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return((gpointer)filter_table(paccount, PLUGIN_NAME));
}

/*this is called when storing configuration options*/
mtc_error imap_put_config(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(filter_enabled(paccount));
}

/*this is called when reading options from the configuration file*/
mtc_error imap_read_config(xmlDocPtr doc, xmlNodePtr node, gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(read_filters(doc, node, paccount));
}

/*this is called when writing the configuration options to file*/
mtc_error imap_write_config(xmlNodePtr node, gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(filter_write(node, paccount));
}

/*this is called when freeing an account*/
mtc_error imap_free(gpointer pdata)
{
	mtc_account *paccount= (mtc_account *)pdata;
    return(free_filters(paccount));
}
#endif /*MTC_NOTMINIMAL*/

/*setup all our plugin stuff so mailtc knows what to do*/
static mtc_plugin imap_pluginfo =
{
	NULL, /*pointer to handle, set to NULL*/
	VERSION,
	PLUGIN_NAME,
	PLUGIN_AUTHOR,
	PLUGIN_DESC,
	PLUGIN_FLAGS,
	DEFAULT_PORT,
	&imap_load,
	&imap_unload, 
	&imap_get_messages,
	&imap_clicked,
    &imap_remove,
#ifdef MTC_NOTMINIMAL
    &imap_get_config,
    &imap_put_config,
    &imap_read_config,
    &imap_write_config,
    &imap_free
#else
    NULL, NULL, NULL, NULL, NULL
#endif /*MTC_NOTMINIMAL*/
};

/*the initialisation function*/
G_MODULE_EXPORT mtc_plugin *init_plugin(void)
{
	/*set the plugin pointer passed to point to the struct*/
	return(&imap_pluginfo);
}


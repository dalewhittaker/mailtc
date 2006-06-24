/* plg_apop.c
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

/*The APOP plugin*/
#include "plg_common.h"

/*This MUST match the mailtc revision it is used with, if not, mailtc will report that it is an invalid plugin*/
#define PLUGIN_NAME "POP (APOP)"
#define PLUGIN_AUTHOR "Dale Whittaker (dayul@users.sf.net)"
#define PLUGIN_DESC "POP3 network plugin with APOP authentication."
#define DEFAULT_PORT 110

/*this is called every n minutes by mailtc to check for new messages*/
int apop_get_messages(void *pdata)
{
	mail_details *paccount= (mail_details *)pdata;
	return(check_apop_mail(paccount, mtc_dir));
}

/*this is called when the plugin is first loaded. it is here that we set globals passed from the core app*/
int apop_load(const char *cfgdir, void *plog, unsigned int flags)
{
	/*set the network debug flag and log file*/
	mtc_dir= cfgdir;
	net_debug= flags& MTC_DEBUG_MODE;
	plglog= (FILE *)plog;
	
	fprintf(plglog, PLUGIN_NAME " plugin loaded\n");
	return(MTC_RETURN_TRUE);
}

/*this is called when unloading, one use for this is to free memory if needed*/
int apop_unload(void)
{
	plglog= NULL;
	return(MTC_RETURN_TRUE);
}

/*this is called when the docklet is clicked*/
int apop_clicked(void *pdata)
{
	mail_details *paccount= (mail_details *)pdata;
	return(pop_read_mail(paccount, mtc_dir));
}

/*setup all our plugin stuff so mailtc knows what to do*/
static mtc_plugin_info apop_pluginfo =
{
	NULL, /*pointer to handle, set to NULL*/
	VERSION,
	PLUGIN_NAME,
	PLUGIN_AUTHOR,
	PLUGIN_DESC,
	MTC_ENABLE_FILTERS,
	DEFAULT_PORT,
	&apop_load,
	&apop_unload, 
	&apop_get_messages,
	&apop_clicked
};

/*the initialisation function*/
mtc_plugin_info *init_plugin(void)
{
	/*set the plugin pointer passed to point to the struct*/
	return(&apop_pluginfo);
}


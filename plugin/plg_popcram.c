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
#include "plg_common.h"

/*This MUST match the mailtc revision it is used with, if not, mailtc will report that it is an invalid plugin*/
#define PLUGIN_NAME "POP (CRAM-MD5)"
#define PLUGIN_AUTHOR "Dale Whittaker (dayul@users.sf.net)"
#define PLUGIN_DESC "POP3 network plugin with CRAM-MD5 authentication."
#define DEFAULT_PORT 110

/*this is called every n minutes by mailtc to check for new messages*/
int popcram_get_messages(void *pdata, const char *cfgdir, void *plog, unsigned int flags)
{
	/*set the network debug flag and log file*/
	mail_details *paccount= (mail_details *)pdata;
	net_debug= flags& MTC_DEBUG_MODE;
	plglog= (FILE *)plog;
	
	return(check_crampop_mail(paccount, cfgdir));
}

/*this is called when unloading, one use for this is to free memory*/
/*int unload(void *paccount)
{
	printf("unload %d\n", paccount->id);
	return 1;
}*/

/*this is called when the docklet is clicked*/
int popcram_clicked(void *pdata, const char *cfgdir)
{
	mail_details *paccount= (mail_details *)pdata;
	return(pop_read_mail(paccount, cfgdir));
}

/*setup all our plugin stuff so mailtc knows what to do*/
static mtc_plugin_info popcram_pluginfo =
{
	NULL, /*pointer to handle, set to NULL*/
	VERSION,
	PLUGIN_NAME,
	PLUGIN_AUTHOR,
	PLUGIN_DESC,
	MTC_ENABLE_FILTERS,
	DEFAULT_PORT,
	NULL/*load*/, /*currently does nothing*/
	NULL/*unload*/, /*currently does nothing*/
	&popcram_get_messages,
	&popcram_clicked
};

/*the initialisation function*/
mtc_plugin_info *init_plugin(void)
{
	/*set the plugin pointer passed to point to the struct*/
	return(&popcram_pluginfo);
}

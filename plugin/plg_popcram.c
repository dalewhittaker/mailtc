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
#define MTC_REVISION 0.1
#define PLUGIN_NAME "POP (CRAM-MD5)"
#define PLUGIN_AUTHOR "Dale Whittaker (dayul@users.sf.net)"
#define PLUGIN_DESC "POP3 network plugin with CRAM-MD5 authentication."

/*simply calls check_pop_mail with correct params*/
static int check_crampop_mail(mail_details *paccount, const char *cfgdir)
{
	enum pop_protocol protocol= POPCRAM_PROTOCOL;
	return(check_pop_mail(paccount, cfgdir, protocol));
}

/*this is called every n minutes by mailtc to check for new messages*/
int load(mail_details *paccount, const char *cfgdir, unsigned int flags, FILE *plog)
{
	/*set the network debug flag and log file*/
	net_debug= flags& MTC_DEBUG_MODE;
	plglog= plog;

	return(check_crampop_mail(paccount, cfgdir));
}

/*this is called when unloading, one use for this is to free memory*/
/*int unload(mail_details *paccount)
{
	printf("unload %d\n", paccount->id);
	return 1;
}*/

/*this is called when the docklet is clicked*/
int clicked(mail_details *paccount)
{
	/*TODO we need to sort this bit*/
	printf("docklet clicked %d!\n", paccount->id);
	return(MTC_RETURN_TRUE);
}

/*setup all our plugin stuff so mailtc knows what to do*/
mtc_plugin_info pluginfo =
{
	MTC_REVISION,
	(const char *)PLUGIN_NAME,
	(const char *)PLUGIN_AUTHOR,
	(const char *)PLUGIN_DESC,
	MTC_ENABLE_FILTERS,
	load,
	NULL/*unload*/, /*currently nothing needs to be unloaded*/
	clicked
};


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
#define MTC_REVISION 0.1
#define PLUGIN_NAME "POP (APOP)"
#define PLUGIN_AUTHOR "Dale Whittaker (dayul@users.sf.net)"
#define PLUGIN_DESC "POP3 network plugin with APOP authentication."

/*function to create md5 digest for APOP authentication*/
unsigned int encrypt_apop_string(char *decstring, char *encstring)
{
	unsigned int md_len= 0, i;
	char octet[3]= "";
	
	EVP_MD_CTX ctx;
	const EVP_MD *md;
	unsigned char md_value[EVP_MAX_MD_SIZE]; /*string to hold the digest*/

	/*Enable openssl to use all digests and set digest to MD5*/
	OpenSSL_add_all_digests();
	md= EVP_md5();
	
	/*Initialise message digest cipher content*/
	EVP_MD_CTX_init(&ctx);
	if(!EVP_DigestInit_ex(&ctx, md, NULL))
		error_and_log_no_exit(S_PLG_APOP_ERR_DIGEST_INIT);

	/*Encrypt the string and final padding to MD5*/
	if(!EVP_DigestUpdate(&ctx, (unsigned char *)decstring, strlen(decstring)))
		error_and_log_no_exit(S_PLG_APOP_ERR_DIGEST_CREATE);
	if(!EVP_DigestFinal_ex(&ctx, md_value, &md_len))
		error_and_log_no_exit(S_PLG_APOP_ERR_DIGEST_FINAL);

	/*cleanup*/
	EVP_MD_CTX_cleanup(&ctx);
	
	/*convert digest into hex string for APOP and return length of string*/
	for(i=0; i< md_len; i++)
	{	
		sprintf(octet, "%02x", md_value[i]);
		strcat(encstring, octet);
	}	
	EVP_cleanup();

	return(md_len);
	
}

/*simply calls check_pop_mail with correct params*/
static int check_apop_mail(mail_details *paccount, const char *cfgdir)
{
	enum pop_protocol protocol= APOP_PROTOCOL;
	return(check_pop_mail(paccount, cfgdir, protocol));
}

/*this is called every n minutes by mailtc to check for new messages*/
int load(mail_details *paccount, const char *cfgdir, unsigned int flags, FILE *plog)
{
	/*set the network debug flag and log file*/
	net_debug= flags& MTC_DEBUG_MODE;
	plglog= plog;

	return(check_apop_mail(paccount, cfgdir));
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


/* plg_common.c
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

/*common functions that the default plugins use*/
#include "plg_common.h"

/*pointer to the config structure, this is added here for convenience
 *NOTE i don't like this*/
static mtc_cfg *pcfg;

/*get the configuration struct*/
mtc_cfg *cfg_get(void)
{
    return(pcfg);
}

/*initialise the configuration struct*/
void cfg_load(mtc_cfg *pconfig)
{
    pcfg= pconfig;
}

/*unitialise the configuration struct*/
void cfg_unload(void)
{
    pcfg= NULL;
}

/*Report the time to the log
 *by default, asctime() adds a line feed at the end of the string, so we remove it*/
static void print_time(void)
{
	time_t rawtime;
	struct tm *timeinfo;
	gchar *ptimestring;

	/*get the local time string and print it*/
	time(&rawtime);
	timeinfo= localtime(&rawtime);
	
	ptimestring= asctime(timeinfo);
	g_fprintf(pcfg->logfile, "%s: plugin: ", g_strchomp(ptimestring));
}

/*function to report error and log*/
gint plg_err(gchar *errmsg, ...)
{
	/*create va_list of arguments*/
	va_list list;
	va_start(list, errmsg);
	
	/*output to stderr and logfile*/
	g_vfprintf(stderr, errmsg, list);
	print_time();
	g_vfprintf(pcfg->logfile, errmsg, list);
	
	va_end(list);
	fflush(stderr);
	fflush(pcfg->logfile);
	
	return(MTC_RETURN_TRUE);
}

/*function to return relevant filename in mailtc directory filename*/
gchar *mtc_file(gchar *fullpath, const gchar *cfgdir, gchar *filename, guint account)
{
	/*clear the buffer and create the full name from dir and filename*/
	memset(fullpath, '\0', NAME_MAX);
    g_snprintf(fullpath, NAME_MAX, "%s%c%s%d", cfgdir, G_DIR_SEPARATOR, filename, account);
	return(fullpath);
}

/*create CRAM-MD5 string to send to server*/
#ifdef SSL_PLUGIN
gchar *mk_cramstr(mtc_account *paccount, gchar *serverdigest, gchar *clientdigest)
{
	HMAC_CTX ctx;
	gchar octet[3];
	gchar *tmpbuf= NULL;
	guchar keyed[16];
	gint slen= 0;
    guint i= 0, keyed_len= 0, tmpbuflen= 0;
	
    memset(keyed, '\0', sizeof(keyed));
	keyed_len= sizeof(keyed);
	slen= strlen(serverdigest);
	tmpbuf= (gchar *)g_malloc0(sizeof(gchar)* (slen+ 1));

	/*stage 1, decode the base64 encoded string from the server*/
	if(EVP_DecodeBlock((guchar *)tmpbuf, (guchar *)serverdigest, slen)> slen)
	{
		plg_err(S_PLG_COMMON_ERR_BASE64_DECODE);
		g_free(tmpbuf);
		return(NULL);
	}

	/*stage 2, now do the keyed-md5 (hmac) stuff*/
	HMAC_CTX_init(&ctx);
	HMAC_Init(&ctx, (guchar *)paccount->password, strlen(paccount->password), EVP_md5());
	HMAC_Update(&ctx, (guchar *)tmpbuf, strlen(tmpbuf));
	HMAC_Final(&ctx, keyed, &keyed_len);
	HMAC_CTX_cleanup(&ctx);
	g_free(tmpbuf);

	/*stage 3, we now do "username digest"*/
    tmpbuflen= (keyed_len* 2)+ strlen(paccount->username)+ 2;
	tmpbuf= (gchar *)g_malloc0(tmpbuflen);
	g_strlcpy(tmpbuf, paccount->username, tmpbuflen);
	g_strlcat(tmpbuf, " ", tmpbuflen);

	/*This is a bit crap, but works*/
	for(i=0; i< keyed_len; i++)
	{	
		g_snprintf(octet, sizeof(octet), "%02x", keyed[i]);
		g_strlcat(tmpbuf, octet, tmpbuflen);
	}
	
	/*stage 4, encode keyed digest back to base64*/
	/*allocate memory for our final digest to send to server*/
    tmpbuflen= strlen(tmpbuf);
	slen= 4 *((tmpbuflen+ 2)/ 3);
	clientdigest= (gchar *)g_malloc0(slen+ 1);
	
	/*now base64 encode the string*/
	if(EVP_EncodeBlock((guchar *)clientdigest, (guchar *)tmpbuf, tmpbuflen)!= slen)
	{
		plg_err(S_PLG_COMMON_ERR_BASE64_ENCODE);
		g_free(tmpbuf);
		return(NULL);
	}
	g_free(tmpbuf);
	clientdigest[slen]= '\0';
	return(clientdigest);
}

/*function to create md5 digest for APOP authentication*/
guint apop_encrypt(gchar *decstring, gchar *encstring)
{
	guint md_len= 0, i;
	gchar octet[3];
	
	EVP_MD_CTX ctx;
	const EVP_MD *md;
	guchar md_value[EVP_MAX_MD_SIZE]; /*string to hold the digest*/

	/*Enable openssl to use all digests and set digest to MD5*/
	OpenSSL_add_all_digests();
	md= EVP_md5();
	
	/*Initialise message digest cipher content*/
	EVP_MD_CTX_init(&ctx);
	if(!EVP_DigestInit_ex(&ctx, md, NULL))
		plg_err(S_PLG_APOP_ERR_DIGEST_INIT);

	/*Encrypt the string and final padding to MD5*/
	if(!EVP_DigestUpdate(&ctx, (guchar *)decstring, strlen(decstring)))
		plg_err(S_PLG_APOP_ERR_DIGEST_CREATE);
	if(!EVP_DigestFinal_ex(&ctx, md_value, &md_len))
		plg_err(S_PLG_APOP_ERR_DIGEST_FINAL);

	/*cleanup*/
	EVP_MD_CTX_cleanup(&ctx);
	
	/*convert digest into hex string for APOP and return length of string*/
	for(i= 0; i< md_len; i++)
	{	
		g_snprintf(octet, sizeof(octet), "%02x", md_value[i]);
		g_strlcat(encstring, octet, sizeof(encstring)+ 1);
	}	
	EVP_cleanup();

	return(md_len);
	
}
#endif /*SSL_PLUGIN*/

/*case insensitive search returns position, or -1*/
gint strstr_cins(gchar *haystack, gchar *needle)
{
	guint i= 0, j= 0, nlen= 0;

    nlen= strlen(needle);

	/*iterate though each haystack char*/
	for(i= 0; i< strlen(haystack); i++)
	{
		/*if char found to be same as first needle char*/
		if(g_ascii_tolower(haystack[i])== g_ascii_tolower(needle[0]))
		{
			j= 0;
			/*compare needle chars with subsequent haystack chars*/
			for(j= 0; j< nlen; j++)
			{
				if(g_ascii_tolower(haystack[i+ j])!= g_ascii_tolower(needle[j]))
					break;
			}	
			/*all chars found so success*/
			if(j== nlen)
				return((gint)i);
		}
	}
	return -1;
}


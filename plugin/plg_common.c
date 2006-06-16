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

/*Report the time to the log
 *by default, asctime() adds a line feed at the end of the string, so we remove it*/
static void print_time_string(void)
{
	time_t rawtime;
	struct tm *timeinfo;
	char *ptimestring;

	/*get the local time string and print it*/
	time(&rawtime);
	timeinfo= localtime(&rawtime);
	
	ptimestring= asctime(timeinfo);
	if(ptimestring[strlen(ptimestring)- 1]== '\n')
		ptimestring[strlen(ptimestring)- 1]= '\0';
	
	fprintf(plglog, "%s: plugin: ", ptimestring);
}

/*function to report error and log*/
int plg_report_error(char *errmsg, ...)
{
	/*create va_list of arguments*/
	va_list list;
	va_start(list, errmsg);
	
	/*output to stderr and logfile*/
	vfprintf(stderr, errmsg, list);
	print_time_string();
	vfprintf(plglog, errmsg, list);
	
	va_end(list);
	fflush(stderr);
	fflush(plglog);
	
	return(MTC_RETURN_TRUE);
}

/*function to allocate memory for a filename*/
void *alloc_mem(size_t size, void *pmem)
{
	/*allocate the memory and return the pointer if allocated successfully*/
	if((pmem= calloc(size, sizeof(char)))== NULL)
	{
		plg_report_error(S_PLG_COMMON_ERR_ALLOC);
		exit(EXIT_FAILURE);
	}
	return(pmem);
}

/*function to re-allocate memory*/
void *realloc_mem(size_t size, void *pmem)
{
	/*allocate the memory and return the pointer if allocated successfully*/
	if((pmem= realloc(pmem, size))== NULL)
	{
		plg_report_error(S_PLG_COMMON_ERR_REALLOC);
		exit(EXIT_FAILURE);
	}
	return(pmem);
}

/*function to append to a string*/
char *str_cat(char *dest, const char *source)
{
	size_t dlen= (dest== NULL)? 0: strlen(dest);
	size_t slen= (source== NULL)? 0: strlen(source);
	
	if(source== NULL)
		return(dest);
	
	dest= realloc_mem(slen+ dlen+ 1, dest);
	
	if(dlen== 0)
		memset(dest, '\0', slen+ dlen+ 1);

	dest= strcat(dest, source);
	dest[dlen+ slen]= '\0';

	return(dest);
}

/*function to copy to a string*/
char *str_cpy(char *dest, const char *source)
{
	size_t slen= (source== NULL)? 0: strlen(source);
	
	if((dest!= NULL)&& (source!= NULL)&& (strcmp(dest, source)== 0))
		return(dest);
	
	dest= realloc_mem(slen+ 1, dest);
	
	memset(dest, '\0', slen+ 1);
	return((slen== 0)? dest: strcpy(dest, source));

}

/*function to return relevant filename in mailtc directory filename*/
char *plg_get_account_file(char *fullpath, const char *cfgdir, char *filename, unsigned int account)
{
	/*clear the buffer and create the full name from base_name and filename*/
	memset(fullpath, '\0', NAME_MAX);
	sprintf(fullpath, "%s/%s%d", cfgdir, filename, account);
	return(fullpath);
}

/*create CRAM-MD5 string to send to server*/
#ifdef SSL_PLUGIN
/*TODO replace this with our SSL stuff*/
char *create_cram_string(mail_details *paccount, char *serverdigest, char *clientdigest)
{
	HMAC_CTX ctx;
	char octet[3];
	unsigned char *tmpbuf= NULL;
	unsigned char keyed[16];
	int slen, keyed_len, i;

	memset(keyed, '\0', sizeof(keyed));
	keyed_len= sizeof(keyed);
	slen= strlen(serverdigest);
	tmpbuf= (unsigned char *)alloc_mem(slen+ 1, tmpbuf);

	/*stage 1, decode the base64 encoded string from the server*/
	if(EVP_DecodeBlock(tmpbuf, (unsigned char *)serverdigest, slen)> slen)
	{
		plg_report_error(S_PLG_COMMON_ERR_BASE64_DECODE);
		free(tmpbuf);
		return(NULL);
	}

	/*stage 2, now do the keyed-md5 (hmac) stuff*/
	HMAC_CTX_init(&ctx);
	HMAC_Init(&ctx, (unsigned char *)paccount->password, strlen(paccount->password), EVP_md5());
	HMAC_Update(&ctx, tmpbuf, strlen(tmpbuf));
	HMAC_Final(&ctx, keyed, &keyed_len);
	HMAC_CTX_cleanup(&ctx);
	free(tmpbuf);

	/*stage 3, we now do "username digest"*/
	tmpbuf= (unsigned char *)alloc_mem((keyed_len* 2)+ strlen(paccount->username)+ 2, tmpbuf);
	strcpy(tmpbuf, paccount->username);
	strcat(tmpbuf, " ");

	/*TODO This works, but is a bit crap*/
	for(i=0; i< keyed_len; i++)
	{	
		sprintf(octet, "%02x", keyed[i]);
		strcat(tmpbuf, octet);
	}
	
	/*stage 4, encode keyed digest back to base64*/
	/*allocate memory for our final digest to send to server*/
	slen= 4 *((strlen(tmpbuf)+ 2)/ 3);
	clientdigest= (char *)calloc(slen+ 1, sizeof(char));
	
	/*now base64 encode the string*/
	if(EVP_EncodeBlock((unsigned char *)clientdigest, tmpbuf, strlen(tmpbuf))!= slen)
	{
		plg_report_error(S_PLG_COMMON_ERR_BASE64_ENCODE);
		free(tmpbuf);
		return(NULL);
	}
	free(tmpbuf);
	clientdigest[slen]= '\0';
	return(clientdigest);
}

/*function to create md5 digest for APOP authentication*/
unsigned int encrypt_apop_string(char *decstring, char *encstring)
{
	unsigned int md_len= 0, i;
	char octet[3];
	
	EVP_MD_CTX ctx;
	const EVP_MD *md;
	unsigned char md_value[EVP_MAX_MD_SIZE]; /*string to hold the digest*/

	/*Enable openssl to use all digests and set digest to MD5*/
	OpenSSL_add_all_digests();
	md= EVP_md5();
	
	/*Initialise message digest cipher content*/
	EVP_MD_CTX_init(&ctx);
	if(!EVP_DigestInit_ex(&ctx, md, NULL))
		plg_report_error(S_PLG_APOP_ERR_DIGEST_INIT);

	/*Encrypt the string and final padding to MD5*/
	if(!EVP_DigestUpdate(&ctx, (unsigned char *)decstring, strlen(decstring)))
		plg_report_error(S_PLG_APOP_ERR_DIGEST_CREATE);
	if(!EVP_DigestFinal_ex(&ctx, md_value, &md_len))
		plg_report_error(S_PLG_APOP_ERR_DIGEST_FINAL);

	/*cleanup*/
	EVP_MD_CTX_cleanup(&ctx);
	
	/*convert digest into hex string for APOP and return length of string*/
	for(i= 0; i< md_len; i++)
	{	
		sprintf(octet, "%02x", md_value[i]);
		strcat(encstring, octet);
	}	
	EVP_cleanup();

	return(md_len);
	
}

/*Initialise SSL connection*/
SSL_CTX *initialise_ssl_ctx(SSL_CTX *ctx)
{
	SSL_METHOD *method= NULL;
		
	/*initialise SSL*/
	SSL_load_error_strings();
	SSL_library_init();

	/*choose TLS method and create CTX*/
	method= SSLv23_client_method();
	if((ctx= SSL_CTX_new(method))== NULL)
		plg_report_error(S_PLG_COMMON_ERR_CTX);

	return(ctx);
}

/*initialise SSL connection*/
SSL *initialise_ssl_connection(SSL *ssl, SSL_CTX *ctx, int *sockfd)
{
	/*create an SSL structure for the TLS connection*/
	if((ssl= SSL_new(ctx))== NULL)
		plg_report_error(S_PLG_COMMON_ERR_CREATE_SSL_STRUCT);

	/*set the SSL file descriptor*/
	if(SSL_set_fd(ssl, *sockfd)== 0)
		plg_report_error(S_PLG_COMMON_ERR_SSL_DESC);

	
	/*connect with SSL*/
	if(SSL_connect(ssl)<= 0)
		plg_report_error(S_PLG_COMMON_ERR_SSL_CONNECT);
	
	return(ssl);
}

/*Uninitialise SSL connection*/
int uninitialise_ssl(SSL *ssl, SSL_CTX *ctx)
{
	/*TODO see SSL_shutdown, this may need to be done a different way*/
	if(SSL_shutdown(ssl)== -1)
		plg_report_error(S_PLG_COMMON_ERR_SSL_SHUTDOWN);
		
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	ERR_free_strings();

	return(MTC_RETURN_TRUE);
}
#endif /*SSL_PLUGIN*/

/*case insensitive search returns position, or -1*/
static int str_case_search(char *haystack, char *needle)
{
	unsigned int i= 0;
	
	/*iterate though each haystack char*/
	for(i= 0; i< strlen(haystack); i++)
	{
		/*if char found to be same as first needle char*/
		if(tolower(haystack[i])== tolower(needle[0]))
		{
			unsigned int j= 0;
			/*compare needle chars with subsequent haystack chars*/
			for(j= 0; j< strlen(needle); j++)
			{
				if(tolower(haystack[i+ j])!= tolower(needle[j]))
					break;
			}	
			/*all chars found so success*/
			if(j== strlen(needle))
				return(i);
		}
	}
	return -1;
}

/*default function to search header for filter match
  this can be easily dropped into a plugin, or it is possible to write your own*/
int search_for_filter_match(mail_details **paccount, char *header)
{
	char *spos= NULL;
	int initpos= 0;
	int found= 0;
	unsigned int i= 0;
	filter_details *pfilter= (*paccount)->pfilters;

	/*for each filter value*/
	for(i= 0; i< MAX_FILTER_EXP; ++i)
	{
		char field[15];
		
		/*set the field to search for*/
		sprintf(field, "\r\n%s", (pfilter->subject[i])? "Subject": "From");
		
		/*if the search string is empty no more filters*/
		if(strcmp(pfilter->search_string[i], "")== 0)
			break;
		
		/*perform a case insensitive search on either subject or sender*/
		if((initpos= str_case_search(header, field))== -1)
			return(MTC_RETURN_FALSE);

		/*move to the found position*/
		spos= header+ initpos;
			
		/*find where we can get our data*/
		if((spos= strchr(spos, ':'))!= NULL)
		{
			/*set endpos to startpos to begin our search for end of data*/
			char *epos= spos;
			char *tmpbuf= NULL;
					
			/*while '\r\n' is found...*/
			while((epos= strstr(epos+ 1, "\r\n"))!= NULL)
			{
				/*if a ' ' or '\t' is found carry on, otherwise break*/
				if((*(epos+ 2)!= ' ')&& (*(epos+ 2)!= '\t'))
					break;
			}
				
			/*check they were valid*/
			if(epos > spos)
			{
				/*Allocate a temporary string to hold the data to search*/
				tmpbuf= alloc_mem((epos- spos)+ 2, tmpbuf);
				strncpy(tmpbuf, spos+ 1, (epos- spos));
				tmpbuf[epos- spos -1]= '\0';
				
				/*if the string is found, return 1 to say so, test if -1 for match all*/
				if((strstr(tmpbuf, pfilter->search_string[i])!= NULL)&& (found!= -1))
				{
					if(!pfilter->contains[i])
					{
						if(pfilter->matchall)
							found= -1;
					}
					else
						found= 1;
				}
				/*set to -1 if not found as all must match to return 1*/
				else
				{
					if(pfilter->contains[i])
					{	
						if(pfilter->matchall)
							found= -1;
					}
					else
						found= 1;
				}

				free(tmpbuf);
			}
			
		}
	}
	/*if found is -1 return 0 as was not matched*/
	return((found== -1)? MTC_RETURN_FALSE: found);
}


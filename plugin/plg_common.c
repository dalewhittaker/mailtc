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
	
	fprintf(plglog, "%s: ", ptimestring);
}

/*function to report error and log*/
int error_and_log_no_exit(char *errmsg, ...)
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
		error_and_log_no_exit(S_PLG_COMMON_ERR_ALLOC);
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
		error_and_log_no_exit(S_PLG_COMMON_ERR_REALLOC);
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
char *get_account_file(char *fullpath, const char *cfgdir, char *filename, int account)
{
	/*clear the buffer and create the full name from base_name and filename*/
	memset(fullpath, '\0', NAME_MAX);
	sprintf(fullpath, "%s%s%d", cfgdir, filename, account);
	return(fullpath);
}

/*create CRAM-MD5 string to send to server*/
#ifdef MTC_USE_SASL
char *create_cram_string(Gsasl *ctx, char *username, char *password, char *serverdigest, char *clientdigest)
{
	Gsasl_session *session;
	const char *mech= "CRAM-MD5";
	int rc;

	/*Create new authentication session*/
	if((rc= gsasl_client_start(ctx, mech, &session)) != GSASL_OK)
	{
		error_and_log_no_exit(S_PLG_COMMON_ERR_SASL_INIT, rc, gsasl_strerror(rc));
		return(NULL);
	}

	/*set username and password in session handle*/
	gsasl_property_set(session, GSASL_AUTHID, username);
	gsasl_property_set(session, GSASL_PASSWORD, password);

	/*create the base64 string to send back to the server
	 *from the username, password and server base64 string*/
	if((rc= gsasl_step64(session, serverdigest, &clientdigest))!= GSASL_OK)
	{
		error_and_log_no_exit(S_PLG_COMMON_ERR_CRAM_MD5_AUTH, rc, gsasl_strerror(rc));
		free(clientdigest);
		return(NULL);
	}
	
	/*cleanup*/
	gsasl_finish(session);

	return(clientdigest);
}
#endif /*MTC_USE_SASL*/

#ifdef MTC_USE_SSL
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
		error_and_log_no_exit(S_PLG_COMMON_ERR_CTX);

	return(ctx);
}

/*initialise SSL connection*/
SSL *initialise_ssl_connection(SSL *ssl, SSL_CTX *ctx, int *sockfd)
{
	/*create an SSL structure for the TLS connection*/
	if((ssl= SSL_new(ctx))== NULL)
		error_and_log_no_exit(S_PLG_COMMON_ERR_CREATE_SSL_STRUCT);

	/*set the SSL file descriptor*/
	if(SSL_set_fd(ssl, *sockfd)== 0)
		error_and_log_no_exit(S_PLG_COMMON_ERR_SSL_DESC);

	
	/*connect with SSL*/
	if(SSL_connect(ssl)<= 0)
		error_and_log_no_exit(S_PLG_COMMON_ERR_SSL_CONNECT);
	
	return(ssl);
}

/*Uninitialise SSL connection*/
int uninitialise_ssl(SSL *ssl, SSL_CTX *ctx)
{
	/*TODO see SSL_shutdown, this may need to be done a different way*/
	if(SSL_shutdown(ssl)== -1)
		error_and_log_no_exit(S_PLG_COMMON_ERR_SSL_SHUTDOWN);
		
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	ERR_free_strings();

	return(MTC_RETURN_TRUE);
}
#endif /*MTC_USE_SSL*/

/*case insensitive search returns position, or -1*/
int str_case_search(char *haystack, char *needle)
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


/* popfunc.c
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

#include "plg_common.h"

/*this is a function to send a QUIT command to the pop server so that the program can retry to connect later*/
#ifdef SSL_PLUGIN
static int close_pop_connection(int sockfd, SSL *ssl, SSL_CTX *ctx)
#else
static int close_pop_connection(int sockfd, char *ssl, char *ctx)
#endif /*SSL_PLUGIN*/
{
	/*send the QUIT command to try and exit nicely then close the socket and report error*/
	SEND_NET_STRING(sockfd, "QUIT\r\n", ssl);
	
	plg_report_error(S_POPFUNC_ERR_CONNECT, PACKAGE);

#ifdef SSL_PLUGIN
	/*close the SSL connection*/
	if(ssl)
		uninitialise_ssl(ssl, ctx);
#endif /*SSL_PLUGIN*/
	
	close(sockfd);
	
	return(MTC_RETURN_TRUE);
	
}

/* function to receive a message from the pop3 server */
#ifdef SSL_PLUGIN
static char *receive_pop_string(int sockfd, SSL *ssl, char *buf)
#else
static char *receive_pop_string(int sockfd, char *ssl, char *buf)
#endif /*SSL_PLUGIN*/
{
	int numbytes= 1;
	char tmpbuf[MAXDATASIZE];

	buf= NULL;

	/*while there is data available receive data*/
	while(NET_DATA_AVAILABLE(sockfd, ssl))
	{

		if((numbytes= RECEIVE_NET_STRING(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return(NULL);
		}
		if(!numbytes)
			break;

		/*add the received string to the buffer*/
		buf= str_cat(buf, tmpbuf);

		/*added so we dont have to wait for select() timeout every time*/
		if((buf[strlen(buf)-2]== '\r')&& (buf[strlen(buf)- 1]== '\n'))
			break;
	}
	
	/*check if there was an error with the command*/
	if((buf!= NULL)&& (strncmp(buf, "-ERR", 4))== 0)
	{	
		free(buf);
		return(NULL);
	}
	
	/*check that the received string was receive fully (i.e ends with \r\n*/
	if((buf!= NULL)&& ((buf[strlen(buf)- 2]!= '\r')|| (buf[strlen(buf)- 1]!= '\n')))
	{
		free(buf);
		return(NULL);
	}
	return(buf);
}

/* function to receive a message from the pop3 server */
#ifdef SSL_PLUGIN
static char *receive_header_string(int sockfd, SSL *ssl, char *buf)
#else
static char *receive_header_string(int sockfd, char *ssl, char *buf)
#endif /*SSL_PLUGIN*/
{
	int numbytes= 1;
	char tmpbuf[MAXDATASIZE];

	buf= NULL;

	/*while there is data available receive data*/
	while(NET_DATA_AVAILABLE(sockfd, ssl))
	{

		if((numbytes= RECEIVE_NET_STRING(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return(NULL);
		}
		if(!numbytes)
			break;

		/*add the received string to the buffer*/
		buf= str_cat(buf, tmpbuf);

		/*added so we dont have to wait for select() timeout every time*/
		if(strstr(buf, "\r\n\r\n.\r\n")!= NULL)
			break;

	}
	
	/*check if there was an error with the command*/
	if((buf!= NULL)&& (strncmp(buf, "-ERR", 4))== 0)
	{	
		free(buf);
		return(NULL);
	}
	
	/*check that the received string was receive fully (i.e ends with \r\n*/
	if((buf!= NULL)&& (strstr(buf, "\r\n\r\n.\r\n")== NULL))
	{
		/*It was found that some servers return headers from TOP that do not conform to spec
		 *such emails are handled here (although will be delayed as punishment ;)*/
		if(strstr(buf, "\r\n.\r\n")!= NULL)
		{
			plg_report_error(S_POPFUNC_ERR_BAD_MAIL_HEADER);
			return(buf);
		}
		free(buf);
		return(NULL);
	}
	return(buf);
}

#ifdef SSL_PLUGIN
/*function to login to APOP server*/
static int login_to_apop_server(int sockfd, mail_details *paccount, char *buf)
{
	char apopstring[LOGIN_NAME_MAX+ HOST_NAME_MAX+ PASSWORD_LEN+ 3];
	char digest[DIGEST_LEN+ 1];
	char *startpos= NULL, *endpos= NULL, *rstring= NULL;
	
	/*search for the timestamp character start and end*/
	if((buf)&& ((startpos= strchr(buf, '<'))!= NULL))
		endpos= strchr(startpos, '>');

	/*if timestamp character and end arent found report that APOP is not supported*/
	if(((startpos== NULL)|| (endpos== NULL))&& (close_pop_connection(sockfd, NULL, NULL)))
	{
		plg_report_error(S_POPFUNC_ERR_APOP_NOT_SUPPORTED);
		return(MTC_RETURN_FALSE);
	}
	
	/*copy the timestamp part of message to apopstring and append the password*/
	memset(apopstring, '\0', LOGIN_NAME_MAX+ HOST_NAME_MAX+ PASSWORD_LEN+ 3);
	strncpy(apopstring, startpos, (endpos- startpos)+ 1);
	strcat(apopstring, paccount->password);

	/*encrypt the apop string and copy it into the digest buffer*/
	memset(digest, '\0', DIGEST_LEN);
	if((encrypt_apop_string(apopstring, digest)!= ((unsigned int)DIGEST_LEN/ 2)))
	{
		plg_report_error(S_POPFUNC_ERR_APOP_ENCRYPT_TIMESTAMP);
		return(MTC_ERR_EXIT);
	}
	/*create the full APOP string to send from the digest and username*/
	memset(apopstring, '\0', LOGIN_NAME_MAX+ HOST_NAME_MAX+ PASSWORD_LEN+ 3);
	sprintf(apopstring, "APOP %s %s\r\n", paccount->username, digest);
	
	/*send the APOP auth string and check it was successfull*/
	SEND_NET_STRING(sockfd, apopstring, NULL);
	
	if((!(rstring= receive_pop_string(sockfd, NULL, rstring)))&& (close_pop_connection(sockfd, NULL, NULL)))
	{
		plg_report_error(S_POPFUNC_ERR_APOP_SEND_DETAILS, paccount->hostname);
		return(MTC_RETURN_FALSE);
	}
	free(rstring);
		
	return(MTC_RETURN_TRUE);

}
#endif /*SSL_PLUGIN*/

/*function to login to POP server*/
#ifdef SSL_PLUGIN
static int login_to_pop_server(int sockfd, mail_details *paccount, SSL *ssl, SSL_CTX *ctx)
#else
static int login_to_pop_server(int sockfd, mail_details *paccount, char *ssl, char *ctx)
#endif /*SSL_PLUGIN*/
{
	/*create the string for the username and send it*/
	char *buf= NULL;
	char *pop_message= NULL;
	
	pop_message= (char*)alloc_mem(strlen(paccount->username)+ strlen("USER \r\n")+ 1, pop_message);
	sprintf(pop_message,"USER %s\r\n", paccount->username);
	SEND_NET_STRING(sockfd, pop_message, ssl);
	free(pop_message);
	
	/*receive back from server to check username was sent ok*/
	if((!(buf= receive_pop_string(sockfd, ssl, buf)))&& (close_pop_connection(sockfd, ssl, ctx)))
	{
		plg_report_error(S_POPFUNC_ERR_SEND_USERNAME, paccount->hostname);
		return(MTC_RETURN_FALSE);
	}
	free(buf);

	/*create password string and send it*/
	pop_message = (char*)alloc_mem(strlen(paccount->password)+ strlen("PASS \r\n")+ 1, pop_message);
	sprintf(pop_message,"PASS %s\r\n", paccount->password); /*send the password*/
	SEND_NET_PW_STRING(sockfd, pop_message, ssl);
	free(pop_message);
	
	/*receive message back from server to check that login was successful*/
	if((!(buf= receive_pop_string(sockfd, ssl, buf)))&& (close_pop_connection(sockfd, ssl, ctx)))
	{
		plg_report_error(S_POPFUNC_ERR_SEND_PASSWORD, paccount->hostname);
		return(MTC_RETURN_FALSE);
	}
	free(buf);
	return(MTC_RETURN_TRUE);
}

#ifdef SSL_PLUGIN	
/*function to login to POP server with CRAM-MD5 auth*/
static int login_to_crammd5_server(int sockfd, mail_details *paccount)
{
	char *buf= NULL;
	char *digest= NULL;

	/*send auth command*/
	SEND_NET_STRING(sockfd, "AUTH CRAM-MD5\r\n", NULL);

	/*receive back from server to check username was sent ok*/
	if((!(buf= receive_pop_string(sockfd, NULL, buf)))&& (close_pop_connection(sockfd, NULL, NULL)))
	{
		plg_report_error(S_POPFUNC_ERR_SEND_CRAM_MD5_AUTH, paccount->hostname);
		return(MTC_RETURN_FALSE);
	}

	/*remove '\r\n' etc*/
	if(buf[strlen(buf)- 2]== '\r')
		buf[strlen(buf)- 2]= '\0';
	if(buf[strlen(buf)- 1]== '\n')
		buf[strlen(buf)- 1]= '\0';
	
	/*create CRAM-MD5 string to send to server*/
	digest= create_cram_string(paccount, buf+ 2, digest);
	free(buf);
	
	/*check the digest value that is returned*/
	if(digest== NULL)
		return(MTC_RETURN_FALSE);

	/*send the digest to log in*/
	digest= realloc_mem(strlen(digest)+ 3, digest);
	strcat(digest, "\r\n");
	SEND_NET_STRING(sockfd, digest, NULL);
	free(digest);
	
	/*receive back from server to check username was sent ok*/
	if((!(buf= receive_pop_string(sockfd, NULL, buf)))&& (close_pop_connection(sockfd, NULL, NULL)))
	{
		plg_report_error(S_POPFUNC_ERR_SEND_USERNAME, paccount->hostname);
		return(MTC_RETURN_FALSE);
	}
	free(buf);

	return(MTC_RETURN_TRUE);
}
#endif

/*function to get the number of messages from POP server*/
#ifdef SSL_PLUGIN
static int get_number_of_messages(int sockfd, mail_details *paccount, SSL *ssl, SSL_CTX *ctx)
#else
static int get_number_of_messages(int sockfd, mail_details *paccount, char *ssl, char *ctx)
#endif /*SSL_PLUGIN*/
{
	int num_messages= 0;
	char *buf= NULL;

	/*get the total number of messages from server*/
	SEND_NET_STRING(sockfd, "STAT\r\n", ssl);
	
	if((!(buf= receive_pop_string(sockfd, ssl, buf)))&& (close_pop_connection(sockfd, ssl, ctx)))
	{
		plg_report_error(S_POPFUNC_ERR_RECEIVE_NUM_MESSAGES, paccount->hostname);
		return(MTC_ERR_CONNECT);
	}
	
	/*if received ok, create the number of messages from string and return it*/
	if(sscanf(buf, "%*s%d%*d", &num_messages)!= 1)
	{
		plg_report_error(S_POPFUNC_ERR_GET_TOTAL_MESSAGES);
		return(MTC_ERR_CONNECT);
	}
	free(buf);

	return(num_messages);
}

/*function to logout and close the pop connection*/
#ifdef SSL_PLUGIN
static int logout_of_pop_server(int sockfd, mail_details *paccount, SSL *ssl, SSL_CTX *ctx)
#else
static int logout_of_pop_server(int sockfd, mail_details *paccount, char *ssl, char *ctx)
#endif /*SSL_PLUGIN*/
{
	char *buf= NULL;

	/*send the message to logout of the server*/
	SEND_NET_STRING(sockfd,"QUIT\r\n", ssl); /*quit from the POP server*/
	
	if((!(buf= receive_pop_string(sockfd, ssl, buf)))&& (close_pop_connection(sockfd, ssl, ctx)))
	{
		plg_report_error(S_POPFUNC_ERR_SEND_QUIT, paccount->hostname);
		return(MTC_RETURN_FALSE);
	}
	free(buf);

#ifdef SSL_PLUGIN
	/*close the SSL connection*/
	if(ssl)
		uninitialise_ssl(ssl, ctx);
#endif

	/*if logout was successful close the socket*/
	close(sockfd);
	return(MTC_RETURN_TRUE);
}

/*function to test capabilities of POP/IMAP server*/
#ifdef SSL_PLUGIN
static int test_pop_server_capabilities(int sockfd, enum pop_protocol protocol, SSL *ssl, SSL_CTX *ctx)
#else
static int test_pop_server_capabilities(int sockfd, enum pop_protocol protocol, char *ssl, char *ctx)
#endif /*SSL_PLUGIN*/
{
	char *buf= NULL;

	/*send the message to check capabilities of the server*/
	SEND_NET_STRING(sockfd, "CAPA\r\n", ssl); 
	
	/*assumption is made that ERR means server does not have CAPA command*/
	/*also means that CRAM-MD5 is not supported*/
	if((!(buf= receive_pop_string(sockfd, ssl, buf))))
	{
		if(protocol== POPCRAM_PROTOCOL)
		{
			close_pop_connection(sockfd, ssl, ctx);
			plg_report_error(S_POPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED);
			return(MTC_RETURN_FALSE);
		}
		return(MTC_RETURN_TRUE);
	}
	/*First we check for OK, otherwise report that its not a valid POP server and return*/
	if(strncmp(buf, "+OK", 3)!= 0)
	{
		free(buf);
		close_pop_connection(sockfd, ssl, ctx);
		plg_report_error(S_POPFUNC_ERR_TEST_CAPABILITIES);
		return(MTC_RETURN_FALSE);
	}
	
	/*here we test the capabilites*/
	/*no need to test simple POP as it would just return OK*/
	/*APOP is not handled by CAPA command*/
	if(protocol== POPCRAM_PROTOCOL)
	{
		/*If CRAM_MD5 is not found in the string it is not supported*/
		if(strstr(buf, "CRAM-MD5")== NULL)
		{
			free(buf);
			close_pop_connection(sockfd, ssl, ctx);
			plg_report_error(S_POPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED);
			return(MTC_RETURN_FALSE);
		}
	}
	free(buf);

	return(MTC_RETURN_TRUE);
}

/*function to check whether message is filtered out*/
#ifdef SSL_PLUGIN
static int filter_message(int sockfd, mail_details *paccount, int message, SSL *ssl)
#else
static int filter_message(int sockfd, mail_details *paccount, int message, char *ssl)
#endif
{
	unsigned int found= MTC_RETURN_FALSE;

	/*check if filters need to be applied*/
	if(paccount->runfilter)
	{
		char pop_message[MAXDATASIZE];	
		char *buf= NULL;

		/*send the TOP message to get the mail header*/
		memset(pop_message, '\0', MAXDATASIZE);
		sprintf(pop_message, "TOP %d 0\r\n", message);
		SEND_NET_STRING(sockfd, pop_message, ssl);

		if(!(buf= receive_header_string(sockfd, ssl, buf))&& (close_pop_connection(sockfd, NULL, NULL))) 
		{	
			plg_report_error(S_POPFUNC_ERR_RECEIVE_TOP, message);
			if(buf!= NULL) free(buf);
			return(MTC_RETURN_FALSE);
		}
		
		/*search for a filter match now*/
		found= search_for_filter_match(&paccount, buf);
	
		free(buf);
	}
	return(found);
	
}


/*function to get the uidl for a message*/
#ifdef SSL_PLUGIN
static char *get_uidl_of_message(int sockfd, int message, SSL *ssl, char *buf)
#else
static char *get_uidl_of_message(int sockfd, int message, char *ssl, char *buf)
#endif
{
	/*clear the buffer*/
	char pop_message[MAXDATASIZE];
	
	buf= NULL;

	/*send the uidl string to get the uidl for the current message*/
	memset(pop_message, '\0', MAXDATASIZE);
	sprintf(pop_message, "UIDL %d\r\n", message);
	SEND_NET_STRING(sockfd, pop_message, ssl);

	if(!(buf= receive_pop_string(sockfd, ssl, buf))&& (close_pop_connection(sockfd, NULL, NULL))) 
	{	
		plg_report_error(S_POPFUNC_ERR_RECEIVE_UIDL, message);
		if(buf!= NULL) free(buf);
		return(NULL);
	}
	/*return ok if uidl was returned*/
	return(buf);
}

/* function to get uidl values of messages on server, 
 * and compare them with values in stored uidl file
 * to check if there are any new messages */
#ifdef SSL_PLUGIN
int output_uidls_to_file(int sockfd, mail_details *paccount, const char *cfgdir, int num_messages, SSL *ssl)
#else
int output_uidls_to_file(int sockfd, mail_details *paccount, const char *cfgdir, int num_messages, char *ssl)
#endif
{
	int new_messages= 0, i;
	FILE *outfile;
	char uidl_string[UIDL_LEN];
	char uidlfile[NAME_MAX]; 
	char tmpuidlfile[NAME_MAX];
	
	/*get the full path for the uidl file and temp uidl file*/
	plg_get_account_file(uidlfile, cfgdir, UIDL_FILE, paccount->id);
	plg_get_account_file(tmpuidlfile, cfgdir, TMP_UIDL_FILE, paccount->id);

	/*open temp file for writing*/
	if((outfile= fopen(tmpuidlfile, "wt"))==NULL)
	{
		plg_report_error(S_POPFUNC_ERR_OPEN_FILE, tmpuidlfile);
		return(MTC_ERR_EXIT);
	}
	/*for each message on server*/
	for(i= 1; i<= num_messages; ++i)
	{
		char *buf= NULL;

		/*get the uidl of the message*/
		if((buf= get_uidl_of_message(sockfd, i, ssl, buf)))
		{
			FILE *infile;
			int uidlfound= 0;
			
			/*get the uidl from the received string*/
			char *pos;
			if((pos= strrchr(buf, ' '))== NULL)
			{
				plg_report_error(S_POPFUNC_ERR_GET_UIDL);
				return(MTC_ERR_EXIT);
			}
			
			memset(uidl_string, '\0', UIDL_LEN);
			strcpy(uidl_string, pos+ 1);
			
			free(buf);

			if(isspace(uidl_string[strlen(uidl_string)- 2]))
				uidl_string[strlen(uidl_string)- 2]= '\0';
			else if(isspace(uidl_string[strlen(uidl_string)- 1]))
				uidl_string[strlen(uidl_string)- 1]= '\0';
					
			strcat(uidl_string, "\n");
			
			/*if there is no uidl file already there simply increment the new message count*/
			if((access(uidlfile, F_OK))== -1)
			{
				/*check if message should be filtered out before incrementing*/
				if(!filter_message(sockfd, paccount, i, ssl))
					new_messages++;
			}
			/*if the uidl file does exist*/
			else 
			{
				char line[LINE_LENGTH];
				
				/*open the uidl file for reading*/
				if((infile= fopen(uidlfile, "rt"))== NULL)
				{
					plg_report_error(S_POPFUNC_ERR_OPEN_FILE, uidlfile);
					return(MTC_ERR_EXIT);
				}
			
				/*get each line from uidl file and compare it with the uidl value received*/
				while(fgets(line, LINE_LENGTH, infile)!= NULL)
				{
					if(strcmp(uidl_string, line)== 0)
						uidlfound=1;
						
					memset(line, '\0', LINE_LENGTH);
				}
			
				/*if the uidl was not found in the uidl file increment the new message count*/
				/*also check if filters should be applied*/
				if((!uidlfound)&& (!filter_message(sockfd, paccount, i, ssl)))
					new_messages++;
			
				if(fclose(infile)== EOF)
					plg_report_error(S_POPFUNC_ERR_CLOSE_FILE, uidlfile);
			}
			/*output the uidl string to the temp file*/
			fputs(uidl_string, outfile);
		}
	
	}
	
	if(fclose(outfile)== EOF)
		plg_report_error(S_POPFUNC_ERR_CLOSE_FILE, tmpuidlfile);
	
	return(new_messages);
}


/*function to check mail (in clear text mode)*/
static int check_mail(mail_details *paccount, const char *cfgdir, enum pop_protocol protocol)
{
	int sockfd= 0;
	int num_messages= 0;
	char *buf= NULL;

#ifdef SSL_PLUGIN
	int retval= 0;
	SSL *ssl= NULL;
	SSL_CTX *ctx= NULL;
#else
	/*use char * to save code (values are unused)*/
	char *ssl= NULL;
	char *ctx= NULL;
#endif /*SSL_PLUGIN*/
	
	/*connect to the server and receive the string back*/
	if(!(connect_to_server(&sockfd, paccount)))
		return(MTC_ERR_CONNECT);
	
#ifdef SSL_PLUGIN
	if(protocol== POPSSL_PROTOCOL)
	{
		/*initialise SSL connection*/
		ctx= initialise_ssl_ctx(ctx);
		ssl= initialise_ssl_connection(ssl, ctx, &sockfd);
	}
#endif /*SSL_PLUGIN*/
	
	buf= receive_pop_string(sockfd, ssl, buf);

	/*test server capabilities*/
	if(!(test_pop_server_capabilities(sockfd, protocol, ssl, ctx)))
	{
		if(buf!= NULL) free(buf);
		return(MTC_ERR_CONNECT);
	}
	
	/*test the type of login*/
	switch(protocol)
	{
		/*TLS auth before pop connection and POP*/
		case POP_PROTOCOL:
		case POPSSL_PROTOCOL:
			if(!(login_to_pop_server(sockfd, paccount, ssl, ctx)))
			{
				if(buf!= NULL) free(buf);
				return(MTC_ERR_CONNECT);
			}
		break;

#ifdef SSL_PLUGIN
		/*APOP*/
		case APOP_PROTOCOL:
			if(((retval= login_to_apop_server(sockfd, paccount, buf))== 0)||
				(retval== MTC_ERR_EXIT))
			{
				if(buf!= NULL) free(buf);
				return((retval== 0)? MTC_ERR_CONNECT: MTC_ERR_EXIT);
			}
		break;
#endif /*SSL_PLUGIN*/

#ifdef SSL_PLUGIN
		/*CRAM-MD5*/
		case POPCRAM_PROTOCOL:
			if(!(login_to_crammd5_server(sockfd, paccount)))
			{
				if(buf!= NULL) free(buf);
				return(MTC_ERR_CONNECT);
			}
		break;
#endif /*SSL_PLUGIN*/
		
		default: 
			plg_report_error(S_POPFUNC_ERR_INVALID_AUTH);
			if(buf!= NULL) free(buf);
			return(MTC_ERR_CONNECT);
	}

	if(buf!= NULL) free(buf);

	/*get the total number of messages from the server*/
	if((num_messages= get_number_of_messages(sockfd, paccount, ssl, ctx))== MTC_ERR_CONNECT)
		return(MTC_ERR_CONNECT);
	
	/*output the uidls to the temp file and get the number of new messages*/
	paccount->num_messages= output_uidls_to_file(sockfd, paccount, cfgdir, num_messages, ssl);

	/*logout of the server and return the number of new messages*/
	if(!(logout_of_pop_server(sockfd, paccount, ssl, ctx)))
		return(MTC_ERR_CONNECT);

	return(MTC_RETURN_TRUE);
}

/*function to read the POP mail*/
int pop_read_mail(mail_details *paccount, const char *cfgdir)
{
	char uidlfile[NAME_MAX], tmpuidlfile[NAME_MAX];

	/*get full paths of files*/
	plg_get_account_file(uidlfile, cfgdir, UIDL_FILE, paccount->id);
	plg_get_account_file(tmpuidlfile, cfgdir, TMP_UIDL_FILE, paccount->id);

	/*rename the temp uidl file to uidl file if it exists*/
	if((access(tmpuidlfile, F_OK)!= -1)&& (rename(tmpuidlfile, uidlfile)== -1))
	{
		plg_report_error(S_POPFUNC_ERR_RENAME_FILE, tmpuidlfile, uidlfile);
		return(MTC_RETURN_FALSE);
	}

	/*remove the temp file and cleanup*/
	remove(tmpuidlfile);

	return(MTC_RETURN_TRUE);

}

/*call check_mail for POP*/
int check_pop_mail(mail_details *paccount, const char *cfgdir)
{
	enum pop_protocol protocol= POP_PROTOCOL;
	return(check_mail(paccount, cfgdir, protocol));
}

/*call check_mail for APOP*/
int check_apop_mail(mail_details *paccount, const char *cfgdir)
{
	enum pop_protocol protocol= APOP_PROTOCOL;
	return(check_mail(paccount, cfgdir, protocol));
}

/*call check_mail for POP (CRAM-MD5)*/
int check_crampop_mail(mail_details *paccount, const char *cfgdir)
{
	enum pop_protocol protocol= POPCRAM_PROTOCOL;
	return(check_mail(paccount, cfgdir, protocol));
}

/*call check_mail for POP (SSL/TLS)*/
int check_popssl_mail(mail_details *paccount, const char *cfgdir)
{
	enum pop_protocol protocol= POPSSL_PROTOCOL;
	return(check_mail(paccount, cfgdir, protocol));
}



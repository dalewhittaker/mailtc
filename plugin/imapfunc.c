/* imapfunc.c
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

/*function to close the connection if an error has occured*/
#ifdef SSL_PLUGIN
static int close_imap_connection(int sockfd, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int close_imap_connection(int sockfd, unsigned int *msgid, char *ssl, char *ctx)
#endif /*SSL_PLUGIN*/
{
	/*basically try to logout then terminate the connection*/
	char imap_message[IMAP_ID_LEN+ strlen("a LOGOUT\r\n")];
	
	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a LOGOUT\r\n"));
	sprintf(imap_message, "a%.4d LOGOUT\r\n", (*msgid)++);
	
	SEND_NET_STRING(sockfd, imap_message, ssl); 
	
	plg_report_error(S_IMAPFUNC_ERR_CONNECT, PACKAGE);

#ifdef SSL_PLUGIN
	/*close the connection*/
	if(ssl)
		uninitialise_ssl(ssl, ctx);
#endif /*SSL_PLUGIN*/
	
	close(sockfd);

	return(MTC_RETURN_TRUE);

}

/*function to receive the initial string from the server*/
#ifdef SSL_PLUGIN
static int receive_imap_initial_string(int sockfd, SSL *ssl)
#else
static int receive_imap_initial_string(int sockfd, char *ssl)
#endif /*SSL_PLUGIN*/
{
	/*clear the buffer*/
	int numbytes= 1;
	char tmpbuf[MAXDATASIZE];
	char *buf= NULL;
	
	/*while there is data available read from server*/
	while(NET_DATA_AVAILABLE(sockfd, ssl))
	{
		if((numbytes= RECEIVE_NET_STRING(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return(MTC_RETURN_FALSE);
		}
		
		if(!numbytes)
			break;

		/*add the read data to the buffer*/
		buf= str_cat(buf, tmpbuf);
		
		/*added for massive speed improvements (doesnt check imap server again)*/
		if(((strncmp(buf, "* OK", 4)== 0)|| (strncmp(buf, "* PREAUTH", 9)== 0)|| (strncmp(buf, "* BYE", 5)== 0))
			&& (strncmp(buf+ (strlen(buf)- 2), "\r\n", 2)== 0))
			break;
	}
	
	if(buf== NULL)
		return(MTC_RETURN_FALSE);
	
	/*test that the data was fully received and successful*/
	if((strncmp(buf, "* OK", 4)!= 0)|| (buf[strlen(buf)- 1]!= '\n')) 
	{	
		free(buf);
		return(MTC_RETURN_FALSE);
	}
	
	free(buf);
	return(MTC_RETURN_TRUE);
}

/*function to receive the string for login/logout commands*/
#ifdef SSL_PLUGIN
static int receive_imap_login_string(int sockfd, unsigned int msgid, SSL *ssl)
#else
static int receive_imap_login_string(int sockfd, unsigned int msgid, char *ssl)
#endif /*SSL_PLUGIN*/
{
	char okstring[12], badstring[12], nostring[12];
	int numbytes= 1;
	char tmpbuf[MAXDATASIZE];
	char *buf= NULL;

	/*create the message with the id and clear the buffer*/
	sprintf(okstring, "a%.4d OK", msgid);
	sprintf(badstring, "a%.4d BAD", msgid);
	sprintf(nostring, "a%.4d NO", msgid);

	/*while there is data available read from server*/
	while(NET_DATA_AVAILABLE(sockfd, ssl))
	{
		if((numbytes= RECEIVE_NET_STRING(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return(MTC_RETURN_FALSE);
		}
		
		/*added as sometimes select reports data when there is none*/
		if(!numbytes)
			break;

		/*add the received data to the buffer*/
		buf= str_cat(buf, tmpbuf);

		if(((strstr(buf, okstring)!= NULL)|| (strstr(buf, badstring)!= NULL)||
			(strstr(buf, nostring)!= NULL)|| (strncmp(buf, "* BYE", 5)== 0))&&
		  	(strncmp(buf+ (strlen(buf)- 2), "\r\n", 2)== 0))
			break;
	}
	
	if(buf== NULL)
		return(MTC_RETURN_FALSE);
	
	/*test that data was fully received and command was successful*/
	if((strstr(buf, okstring)== NULL)|| (buf[strlen(buf)- 1]!= '\n')) 
	{
		free(buf);
		return(MTC_RETURN_FALSE);
	}
	free(buf);
	return(MTC_RETURN_TRUE);
}

/*function to receive the string for login/logout commands*/
#ifdef SSL_PLUGIN
static char *receive_imap_select_string(int sockfd, unsigned int msgid, SSL *ssl, char *buf)
#else
static char *receive_imap_select_string(int sockfd, unsigned int msgid, char *ssl, char *buf)
#endif
{
	char okstring[12], badstring[12], nostring[12];
	int numbytes= 1;
	char *spos= NULL, *epos= NULL;
	char tmpbuf[MAXDATASIZE];
	buf= NULL;

	/*create the message with the id and clear the buffer*/
	sprintf(okstring, "a%.4d OK", msgid);
	sprintf(badstring, "a%.4d BAD", msgid);
	sprintf(nostring, "a%.4d NO", msgid);


	/*while there is data available read from server*/
	while(NET_DATA_AVAILABLE(sockfd, ssl))
	{
		if((numbytes= RECEIVE_NET_STRING(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return(NULL);
		}
		
		/*added as sometimes select reports data when there is none*/
		if(!numbytes)
			break;

		/*add the received data to the buffer*/
		/*strcat(buf, tmpbuf);*/
		buf= str_cat(buf, tmpbuf);

		if(((strstr(buf, okstring)!= NULL)|| (strstr(buf, badstring)!= NULL)||
			(strstr(buf, nostring)!= NULL)|| (strncmp(buf, "* BYE", 5)== 0))&&
		  	(strncmp(buf+ (strlen(buf)- 2), "\r\n", 2)== 0))
			break;
	}
	
	if(buf== NULL)
		return(NULL);
	
	/*test that data was fully received and command was successful*/
	if((strstr(buf, okstring)== NULL)|| (buf[strlen(buf)- 1]!= '\n'))
	{
		free(buf);
		return(NULL);
	}

	/*get the UIDVALIDITY value, copy 0 if not found*/
	if((spos= strstr(buf, "UIDVALIDITY "))== NULL)
		strcpy(buf, "0");
	
	if((epos= strchr(spos+ strlen("UIDVALIDITY "), ']'))== NULL)
	{
		free(buf);
		return(NULL);
	}
	else
	{
		char *tempbuf= NULL;
		
		tempbuf= alloc_mem(epos- spos, tempbuf);
		strncpy(tempbuf, spos+ strlen("UIDVALIDITY "), epos- (spos+ strlen("UIDVALIDITY ")));
		str_cpy(buf, tempbuf);
		free(tempbuf);
	}
	
	return(buf);
}

/*function to receive the imap LOGOUT command result*/
#ifdef SSL_PLUGIN
static unsigned int receive_imap_logout_string(int sockfd, unsigned int msgid, SSL *ssl)
#else
static unsigned int receive_imap_logout_string(int sockfd, unsigned int msgid, char *ssl)
#endif /*SSL_PLUGIN*/
{
	/*function is identical to login so just use that*/
	return(receive_imap_login_string(sockfd, msgid, ssl));	
}

/*function to login to IMAP server*/
#ifdef SSL_PLUGIN
static int login_to_imap_server(int sockfd, mail_details *paccount, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int login_to_imap_server(int sockfd, mail_details *paccount, unsigned int *msgid, char *ssl, char *ctx)
#endif /*SSL_PLUGIN*/
{
	/*send command to login with username and password and check login was successful*/
	char *imap_message= NULL;
	
	imap_message= (char*)alloc_mem(IMAP_ID_LEN+ strlen(paccount->username)+ strlen("a LOGIN  \r\n")+ strlen(paccount->password)+ 1, imap_message);
	sprintf(imap_message,"a%.4d LOGIN %s %s\r\n", *msgid, paccount->username, paccount->password);
	SEND_NET_PW_STRING(sockfd, imap_message, ssl);
	free(imap_message);

	if((!(receive_imap_login_string(sockfd, (*msgid)++, ssl)))&& (close_imap_connection(sockfd, msgid, ssl, ctx))) 
	{	
		plg_report_error(S_IMAPFUNC_ERR_SEND_LOGIN, paccount->hostname);
		return(MTC_RETURN_FALSE);
	}
	return(MTC_RETURN_TRUE);
}

/*function to logout of IMAP server*/
#ifdef SSL_PLUGIN
static int logout_from_imap_server(int sockfd, mail_details *paccount, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int logout_from_imap_server(int sockfd, mail_details *paccount, unsigned int *msgid, char *ssl, char *ctx)
#endif
{
	/*send the message to logout from imap server and check logout was successful*/
	char imap_message[IMAP_ID_LEN+ strlen("a LOGOUT\r\n")];

	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a LOGOUT\r\n"));
	sprintf(imap_message, "a%.4d LOGOUT\r\n", *msgid);
	SEND_NET_STRING(sockfd, imap_message, ssl); 
	
	if((!(receive_imap_logout_string(sockfd, (*msgid)++, ssl)))&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
	{
		plg_report_error(S_IMAPFUNC_ERR_SEND_LOGOUT, paccount->hostname);
		return(MTC_RETURN_FALSE);
	}

#ifdef SSL_PLUGIN
	/*close the SSL connection*/
	if(ssl)
		uninitialise_ssl(ssl, ctx);
#endif
	
	close(sockfd);
	return(MTC_RETURN_TRUE);
}

#ifdef SSL_PLUGIN
/*function to receive the CRAM-MD5 string from the server*/
static char *receive_crammd5_string(int sockfd, char *buf)
{
	/*clear the buffer*/
	int numbytes= 1;
	char tmpbuf[MAXDATASIZE];
	buf= NULL;
	
	/*while there is data available read from server*/
	while(NET_DATA_AVAILABLE(sockfd, NULL))
	{
		if((numbytes= RECEIVE_NET_STRING(sockfd, tmpbuf, NULL))== -1)
		{
			if(buf!= NULL) free(buf);
			return(NULL);
		}
		
		if(!numbytes)
			break;

		/*add the read data to the buffer*/
		buf= str_cat(buf, tmpbuf);
		
		/*added for massive speed improvements (doesnt check imap server again)*/
		if(strncmp(buf+ (strlen(buf)- 2), "\r\n", 2)== 0)
			break;
	}
	
	/*test that the data was fully received and successful*/
	if((buf!= NULL)&& ((strncmp(buf, "+ ", 2)!= 0)|| (buf[strlen(buf)- 1]!= '\n'))) 
	{	
		free(buf);
		return(NULL);
	}
	
	return(buf);
}

/*function to login to IMAP server with CRAM-MD5 auth*/
static int login_to_cramimap_server(int sockfd, mail_details *paccount, unsigned int *msgid)
{
	char imap_message[IMAP_ID_LEN+ strlen("a AUTHENTICATE CRAM-MD5\r\n")];
	char *buf= NULL;
	char *digest= NULL;
	
	/*send auth command*/
	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a AUTHENTICATE CRAM-MD5\r\n"));
	sprintf(imap_message, "a%.4d AUTHENTICATE CRAM-MD5\r\n", *msgid);
	SEND_NET_STRING(sockfd, imap_message, NULL);

	/*receive back from server to check username was sent ok*/
	if((!(buf= receive_crammd5_string(sockfd, buf)))&& (close_imap_connection(sockfd, msgid, NULL, NULL)))
	{
		plg_report_error(S_IMAPFUNC_ERR_SEND_AUTHENTICATE, paccount->hostname);
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

	/*check the digest value before continuing*/
	if(digest== NULL)
		return(MTC_RETURN_FALSE);

	/*send the digest to log in*/
	digest= realloc_mem(strlen(digest)+ 3, digest);
	strcat(digest, "\r\n");
	SEND_NET_STRING(sockfd, digest, NULL);
	free(digest);

	/*receive back from server to check username was sent ok*/
	if((!(receive_imap_login_string(sockfd, (*msgid)++, NULL)))&& (close_imap_connection(sockfd, msgid, NULL, NULL))) 
	{	
		plg_report_error(S_IMAPFUNC_ERR_SEND_CRAM_MD5, paccount->hostname);
		return(MTC_RETURN_FALSE);
	}

	return(MTC_RETURN_TRUE);
}
#endif /*SSL_PLUGIN*/

/*function to receive the string for login/logout commands*/
#ifdef SSL_PLUGIN
static char *receive_imap_capability_string(int sockfd, unsigned int msgid, SSL *ssl, char *buf)
#else
static char *receive_imap_capability_string(int sockfd, unsigned int msgid, char *ssl, char *buf)
#endif
{

	char okstring[12], badstring[12];
	int numbytes= 1;
	char tmpbuf[MAXDATASIZE];
	buf= NULL;

	/*create the message with the id and clear the buffer*/
	sprintf(okstring, "a%.4d OK", msgid);
	sprintf(badstring, "a%.4d BAD", msgid);

	/*while there is data available read from server*/
	while(NET_DATA_AVAILABLE(sockfd, ssl))
	{
		if((numbytes= RECEIVE_NET_STRING(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return(NULL);
		}
		
		/*added as sometimes select reports data when there is none*/
		if(!numbytes)
			break;

		/*add the received data to the buffer*/
		buf= str_cat(buf, tmpbuf);

		if(((strstr(buf, okstring)!= NULL)|| (strstr(buf, badstring)!= NULL)||
			(strncmp(buf, "-ERR", 4)== 0)|| (strncmp(buf, "* BYE", 5)== 0))&&
		  	(strncmp(buf+ (strlen(buf)- 2), "\r\n", 2)== 0))
			break;
	}
	
	if(buf== NULL)
		return(buf);
	
	/*test if is valid IMAP server*/
	if((strncmp(buf, "* CAPABILITY", strlen("* CAPABILITY")))!= 0)
	{
		free(buf);
		plg_report_error(S_IMAPFUNC_ERR_NOT_IMAP_SERVER);
		return(NULL);
	}
	
	/*test that data was fully received and command was successful*/
	if((strstr(buf, okstring)== NULL)|| (buf[strlen(buf)- 1]!= '\n')) 
	{
		free(buf);
		plg_report_error(S_IMAPFUNC_ERR_RECEIVE_CAPABILITY);
		return(NULL);
	}
	
	return(buf);
}

/*test the capabilities of the IMAP server*/
#ifdef SSL_PLUGIN
static int test_imap_server_capabilities(int sockfd, enum imap_protocol protocol, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int test_imap_server_capabilities(int sockfd, enum imap_protocol protocol, unsigned int *msgid, char *ssl, char *ctx)
#endif
{
	/*send command to test capabilities and check if successful*/
	char imap_message[IMAP_ID_LEN+ strlen("a CAPABILITY\r\n")];
	char *buf= NULL;

	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a CAPABILITY\r\n"));
	sprintf(imap_message,"a%.4d CAPABILITY\r\n", *msgid);
	SEND_NET_STRING(sockfd, imap_message, ssl);

	/*get the capability string*/
	if(((buf= receive_imap_capability_string(sockfd, (*msgid)++, ssl, buf))== NULL)&& (close_imap_connection(sockfd, msgid, ssl, ctx))) 
	{	
		plg_report_error(S_IMAPFUNC_ERR_GET_IMAP_CAPABILITIES);
		return(MTC_RETURN_FALSE);
	}
	
	/*Test if CRAM-MD5 is supported*/
	if((protocol== IMAPCRAM_PROTOCOL)&& (strstr(buf, "CRAM-MD5")== NULL))
	{
		close_imap_connection(sockfd, msgid, ssl, ctx);
		plg_report_error(S_IMAPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED);
		free(buf);
		return(MTC_RETURN_FALSE);
	}
	free(buf);
	return(MTC_RETURN_TRUE);
}

/*function to output the UID's to file*/
static int output_uids_to_file(FILE *infile, FILE *outfile, FILE *outreadfile, char *uidvalidity, char *uid)
{
	int counter= 0;
	
	/*otherwise we need to compare our string*/
	if(infile!= NULL)
	{
			
		char *uid_string= NULL;
		char line[LINE_LENGTH];
		
		uid_string= (char *)alloc_mem(strlen(uidvalidity)+ strlen(uid)+ 3, uid_string);
		/*position the file pointer back at the beginning*/
		rewind(infile);
		
		sprintf(uid_string, "%s-%s\n", uidvalidity, uid);
		memset(line, '\0', LINE_LENGTH);
		
		while(fgets(line, LINE_LENGTH, infile)!= NULL)
		{
			if(strcmp(uid_string, line)== 0)
				++counter;
		}

		/*if the id is found we output to read file to be marked as read
		 *otherwise output to normal output id file*/
		fputs(uid_string, (counter)? outreadfile: outfile);
		free(uid_string);
		
	}
	/*Do simple output other*/
	else
	{
		fprintf(outfile, "%s-%s\n", uidvalidity, uid);
	}

	return((counter)? MTC_RETURN_FALSE: MTC_RETURN_TRUE) ;
}

/*function to receive the string for login/logout commands*/
#ifdef SSL_PLUGIN
static char *receive_imap_message_header(int sockfd, unsigned int msgid, SSL *ssl, char *buf)
#else
static char *receive_imap_message_header(int sockfd, unsigned int msgid, char *ssl, char *buf)
#endif /*SSL_PLUGIN*/
{
	char okstring[12], badstring[12], nostring[12];
	int numbytes= 1;
	char tmpbuf[MAXDATASIZE];
	buf= NULL;

	/*create the message with the id and clear the buffer*/
	sprintf(okstring, "a%.4d OK", msgid);
	sprintf(badstring, "a%.4d BAD", msgid);
	sprintf(nostring, "a%.4d NO", msgid);

	/*while there is data available read from server*/
	while(NET_DATA_AVAILABLE(sockfd, ssl))
	{
		if((numbytes= RECEIVE_NET_STRING(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return(NULL);
		}
		
		/*added as sometimes select reports data when there is none*/
		if(!numbytes)
			break;

		/*add the received data to the buffer*/
		buf= str_cat(buf, tmpbuf);

		if(((strstr(buf, okstring)!= NULL)|| (strstr(buf, badstring)!= NULL)||
			(strstr(buf, nostring)!= NULL)|| (strncmp(buf, "* BYE", 5)== 0))&&
		  	(strncmp(buf+ (strlen(buf)- 2), "\r\n", 2)== 0))
			break;
	}
	
	if(buf== NULL)
		return(NULL);
	
	/*test that data was fully received and command was successful*/
	if((strstr(buf, okstring)== NULL)|| (buf[strlen(buf)- 1]!= '\n')) 
	{
		free(buf);
		return(NULL);
	}
	return(buf);
}

/*function to check whether message is filtered out*/
#ifdef SSL_PLUGIN
static int filter_messages(int sockfd, mail_details *paccount, const char *cfgdir, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int filter_messages(int sockfd, mail_details *paccount, const char *cfgdir, unsigned int *msgid, char *ssl, char *ctx)
#endif
{
	unsigned int found= MTC_RETURN_FALSE;

	/*check if filters need to be applied*/
	if(paccount->runfilter)
	{
		char imap_message[strlen("a UID FETCH +(FLAGS BODY[HEADER])\r\n")+ 25];	
		char uidfile[NAME_MAX], line[LINE_LENGTH];
		FILE *infile= NULL;
			
		/*get the full path for the uidl file*/
		plg_get_account_file(uidfile, cfgdir, UIDL_FILE, paccount->id);

		/*if it does not exist we do not need to mark as read*/
		if(access(uidfile, F_OK)== -1)
			return(MTC_RETURN_TRUE);
	
		/*open the read file*/
		if((infile= fopen(uidfile, "r"))== NULL)
		{
			plg_report_error(S_IMAPFUNC_ERR_OPEN_READ_FILE, uidfile);
			return(MTC_ERR_EXIT);
		}
		memset(line, '\0', LINE_LENGTH);
	
		/*loop for each uid read from the readfile*/
		while(fgets(line, LINE_LENGTH, infile)!= NULL)
		{
			char *spos= NULL;
			char *buf= NULL;
				
			/*strip the uid from the line*/ 
			if(line[strlen(line)- 2]== '\r') line[strlen(line)- 2]= '\0';
			if(line[strlen(line)- 1]== '\n') line[strlen(line)- 1]= '\0';
			
			if((spos= strchr(line, '-'))== NULL)
			{
				plg_report_error(S_IMAPFUNC_ERR_GET_UID);
				return(MTC_ERR_EXIT);
			}
			/*send the message to get the UID header*/
			memset(imap_message, '\0', strlen("a UID FETCH +(FLAGS BODY[HEADER])\r\n")+ 25);
			sprintf(imap_message, "a%.4d UID FETCH %s (FLAGS BODY[HEADER])\r\n", *msgid, spos+ 1);
			SEND_NET_STRING(sockfd, imap_message, ssl);
				
			/*get the message header*/
			if((!(buf= receive_imap_message_header(sockfd, (*msgid)++, ssl, buf)))&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
			{
				plg_report_error(S_IMAPFUNC_ERR_SEND_UID_FETCH, paccount->hostname);
				return(MTC_RETURN_FALSE);
			}
					
			/*run filters on the message and clear the buffer*/
			found+= search_for_filter_match(&paccount, buf);
		
			free(buf);
			
			/*As header has now been read we need to remark as unseen*/
			memset(imap_message, '\0', strlen("a UID FETCH +(FLAGS BODY[HEADER]\r\n")+ 25);
			sprintf(imap_message, "a%.4d UID STORE %s -FLAGS.SILENT (\\Seen)\r\n", *msgid, spos+ 1);
			SEND_NET_STRING(sockfd, imap_message, ssl);
	
			/*recieve message to tell us it was successful*/
			if((!(receive_imap_login_string(sockfd, (*msgid)++, ssl)))&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
			{
				plg_report_error(S_IMAPFUNC_ERR_SEND_STORE, paccount->hostname);
				return(MTC_RETURN_FALSE);
			}
		
			memset(line, '\0', LINE_LENGTH);
		}
		/*close file and cleanup*/
		if(fclose(infile)== EOF)
		{
			plg_report_error(S_IMAPFUNC_ERR_CLOSE_READ_FILE);
			return(MTC_ERR_EXIT);
		}
	}
	return(found);
		
}

/*function to receive the string for uids*/
#ifdef SSL_PLUGIN
static int receive_imap_messages_string(int sockfd, mail_details *paccount, const char *cfgdir, char *uidvalidity, unsigned int msgid, SSL *ssl)
#else
static int receive_imap_messages_string(int sockfd, mail_details *paccount, const char *cfgdir, char *uidvalidity, unsigned int msgid, char *ssl)
#endif
{

	char okstring[12], nostring[12], badstring[12];
	FILE *outfile= NULL, *outreadfile= NULL, *infile= NULL;
	int numbytes= 1, num_messages= 0;
	char tmpbuf[MAXDATASIZE], readfile[NAME_MAX];
	char uidlfile[NAME_MAX], tmpuidlfile[NAME_MAX];
	char *buf= NULL;
	
	/*create the message with the id and clear the buffer*/
	sprintf(okstring, "a%.4d OK", msgid);
	sprintf(nostring, "a%.4d NO", msgid);
	sprintf(badstring, "a%.4d BAD", msgid);
	
	/*get the full path for the uidl file*/
	plg_get_account_file(uidlfile, cfgdir, UIDL_FILE, paccount->id);
	plg_get_account_file(tmpuidlfile, cfgdir, TMP_UIDL_FILE, paccount->id);
	plg_get_account_file(readfile, cfgdir, ".readfile", paccount->id);

	if((outfile= fopen(uidlfile, "wt"))== NULL)
	{
		plg_report_error(S_IMAPFUNC_ERR_OPEN_FILE, uidlfile);
		return(MTC_ERR_EXIT);
	}
	if((outreadfile= fopen(readfile, "wt"))== NULL)
	{
		plg_report_error(S_IMAPFUNC_ERR_OPEN_FILE, readfile);
		return(MTC_ERR_EXIT);
	}
	/*open temp file for reading*/
	if(access(tmpuidlfile, F_OK)!= -1)
	{
		if((infile= fopen(tmpuidlfile, "r"))== NULL)
		{	
			plg_report_error(S_IMAPFUNC_ERR_OPEN_TEMP_FILE);
			return(MTC_ERR_EXIT);
		}
	}
		
	/*while there is data available read from server*/
	while(NET_DATA_AVAILABLE(sockfd, ssl))
	{
		char *startpos= NULL, *endpos= NULL;
	
		/* OK, if we find no '*' in the string, assuming end of receive and break
		 * otherwise get uid, increment and copy end bit*/
		
		if((numbytes= RECEIVE_NET_STRING(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return(MTC_ERR_CONNECT);
		}
		
		/*added as sometimes select reports data when there is none*/
		if(!numbytes)
			break;
	
		/*add the received data to the buffer*/
		buf= str_cat(buf, tmpbuf);
		
		/*set startpos at beginning of string*/
		startpos= buf- 1;
		
		/*otherwise get our ids*/
		/*here find '\r\n's and *'s*/
			while(((startpos= strstr(startpos+ 1, "*"))!= NULL)&&
				((endpos= strstr(startpos+ strlen("*"), ")\r\n"))!= NULL))
			{
					
				/*ok so they have both been found, now we need to repeatedly copy the string*/
				char *tempbuf= NULL;
				
				tempbuf= (char *)alloc_mem((endpos- startpos)+ 2, tempbuf);
				strncpy(tempbuf, startpos, (endpos- startpos)+ 1);
				tempbuf[(endpos- startpos)+ 1]= '\0';

				/*if we do not find \Seen it is an unread message*/
				if(strstr(tempbuf, "\\Seen")== NULL)
				{	
					char *uidspos= NULL, *uidepos= NULL;
					/*filter out the UID and output it*/
					if((uidspos= strstr(tempbuf, "UID "))!= NULL)
					{
						if(((uidepos= strstr(uidspos+ strlen("UID "), " "))!= NULL)&&
								((uidepos= strstr(uidspos+ strlen("UID "), " "))!= NULL))
						{
							char *uid= NULL;
							uid= (char *)alloc_mem(uidepos- (uidspos+ strlen("UID "))+ 1, uid);
							strncpy(uid, uidspos+ strlen("UID "), uidepos- (uidspos+ strlen("UID ")));
							uid[uidepos- (uidspos+ strlen("UID "))]= '\0';
							
							/*output uid's that are unseen to uid file*/
							num_messages+= output_uids_to_file(infile, outfile, outreadfile, uidvalidity, uid);
							free(uid);
						}
					}
				}
				free(tempbuf);
			}
			
			/*test for end of string*/
			if(numbytes< (MAXDATASIZE- 1))
			{
				if(((strstr(buf, okstring)!= NULL)|| (strstr(buf, badstring)!= NULL)||
					(strstr(buf, nostring)!= NULL)|| (strncmp(buf, "* BYE", 5)== 0))&&
		  			(strncmp(buf+ (strlen(buf)- 2), "\r\n", 2)== 0))
						break;
			}

			/*here we shift the string back*/
			{
				char *tempbuf= NULL;
				tempbuf= str_cpy(tempbuf, endpos? (endpos+ strlen(")\r\n")): startpos);
				memset(buf, '\0', strlen(buf));
				buf= str_cpy(buf, tempbuf);
				free(tempbuf);
			}
	}

	/*close the temp uidlfile*/
	if(infile!= NULL)
	{
		if(fclose(infile)== EOF)
		{
			plg_report_error(S_IMAPFUNC_ERR_CLOSE_TEMP_FILE);
			if(buf!= NULL) free(buf);
			return(MTC_ERR_EXIT);
		}
	
		if(remove(tmpuidlfile)== -1)
		{
			plg_report_error(S_IMAPFUNC_ERR_CLOSE_TEMP_FILE);
			if(buf!= NULL) free(buf);
			return(MTC_ERR_EXIT);
		}
	}
	
	/*close the uidlfile*/
	if(fclose(outreadfile)== EOF)
		plg_report_error(S_IMAPFUNC_ERR_CLOSE_FILE, readfile);
	if(fclose(outfile)== EOF)
		plg_report_error(S_IMAPFUNC_ERR_CLOSE_FILE, uidlfile);
	
	if(buf== NULL)
		return(MTC_ERR_CONNECT);
	
	/*test that full message was received and it was successful*/
	if((strstr(buf, okstring)== NULL)|| (buf[strlen(buf)- 1]!= '\n'))
	{
		free(buf);
		return(MTC_ERR_CONNECT);
	}
	free(buf);

	return(num_messages);
}

/*function to Select the INBOX as the mailbox to use*/
#ifdef SSL_PLUGIN
static char *select_inbox_as_mailbox(int sockfd, mail_details *paccount, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx, char *buf)
#else
static char *select_inbox_as_mailbox(int sockfd, mail_details *paccount, unsigned int *msgid, char *ssl, char *ctx, char *buf)
#endif
{
	/*create the message to select the inbox and return if successful*/
	char imap_message[IMAP_ID_LEN+ strlen("a SELECT INBOX\r\n")];
	buf= NULL;

	/*increment msgid as it is not done in get_message_uids*/
	(*msgid)++;

	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a SELECT INBOX\r\n"));
	sprintf(imap_message, "a%.4d SELECT INBOX\r\n", *msgid);
	SEND_NET_STRING(sockfd, imap_message, ssl);
	
	if(((buf= receive_imap_select_string(sockfd, (*msgid)++, ssl, buf))== NULL)&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
	{
		plg_report_error(S_IMAPFUNC_ERR_SEND_SELECT, paccount->hostname);
		return(NULL);
	}

	return(buf);
}

/*function to output the uids of each message*/
#ifdef SSL_PLUGIN
static int get_num_messages(int sockfd, mail_details *paccount, const char *cfgdir, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int get_num_messages(int sockfd, mail_details *paccount, const char *cfgdir, unsigned int *msgid, char *ssl, char *ctx)
#endif /*SSL_PLUGIN*/
{
	char imap_message[IMAP_ID_LEN+ strlen("a UID FETCH 1:* FLAGS\r\n")];
	char *uidvalidity= NULL;
	int num_messages= 0;
	
	/*select INBOX first and get UIDVALIDITY value*/
	if((uidvalidity= select_inbox_as_mailbox(sockfd, paccount, msgid, ssl, ctx, uidvalidity))== NULL)
		return(MTC_ERR_CONNECT);

	/*send message to get uids command was successful*/
	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a UID FETCH 1:* FLAGS\r\n"));
	sprintf(imap_message,"a%.4d UID FETCH 1:* FLAGS\r\n", *msgid);
	SEND_NET_STRING(sockfd, imap_message, ssl);

	/*get the UID's of the messages*/
	if((((num_messages= receive_imap_messages_string(sockfd, paccount, cfgdir, uidvalidity, (*msgid)++, ssl))== MTC_ERR_CONNECT)||
		(num_messages== MTC_ERR_EXIT))
		&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
	{
		plg_report_error(S_IMAPFUNC_ERR_SEND_UID_FETCH, paccount->hostname);
	}
	free(uidvalidity);
	return(num_messages);
}

/*function to mark the messages as read*/
#ifdef SSL_PLUGIN
static int mark_as_read(int sockfd, mail_details *paccount, const char *cfgdir, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int mark_as_read(int sockfd, mail_details *paccount, const char *cfgdir, unsigned int *msgid, char *ssl, char *ctx)
#endif /*SSL_PLUGIN*/
{
	/*create the string to send to mark messages as read and check that they were successfully marked*/
	char imap_message[IMAP_ID_LEN+ strlen("a STORE 1: +FLAGS.SILENT (\\Seen)\r\n")+ 15];
	char readfile[NAME_MAX], line[LINE_LENGTH];
	FILE *infile= NULL;

	/*get the full path for the uidl file*/
	plg_get_account_file(readfile, cfgdir, ".readfile", paccount->id);

	/*if it does not exist we do not need to mark as read*/
	if(access(readfile, F_OK)== -1)
		return(MTC_RETURN_TRUE);
	
	/*open the read file*/
	if((infile= fopen(readfile, "r"))== NULL)
	{
		plg_report_error(S_IMAPFUNC_ERR_OPEN_READ_FILE, readfile);
		return(MTC_ERR_EXIT);
	}
	memset(line, '\0', LINE_LENGTH);
	
	/*loop for each uid read from the readfile*/
	while(fgets(line, LINE_LENGTH, infile)!= NULL)
	{
		char *spos= NULL;
		
		/*strip the uid from the line*/ 
		if(line[strlen(line)- 2]== '\r') line[strlen(line)- 2]= '\0';
		if(line[strlen(line)- 1]== '\n') line[strlen(line)- 1]= '\0';
		
		if((spos= strchr(line, '-'))== NULL)
		{
			plg_report_error(S_IMAPFUNC_ERR_GET_UID);
			return(MTC_ERR_EXIT);
		}
		/*send message to mark as read and receive*/
		memset(imap_message, '\0', strlen("a UID STORE : +FLAGS.SILENT (\\Seen)\r\n")+ 16);
		sprintf(imap_message, "a%.4d UID STORE %s +FLAGS.SILENT (\\Seen)\r\n", *msgid, spos+ 1);
		SEND_NET_STRING(sockfd, imap_message, ssl);
	
		if((!(receive_imap_login_string(sockfd, (*msgid)++, ssl)))&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
		{
			plg_report_error(S_IMAPFUNC_ERR_SEND_STORE, paccount->hostname);
			return(MTC_RETURN_FALSE);
		}
		memset(line, '\0', LINE_LENGTH);
		
	}
	
	/*close file and cleanup*/
	if(fclose(infile)== EOF)
	{
		plg_report_error(S_IMAPFUNC_ERR_CLOSE_READ_FILE);
		return(MTC_ERR_EXIT);
	}
	
	/*remove the file after marking as read*/
	if(remove(readfile)== -1)
	{	
		plg_report_error(S_IMAPFUNC_ERR_REMOVE_READ_FILE);
		return(MTC_ERR_EXIT);
	}
	return(MTC_RETURN_TRUE);
}

/*function to check mail (in clear text mode)*/
static int check_mail(mail_details *paccount, const char *cfgdir, enum imap_protocol protocol)
{
	int sockfd= 0;
	unsigned int msgid= 1;
	int num_messages= 0;

#ifdef SSL_PLUGIN
	SSL *ssl= NULL;
	SSL_CTX *ctx= NULL;
#else
	/*use char * to save code (values are unused)*/
	char *ssl= NULL;
	char *ctx= NULL;
#endif /*SSL_PLUGIN*/
	
	/*connect to the imap server*/
	if(!connect_to_server(&sockfd, paccount))
		return(MTC_ERR_CONNECT);

#ifdef SSL_PLUGIN
	if(protocol== IMAPSSL_PROTOCOL)
	{
		/*initialise SSL connection*/
		ctx= initialise_ssl_ctx(ctx);
		ssl= initialise_ssl_connection(ssl, ctx, &sockfd);
	}
#endif /*SSL_PLUGIN*/
	
	/*receive the string back from the server*/
	receive_imap_initial_string(sockfd, ssl); 

	/*test the IMAP server capabilites*/
	if(!test_imap_server_capabilities(sockfd, protocol, &msgid, ssl, ctx))
		return(MTC_ERR_CONNECT);
	
	/*login to the imap server*/
	if((protocol== IMAPSSL_PROTOCOL)|| (protocol== IMAP_PROTOCOL))
	{		
		if(!login_to_imap_server(sockfd, paccount, &msgid, ssl, ctx))
			return(MTC_ERR_CONNECT);
	}

#ifdef SSL_PLUGIN
	/*CRAM-MD5 login to imap server*/
	else if(protocol== IMAPCRAM_PROTOCOL)
	{
		if(!login_to_cramimap_server(sockfd, paccount, &msgid))
			return(MTC_ERR_CONNECT);
	}
#endif /*SSL_PLUGIN*/
	
	/*get the number of messages and outputs the uids to file*/
	if(((num_messages= get_num_messages(sockfd, paccount, cfgdir, &msgid, ssl, ctx))== MTC_ERR_CONNECT)||
		(num_messages== MTC_ERR_EXIT))
		return(num_messages);

	/*deduct number of filtered messages from new messages*/
	num_messages-= filter_messages(sockfd, paccount, cfgdir, &msgid, ssl, ctx);
	
	/*mark all of the messages in INBOX as read*/
	if(!mark_as_read(sockfd, paccount, cfgdir, &msgid, ssl, ctx))
		return(MTC_ERR_CONNECT);
	
	/*logout from the imap server*/
	if(!logout_from_imap_server(sockfd, paccount, &msgid, ssl, ctx))
		return(MTC_ERR_CONNECT);

	return(num_messages);
}

/*function to read the IMAP mail*/
int imap_read_mail(mail_details *paccount, const char *cfgdir)
{
	/*set the flag to mark as read on next mail check*/

	FILE *infile= NULL, *outfile= NULL;
	char uidlfile[NAME_MAX], tmpuidlfile[NAME_MAX];
	char buf[MAXDATASIZE];
	int numbytes= 0;
	
	memset(buf, '\0', MAXDATASIZE);
		
	/*get full paths of files*/
	plg_get_account_file(uidlfile, cfgdir, UIDL_FILE, paccount->id);
	plg_get_account_file(tmpuidlfile, cfgdir, TMP_UIDL_FILE, paccount->id);

	/*rename the temp uidl file to uidl file if it exists*/
	if(access(uidlfile, F_OK)== -1)
	{
		plg_report_error(S_IMAPFUNC_ERR_ACCESS_FILE, uidlfile); 
		return(MTC_RETURN_FALSE);
		
	}	
	/*Open the read/write files*/
	if((infile= fopen(uidlfile, "rt"))== NULL)
	{
		plg_report_error(S_IMAPFUNC_ERR_OPEN_FILE_READ, uidlfile);
		return(MTC_RETURN_FALSE);
	}
	if((outfile= fopen(tmpuidlfile, "wt"))== NULL)
	{
		plg_report_error(S_IMAPFUNC_ERR_OPEN_FILE_WRITE, tmpuidlfile); 
		return(MTC_RETURN_FALSE);
	}
	while(!feof(infile))
	{
		numbytes= fread(buf, sizeof(char), MAXDATASIZE, infile);
		
		if(ferror(infile))
		{
			plg_report_error(S_IMAPFUNC_ERR_READ_FILE, uidlfile); 
			return(MTC_RETURN_FALSE);

		}
		fwrite(buf, sizeof(char), numbytes, outfile);

		if(ferror(outfile))
		{
			plg_report_error(S_IMAPFUNC_ERR_WRITE_FILE, tmpuidlfile); 
			return(MTC_RETURN_FALSE);
		}
	}
			
	/*Close the files*/
	if((fclose(outfile)== EOF)|| (fclose(infile)== EOF))
	{
		plg_report_error(S_IMAPFUNC_ERR_CLOSE_FILE, tmpuidlfile); 
		return(MTC_RETURN_FALSE);
	}	
	return(MTC_RETURN_TRUE);
}

/*call check_mail for IMAP*/
int check_imap_mail(mail_details *paccount, const char *cfgdir)
{
	enum imap_protocol protocol= IMAP_PROTOCOL;
	return(check_mail(paccount, cfgdir, protocol));
}

/*call check_mail for IMAP (CRAM-MD5)*/
int check_cramimap_mail(mail_details *paccount, const char *cfgdir)
{
	enum imap_protocol protocol= IMAPCRAM_PROTOCOL;
	return(check_mail(paccount, cfgdir, protocol));
}

/*call check_mail for IMAP (SSL/TLS)*/
int check_imapssl_mail(mail_details *paccount, const char *cfgdir)
{
	enum imap_protocol protocol= IMAPSSL_PROTOCOL;
	return(check_mail(paccount, cfgdir, protocol));
}


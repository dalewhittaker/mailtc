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

#include "header.h"

/*function to close the connection if an error has occured*/
#ifdef MTC_USE_SSL
static int close_imap_connection(int sockfd, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int close_imap_connection(int sockfd, unsigned int *msgid, char *ssl, char *ctx)
#endif /*MTC_USE_SSL*/
{
	/*basically try to logout then terminate the connection*/
	char imap_message[IMAP_ID_LEN+ strlen("a LOGOUT\r\n")];
	
	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a LOGOUT\r\n"));
	sprintf(imap_message, "a%.4d LOGOUT\r\n", (*msgid)++);
	
	send_net_string(sockfd, imap_message, ssl); 
	
	error_and_log_no_exit(S_IMAPFUNC_ERR_CONNECT, PROGRAM_NAME);

#ifdef MTC_USE_SSL
	/*close the connection*/
	if(ssl)
		uninitialise_ssl(ssl, ctx);
#endif /*MTC_USE_SSL*/
	
	close(sockfd);
	return 1;

}

/*function to receive the initial string from the server*/
#ifdef MTC_USE_SSL
static int receive_imap_initial_string(int sockfd, SSL *ssl)
#else
static int receive_imap_initial_string(int sockfd, char *ssl)
#endif /*MTC_USE_SSL*/
{
	/*clear the buffer*/
	int numbytes= 1;
	char tmpbuf[MAXDATASIZE];
	char *buf= NULL;
	
	/*while there is data available read from server*/
	while(net_data_available(sockfd, ssl))
	{
		if((numbytes= receive_net_string(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return 0;
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
		return 0;
	
	/*test that the data was fully received and successful*/
	if((strncmp(buf, "* OK", 4)!= 0)|| (buf[strlen(buf)- 1]!= '\n')) 
	{	
		free(buf);
		return 0;
	}
	
	free(buf);
	return 1;
}

/*function to receive the string for login/logout commands*/
#ifdef MTC_USE_SSL
static int receive_imap_login_string(int sockfd, unsigned int msgid, SSL *ssl)
#else
static int receive_imap_login_string(int sockfd, unsigned int msgid, char *ssl)
#endif /*MTC_USE_SSL*/
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
	while(net_data_available(sockfd, ssl))
	{
		if((numbytes= receive_net_string(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return 0;
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
		return 0;
	
	/*test that data was fully received and command was successful*/
	if((strstr(buf, okstring)== NULL)|| (buf[strlen(buf)- 1]!= '\n')) 
	{
		free(buf);
		return 0;
	}
	free(buf);
	return 1;
}

/*function to receive the string for login/logout commands*/
#ifdef MTC_USE_SSL
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
	while(net_data_available(sockfd, ssl))
	{
		if((numbytes= receive_net_string(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return NULL;
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
		return 0;
	
	/*test that data was fully received and command was successful*/
	if((strstr(buf, okstring)== NULL)|| (buf[strlen(buf)- 1]!= '\n'))
	{
		free(buf);
		return NULL;
	}

	/*get the UIDVALIDITY value, copy 0 if not found*/
	if((spos= strstr(buf, "UIDVALIDITY "))== NULL)
		strcpy(buf, "0");
	
	if((epos= strchr(spos+ strlen("UIDVALIDITY "), ']'))== NULL)
	{
		free(buf);
		return NULL;
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
#ifdef MTC_USE_SSL
static unsigned int receive_imap_logout_string(int sockfd, unsigned int msgid, SSL *ssl)
#else
static unsigned int receive_imap_logout_string(int sockfd, unsigned int msgid, char *ssl)
#endif /*MTC_USE_SSL*/
{
	/*function is identical to login so just use that*/
	return(receive_imap_login_string(sockfd, msgid, ssl));	
}

/*function to login to IMAP server*/
#ifdef MTC_USE_SSL
static int login_to_imap_server(int sockfd, mail_details *paccount, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int login_to_imap_server(int sockfd, mail_details *paccount, unsigned int *msgid, char *ssl, char *ctx)
#endif /*MTC_USE_SSL*/
{
	/*send command to login with username and password and check login was successful*/
	char *imap_message= NULL;
	
	imap_message= (char*)alloc_mem(IMAP_ID_LEN+ strlen(paccount->username)+ strlen("a LOGIN  \r\n")+ strlen(paccount->password)+ 1, imap_message);
	sprintf(imap_message,"a%.4d LOGIN %s %s\r\n", *msgid, paccount->username, paccount->password);
	send_net_string(sockfd, imap_message, ssl);
	free(imap_message);

	if((!(receive_imap_login_string(sockfd, (*msgid)++, ssl)))&& (close_imap_connection(sockfd, msgid, ssl, ctx))) 
	{	
		error_and_log_no_exit(S_IMAPFUNC_ERR_SEND_LOGIN, paccount->hostname);
		return 0;
	}
	return 1;
}

/*function to logout of IMAP server*/
#ifdef MTC_USE_SSL
static int logout_from_imap_server(int sockfd, mail_details *paccount, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int logout_from_imap_server(int sockfd, mail_details *paccount, unsigned int *msgid, char *ssl, char *ctx)
#endif
{
	/*send the message to logout from imap server and check logout was successful*/
	char imap_message[IMAP_ID_LEN+ strlen("a LOGOUT\r\n")];

	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a LOGOUT\r\n"));
	sprintf(imap_message, "a%.4d LOGOUT\r\n", *msgid);
	send_net_string(sockfd, imap_message, ssl); 
	
	if((!(receive_imap_logout_string(sockfd, (*msgid)++, ssl)))&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
	{
		error_and_log_no_exit(S_IMAPFUNC_ERR_SEND_LOGOUT, paccount->hostname);
		return 0;
	}

#ifdef MTC_USE_SSL
	/*close the SSL connection*/
	if(ssl)
		uninitialise_ssl(ssl, ctx);
#endif
	
	close(sockfd);
	return 1;
}

#ifdef MTC_USE_SASL
/*function to receive the CRAM-MD5 string from the server*/
static char *receive_crammd5_string(int sockfd, char *buf)
{
	/*clear the buffer*/
	int numbytes= 1;
	char tmpbuf[MAXDATASIZE];
	buf= NULL;
	
	/*while there is data available read from server*/
	while(net_data_available(sockfd, NULL))
	{
		if((numbytes= receive_net_string(sockfd, tmpbuf, NULL))== -1)
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
#endif /*MTC_USE_SASL*/

/*function to login to IMAP server with CRAM-MD5 auth*/
static int login_to_cramimap_server(int sockfd, mail_details *paccount, unsigned int *msgid)
{
#ifdef MTC_USE_SASL
	Gsasl *ctx= NULL;
	char imap_message[IMAP_ID_LEN+ strlen("a AUTHENTICATE CRAM-MD5\r\n")];

	char *buf= NULL;
	char *digest= NULL;
	int rc= 0;
	
	if((rc= gsasl_init(&ctx))!= GSASL_OK)
	{
		error_and_log_no_exit(S_IMAPFUNC_ERR_SASL_INIT, rc, gsasl_strerror(rc));
		return 0;
	}

	/*send auth command*/
	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a AUTHENTICATE CRAM-MD5\r\n"));
	sprintf(imap_message, "a%.4d AUTHENTICATE CRAM-MD5\r\n", *msgid);
	send_net_string(sockfd, imap_message, NULL);

	/*receive back from server to check username was sent ok*/
	if((!(buf= receive_crammd5_string(sockfd, buf)))&& (close_imap_connection(sockfd, msgid, NULL, NULL)))
	{
		error_and_log_no_exit(S_IMAPFUNC_ERR_SEND_AUTHENTICATE, paccount->hostname);
		return 0;
	}
	
	
	/*remove '\r\n' etc*/
	if(buf[strlen(buf)- 2]== '\r')
		buf[strlen(buf)- 2]= '\0';
	if(buf[strlen(buf)- 1]== '\n')
		buf[strlen(buf)- 1]= '\0';

	/*create CRAM-MD5 string to send to server*/
	digest= create_cram_string(ctx, paccount->username, paccount->password, buf+ 2, digest);
	gsasl_done(ctx);
	free(buf);

	/*send the digest to log in*/
	buf= alloc_mem(strlen(digest)+ 3, buf);
	strcpy(buf, digest);
	strcat(buf, "\r\n");
	send_net_string(sockfd, buf, NULL);
	free(buf);
	free(digest);

	/*receive back from server to check username was sent ok*/
	if((!(receive_imap_login_string(sockfd, (*msgid)++, NULL)))&& (close_imap_connection(sockfd, msgid, NULL, NULL))) 
	{	
		error_and_log_no_exit(S_IMAPFUNC_ERR_SEND_CRAM_MD5, paccount->hostname);
		return 0;
	}

#endif /*MTC_USE_SASL*/

	return 1;
}

/*function to receive the string for login/logout commands*/
#ifdef MTC_USE_SSL
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
	while(net_data_available(sockfd, ssl))
	{
		if((numbytes= receive_net_string(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return NULL;
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
		error_and_log_no_exit(S_IMAPFUNC_ERR_NOT_IMAP_SERVER);
		return NULL;
	}
	
	/*test that data was fully received and command was successful*/
	if((strstr(buf, okstring)== NULL)|| (buf[strlen(buf)- 1]!= '\n')) 
	{
		free(buf);
		error_and_log_no_exit(S_IMAPFUNC_ERR_RECEIVE_CAPABILITY);
		return NULL;
	}
	
	return(buf);
}

/*test the capabilities of the IMAP server*/
#ifdef MTC_USE_SSL
static int test_imap_server_capabilities(int sockfd, mail_details *paccount, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int test_imap_server_capabilities(int sockfd, mail_details *paccount, unsigned int *msgid, char *ssl, char *ctx)
#endif
{
	/*send command to test capabilities and check if successful*/
	char imap_message[IMAP_ID_LEN+ strlen("a CAPABILITY\r\n")];
	char *buf= NULL;

	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a CAPABILITY\r\n"));
	sprintf(imap_message,"a%.4d CAPABILITY\r\n", *msgid);
	send_net_string(sockfd, imap_message, ssl);

	/*get the capability string*/
	if(((buf= receive_imap_capability_string(sockfd, (*msgid)++, ssl, buf))== NULL)&& (close_imap_connection(sockfd, msgid, ssl, ctx))) 
	{	
		error_and_log_no_exit(S_IMAPFUNC_ERR_GET_IMAP_CAPABILITIES);
		return 0;
	}
	
	/*Test if CRAM-MD5 is supported*/
	if((strcmp(paccount->protocol, PROTOCOL_IMAP_CRAM_MD5)== 0)&& (strstr(buf, "CRAM-MD5")== NULL))
	{
		close_imap_connection(sockfd, msgid, ssl, ctx);
		error_and_log_no_exit(S_IMAPFUNC_ERR_CRAM_MD5_NOT_SUPPORTED);
		free(buf);
		return 0;
	}
	free(buf);
	return 1;
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

	return((counter)? 0: 1) ;
}

/*function to receive the string for login/logout commands*/
#ifdef MTC_USE_SSL
static char *receive_imap_message_header(int sockfd, unsigned int msgid, SSL *ssl, char *buf)
#else
static char *receive_imap_message_header(int sockfd, unsigned int msgid, char *ssl, char *buf)
#endif /*MTC_USE_SSL*/
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
	while(net_data_available(sockfd, ssl))
	{
		if((numbytes= receive_net_string(sockfd, tmpbuf, ssl))== -1)
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
#ifdef MTC_USE_SSL
static int filter_messages(int sockfd, mail_details *paccount, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int filter_messages(int sockfd, mail_details *paccount, unsigned int *msgid, char *ssl, char *ctx)
#endif
{
	unsigned int found= 0;

	/*check if filters need to be applied*/
	if(paccount->runfilter)
	{
		char imap_message[strlen("a UID FETCH +(FLAGS BODY[HEADER]\r\n")+ 25];	
		char uidfile[NAME_MAX], line[LINE_LENGTH];
		FILE *infile= NULL;
			
		/*get the full path for the uidl file*/
		get_account_file(uidfile, UIDL_FILE, paccount->id);

		/*if it does not exist we do not need to mark as read*/
		if(access(uidfile, F_OK)== -1)
			return 1;
	
		/*open the read file*/
		if((infile= fopen(uidfile, "r"))== NULL)
			error_and_log(S_IMAPFUNC_ERR_OPEN_READ_FILE, uidfile);
	
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
				error_and_log(S_IMAPFUNC_ERR_GET_UID);

			/*send the message to get the UID header*/
			memset(imap_message, '\0', strlen("a UID FETCH +(FLAGS BODY[HEADER]\r\n")+ 25);
			sprintf(imap_message, "a%.4d UID FETCH %s (FLAGS BODY[HEADER])\r\n", *msgid, spos+ 1);
			send_net_string(sockfd, imap_message, ssl);
				
			/*get the message header*/
			if((!(buf= receive_imap_message_header(sockfd, (*msgid)++, ssl, buf)))&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
			{
				error_and_log_no_exit(S_IMAPFUNC_ERR_SEND_UID_FETCH, paccount->hostname);
				return 0;
			}
					
			/*run filters on the message and clear the buffer*/
			found+= search_for_filter_match(&paccount, buf);
		
			free(buf);
			
			/*As header has now been read we need to remark as unseen*/
			memset(imap_message, '\0', strlen("a UID FETCH +(FLAGS BODY[HEADER]\r\n")+ 25);
			sprintf(imap_message, "a%.4d UID STORE %s -FLAGS.SILENT (\\Seen)\r\n", *msgid, spos+ 1);
			send_net_string(sockfd, imap_message, ssl);
	
			/*recieve message to tell us it was successful*/
			if((!(receive_imap_login_string(sockfd, (*msgid)++, ssl)))&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
			{
				error_and_log_no_exit(S_IMAPFUNC_ERR_SEND_STORE, paccount->hostname);
				return 0;
			}
		
			memset(line, '\0', LINE_LENGTH);
		}
		/*close file and cleanup*/
		if(fclose(infile)== EOF)
			error_and_log(S_IMAPFUNC_ERR_CLOSE_READ_FILE);
	
	}
	return(found);
		
}

/*function to receive the string for uids*/
#ifdef MTC_USE_SSL
static int receive_imap_messages_string(int sockfd, mail_details *paccount, char *uidvalidity, unsigned int msgid, SSL *ssl)
#else
static int receive_imap_messages_string(int sockfd, mail_details *paccount, char *uidvalidity, unsigned int msgid, char *ssl)
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
	get_account_file(uidlfile, UIDL_FILE, paccount->id);
	get_account_file(tmpuidlfile, ".tmpuidlfile", paccount->id);
	get_account_file(readfile, ".readfile", paccount->id);

	if((outfile= fopen(uidlfile, "wt"))== NULL)
		error_and_log(S_IMAPFUNC_ERR_OPEN_FILE, uidlfile);
	
	if((outreadfile= fopen(readfile, "wt"))== NULL)
		error_and_log(S_IMAPFUNC_ERR_OPEN_FILE, readfile);
	
	/*open temp file for reading*/
	if(access(tmpuidlfile, F_OK)!= -1)
	{
		if((infile= fopen(tmpuidlfile, "r"))== NULL)
			error_and_log(S_IMAPFUNC_ERR_OPEN_TEMP_FILE);
	}
		
	/*while there is data available read from server*/
	while(net_data_available(sockfd, ssl))
	{
		char *startpos= NULL, *endpos= NULL;
	
		/* OK, if we find no '*' in the string, assuming end of receive and break
		 * otherwise get uid, increment and copy end bit*/
		
		if((numbytes= receive_net_string(sockfd, tmpbuf, ssl))== -1)
		{
			if(buf!= NULL) free(buf);
			return -1;
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
			error_and_log(S_IMAPFUNC_ERR_CLOSE_TEMP_FILE);
	
		if(remove(tmpuidlfile)== -1)
			error_and_log(S_IMAPFUNC_ERR_CLOSE_TEMP_FILE);
	}
	
	/*close the uidlfile*/
	if(fclose(outreadfile)== EOF)
		error_and_log_no_exit(S_IMAPFUNC_ERR_CLOSE_FILE, readfile);
	if(fclose(outfile)== EOF)
		error_and_log_no_exit(S_IMAPFUNC_ERR_CLOSE_FILE, uidlfile);
	
	if(buf== NULL)
		return -1;
	
	/*test that full message was received and it was successful*/
	if((strstr(buf, okstring)== NULL)|| (buf[strlen(buf)- 1]!= '\n'))
	{
		free(buf);
		return -1;
	}
	free(buf);

	return(num_messages);
}

/*function to Select the INBOX as the mailbox to use*/
#ifdef MTC_USE_SSL
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
	send_net_string(sockfd, imap_message, ssl);
	
	if(((buf= receive_imap_select_string(sockfd, (*msgid)++, ssl, buf))== NULL)&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
	{
		error_and_log_no_exit(S_IMAPFUNC_ERR_SEND_SELECT, paccount->hostname);
		return NULL;
	}

	return(buf);
}

/*function to output the uids of each message*/
#ifdef MTC_USE_SSL
static int get_num_messages(int sockfd, mail_details *paccount, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int get_num_messages(int sockfd, mail_details *paccount, unsigned int *msgid, char *ssl, char *ctx)
#endif /*MTC_USE_SSL*/
{
	char imap_message[IMAP_ID_LEN+ strlen("a UID FETCH 1:* FLAGS\r\n")];
	char *uidvalidity= NULL;
	int num_messages= 0;
	
	/*select INBOX first and get UIDVALIDITY value*/
	if((uidvalidity= select_inbox_as_mailbox(sockfd, paccount, msgid, ssl, ctx, uidvalidity))== NULL)
		return -1;

	/*send message to get uids command was successful*/
	memset(imap_message, '\0', IMAP_ID_LEN+ strlen("a UID FETCH 1:* FLAGS\r\n"));
	sprintf(imap_message,"a%.4d UID FETCH 1:* FLAGS\r\n", *msgid);
	send_net_string(sockfd, imap_message, ssl);

	/*get the UID's of the messages*/
	if(((num_messages= receive_imap_messages_string(sockfd, paccount, uidvalidity, (*msgid)++, ssl))== -1)&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
	{
		error_and_log_no_exit(S_IMAPFUNC_ERR_SEND_UID_FETCH, paccount->hostname);
		free(uidvalidity);
		return -1;
	}
	free(uidvalidity);
	return(num_messages);
}

/*function to mark the messages as read*/
#ifdef MTC_USE_SSL
static int mark_as_read(int sockfd, mail_details *paccount, unsigned int *msgid, SSL *ssl, SSL_CTX *ctx)
#else
static int mark_as_read(int sockfd, mail_details *paccount, unsigned int *msgid, char *ssl, char *ctx)
#endif /*MTC_USE_SSL*/
{
	/*create the string to send to mark messages as read and check that they were successfully marked*/
	char imap_message[IMAP_ID_LEN+ strlen("a STORE 1: +FLAGS.SILENT (\\Seen)\r\n")+ 15];
	char readfile[NAME_MAX], line[LINE_LENGTH];
	FILE *infile= NULL;

	/*get the full path for the uidl file*/
	get_account_file(readfile, ".readfile", paccount->id);

	/*if it does not exist we do not need to mark as read*/
	if(access(readfile, F_OK)== -1)
		return 1;
	
	/*open the read file*/
	if((infile= fopen(readfile, "r"))== NULL)
		error_and_log(S_IMAPFUNC_ERR_OPEN_READ_FILE, readfile);
	
	memset(line, '\0', LINE_LENGTH);
	
	/*loop for each uid read from the readfile*/
	while(fgets(line, LINE_LENGTH, infile)!= NULL)
	{
		char *spos= NULL;
		
		/*strip the uid from the line*/ 
		if(line[strlen(line)- 2]== '\r') line[strlen(line)- 2]= '\0';
		if(line[strlen(line)- 1]== '\n') line[strlen(line)- 1]= '\0';
		
		if((spos= strchr(line, '-'))== NULL)
			error_and_log(S_IMAPFUNC_ERR_GET_UID);

		/*send message to mark as read and receive*/
		memset(imap_message, '\0', strlen("a UID STORE : +FLAGS.SILENT (\\Seen)\r\n")+ 16);
		sprintf(imap_message, "a%.4d UID STORE %s +FLAGS.SILENT (\\Seen)\r\n", *msgid, spos+ 1);
		send_net_string(sockfd, imap_message, ssl);
	
		if((!(receive_imap_login_string(sockfd, (*msgid)++, ssl)))&& (close_imap_connection(sockfd, msgid, ssl, ctx)))
		{
			error_and_log_no_exit(S_IMAPFUNC_ERR_SEND_STORE, paccount->hostname);
			return 0;
		}
		memset(line, '\0', LINE_LENGTH);
		
	}
	
	/*close file and cleanup*/
	if(fclose(infile)== EOF)
		error_and_log(S_IMAPFUNC_ERR_CLOSE_READ_FILE);
	
	/*remove the file after marking as read*/
	if(remove(readfile)== -1)
			error_and_log(S_IMAPFUNC_ERR_REMOVE_READ_FILE);
	
	return 1;
}

/*function to check mail (in clear text mode)*/
int check_imap_mail(mail_details *paccount)
{
	int sockfd= 0;
	unsigned int msgid= 1;

#ifdef MTC_USE_SSL
	SSL *ssl= NULL;
	SSL_CTX *ctx= NULL;
#else
	/*use char * to save code (values are unused)*/
	char *ssl= NULL;
	char *ctx= NULL;
#endif /*MTC_USE_SSL*/
	
	/*connect to the imap server*/
	if(!connect_to_server(&sockfd, paccount))
		return -1;

#ifdef MTC_USE_SSL
	if(strcmp(paccount->protocol, PROTOCOL_IMAP_SSL)== 0)
	{
		/*initialise SSL connection*/
		ctx= initialise_ssl_ctx(ctx);
		ssl= initialise_ssl_connection(ssl, ctx, &sockfd);
	}
#endif /*MTC_USE_SSL*/
	
	/*receive the string back from the server*/
	receive_imap_initial_string(sockfd, ssl); 

	/*test the IMAP server capabilites*/
	if(!test_imap_server_capabilities(sockfd, paccount, &msgid, ssl, ctx))
		return -1;
	
	/*login to the imap server*/
	if((strcmp(paccount->protocol, PROTOCOL_IMAP_SSL)== 0)||
		strcmp(paccount->protocol, PROTOCOL_IMAP)== 0)
	{		
		if(!login_to_imap_server(sockfd, paccount, &msgid, ssl, ctx))
			return -1;
	}
	/*CRAM-MD5 login to imap server*/
	else if(strcmp(paccount->protocol, PROTOCOL_IMAP_CRAM_MD5)== 0)
	{
		if(!login_to_cramimap_server(sockfd, paccount, &msgid))
			return -1;
	}
	
	/*get the number of messages and outputs the uids to file*/
	if((paccount->num_messages= get_num_messages(sockfd, paccount, &msgid, ssl, ctx))== -1)
		return -1;

	/*deduct number of filtered messages from new messages*/
	paccount->num_messages-= filter_messages(sockfd, paccount, &msgid, ssl, ctx);
	
	/*mark all of the messages in INBOX as read*/
	if(!mark_as_read(sockfd, paccount, &msgid, ssl, ctx))
		return -1;
	
	/*logout from the imap server*/
	if(!logout_from_imap_server(sockfd, paccount, &msgid, ssl, ctx))
		return -1;

	return(1);
}


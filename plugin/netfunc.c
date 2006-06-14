/* netfunc.c
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

/*function to connect to mail server*/
int connect_to_server(int *sockfd, mail_details *paccount)
{
	struct hostent *he;
	struct sockaddr_in their_addr;
	
	/*get the ip from the hostname*/
	if((he= gethostbyname(paccount->hostname))== NULL)
	{
		plg_report_error(S_NETFUNC_ERR_IP, paccount->hostname);
		return(MTC_RETURN_FALSE);
	}
		
	/*get the socket for the connection*/
	if((*sockfd= socket(AF_INET, SOCK_STREAM, 0))== -1)
	{
		plg_report_error(S_NETFUNC_ERR_SOCKET);
		return(MTC_RETURN_FALSE);
	}

	/*setup the connection details*/
	their_addr.sin_family= AF_INET;
	their_addr.sin_port= htons(atoi(paccount->port));
	their_addr.sin_addr= *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero),'\0', 8);
	
	/*try to connect only if doconnect is true (for non SSL)*/
	if(connect(*sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr))== -1)
	{
		plg_report_error(S_NETFUNC_ERR_CONNECT, paccount->hostname);
		close(*sockfd);
		return(MTC_RETURN_FALSE);
	}

	return(MTC_RETURN_TRUE);
}

/*function to blank out the password chars before sending*/
static int print_pw_string(const char *sendstring)
{
	char *pwstring= NULL, *ptr= NULL;
	unsigned int slen;
			
	slen= strlen(sendstring);
	pwstring= alloc_mem(slen+ 1, pwstring);
			
	/*we replace all the password chars with * if it is a password*/
	strcpy(pwstring, sendstring);
	if((ptr= strrchr(pwstring, ' '))== NULL)
	{
		plg_report_error(S_NETFUNC_ERR_PW_STRING);
		return(MTC_RETURN_FALSE);
	}
			
	/*iterate until '\r' is found*/
	while((*(++ptr))&& (*ptr!= '\r'))
		*ptr= '*';
			
	printf(pwstring);
	free(pwstring);
	
	return(MTC_RETURN_TRUE);
}

/* function to send a message to the network server */
#ifdef SSL_PLUGIN
int ssl_send_net_string(int sockfd, char *sendstring, SSL *ssl, unsigned int pw)
#else
int std_send_net_string(int sockfd, char *sendstring, char *ssl, unsigned int pw)
#endif /*SSL_PLUGIN*/
{
	
	/*if debug mode is selected print the string to send*/
	if(net_debug)
	{
		if(pw)
		{
			if(!print_pw_string(sendstring))
				return(MTC_RETURN_FALSE);
		}
		else
			printf(sendstring);
		
		fflush(stdout);
	}

	/*send the string to the server*/
	if(!ssl)
	{
		if(send(sockfd, sendstring, strlen(sendstring), 0)== -1)
		{
			plg_report_error(S_NETFUNC_ERR_SEND);
			return(MTC_RETURN_FALSE);
		}
	}
#ifdef SSL_PLUGIN
	else
	{
		if(SSL_write(ssl, sendstring, strlen(sendstring))<= 0)
		{
			plg_report_error(S_NETFUNC_ERR_SEND);
			return(MTC_RETURN_FALSE);
		}
	}
#endif /*SSL_PLUGIN*/
	
	return(MTC_RETURN_TRUE);
}

/*function to check if data is available at the server*/
#ifdef SSL_PLUGIN
int ssl_net_data_available(int sockfd, SSL *ssl)
#else
int std_net_data_available(int sockfd, char *ssl)
#endif /*SSL_PLUGIN*/
{
	fd_set fds;
	struct timeval tv;
	int n;

#ifdef SSL_PLUGIN
	/*SSL handles things differently, so we have to check if there is any pending first*/
	if((ssl)&& (SSL_pending(ssl)))
		return(MTC_RETURN_TRUE);
#endif
		
	/*set up the file descriptor set*/
	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);
			
	/*setup the timeval for the timeout*/
	tv.tv_sec= NET_TIMEOUT; 
	tv.tv_usec= 0;

	/*wait until timeout or data is available*/
	n= select(sockfd+ 1, &fds, NULL, NULL, &tv);
	
	/*timeout*/
	if(n== 0)
		return(MTC_RETURN_FALSE);
	
	/*error with select command*/
	else if(n== -1)
	{
		plg_report_error(S_NETFUNC_ERR_DATA_AVAILABLE);
		return(MTC_RETURN_FALSE);
	}

	return(MTC_RETURN_TRUE);
	
}

/*general function for network receiving*/
#ifdef SSL_PLUGIN
int ssl_receive_net_string(int sockfd, char *recvstring, SSL *ssl)
#else
int std_receive_net_string(int sockfd, char *recvstring, char *ssl)
#endif
{
	int numbytes= 0;

	/*clear the buffer*/
	memset(recvstring, '\0', MAXDATASIZE);
	
	/*receive a string from the server*/
	if(ssl== NULL)
	{
		if((numbytes= recv(sockfd, recvstring, MAXDATASIZE- 1, 0))== -1)
		{
			plg_report_error(S_NETFUNC_ERR_RECEIVE);
			return(MTC_ERR_CONNECT);
		}
	}
#ifdef SSL_PLUGIN
	else
	{
		if((numbytes= SSL_read(ssl, recvstring, MAXDATASIZE- 1))<= 0)
		{
			plg_report_error(S_NETFUNC_ERR_RECEIVE);
			return(MTC_ERR_CONNECT);
		}
	}
#endif /*SSL_PLUGIN*/
	
	/*terminate the string*/
	recvstring[numbytes]= '\0';
	
	/*if debug mode print the received string*/
	if(net_debug)
	{
		printf(recvstring);
		fflush(stdout);
	}

	/*return number of bytes received*/
	return(numbytes);

}


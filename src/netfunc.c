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

#include "core.h"

/*function to connect to mail server*/
int connect_to_server(int *sockfd, mail_details *paccount)
{
	struct hostent *he;
	struct sockaddr_in their_addr;
	
	/*get the ip from the hostname*/
	if((he= gethostbyname(paccount->hostname))== NULL)
	{
		error_and_log_no_exit(S_NETFUNC_ERR_IP, paccount->hostname);
		return 0;
	}
		
	/*get the socket for the connection*/
	if((*sockfd= socket(AF_INET, SOCK_STREAM, 0))== -1)
	{
		error_and_log_no_exit(S_NETFUNC_ERR_SOCKET);
		return 0;
	}

	/*setup the connection details*/
	their_addr.sin_family= AF_INET;
	their_addr.sin_port= htons(atoi(paccount->port));
	their_addr.sin_addr= *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero),'\0', 8);
	
	/*try to connect only if doconnect is true (for non SSL)*/
	if(connect(*sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr))== -1)
	{
		error_and_log_no_exit(S_NETFUNC_ERR_CONNECT, paccount->hostname);
		close(*sockfd);
		return 0;
	}

	return 1;
}

/* function to send a message to the network server */
#ifdef MTC_USE_SSL
int send_net_string(int sockfd, char *sendstring, SSL *ssl)
#else
int send_net_string(int sockfd, char *sendstring, char *ssl)
#endif /*MTC_USE_SSL*/
{

	/*if debug mode is selected print the string to send*/
	if(config.net_debug)
	{
		printf(sendstring);
		fflush(stdout);
	}

	/*send the string to the server*/
	if(!ssl)
	{
		if(send(sockfd, sendstring, strlen(sendstring), 0)== -1)
		{
			error_and_log_no_exit(S_NETFUNC_ERR_SEND);
			return 0;
		}
	}
#ifdef MTC_USE_SSL
	else
	{
		if(SSL_write(ssl, sendstring, strlen(sendstring))<= 0)
		{
			error_and_log_no_exit(S_NETFUNC_ERR_SEND);
			return 0;
		}
	}
#endif /*MTC_USE_SSL*/
	
	return 1;
}

/*function to check if data is available at the server*/
#ifdef MTC_USE_SSL
int net_data_available(int sockfd, SSL *ssl)
#else
int net_data_available(int sockfd, char *ssl)
#endif /*MTC_USE_SSL*/
{
	fd_set fds;
	struct timeval tv;
	int n;

#ifdef MTC_USE_SSL
	/*SSL handles things differently, so we have to check if there is any pending first*/
	if((ssl)&& (SSL_pending(ssl)))
		return 1;
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
	{	
		return 0;
	}
	/*error with select command*/
	if(n== -1)
	{
		error_and_log_no_exit(S_NETFUNC_ERR_DATA_AVAILABLE);
		return 0;
	}

	return 1;
	
}

/*general function for network receiving*/
#ifdef MTC_USE_SSL
int receive_net_string(int sockfd, char *buf, SSL *ssl)
#else
int receive_net_string(int sockfd, char *buf, char *ssl)
#endif
{
	int numbytes= 0;

	/*clear the buffer*/
	memset(buf, '\0', MAXDATASIZE);
	
	/*receive a string from the server*/

	if(ssl== NULL)
	{
		if((numbytes= recv(sockfd, buf, MAXDATASIZE- 1, 0))== -1)
		{
			error_and_log_no_exit(S_NETFUNC_ERR_RECEIVE);
			return -1;
		}
	}
#ifdef MTC_USE_SSL
	else
	{
		if((numbytes= SSL_read(ssl, buf, MAXDATASIZE- 1))<= 0)
		{
			error_and_log_no_exit(S_NETFUNC_ERR_RECEIVE);
			return -1;
		}
	}
#endif /*MTC_USE_SSL*/
	
	/*terminate the string*/
	buf[numbytes]= '\0';
	
	/*if debug mode print the received string*/
	if(config.net_debug)
	{
		printf(buf);
		fflush(stdout);
	}

	/*return number of bytes received*/
	return(numbytes);

}



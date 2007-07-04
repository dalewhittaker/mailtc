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

#include "netfunc.h"
#include <stdlib.h>

/*win32 defines SOCKET_ERROR for setsockopt()/getsockopt()*/
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif /*SOCKET_ERROR*/

/*win32 defines INVALID_SOCKET for socket()*/
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif /*INVALID_SOCKET*/

#define NET_TIMEOUT 5
#define CONNECT_TIMEOUT 30
#define IS_SSL_AUTH(auth) ((auth)== POPSSL_PROTOCOL|| (auth)== IMAPSSL_PROTOCOL)

#ifdef SSL_PLUGIN
/*initialise SSL connection*/
static mtc_error ssl_net_connect(mtc_net *pnetinfo)
{
    SSL_METHOD *method= NULL;
	gboolean err= FALSE;

	/*initialise SSL*/
	SSL_load_error_strings();
	SSL_library_init();

	/*choose TLS method and create CTX*/
	method= SSLv23_client_method();
	if((pnetinfo->pctx= SSL_CTX_new(method))== NULL)
	{
        plg_err(S_PLG_COMMON_ERR_CTX);
        err= TRUE;
    }
	/*create an SSL structure for the TLS connection*/
    if((pnetinfo->pssl= SSL_new(pnetinfo->pctx))== NULL)
	{
        plg_err(S_PLG_COMMON_ERR_CREATE_SSL_STRUCT);
        err= TRUE;
    }
	/*set the SSL file descriptor*/
	if(SSL_set_fd(pnetinfo->pssl, pnetinfo->sockfd)== 0)
	{
        plg_err(S_PLG_COMMON_ERR_SSL_DESC);
	    err= TRUE;
    }
	/*connect with SSL*/
	if(SSL_connect(pnetinfo->pssl)<= 0)
	{
        plg_err(S_PLG_COMMON_ERR_SSL_CONNECT);
	    err= TRUE;
    }

    if(err)
    {    
        net_disconnect(pnetinfo);
        return(MTC_ERR_CONNECT);
    }
    return(MTC_RETURN_TRUE);
}

/*Uninitialise SSL connection*/
static mtc_error ssl_net_disconnect(mtc_net *pnetinfo)
{
	/*I think this is ok, if not see SSL_shutdown, this may need to be done a different way*/
	if(SSL_shutdown(pnetinfo->pssl)== -1)
		plg_err(S_PLG_COMMON_ERR_SSL_SHUTDOWN);
		
	if(pnetinfo->pssl)
        SSL_free(pnetinfo->pssl);
	if(pnetinfo->pctx)
        SSL_CTX_free(pnetinfo->pctx);
	ERR_free_strings();

	return(MTC_RETURN_TRUE);
}

#endif /*SSL_PLUGIN*/

/*wrapper function to close a socket and cleanup*/
static gint sock_close(mtc_net *pnetinfo)
{
#ifdef _WIN32
    closesocket(pnetinfo->sockfd);
    return(WSACleanup());
#else
    return(close(pnetinfo->sockfd));
#endif /*_WIN32*/
}

/*called if there is a network error,
 *and connect() has not yet been called*/
static mtc_error sock_err(void)
{
#ifdef _WIN32
        WSACleanup();
#endif /*_WIN32*/
    return(MTC_ERR_CONNECT);
}

/*function to connect to mail server*/
mtc_error net_connect(mtc_net *pnetinfo, mtc_account *paccount)
{
	struct hostent *he;
	struct sockaddr_in their_addr;

    /*used for connection timeouts*/
	struct timeval t_old;
    socklen_t t_oldlen= sizeof(t_old);
    struct timeval t_new;

#ifdef _WIN32
    /*initialise windows sockets if required*/
    WSADATA wsadata;
    if(WSAStartup(MAKEWORD(2, 2), &wsadata)!= 0)
    {
        plg_err(S_NETFUNC_ERR_WINSOCK_INIT);
        return(MTC_ERR_CONNECT);
    }
#endif /*_WIN32*/

    /*get the ip from the server*/
	if((he= gethostbyname(paccount->server))== NULL)
	{
		plg_err(S_NETFUNC_ERR_IP, paccount->server);
		return(sock_err());
	}
		
	/*get the socket for the connection*/
	if((pnetinfo->sockfd= socket(AF_INET, SOCK_STREAM, 0))== INVALID_SOCKET)
	{
		plg_err(S_NETFUNC_ERR_SOCKET);
		return(sock_err());
	}

	/*setup the connection details*/
	their_addr.sin_family= AF_INET;
	their_addr.sin_addr= *((struct in_addr *)he->h_addr_list[0]);
	/*their_addr.sin_port= htons(atoi(paccount->port));*/
	their_addr.sin_port= htons(paccount->port);
	memset(&(their_addr.sin_zero), '\0', 8);
	
    /*get the current connect timeout val*/
    if(getsockopt(pnetinfo->sockfd, SOL_SOCKET, SO_RCVTIMEO, (gchar *)&t_old, &t_oldlen)== SOCKET_ERROR)
    {
		plg_err(S_NETFUNC_ERR_GET_TIMEOUT);
		return(sock_err());
    }

    /*set the actual no. seconds for timeout*/
    t_new.tv_sec= CONNECT_TIMEOUT;
    t_new.tv_usec = 0;
	
    /*set the new timeout value*/
    if(setsockopt(pnetinfo->sockfd, SOL_SOCKET, SO_RCVTIMEO, (gchar *)&t_new, sizeof(t_new))== SOCKET_ERROR)
    {
    	plg_err(S_NETFUNC_ERR_SET_TIMEOUT);
		return(sock_err());
    }

	/*try to connect only if doconnect is true (for non SSL)*/
	if(connect(pnetinfo->sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr))== SOCKET_ERROR)
	{
		plg_err(S_NETFUNC_ERR_CONNECT, paccount->server);
        sock_close(pnetinfo);
        return(MTC_ERR_CONNECT);
	}

    /*now reset the old timeout value back*/
    if(setsockopt(pnetinfo->sockfd, SOL_SOCKET, SO_RCVTIMEO, (gchar *)&t_old, sizeof(t_old))== SOCKET_ERROR)
    {
		plg_err(S_NETFUNC_ERR_RESET_TIMEOUT);
        sock_close(pnetinfo);
        return(MTC_ERR_CONNECT);
    }

#ifdef SSL_PLUGIN
    pnetinfo->pssl= NULL;
    pnetinfo->pctx= NULL;

    if(IS_SSL_AUTH(pnetinfo->authtype))
		return(ssl_net_connect(pnetinfo));
#endif /*SSL_PLUGIN*/

	return(MTC_RETURN_TRUE);
}

/*function to uninitialise and close the socket*/
mtc_error net_disconnect(mtc_net *pnetinfo)
{
#ifdef SSL_PLUGIN
	/*close the SSL connection*/
	if(pnetinfo->pssl)
		ssl_net_disconnect(pnetinfo);
#endif

    /*finally close the socket*/
    sock_close(pnetinfo);
    return(MTC_RETURN_TRUE);
}

/*function to blank out the password chars before sending*/
static mtc_error pwprint(const gchar *sendstring)
{
	gchar *pwstring= NULL, *ptr= NULL;
	guint slen;
			
	slen= strlen(sendstring);
	pwstring= (gchar *)g_malloc0(sizeof(gchar)* (slen+ 1));
			
	/*we replace all the password chars with * if it is a password*/
	g_strlcpy(pwstring, sendstring, slen+ 1);
	if((ptr= strrchr(pwstring, ' '))== NULL)
	{
		plg_err(S_NETFUNC_ERR_PW_STRING);
		g_free(pwstring);
		return(MTC_RETURN_FALSE);
	}
			
	/*iterate until '\r' is found*/
	while((*(++ptr))&& ((*ptr!= '\r')&& (*ptr!= '\n')))
		*ptr= '*';
			
	g_print(pwstring);
	g_free(pwstring);
	
	return(MTC_RETURN_TRUE);
}

/* function to send a message to the network server */
mtc_error net_send(mtc_net *pnetinfo, gchar *sendstring, gboolean pw)
{
	
	/*if debug mode is selected print the string to send*/
	if(cfg_get()->net_debug)
	{
		if(pw)
		{
			if(!pwprint(sendstring))
				return(MTC_ERR_CONNECT);
		}
		else
			g_print(sendstring);
		
		fflush(stdout);
	}

#ifdef SSL_PLUGIN
	if(pnetinfo->pssl)
	{
        if(SSL_write(pnetinfo->pssl, sendstring, strlen(sendstring))<= 0)
		{
			plg_err(S_NETFUNC_ERR_SEND);
			return(MTC_ERR_CONNECT);
		}
    }
    else
    {
#endif /*SSL_PLUGIN*/
	/*send the string to the server*/
		if(send(pnetinfo->sockfd, sendstring, strlen(sendstring), 0)== SOCKET_ERROR)
		{
			plg_err(S_NETFUNC_ERR_SEND);
			return(MTC_ERR_CONNECT);
		}
#ifdef SSL_PLUGIN
	}
#endif /*SSL_PLUGIN*/	
	return(MTC_RETURN_TRUE);
}

/*function to check if data is available at the server*/
mtc_error net_available(mtc_net *pnetinfo)
{
	fd_set fds;
	struct timeval tv;
	gint n;

#ifdef SSL_PLUGIN
	/*SSL handles things differently, so we have to check if there is any pending first*/
	if((pnetinfo->pssl)&& (SSL_pending(pnetinfo->pssl)))
		return(MTC_RETURN_TRUE);
#endif
		
	/*set up the file descriptor set*/
	FD_ZERO(&fds);
	FD_SET(pnetinfo->sockfd, &fds);
			
	/*setup the timeval for the timeout*/
	tv.tv_sec= NET_TIMEOUT; 
	tv.tv_usec= 0;

	/*wait until timeout or data is available*/
	n= select((gint)(pnetinfo->sockfd+ 1), &fds, NULL, NULL, &tv);
	
	/*timeout*/
	if(n== 0)
		return(MTC_RETURN_FALSE);
	
	/*error with select command*/
	else if(n== SOCKET_ERROR)
	{
		plg_err(S_NETFUNC_ERR_DATA_AVAILABLE);
		return(MTC_RETURN_FALSE);
	}

	return(MTC_RETURN_TRUE);
	
}

/*general function for network receiving*/
gint net_recv(mtc_net *pnetinfo, gchar *recvstring, guint recvslen)
{
	gint numbytes= 0;

	/*clear the buffer*/
	memset(recvstring, '\0', recvslen);
	
#ifdef SSL_PLUGIN
    if(pnetinfo->pssl!= NULL)
 	{
		if((numbytes= SSL_read(pnetinfo->pssl, recvstring, recvslen- 1))<= 0)
		{
			plg_err(S_NETFUNC_ERR_RECEIVE);
			return(MTC_ERR_CONNECT);
		}
	}
	else
    {
#endif /*SSL_PLUGIN*/
	/*receive a string from the server*/
		if((numbytes= recv(pnetinfo->sockfd, recvstring, recvslen- 1, 0))== SOCKET_ERROR)
		{
			plg_err(S_NETFUNC_ERR_RECEIVE);
			return(MTC_ERR_CONNECT);
		}
#ifdef SSL_PLUGIN
	}
#endif /*SSL_PLUGIN*/

	/*terminate the string*/
	recvstring[numbytes]= '\0';
	
	/*if debug mode print the received string*/
	if(cfg_get()->net_debug)
	{
		g_print(recvstring);
		fflush(stdout);
	}

	/*return number of bytes received*/
	return(numbytes);
}


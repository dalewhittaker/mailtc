/* tls.c
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
		error_and_log_no_exit(S_TLS_ERR_CTX);

	return(ctx);
}

/*initialise SSL connection*/
SSL *initialise_ssl_connection(SSL *ssl, SSL_CTX *ctx, int *sockfd)
{
	/*create an SSL structure for the TLS connection*/
	if((ssl= SSL_new(ctx))== NULL)
		error_and_log_no_exit(S_TLS_ERR_CREATE_STRUCT);

	/*set the SSL file descriptor*/
	if(SSL_set_fd(ssl, *sockfd)== 0)
		error_and_log_no_exit(S_TLS_ERR_SSL_DESC);

	
	/*connect with SSL*/
	if(SSL_connect(ssl)<= 0)
		error_and_log_no_exit(S_TLS_ERR_SSL_CONNECT);
	
	return(ssl);
}

/*Uninitialise SSL connection*/
int uninitialise_ssl(SSL *ssl, SSL_CTX *ctx)
{
	if(SSL_shutdown(ssl)== -1)
		error_and_log(S_TLS_ERR_SSL_SHUTDOWN);
		
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	
	return 1;
}

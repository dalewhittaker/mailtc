/* sasl.c
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

/*create CRAM-MD5 string to send to server*/
char *create_cram_string(Gsasl *ctx, char *username, char *password, char *serverdigest, char *clientdigest)
{
	Gsasl_session *session;
	const char *mech= "CRAM-MD5";
	int rc;

	/*Create new authentication session*/
	if((rc= gsasl_client_start(ctx, mech, &session)) != GSASL_OK)
	{
		error_and_log(S_SASL_ERR_INITIALISE, rc, gsasl_strerror(rc));
		return(NULL);
	}

	/*set username and password in session handle*/
	gsasl_property_set(session, GSASL_AUTHID, username);
	gsasl_property_set(session, GSASL_PASSWORD, password);

	/*create the base64 string to send back to the server
	 *from the username, password and server base64 string*/
	if((rc= gsasl_step64(session, serverdigest, &clientdigest))!= GSASL_OK)
	{
		error_and_log(S_SASL_ERR_CRAM_MD5_AUTH, rc, gsasl_strerror(rc));
		free(clientdigest);
		return(NULL);
	}
	
	/*cleanup*/
	gsasl_finish(session);

	return(clientdigest);
}

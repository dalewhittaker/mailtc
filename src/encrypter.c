/* encrypter.c
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

#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include "encrypter.h"

#define ENCRYPTION_KEY "mailtc password encryption key"

static guchar iv[]= {1, 2, 3, 4, 5, 6, 7, 8}; /*string to hold the encryption key*/

/*function to encrypt the password before it gets written to the password file*/
gchar *pw_encrypt(gchar *decstring)
{
	gchar tmpstring[1024];
	gint encrypted_len, tmplen;
    gboolean success= TRUE;
	
	/*initialise cipher content*/
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx); 

	/*set up cipher context with cipher type (blowfish ofb)*/
	EVP_EncryptInit_ex(&ctx, EVP_bf_ofb(), NULL, (guchar *)ENCRYPTION_KEY, iv); 

	/*encrypt the data and the final padding*/
	if(!EVP_EncryptUpdate(&ctx, (guchar *)tmpstring, &encrypted_len, (guchar *)decstring, strlen(decstring)))
	{
        err_noexit(S_ENCRYPTER_ERR_ENC_PW);
	    success= FALSE;
    }
    else if(!EVP_EncryptFinal_ex(&ctx, (guchar *)(tmpstring+ encrypted_len), &tmplen))
	{
        err_noexit(S_ENCRYPTER_ERR_ENC_PW_FINAL);
	    success= FALSE;
    }
	/*set length equal to encrypted string plus encrypted padding*/
	encrypted_len+= tmplen; 

	/*cleanup and return length*/
	EVP_CIPHER_CTX_cleanup(&ctx);

    /*Now base64 the string for the output file*/
    return((success)? g_base64_encode((guchar *)tmpstring, encrypted_len): NULL);
}

/*function to decrypt the password before it gets used*/
gboolean pw_decrypt(gchar *encstring, gchar *decstring)
{
	gint outlen, tmplen;
    guchar *tmpstring= NULL;
    gsize len= 0;
    gboolean retval= TRUE;

    /*decode the base64 string*/
    tmpstring= g_base64_decode(encstring, &len);

	/*initialise cipher content*/
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx);

	/*set up cipher context with cipher type (blowfish ofb)*/
	EVP_DecryptInit_ex(&ctx, EVP_bf_ofb(), NULL, (guchar *)ENCRYPTION_KEY, iv); 
	
	/*Decrypt the data and the final padding at the end*/
    if(!EVP_DecryptUpdate(&ctx, (guchar *)decstring, &outlen, tmpstring, (gint)len))
	{
        err_noexit(S_ENCRYPTER_ERR_DEC_PW);
	    retval= FALSE;
    }
    else if(!EVP_DecryptFinal_ex(&ctx, (guchar *)(decstring+ outlen), &tmplen))
	{
        err_noexit(S_ENCRYPTER_ERR_DEC_PW_FINAL);
	    retval= FALSE;
    }
    /*free the temp buffer*/
    g_free(tmpstring);

	/*set the length and cleanup*/
	outlen+= tmplen;
	EVP_CIPHER_CTX_cleanup(&ctx);
	
	decstring[outlen]= '\0';
	return(retval);
}


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
#include "core.h"

static guchar iv[]= {1, 2, 3, 4, 5, 6, 7, 8}; /*string to hold the encryption key*/

/*function to encrypt the password before it gets written to the password file*/
gulong pw_encrypt(gchar *decstring, gchar *encstring)
{
	gint encrypted_len, tmplen;
	
	/*initialise cipher content*/
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx); 

	/*set up cipher context with cipher type (base64)*/
	EVP_EncryptInit_ex(&ctx, EVP_bf_ofb(), NULL, (guchar *)ENCRYPTION_KEY, iv); 

	/*encrypt the data and the final padding*/
	if(!EVP_EncryptUpdate(&ctx, (guchar *)encstring, &encrypted_len, (guchar *)decstring, strlen(decstring)))
		err_exit(S_ENCRYPTER_ERR_ENC_PW);
	if(!EVP_EncryptFinal_ex(&ctx, (guchar *)(encstring+ encrypted_len), &tmplen))
		err_exit(S_ENCRYPTER_ERR_ENC_PW_FINAL);
	
	/*set length equal to encrypted string plus encrypted padding*/
	encrypted_len+= tmplen; 

	/*cleanup and return length*/
	EVP_CIPHER_CTX_cleanup(&ctx);
	return encrypted_len;
	
}

/*function to decrypt the password before it gets used*/
gboolean pw_decrypt(gchar *encstring, gint enclen, gchar *decstring)
{
	gint outlen, tmplen;

	/*initialise cipher content*/
	EVP_CIPHER_CTX ctx;
	EVP_CIPHER_CTX_init(&ctx);

	/*set up cipher context with cipher type (base64)*/
	EVP_DecryptInit_ex(&ctx, EVP_bf_ofb(), NULL, (guchar *)ENCRYPTION_KEY, iv); 
	
	/*Decrypt the data and the final padding at the end*/
	if(!EVP_DecryptUpdate(&ctx, (guchar *)decstring, &outlen, (guchar *)encstring, enclen))
		err_exit(S_ENCRYPTER_ERR_DEC_PW);
	if(!EVP_DecryptFinal_ex(&ctx, (guchar *)(decstring+ outlen), &tmplen))
		err_exit(S_ENCRYPTER_ERR_DEC_PW_FINAL);
	
	/*set the length and cleanup*/
	outlen+= tmplen;
	EVP_CIPHER_CTX_cleanup(&ctx);
	
	decstring[outlen]= '\0';
	
	return TRUE;
}


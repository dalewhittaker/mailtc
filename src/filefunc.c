/* filefunc.c 
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

/*function to get $HOME + program*/
int get_program_dir(void)
{
	/*get the home path, if not found use /tmp*/
	char *penv;
	if((penv= getenv("HOME"))== NULL)
		sprintf(files.base_name, "/tmp/.%s/", PROGRAM_NAME);
	else 
		sprintf(files.base_name, "%s%s.%s/", penv, (penv[strlen(penv)-1]== '/') ? "" : "/", PROGRAM_NAME);
	
	return 1;
}

/*function to get relevant details/password/uidl filename*/
char *get_account_file(char *fullpath, char *filename, int account)
{
	/*clear the buffer and create the full name from base_name and filename*/
	memset(fullpath, '\0', NAME_MAX);
	sprintf(fullpath, "%s%s", files.base_name, filename);
	
	/*if there is an account param add it to the filename*/
	if(account>= 0)
	{
		char account_string[4]= "    ";
		sprintf(account_string, "%d", account);
		strcat(fullpath, account_string);
	}
	return fullpath;
}

/* function to get details from config file into struct or if no config file exists ask for details*/
int read_config_file(void)
{
	char iconsize_string[4], configfilename[NAME_MAX];
	unsigned int icon_size= 0;
	char multiple= 0;
	FILE *pfile;

	/*get the full path of the config file*/
	get_account_file(configfilename, CONFIG_FILE, -1);
	
	/*if the file does not exist return*/
	if(access(configfilename, F_OK)== -1)
		return 0;

	/*open the config file for reading*/
	if((pfile= fopen(configfilename, "rt"))== NULL)
		error_and_log(S_FILEFUNC_ERR_OPEN_FILE, configfilename);

	/*memset(&config, '\0', sizeof(config_details));*/
	
	/*get the info from the file*/
	if(fgets(config.check_delay, DELAY_STRLEN, pfile)== NULL)
	{
		remove(configfilename);
		error_and_log(S_FILEFUNC_ERR_GET_DELAY);
	}
	if(fgets(config.mail_program, NAME_MAX, pfile)== NULL)
	{
		remove(configfilename);
		error_and_log(S_FILEFUNC_ERR_GET_MAILAPP);
	}
	/*set the default icon size for old versions of mailtc*/
	if(fgets(iconsize_string, 4, pfile)== NULL)
		strcpy(iconsize_string, "24");
	icon_size= atoi(iconsize_string);
	config.icon_size= icon_size;
	
	/*check the multiple value*/
	if((multiple= fgetc(pfile))== EOF)
		config.multiple= 0;

	config.multiple= atoi(&multiple);
	
	/*we need to move through the carriage return now*/
	if(fseek(pfile, ftell(pfile)+ 1, SEEK_SET)== -1)
		error_and_log(S_FILEFUNCT_ERR_MOVE_FP);
	
	/*Get the multiple icon colour*/
	if((fgets(config.icon, ICON_LEN, pfile)== NULL)|| (strlen(config.icon)== 1))
		strcpy(config.icon, "#FFFFFF");
	
	/*remove any extra carriage returns*/
	if((config.check_delay[strlen(config.check_delay)-1])== '\n')
		config.check_delay[strlen(config.check_delay)-1]= '\0';
	if((config.mail_program[strlen(config.mail_program)-1])== '\n')
		config.mail_program[strlen(config.mail_program)-1]= '\0';
	if((config.icon[strlen(config.icon)-1])== '\n')
		config.icon[strlen(config.icon)-1]= '\0';
	
	/*close the config file*/
	if(fclose(pfile)== EOF)
		error_and_log(S_FILEFUNC_ERR_CLOSE_FILE, configfilename);
	
	return 1;
}

/*Function to get a pointer to an account*/
mail_details **get_account(unsigned int item)
{
	mail_details **pfirst= &paccounts;
	/*iterate through them all until it is found*/
	while(*pfirst)
	{
		if((*pfirst)->id== item)
		return(pfirst);
		
		pfirst= &(*pfirst)->next;
	}
	return(NULL);
}

/*Function to create a new account and add to list*/
mail_details *create_account(mail_details **pfirst)
{
	/*allocate memory for new account and add it to the list*/
	mail_details *pnew= (mail_details *)alloc_mem(sizeof(mail_details), pnew);
	pnew->id= (*pfirst== NULL)? 0: (*pfirst)->id+ 1;
	pnew->next= *pfirst;
	pnew->pfilters= NULL;
	*pfirst= pnew;
	return(pnew);
}

/*Function to remove an account from the list*/
void remove_account(unsigned int item)
{
	mail_details **pfirst= &paccounts;
	
	/*flag used when shifting id's*/
	int freed= 0;
	
	/*iterate through the list and remove the one we want*/
	while(*pfirst)
	{
		if((*pfirst)->id== item)
		{
			mail_details *pnew= *pfirst;
			*pfirst= (*pfirst)->next;
			
			/*free the filter struct first if it exists*/
			if(pnew->pfilters)
			{
				free(pnew->pfilters);
				pnew->pfilters= NULL;
			}
			free(pnew);
			pnew= NULL;
			freed= 1;
		}
		else
			pfirst= &(*pfirst)->next;
	}

	pfirst= &paccounts;
	
	/*now shift id's*/
	if(freed)
		while((*pfirst)&& ((*pfirst)->id> item))
		{
			(*pfirst)->id--;
			pfirst= &(*pfirst)->next;
		}
			
}	

/*Function to free all the accounts*/
void free_accounts(void)
{
	mail_details **pfirst= &paccounts;
	while(*pfirst)
	{
		mail_details *pnew= *pfirst;
		*pfirst= (*pfirst)->next;
		
		/*free the filter struct first if it exists*/
		if(pnew->pfilters)
		{
			free(pnew->pfilters);
			pnew->pfilters= NULL;
		}
		
		free(pnew);
		pnew= NULL;
	}
}

/*Function to create/read in a new account*/
mail_details *read_account(mail_details **pfirst, FILE *pfile, const char *detailsfilename)
{
	
	char filterstring[PORT_LEN];
		
	/*allocate mem for new account and copy data to it*/
	mail_details *pnew= (mail_details *)alloc_mem(sizeof(mail_details), pnew);
		
	/*copy the id (0 if it is first account)*/
	pnew->id= (*pfirst== NULL)? 0: (*pfirst)->id+ 1;
	
	/*get the details from the file*/
	if(fgets(pnew->hostname, LOGIN_NAME_MAX+ HOST_NAME_MAX+ 1, pfile)== NULL)
	{
		remove(detailsfilename);
		error_and_log(S_FILEFUNC_ERR_GET_HOSTNAME, pnew->id);
	}
	if(fgets(pnew->port, PORT_LEN, pfile)== NULL)
	{	
		remove(detailsfilename);
		error_and_log(S_FILEFUNC_ERR_GET_PORT, pnew->id);
	}
	if(fgets(pnew->username, LOGIN_NAME_MAX+ HOST_NAME_MAX+ 1, pfile)== NULL)
	{
		remove(detailsfilename);
		error_and_log(S_FILEFUNC_ERR_GET_USERNAME, pnew->id);
	}
	if(fgets(pnew->icon, ICON_LEN, pfile)== NULL)
	{
		remove(detailsfilename);
		error_and_log(S_FILEFUNC_ERR_GET_ICONTYPE, pnew->id);
	}
	/*default to pop protocol for old versions of mailtc*/
	if(fgets(pnew->protocol, PROTOCOL_LEN, pfile)== NULL)
		strcpy(pnew->protocol, PROTOCOL_POP);
	
	/*default to account name for old versions of mailtc*/
	if(fgets(pnew->accname, NAME_MAX+ 1, pfile)== NULL)
		sprintf(pnew->accname, S_FILEFUNC_DEFAULT_ACCNAME, pnew->id+ 1);
	
	/*default to 0 for old versions of mailtc*/
	if(fgets(filterstring, PORT_LEN, pfile)!= NULL)
	{
		if(sscanf(filterstring, "%u", &(pnew->runfilter))!= 1)
			error_and_log(S_FILEFUNC_ERR_GET_FILTER, pnew->id);
	}
	else pnew->runfilter= 0;
		
	/*read in the password*/
	if(!read_password_from_file(pnew))
		error_and_log(S_FILEFUNC_ERR_GET_PASSWORD, pnew->id);
	
	pnew->pfilters= NULL;
			
	/*if filter flag is set, read in filter details*/
	/*if(pnew->runfilter)
	{*/
	if(!read_filter_info(pnew))
		pnew->runfilter= 0;
/*	}*/
	
	/*remove any carriage returns*/
	if((pnew->hostname[strlen(pnew->hostname)-1])== '\n')
		pnew->hostname[strlen(pnew->hostname)-1]= '\0';
	if((pnew->port[strlen(pnew->port)-1])== '\n')
		pnew->port[strlen(pnew->port)-1]= '\0';
	if((pnew->username[strlen(pnew->username)-1])== '\n')
		pnew->username[strlen(pnew->username)-1]= '\0';
	if((pnew->protocol[strlen(pnew->protocol)-1])== '\n')
		pnew->protocol[strlen(pnew->protocol)-1]= '\0';
	if((pnew->icon[strlen(pnew->icon)-1])== '\n')
		pnew->icon[strlen(pnew->icon)-1]= '\0';
	if((pnew->accname[strlen(pnew->accname)-1])== '\n')
		pnew->accname[strlen(pnew->accname)-1]= '\0';
	
	pnew->num_messages= -1;
	pnew->active= 0;
	
	/*next account points to last account*/
	pnew->next= *pfirst;
	*pfirst= pnew;
	return(pnew);
}

/*Function to read all accounts from the files and add to the list*/
int read_accounts(void)
{
	FILE* pfile;
	char detailsfilename[NAME_MAX];
	unsigned int i= 0;
	
	/*iterate through each account*/
	get_account_file(detailsfilename, DETAILS_FILE, i);
	while(access(detailsfilename, F_OK)!= -1)
	{
		
		/*open the details file for reading*/
		if((pfile= fopen(detailsfilename, "rt"))== NULL)
			error_and_log(S_FILEFUNC_ERR_OPEN_FILE, detailsfilename);

		paccounts= read_account(&paccounts, pfile, detailsfilename);
		
		/*close the details file*/
		if(fclose(pfile)== EOF)
			error_and_log(S_FILEFUNC_ERR_CLOSE_FILE, detailsfilename);
	
		get_account_file(detailsfilename, DETAILS_FILE, ++i);
		
	}
	return 1;
}

/*function to read in the password (encrypted or not) from the file*/
int read_password_from_file(mail_details *paccount)
{

	FILE* pfile;
	char passwordfilename[NAME_MAX];
#ifdef MTC_USE_SSL
	char encstring[1024];
	int len= 0;
#endif
	
	/*get the full path of the password file*/
	get_account_file(passwordfilename, PASSWORD_FILE, paccount->id);
	
	/*if the password file does not exist return*/
	if(access(passwordfilename, F_OK)== -1)
		return 0;
	
	/*open the password file for reading*/
	if((pfile= fopen(passwordfilename, "rb"))== NULL)
		error_and_log(S_FILEFUNC_ERR_OPEN_FILE, passwordfilename);

/*if OpenSSL is defined read in an encrypted password*/
#ifdef MTC_USE_SSL
	memset(encstring, '\0', 1024);
	
	/*read in the encrypted password and remove the password if there is an error reading it*/
	if((len= fread(encstring, 1, 1024, pfile))==0)
	{
		remove(passwordfilename);
		error_and_log(S_FILEFUNC_ERR_GET_PW);
	}
	
	/*decrypt the password*/
	decrypt_password(encstring, len, paccount->password);
	
/*otherwise read in clear password*/
#else

	/*get the password value and remove the file if it cannot be read*/
	if(fgets(paccount->password, PASSWORD_LEN, pfile)== NULL)
	{
		remove(passwordfilename);
		error_and_log(S_FILEFUNC_ERR_GET_PW);
	}
	/*remove the carriage returns*/
	if((paccount->password[strlen(paccount->password)-1])== '\n')
		paccount->password[strlen(paccount->password)-1]= '\0';
#endif
	
	/*close the password file*/
	if(fclose(pfile)== EOF)
		error_and_log(S_FILEFUNC_ERR_CLOSE_FILE, passwordfilename);
	
	return 1;
}

/*function to write pop config to config file*/
int write_config_file(void)
{
	FILE *pfile;
	char iconsize_string[3];
	char configfilename[NAME_MAX];

	/*get the full path of the config file*/
	get_account_file(configfilename, CONFIG_FILE, -1);
	
	/*if the file cannot be accessed or removed report error*/
	if((access(configfilename, F_OK)!= -1)&&(remove(configfilename)== -1))
		error_and_log(S_FILEFUNC_ERR_ATTEMPT_WRITE, configfilename);
	
	/*open the config file for writing*/
	if((pfile= fopen(configfilename, "wt"))== NULL)
		error_and_log(S_FILEFUNC_ERR_OPEN_FILE, configfilename);
	
	/*output the config information to the config file*/
	if(fputs(config.check_delay, pfile)== EOF||(fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_DELAY);
	
	if(fputs(config.mail_program, pfile)== EOF||(fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_MAILAPP);
		
	sprintf(iconsize_string, "%d", config.icon_size);
	if(fputs(iconsize_string, pfile)== EOF||(fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_ICONSIZE);
	
	if(fputs((config.multiple)? "1": "0", pfile)== EOF|| (fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_MULTIPLE);
	
	if(fputs(config.icon, pfile)== EOF|| (fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_M_ICON_COLOUR);
	
	fflush(pfile);

	/*close the config file*/
	if(fclose(pfile)== EOF)
		error_and_log_no_exit(S_FILEFUNC_ERR_CLOSE_FILE, configfilename);
	
	/*set the permissions on the file so it can only be read*/
	if(chmod(configfilename, S_IRUSR)== -1)
		error_and_log(S_FILEFUNC_ERR_SET_PERM, configfilename);
	
	return 1;
}

/*function to write pop details to details file*/
int write_user_details(mail_details **pcurrent)
{
	FILE *pfile;
	char detailsfilename[NAME_MAX];
	
	/*get the full path of the details file*/
	get_account_file(detailsfilename, DETAILS_FILE, (*pcurrent)->id);
	
	/*if it exists and cannot be removed report error*/
	if((access(detailsfilename, F_OK)!= -1)&&(remove(detailsfilename)== -1))
		error_and_log(S_FILEFUNC_ERR_ATTEMPT_WRITE, detailsfilename);
	
	/*open the details file for writing*/
	if((pfile= fopen(detailsfilename, "wt"))== NULL)
		error_and_log(S_FILEFUNC_ERR_OPEN_FILE, detailsfilename);
	
	/*output the details information to the details file*/
	if((fputs((*pcurrent)->hostname, pfile)== EOF)||(fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_HOSTNAME);
		
	if(fputs((*pcurrent)->port, pfile)== EOF||(fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_PORT);
		
	if(fputs((*pcurrent)->username, pfile)== EOF||(fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_USERNAME);
		
	if(fputs((*pcurrent)->icon, pfile)== EOF||(fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_ICONTYPE);
	
	if(fputs((*pcurrent)->protocol, pfile)== EOF||(fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_PROTOCOL);
	
	if(fputs((*pcurrent)->accname, pfile)== EOF||(fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_ACCNAME);
	
	if(fprintf(pfile, "%u\n", (*pcurrent)->runfilter)< 0)
		error_and_log(S_FILEFUNC_ERR_WRITE_FILTER);
	
	/*write the password to the password file*/
	write_password_to_file(*pcurrent);
		
	/*close the details file*/
	if(fclose(pfile)== EOF)
		error_and_log_no_exit(S_FILEFUNC_ERR_CLOSE_FILE, detailsfilename);
	
	/*change the permissions so the file can only be read*/
	if(chmod(detailsfilename, S_IRUSR)== -1)
		error_and_log(S_FILEFUNC_ERR_SET_PERM, detailsfilename);

	return 1;
}

/*function to write the password (encrypted or not) to the file*/
int write_password_to_file(mail_details *paccount)
{
	FILE* pfile;
	char passwordfilename[NAME_MAX];
#ifdef MTC_USE_SSL	
	char encstring[1024];
	size_t len= 0;
#endif

	/*get the full path of the password file*/
	get_account_file(passwordfilename, PASSWORD_FILE, paccount->id);
	
	/*if the password exists but cannot be removed report error*/
	if((access(passwordfilename, F_OK)!=-1)&&(remove(passwordfilename)== -1))
		error_and_log(S_FILEFUNC_ERR_ATTEMPT_WRITE, passwordfilename);
	
	/*open the password file for writing*/
	if((pfile= fopen(passwordfilename, "wb"))== NULL)
		error_and_log(S_FILEFUNC_ERR_OPEN_FILE, passwordfilename);

/*if OpenSSL is defined encrypt the password*/
#ifdef MTC_USE_SSL
	memset(encstring, '\0', 1024);
	len= encrypt_password(paccount->password, encstring);
	
	/*write the encrypted password*/
	if(fwrite(encstring, 1, len, pfile)< len)
		error_and_log(S_FILEFUNC_ERR_WRITE_PW);

/*otherwise write a clear password*/
#else
	if(fputs(paccount->password, pfile)== EOF||(fputc('\n', pfile)== EOF))
		error_and_log(S_FILEFUNC_ERR_WRITE_PW);
#endif

	/*close the password file*/
	if(fclose(pfile)== EOF)
		error_and_log(S_FILEFUNC_ERR_CLOSE_FILE, passwordfilename);
	
	/*change the permissions on the file so that it can only be read by user*/
	if(chmod(passwordfilename, S_IRUSR)== -1)
		error_and_log(S_FILEFUNC_ERR_SET_PERM, passwordfilename);

	return 1;
}

/*function to remove a file in ~/.PROGRAM_NAME and shift the files so they are in order*/
int remove_file(char *shortname, int count, int fullcount)
{
	int i= 0;

	/*allocate memory and get full path of the file to delete*/
	char full_filename[NAME_MAX];
	char new_filename[NAME_MAX];

	get_account_file(full_filename, shortname, count- 1);
	
	/*remove the file*/
	if((access(full_filename, F_OK)!= -1)&& (remove(full_filename)== -1))
		error_and_log(S_FILEFUNC_ERR_REMOVE_FILE, full_filename);

	/*traverse through each of the files after the removed one*/
	for(i= count; i< fullcount; i++)
	{
		/*get the name of the file and the file before it*/
		get_account_file(full_filename, shortname, i);
		get_account_file(new_filename, shortname, i- 1);
		
		/*rename file to file- 1 so that there are no gaps*/
		if((access(full_filename, F_OK)!= -1)&& (rename(full_filename, new_filename)== -1))
			error_and_log(S_FILEFUNC_ERR_RENAME_FILE, full_filename, new_filename);
	}
	return 1;
}

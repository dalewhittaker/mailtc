/* common.c
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


/*function to get the date and time*/
char *get_current_time(void)
{
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo= localtime(&rawtime);

	return(asctime(timeinfo));
}

/*by default, asctime() adds a line feed at the end of the string, so we remove it*/
static void print_time_string(void)
{
	char *ptimestring;

	ptimestring= get_current_time();
	if(ptimestring[strlen(ptimestring)- 1]== '\n')
		ptimestring[strlen(ptimestring)- 1]= '\0';
	
	fprintf(files.logfile, "%s: ", ptimestring);
}

/*run an error dialog reporting error*/
int run_error_dialog(char *errmsg, ...)
{
	GtkWidget *dialog;
	va_list list;
	char errstring[200];
	
	/*create a va_list and display it as a dialog*/
	va_start(list, errmsg); 
	vsprintf(errstring, errmsg, list);
	dialog= gtk_message_dialog_new(NULL, GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, errstring);
	
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	va_end(list);
	
	return 1;
}

/*function to report error, log it, and then exit*/
int error_and_log(char *errmsg, ...)
{
	/*create va_list of arguments*/
	va_list list;
	va_start(list, errmsg); 

	/*output to stderr and logfile*/
	vfprintf(stderr, errmsg, list);
	va_end(list);

	print_time_string();
	vfprintf(files.logfile, errmsg, list);
	
	fflush(stderr);
	fflush(files.logfile);
	
	exit(EXIT_FAILURE);

	return 0; /*shouldnt really ever happen*/
}

/*function to report error and log*/
int error_and_log_no_exit(char *errmsg, ...)
{
	/*create va_list of arguments*/
	va_list list;
	va_start(list, errmsg);
	
	/*output to stderr and logfile*/
	vfprintf(stderr, errmsg, list);
	print_time_string();
	vfprintf(files.logfile, errmsg, list);
	
	va_end(list);
	fflush(stderr);
	fflush(files.logfile);
	
	return 1;
}

/*function to allocate memory for a filename*/
void *alloc_mem(size_t size, void *pmem)
{
	/*allocate the memory and return the pointer if allocated successfully*/
	if((pmem= calloc(size, sizeof(char)))== NULL)
		error_and_log(S_COMMON_ERR_ALLOC);

	return pmem;
}

/*function to re-allocate memory*/
void *realloc_mem(size_t size, void *pmem)
{
	/*allocate the memory and return the pointer if allocated successfully*/
	if((pmem= realloc(pmem, size))== NULL)
		error_and_log(S_COMMON_ERR_REALLOC);

	return pmem;
}

/*function to append to a string*/
/*TODO this is actually only used in network stuff so will be moved to plugin dir*/
char *str_cat(char *dest, const char *source)
{
	size_t dlen= (dest== NULL)? 0: strlen(dest);
	size_t slen= (source== NULL)? 0: strlen(source);
	
	if(source== NULL)
		return(dest);
	
	dest= realloc_mem(slen+ dlen+ 1, dest);
	
	if(dlen== 0)
		memset(dest, '\0', slen+ dlen+ 1);

	dest= strcat(dest, source);
	dest[dlen+ slen]= '\0';

	return(dest);
}

/*function to copy to a string*/
char *str_cpy(char *dest, const char *source)
{
	size_t slen= (source== NULL)? 0: strlen(source);
	
	if((dest!= NULL)&& (source!= NULL)&& (strcmp(dest, source)== 0))
		return(dest);
	
	dest= realloc_mem(slen+ 1, dest);
	
	memset(dest, '\0', slen+ 1);
	return((slen== 0)? dest: strcpy(dest, source));

}

/*function to insert a string into another*/
char *str_ins(char *dest, const char *source, int pos)
{
	size_t slen= (source== NULL)? 0: strlen(source);
	size_t dlen= (dest== NULL)? 0: strlen(dest);
	int i= 0;
	
	if((slen== 0))
		return(dest);
	
	/*check it does not go out of range*/
	if((pos< 0)|| ((unsigned int) pos> (dlen- 1)))
		error_and_log(S_COMMON_ERR_STR_INS);
	
	/*reallocate the memory*/
	dest= realloc_mem(dlen+ slen+ 1, dest);
	
	/*shift the chars up*/
	for(i= (dlen- 1); i>= pos; i--)
		dest[i+ slen]= dest[i];
	
	/*insert our new chars*/
	for(i= 0; (unsigned int) i< slen; i++)
		dest[i+ pos]= source[i];
	
	/*add the terminator*/
	dest[dlen+ slen]= '\0';

	return(dest);
}

/*case insensitive search returns position, or -1*/
int str_case_search(char *haystack, char *needle)
{
	unsigned int i= 0;
	
	/*iterate though each haystack char*/
	for(i= 0; i< strlen(haystack); i++)
	{
		/*if char found to be same as first needle char*/
		if(tolower(haystack[i])== tolower(needle[0]))
		{
			unsigned int j= 0;
			/*compare needle chars with subsequent haystack chars*/
			for(j= 0; j< strlen(needle); j++)
			{
				if(tolower(haystack[i+ j])!= tolower(needle[j]))
					break;
			}	
			/*all chars found so success*/
			if(j== strlen(needle))
				return(i);
		}
	}
	return -1;
}

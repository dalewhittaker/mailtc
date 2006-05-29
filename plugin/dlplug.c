#include <stdio.h>
#include "plugin.h"

/*This MUST match the mailtc revision it is used with, if not, mailtc will report that it is an invalid plugin*/
#define MTC_REVISION 0.1
#define PLUGIN_NAME "Mailtc test plugin"
#define PLUGIN_AUTHOR "Dale Whittaker"
#define PLUGIN_DESC "Simple test plugin to show how its done"

int load(mail_details *paccount, const char *cfgdir, unsigned int flags)
{
	/*TODO test this with the config dir and flags*/
	printf("load plugin!\n");
	
	printf("config dir: %s\n", cfgdir);

	if(flags& MTC_USE_FILTERS)
		printf("enable filters\n");
	if(flags& MTC_DEBUG_MODE)
		printf("debug mode\n");
	
	return 1;
}

int unload(mail_details *paccount)
{
	printf("unload plugin!\n");
	return 1;
}

int clicked(mail_details *paccount)
{
	printf("plugin clicked!\n");
	return 1;
}

mtc_plugin_info pluginfo =
{
	MTC_REVISION,
	(const char *)PLUGIN_NAME,
	(const char *)PLUGIN_AUTHOR,
	(const char *)PLUGIN_DESC,
	/*NULL,*/
	load,
	unload,
	clicked
};


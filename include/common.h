/* common.h
 * Copyright (C) 2007 Dale Whittaker <dayul@users.sf.net>
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

#ifndef DW_MAILTC_COMMON
#define DW_MAILTC_COMMON

#include <string.h> /*memset, strlen*/
#include <errno.h> /*not sure if this is actually required*/

#include "plugin.h"
#include "strings.h"

/*global variables*/
mtc_cfg config;
GSList *acclist;
GSList *plglist;

/*common.c functions*/
gchar *str_time(void);
void msgbox_warn(gchar *msg, ...);
void msgbox_info(gchar *msg, ...);
void msgbox_err(gchar *msg, ...);
void msgbox_fatal(gchar *msg, ...);
void msgbox_init(void);
void msgbox_term(void);
mtc_icon *pixbuf_create(mtc_icon *picon);
mtc_icon *icon_create(mtc_icon *picon);

#define MSG_NOGTK(proc) { msgbox_term(); (proc); msgbox_init(); }

#endif /*DW_MAILTC_COMMON*/

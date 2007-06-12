/* docklet.h
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

#ifndef DW_MAILTC_DOCKLET
#define DW_MAILTC_DOCKLET

#include "common.h"

/*values used for the icon status*/
#define ACTIVE_ICON_NONE -1
#define ACTIVE_ICON_MULTI -2 

/*docklet.c functions*/
gboolean mail_thread(gpointer data);
#ifdef MTC_EGGTRAYICON
void docklet_clicked(GtkWidget *button, GdkEventButton *event);
void docklet_destroyed(GtkWidget *widget, gpointer data);
#else
void docklet_rclicked(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data);
void docklet_lclicked(GtkStatusIcon *status_icon, gpointer user_data);
#endif /*MTC_EGGTRAYICON*/

#endif /*DW_MAILTC_DOCKLET*/

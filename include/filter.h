/* filter.h
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

#ifndef DW_MAILTC_FILTER
#define DW_MAILTC_FILTER

#include <libxml/tree.h>

#include "plg_common.h"

/*filterdlg.c functions*/
/*TODO all these will be removed*/
/*gboolean filterdlg_run(mtc_account *paccount);*/
gboolean read_filters(xmlDocPtr doc, xmlNodePtr node, mtc_account *paccount);
gboolean filter_write(xmlNodePtr acc_node, mtc_account *paccount);
void free_filters(mtc_account *paccount);
GtkWidget *filter_table(mtc_account *paccount, gchar *plgname);

#endif /*DW_MAILTC_FILTER*/

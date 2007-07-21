/* msg.h
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

#ifndef DW_MAILTC_MSG
#define DW_MAILTC_MSG

#include "plg_common.h"

/*msg.c functions*/
/*gboolean print_msg_info(mtc_account *paccount);*/ /*just for debugging*/
GSList *msglist_add(mtc_account *paccount, gchar *uid, GString *header);
GSList *msglist_cleanup(mtc_account *paccount);
void msglist_reset(mtc_account *paccount);
gboolean msglist_verify(mtc_account *paccount);

#endif /*DW_MAILTC_MSG*/

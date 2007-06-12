/* imapfunc.h
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

#ifndef DW_MAILTC_IMAPFUNC
#define DW_MAILTC_IMAPFUNC

/*TODO*/
#include "plg_common.h"

/*imapfunc.c functions*/
mtc_error check_imap_mail(mtc_account *paccount, const mtc_cfg *pconfig);
mtc_error imap_read_mail(mtc_account *paccount, const mtc_cfg *pconfig);
#ifdef SSL_PLUGIN
mtc_error check_cramimap_mail(mtc_account *paccount, const mtc_cfg *pconfig);
mtc_error check_imapssl_mail(mtc_account *paccount, const mtc_cfg *pconfig);
#endif /*SSL_PLUGIN*/

#endif /*DW_MAILTC_IMAPFUNC*/

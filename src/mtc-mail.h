/* mtc-mail.h
 * Copyright (C) 2009-2012 Dale Whittaker <dayul@users.sf.net>
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

#ifndef __MAILTC_MAIL_H__
#define __MAILTC_MAIL_H__

#include "mtc.h"

#include "mtc-application.h" /* FIXME */

G_BEGIN_DECLS

/* FIXME
 *
 * This stuct is here only temporarily, to aid removal of mtc_config.
 */
typedef struct
{
    MailtcApplication* app;
    GObject* status_icon;
    gboolean locked;
    guint error_count;
} mtc_run_params;

guint
mailtc_run_main_loop (mtc_run_params* params);

G_END_DECLS

#endif /* __MAILTC_MAIL_H__ */


/* mtc-account.h
 * Copyright (C) 2009-2015 Dale Whittaker <dayul@users.sf.net>
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

#ifndef __MAILTC_ACCOUNT_H__
#define __MAILTC_ACCOUNT_H__

#include "mtc-colour.h"
#include "mtc-extension.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_ACCOUNT     (mailtc_account_get_type ())

G_DECLARE_FINAL_TYPE            (MailtcAccount, mailtc_account, MAILTC, ACCOUNT, GObject)

MailtcAccount*
mailtc_account_new              (void);

void
mailtc_account_set_name         (MailtcAccount*      account,
                                 const gchar*        name);

const gchar*
mailtc_account_get_name         (MailtcAccount*      account);

void
mailtc_account_set_server       (MailtcAccount*      account,
                                 const gchar*        server);

const gchar*
mailtc_account_get_server       (MailtcAccount*      account);

void
mailtc_account_set_port         (MailtcAccount*      account,
                                 guint               port);

guint
mailtc_account_get_port         (MailtcAccount*      account);

void
mailtc_account_set_user         (MailtcAccount*      account,
                                 const gchar*        user);

const gchar*
mailtc_account_get_user         (MailtcAccount*      account);

void
mailtc_account_set_password     (MailtcAccount*      account,
                                 const gchar*        password);

const gchar*
mailtc_account_get_password     (MailtcAccount*      account);

void
mailtc_account_set_protocol     (MailtcAccount*      account,
                                 guint               protocol);

guint
mailtc_account_get_protocol     (MailtcAccount*      account);

void
mailtc_account_set_iconcolour   (MailtcAccount*      account,
                                 const MailtcColour* iconcolour);

void
mailtc_account_get_iconcolour   (MailtcAccount*      account,
                                 MailtcColour*       iconcolour);

MailtcExtension*
mailtc_account_get_extension    (MailtcAccount*      account);

gboolean
mailtc_account_update_extension (MailtcAccount*      account,
                                 MailtcExtension*    extension,
                                 GError**            error);

G_END_DECLS

#endif /* __MAILTC_ACCOUNT_H__ */


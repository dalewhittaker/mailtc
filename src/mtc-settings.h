/* mtc-settings.h
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

#ifndef __MAILTC_SETTINGS_H__
#define __MAILTC_SETTINGS_H__

#include "mtc-colour.h"
#include "mtc-modulemanager.h"

#include <gio/gio.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_SETTINGS        (mailtc_settings_get_type ())

G_DECLARE_FINAL_TYPE                (MailtcSettings, mailtc_settings, MAILTC, SETTINGS, GObject)

void
mailtc_settings_set_command         (MailtcSettings*      settings,
                                     const gchar*         command);

const gchar*
mailtc_settings_get_command         (MailtcSettings*      settings);

void
mailtc_settings_set_interval        (MailtcSettings*      settings,
                                     guint                interval);

guint
mailtc_settings_get_interval        (MailtcSettings*      settings);

void
mailtc_settings_set_neterror        (MailtcSettings*      settings,
                                     guint                neterror);

guint
mailtc_settings_get_neterror        (MailtcSettings*      settings);

void
mailtc_settings_set_iconcolour      (MailtcSettings*      settings,
                                     const MailtcColour*  colour);

void
mailtc_settings_get_iconcolour      (MailtcSettings*      settings,
                                     MailtcColour*        colour);

void
mailtc_settings_set_erroriconcolour (MailtcSettings*      settings,
                                     const MailtcColour*  colour);

void
mailtc_settings_get_erroriconcolour (MailtcSettings*      settings,
                                     MailtcColour*        colour);

void
mailtc_settings_set_accounts        (MailtcSettings*      settings,
                                     GPtrArray*           accounts);

GPtrArray*
mailtc_settings_get_accounts        (MailtcSettings*      settings);

MailtcModuleManager*
mailtc_settings_get_modules         (MailtcSettings*      settings);

gboolean
mailtc_settings_write               (MailtcSettings*      settings,
                                     GError**             error);

MailtcSettings*
mailtc_settings_new                 (gchar*               filename,
                                     MailtcModuleManager* modules,
                                     GError**             error);

G_END_DECLS

#endif /* __MAILTC_SETTINGS_H__ */


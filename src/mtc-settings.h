/* mtc-settings.h
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

#ifndef __MAILTC_SETTINGS_H__
#define __MAILTC_SETTINGS_H__

#include <gdk/gdk.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_SETTINGS            (mailtc_settings_get_type  ())
#define MAILTC_SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_SETTINGS, MailtcSettings))
#define MAILTC_SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_SETTINGS, MailtcSettingsClass))
#define MAILTC_IS_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_SETTINGS))
#define MAILTC_IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_SETTINGS))
#define MAILTC_SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_SETTINGS, MailtcSettingsClass))

typedef struct _MailtcSettings        MailtcSettings;
typedef struct _MailtcSettingsClass   MailtcSettingsClass;
typedef struct _MailtcSettingsPrivate MailtcSettingsPrivate;

void
mailtc_settings_set_command    (MailtcSettings* settings,
                                const gchar*    command);

const gchar*
mailtc_settings_get_command    (MailtcSettings* settings);

void
mailtc_settings_set_interval   (MailtcSettings* settings,
                                guint           interval);

guint
mailtc_settings_get_interval   (MailtcSettings* settings);

void
mailtc_settings_set_neterror   (MailtcSettings* settings,
                                guint           neterror);

guint
mailtc_settings_get_neterror   (MailtcSettings* settings);

void
mailtc_settings_set_iconcolour (MailtcSettings* settings,
                                const GdkColor* colour);

void
mailtc_settings_get_iconcolour (MailtcSettings* settings,
                                GdkColor*       colour);

gboolean
mailtc_settings_write          (MailtcSettings* settings,
                                GError**        error);

GType
mailtc_settings_get_type       (void);

MailtcSettings*
mailtc_settings_new            (gchar*          filename,
                                GPtrArray*      modules,
                                GError**        error);

G_END_DECLS

#endif /* __MAILTC_SETTINGS_H__ */


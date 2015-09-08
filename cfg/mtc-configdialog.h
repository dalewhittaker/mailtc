/* mtc-configdialog.h
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

#ifndef __MAILTC_CONFIG_DIALOG_H__
#define __MAILTC_CONFIG_DIALOG_H__

#include <gtk/gtk.h>
#include "mtc-settings.h"

G_BEGIN_DECLS

#define MAILTC_TYPE_CONFIG_DIALOG (mailtc_config_dialog_get_type ())

G_DECLARE_FINAL_TYPE              (MailtcConfigDialog, mailtc_config_dialog, MAILTC, CONFIG_DIALOG, GtkDialog)

GtkWidget*
mailtc_config_dialog_new          (MailtcSettings* settings);

G_END_DECLS

#endif /* __MAILTC_CONFIG_DIALOG_H__ */


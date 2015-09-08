/* mtc-statusicon.h
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

#ifndef __MAILTC_STATUS_ICON_H__
#define __MAILTC_STATUS_ICON_H__

#include "mtc-colour.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_STATUS_ICON            (mailtc_status_icon_get_type ())
#define MAILTC_STATUS_ICON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_STATUS_ICON, MailtcStatusIcon))
#define MAILTC_STATUS_ICON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_STATUS_ICON, MailtcStatusIconClass))
#define MAILTC_IS_STATUS_ICON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_STATUS_ICON))
#define MAILTC_IS_STATUS_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_STATUS_ICON))
#define MAILTC_STATUS_ICON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_STATUS_ICON, MailtcStatusIconClass))

typedef struct _MailtcStatusIcon        MailtcStatusIcon;
typedef struct _MailtcStatusIconClass   MailtcStatusIconClass;

GType
mailtc_status_icon_get_type           (void);

void
mailtc_status_icon_set_default_colour (MailtcStatusIcon*   status_icon,
                                       const MailtcColour* colour);

void
mailtc_status_icon_set_error_colour   (MailtcStatusIcon*   status_icon,
                                       const MailtcColour* colour);

void
mailtc_status_icon_add_item           (MailtcStatusIcon*   status_icon,
                                       const gchar*        account_name,
                                       const MailtcColour* account_colour);

void
mailtc_status_icon_update             (MailtcStatusIcon*   status_icon,
                                       guint               id,
                                       gint64              nmails,
                                       gint64              ntries);

void
mailtc_status_icon_clear              (MailtcStatusIcon*   status_icon);

MailtcStatusIcon*
mailtc_status_icon_new                (void);

G_END_DECLS

#endif /* __MAILTC_STATUS_ICON_H__ */


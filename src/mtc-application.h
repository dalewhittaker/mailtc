/* mtc-application.h
 * Copyright (C) 2009-2022 Dale Whittaker
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

#ifndef __MAILTC_APPLICATION_H__
#define __MAILTC_APPLICATION_H__

#include "mtc-settings.h"

#include <gio/gio.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_APPLICATION            (mailtc_application_get_type ())
#define MAILTC_APPLICATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_APPLICATION, MailtcApplication))
#define MAILTC_APPLICATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_APPLICATION, MailtcApplicationClass))
#define MAILTC_IS_APPLICATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_APPLICATION))
#define MAILTC_IS_APPLICATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_APPLICATION))
#define MAILTC_APPLICATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_APPLICATION, MailtcApplicationClass))

typedef struct _MailtcApplication        MailtcApplication;
typedef struct _MailtcApplicationClass   MailtcApplicationClass;

GType
mailtc_application_get_type     (void);

MailtcApplication*
mailtc_application_new          (void);

void
mailtc_application_set_debug    (MailtcApplication* app,
                                 gboolean           debug);

gboolean
mailtc_application_get_debug    (MailtcApplication* app);

void
mailtc_application_set_settings (MailtcApplication* app,
                                 MailtcSettings*    settings);

MailtcSettings*
mailtc_application_get_settings (MailtcApplication* app);

G_END_DECLS

#endif /* __MAILTC_APPLICATION_H__ */


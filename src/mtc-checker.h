/* mtc-checker.h
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

#ifndef __MAILTC_CHECKER_H__
#define __MAILTC_CHECKER_H__

#include "mtc-statusicon.h"

#include <glib-object.h>

G_BEGIN_DECLS

#define MAILTC_TYPE_CHECKER            (mailtc_checker_get_type ())
#define MAILTC_CHECKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAILTC_TYPE_CHECKER, MailtcChecker))
#define MAILTC_CHECKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MAILTC_TYPE_CHECKER, MailtcCheckerClass))
#define MAILTC_IS_CHECKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAILTC_TYPE_CHECKER))
#define MAILTC_IS_CHECKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MAILTC_TYPE_CHECKER))
#define MAILTC_CHECKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MAILTC_TYPE_CHECKER, MailtcCheckerClass))

typedef struct _MailtcChecker        MailtcChecker;
typedef struct _MailtcCheckerClass   MailtcCheckerClass;
typedef struct _MailtcCheckerPrivate MailtcCheckerPrivate;

GType
mailtc_checker_get_type        (void);

MailtcChecker*
mailtc_checker_new             (guint timeout);

void
mailtc_checker_set_status_icon (MailtcChecker*    checker,
                                MailtcStatusIcon* statusicon);

MailtcStatusIcon*
mailtc_checker_get_status_icon (MailtcChecker* checker);


void
mailtc_checker_run             (MailtcChecker* checker);

G_END_DECLS

#endif /* __MAILTC_CHECKER_H__ */


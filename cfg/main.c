/* main.c
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
#include <config.h>

#include "mtc-configdialog.h"
#include "mtc-application.h"

static void
mailtc_run_cb (MailtcApplication* app)
{
    MailtcSettings* settings;
    GtkWidget* dialog;

    settings = mailtc_application_get_settings (app);
    dialog = mailtc_config_dialog_new (settings);

    g_object_unref (settings);
    gtk_widget_show (GTK_WIDGET (dialog));
}

int
main (void)
{
    MailtcApplication* app;
    int status;

    app = mailtc_application_new ();

    g_signal_connect (app, "run", G_CALLBACK (mailtc_run_cb), NULL);

    status = g_application_run (G_APPLICATION (app), 0, NULL);

    g_object_unref (app);

    return status;
}


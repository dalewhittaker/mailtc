/* test-pop.c
 * Copyright (C) 2009-2011 Dale Whittaker <dayul@users.sf.net>
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

#include <glib.h>

static void
test_pop1 (void)
{
    g_assert (1); /* FIXME */
}

static void
test_pop2 (void)
{
    g_assert (1); /* FIXME */
}

int
main (int    argc,
      char** argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/plugin/pop1", test_pop1);
    g_test_add_func ("/plugin/pop2", test_pop2);

    return g_test_run ();
}


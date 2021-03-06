/****************************************************************************
|
| Copyright (c) 2011 Novell, Inc.
| All Rights Reserved.
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of version 2 of the GNU General Public License as
| published by the Free Software Foundation.
|
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.   See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; if not, contact Novell, Inc.
|
| To contact Novell about this file by physical or electronic mail,
| you may find current contact information at www.novell.com
|
 \***************************************************************************/

#ifndef MYINTL_H
#define MYINTL_H


#include <libintl.h>
#include <qstring.h>


inline QString _(const char* msgid)
{
    return QString::fromUtf8 (gettext(msgid));
}

inline QString _(const char* msgid1, const char* msgid2, unsigned long int n)
{
    return QString::fromUtf8 (ngettext (msgid1, msgid2, n));
}

void set_textdomain (const char* domain);


#endif

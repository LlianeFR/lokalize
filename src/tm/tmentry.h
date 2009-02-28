/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2009 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of
  the License or (at your option) version 3 or any later version
  accepted by the membership of KDE e.V. (or its successor approved
  by the membership of KDE e.V.), which shall act as a proxy 
  defined in Section 14 of version 3 of the license.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.


**************************************************************************** */

#ifndef TMENTRY_H
#define TMENTRY_H

#include <QString>

namespace TM {

struct TMEntry
{
    QString english;
    QString target;

    QString date;

    //the remaining are used only for results
    qlonglong id;
    short score:16;//100.00%==10000
    ushort hits:16;

    QString diff;

    //different databases can have different settings:
    QString accel;
    QString markup;

    bool operator<(const TMEntry& other)const
    {
        //we wanna items with higher score to appear in the front after qSort
        if (score==other.score)
            return date>other.date;
        return score>other.score;
    }
};

}

#endif
/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#ifndef KCATEGORYDRAWER_H
#define KCATEGORYDRAWER_H

#include "kcategorizedsortfilterproxymodel.h"
#define KDEUI_EXPORT

class QPainter;
class QLinearGradient;
class QModelIndex;
class QStyleOption;

class KDEUI_EXPORT KCategoryDrawer
{
public:
    KCategoryDrawer();

    virtual ~KCategoryDrawer();

    /**
      * This method purpose is to draw a category represented by the given
      * @param index with the given @param sortRole sorting role
      *
      * @note This method will be called one time per category, always with the
      *       first element in that category
      */
    virtual void drawCategory(const QModelIndex &index,
                              int sortRole,
                              const QStyleOption &option,
                              QPainter *painter) const;

    virtual int categoryHeight(const QModelIndex &index, const QStyleOption &option) const;

    QLinearGradient linearG( const QStyleOption &option, const QColor col ) const;
};

#endif // KCATEGORYDRAWER_H

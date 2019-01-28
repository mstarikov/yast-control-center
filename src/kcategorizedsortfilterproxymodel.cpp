/**
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  * Copyright (C) 2007 John Tapsell <tapsell@kde.org>
  * Copyright (C) 2006 by Dominic Battre <dominic@battre.de>
  * Copyright (C) 2006 by Martin Pool <mbp@canonical.com>
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

#include "kcategorizedsortfilterproxymodel.h"
#include "yqdesktopfilesmodel.h"
#include "kcategorizedsortfilterproxymodel_p.h"

#include <limits.h>
#include <iostream>

#include <QItemSelection>
#include <QStringList>
#include <QSize>
#include <QDebug>

KCategorizedSortFilterProxyModel::KCategorizedSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new Private())

{
    setFilterCaseSensitivity( Qt::CaseInsensitive );
}

KCategorizedSortFilterProxyModel::~KCategorizedSortFilterProxyModel()
{
    delete d;
}

void KCategorizedSortFilterProxyModel::sort(int column, Qt::SortOrder order)
{
    d->sortColumn = column;
    d->sortOrder = order;

    QSortFilterProxyModel::sort(column, order);
}

bool KCategorizedSortFilterProxyModel::isCategorizedModel() const
{
    return d->categorizedModel;
}

void KCategorizedSortFilterProxyModel::setCategorizedModel(bool categorizedModel)
{
    if (categorizedModel == d->categorizedModel)
    {
        return;
    }

    d->categorizedModel = categorizedModel;

    invalidate();
}

int KCategorizedSortFilterProxyModel::sortColumn() const
{
    return d->sortColumn;
}

Qt::SortOrder KCategorizedSortFilterProxyModel::sortOrder() const
{
    return d->sortOrder;
}

void KCategorizedSortFilterProxyModel::setSortCategoriesByNaturalComparison(bool sortCategoriesByNaturalComparison)
{
    if (sortCategoriesByNaturalComparison == d->sortCategoriesByNaturalComparison)
    {
        return;
    }

    d->sortCategoriesByNaturalComparison = sortCategoriesByNaturalComparison;

    invalidate();
}

bool KCategorizedSortFilterProxyModel::sortCategoriesByNaturalComparison() const
{
    return d->sortCategoriesByNaturalComparison;
}

int KCategorizedSortFilterProxyModel::naturalCompare(const QString &a,
                                                     const QString &b)
{
    // This method chops the input a and b into pieces of
    // digits and non-digits (a1.05 becomes a | 1 | . | 05)
    // and compares these pieces of a and b to each other
    // (first with first, second with second, ...).
    //
    // This is based on the natural sort order code by Martin Pool
    // http://sourcefrog.net/projects/natsort/
    // Martin Pool agreed to license this under LGPL or GPL.

    const QChar* currA = a.unicode(); // iterator over a
    const QChar* currB = b.unicode(); // iterator over b

    if (currA == currB) {
        return 0;
    }

    const QChar* begSeqA = currA; // beginning of a new character sequence of a
    const QChar* begSeqB = currB;

    while (!currA->isNull() && !currB->isNull()) {
        if (currA->unicode() == QChar::ObjectReplacementCharacter) {
            return 1;
        }

        if (currB->unicode() == QChar::ObjectReplacementCharacter) {
            return -1;
        }

        if (currA->unicode() == QChar::ReplacementCharacter) {
            return 1;
        }

        if (currB->unicode() == QChar::ReplacementCharacter) {
            return -1;
        }

        // find sequence of characters ending at the first non-character
        while (!currA->isNull() && !currA->isDigit()) {
            ++currA;
        }

        while (!currB->isNull() && !currB->isDigit()) {
            ++currB;
        }

        // compare these sequences
        const QString subA(begSeqA, currA - begSeqA);
        const QString subB(begSeqB, currB - begSeqB);
        const int cmp = QString::localeAwareCompare(subA, subB);
        if (cmp != 0) {
            return cmp;
        }

        if (currA->isNull() || currB->isNull()) {
            break;
        }

        // now some digits follow...
        if ((*currA == '0') || (*currB == '0')) {
            // one digit-sequence starts with 0 -> assume we are in a fraction part
            // do left aligned comparison (numbers are considered left aligned)
            while (1) {
                if (!currA->isDigit() && !currB->isDigit()) {
                    break;
                } else if (!currA->isDigit()) {
                    return -1;
                } else if (!currB->isDigit()) {
                    return + 1;
                } else if (*currA < *currB) {
                    return -1;
                } else if (*currA > *currB) {
                    return + 1;
                }
                ++currA;
                ++currB;
            }
        } else {
            // No digit-sequence starts with 0 -> assume we are looking at some integer
            // do right aligned comparison.
            //
            // The longest run of digits wins. That aside, the greatest
            // value wins, but we can't know that it will until we've scanned
            // both numbers to know that they have the same magnitude.

            int weight = 0;
            while (1) {
                if (!currA->isDigit() && !currB->isDigit()) {
                    if (weight != 0) {
                        return weight;
                    }
                    break;
                } else if (!currA->isDigit()) {
                    return -1;
                } else if (!currB->isDigit()) {
                    return + 1;
                } else if ((*currA < *currB) && (weight == 0)) {
                    weight = -1;
                } else if ((*currA > *currB) && (weight == 0)) {
                    weight = + 1;
                }
                ++currA;
                ++currB;
            }
        }

        begSeqA = currA;
        begSeqB = currB;
    }

    if (currA->isNull() && currB->isNull()) {
        return 0;
    }

    return currA->isNull() ? -1 : + 1;
}

bool KCategorizedSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (d->categorizedModel)
    {
        int compare = compareCategories(left, right);

        if (compare > 0) // left is greater than right
        {
            return false;
        }
        else if (compare < 0) // left is less than right
        {
            return true;
        }
    }

    return subSortLessThan(left, right);
}

bool KCategorizedSortFilterProxyModel::subSortLessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QModelIndex i1 = createIndex(left.row(), YQDesktopFilesModel::Name);
    QModelIndex i2 = createIndex(right.row(), YQDesktopFilesModel::Name);

    QVariant l = (left.model() ? left.model()->data(i1, Qt::UserRole) : QVariant());
    QVariant r = (right.model() ? right.model()->data(i2, Qt::UserRole) : QVariant());

    return l.toString() < r.toString();
}

int KCategorizedSortFilterProxyModel::compareCategories(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant l = (left.model() ? left.model()->data(left, CategorySortRole) : QVariant());
    QVariant r = (right.model() ? right.model()->data(right, CategorySortRole) : QVariant());

    QVariant s1 = (left.model() ? left.model()->data( left, CategoryDisplayRole ) : QVariant());
    QVariant s2 = (right.model() ? right.model()->data( right, CategoryDisplayRole ) : QVariant());

    Q_ASSERT(l.isValid());
    Q_ASSERT(r.isValid());
    Q_ASSERT(s1.isValid());
    Q_ASSERT(s2.isValid());
    Q_ASSERT(l.type() == r.type());
    Q_ASSERT(s1.type() == s2.type());

    qlonglong lint = l.toLongLong();
    qlonglong rint = r.toLongLong();

    if (lint < rint)
    {
        return -1;
    }

    if (lint > rint)
    {
        return 1;
    }

    if (lint == rint)
    {

        QString lstr = s1.toString();
        QString rstr = s2.toString();

        if (d->sortCategoriesByNaturalComparison)
        {
            return naturalCompare(lstr, rstr);
        }
        else
        {
            if (lstr < rstr)
            {
                return -1;
            }

            if (lstr > rstr)
            {
                return 1;
            }

            return 0;
        }
    }

    return 0;
}

bool KCategorizedSortFilterProxyModel::filterAcceptsRow( int row, const QModelIndex &srcindex ) const
{
    QModelIndex i0 = sourceModel()->index( row, YQDesktopFilesModel::Group, srcindex);
    QModelIndex i1 = sourceModel()->index( row, 0, srcindex);
    QStringList keywordList = sourceModel()->data( i1, KeywordsRole ).toStringList();
    QString keywords = keywordList.join(" ");

    bool nameMatches = QSortFilterProxyModel::filterAcceptsRow( row, srcindex ); 
    bool keywordMatches = ( !keywords.isEmpty() && keywords.contains( filterFixedString()) );

    if( nameMatches || keywordMatches )
    {
        QString gr = sourceModel()->data(i0, Qt::UserRole).toString();
	d->filterGroups.insert( gr );
    }

    return ( nameMatches || keywordMatches );
}

void KCategorizedSortFilterProxyModel::customFilterFunction( const QString &s )
{    
    d->filterGroups.clear();
    setFilterFixedString( s );
    d->filterString = s; 
    invalidateFilter();
    invalidate();
}

QString KCategorizedSortFilterProxyModel::matchingGroupFilterRegexp()
{
    QStringList slist = d->filterGroups.toList();

    if ( !slist.isEmpty() )
	return slist.join("|");
    else
	// dumb constant, make sure nothing matches if the list of matching groups is empty
	return QString("zzzz");
}

QString KCategorizedSortFilterProxyModel::filterFixedString()  const
{
    return d->filterString;
}

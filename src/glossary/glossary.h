/* ****************************************************************************
  This file is part of KAider

  Copyright (C) 2007 by Nick Shaforostoff <shafff@ukr.net>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  In addition, as a special exception, the copyright holders give
  permission to link the code of this program with any edition of
  the Qt library by Trolltech AS, Norway (or with modified versions
  of Qt that use the same license as Qt), and distribute linked
  combinations including the two.  You must obey the GNU General
  Public License in all respects for all of the code used other than
  Qt. If you modify this file, you may extend this exception to
  your version of the file, but you are not obligated to do so.  If
  you do not wish to do so, delete this exception statement from
  your version.

**************************************************************************** */

#ifndef GLOSSARY_H
#define GLOSSARY_H

#include <QStringList>
#include <QMultiHash>
#include <QAbstractItemModel>
#include <QList>

/**
 * struct that contains types data we work with.
 * this data can also be added to the TBX file
 *
 * the entry represents term, not word(s),
 * so there can be only one subjectField.
 */
struct TermEntry
{
    QStringList english;
    QStringList target;
    QString definition;
    int subjectField; //index in global Glossary's subjectFields list
    QString id;       //used to identify entry on edit action
    //TODO <descrip type="context"></descrip>

    TermEntry(const QStringList& _english,
              const QStringList& _target,
              const QString& _definition,
              int _subjectField,
              const QString& _id=QString()
             )
    : english(_english)
    , target(_target)
    , definition(_definition)
    , subjectField(_subjectField)
    , id(_id)
    {}

    TermEntry()
    : subjectField(0)
    {}

    void clear()
    {
        english.clear();
        target.clear();
        definition.clear();
        subjectField=0;
    }
};



/**
 * internal representation of glossary.
 * we store only data we need (i.e. only subset of TBX format)
 */
class Glossary: public QObject
{
    Q_OBJECT

public:
    QMultiHash<QString,int> wordHash;//isn't used anymore
    QList<TermEntry> termList;
    QStringList subjectFields;//frist entry is always empty!

    QString path;

    //for delayed saving
    QStringList addedIds;
    QStringList changedIds;
    QStringList removedIds;

    Glossary(QObject* parent)
     : QObject(parent)
     , subjectFields(QStringList(QLatin1String("")))
    {}

    ~Glossary()
    {}

    void clear()
    {
        wordHash.clear();
        termList.clear();
        subjectFields=QStringList(QLatin1String(""));
//        path.clear();
        changedIds.clear();
        removedIds.clear();
        addedIds.clear();
    }

    //saving to disk
    void load(const QString&);
    void save();

    void add(const TermEntry&);
    void change(const TermEntry&);

    //in-memory changing
    QString generateNewId();
    void append(const QString& _english,const QString& _target);
    void remove(int i);


    void forceChangeSignal(){emit changed();}
signals:
    void changed();
};



/**
 *	@author Nick Shaforostoff <shafff@ukr.net>
 */
class GlossaryModel: public QAbstractItemModel
{
    //Q_OBJECT
public:

    enum GlossaryModelColumns
    {
//         ID = 0,
        English=0,
        Target,
        SubjectField,
        GlossaryModelColumnCount
    };

    GlossaryModel(QObject* parent/*, Glossary* glossary*/);
    ~GlossaryModel();

    QModelIndex index (int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent(const QModelIndex&) const;
    int rowCount(const QModelIndex& parent=QModelIndex()) const;
    int columnCount(const QModelIndex& parent=QModelIndex()) const;
    QVariant data(const QModelIndex&,int role=Qt::DisplayRole) const;
    QVariant headerData(int section,Qt::Orientation, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags(const QModelIndex&) const;

    bool removeRows(int row,int count,const QModelIndex& parent=QModelIndex());
    //bool insertRows(int row,int count,const QModelIndex& parent=QModelIndex());
    bool appendRow(const QString& _english,const QString& _target);
    void forceReset();

// private:
//     Glossary* m_glossary;
//^ we take it from Project::instance()->glossary()
};




inline
GlossaryModel::GlossaryModel(QObject* parent)
 : QAbstractItemModel(parent)
{
}

inline
GlossaryModel::~GlossaryModel()
{
}

inline
QModelIndex GlossaryModel::index (int row,int column,const QModelIndex& /*parent*/) const
{
    return createIndex (row, column);
}

inline
QModelIndex GlossaryModel::parent(const QModelIndex& /*index*/) const
{
    return QModelIndex();
}

inline
int GlossaryModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return GlossaryModelColumnCount;
//     if (parent==QModelIndex())
//         return CatalogModelColumnCount;
//     return 0;
}

inline
Qt::ItemFlags GlossaryModel::flags ( const QModelIndex & index ) const
{
/*    if (index.column()==FuzzyFlag)
        return Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled;*/
    return QAbstractItemModel::flags(index);
}

inline
void GlossaryModel::forceReset()
{
    emit reset();
}

#endif

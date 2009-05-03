/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>
  Copyright (C) 2009 by Viesturs Zarins <viesturs.zarins@mii.lu.lv>

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

#include "projectmodel.h"
#include "project.h"

#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/Thread.h>

#include <kio/netaccess.h>
#include <klocale.h>
#include <kapplication.h>

#include <QTime>
#include <QFile>
#include <QtAlgorithms>

#undef KDE_NO_DEBUG_OUTPUT
static int nodeCounter=0;

ProjectModel::ProjectModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_poModel(this)
    , m_potModel(this)
    , m_rootNode(ProjectNode(NULL, -1, -1, -1))
    , m_dirIcon(KIcon(QLatin1String("inode-directory")))
    , m_poIcon(KIcon(QLatin1String("flag-blue")))
    , m_poComplIcon(KIcon(QLatin1String("flag-green")))
    , m_potIcon(KIcon(QLatin1String("flag-black")))
    , m_activeJob(NULL)
    , m_activeNode(NULL)
{

    m_poModel.dirLister()->setAutoUpdate(true);
    m_poModel.dirLister()->setAutoErrorHandlingEnabled(false, NULL);
    m_poModel.dirLister()->setNameFilter("*.po *.pot *.xlf");

    m_potModel.dirLister()->setAutoUpdate(true);
    m_potModel.dirLister()->setAutoErrorHandlingEnabled(false, NULL);
    m_potModel.dirLister()->setNameFilter("*.pot");

    connect(&m_poModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(po_dataChanged(QModelIndex,QModelIndex)));

    connect(&m_poModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(po_rowsInserted(QModelIndex,int,int)));

    connect(&m_poModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(po_rowsRemoved(QModelIndex,int,int)));

    connect(&m_potModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(pot_dataChanged(QModelIndex,QModelIndex)));

    connect(&m_potModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(pot_rowsInserted(QModelIndex,int,int)));

    connect(&m_potModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(pot_rowsRemoved(QModelIndex,int,int)));

    setUrl(KUrl(), KUrl());
}


ProjectModel::~ProjectModel()
{
    m_dirsWaitingForMetadata.clear();

    if (m_activeJob != NULL)
        m_activeJob->setStatus(-2);

    m_activeJob = NULL;
}

void ProjectModel::setUrl(const KUrl &poUrl, const KUrl &potUrl)
{
    //kDebug() << "ProjectModel::openUrl("<< poUrl.pathOrUrl() << +", " << potUrl.pathOrUrl() << ")";

    //cleanup old data

    m_dirsWaitingForMetadata.clear();

    if (m_activeJob != NULL)
        m_activeJob->setStatus(-1);
    m_activeJob = NULL;

    if (m_rootNode.rows.count())
    {
        beginRemoveRows(QModelIndex(), 0, m_rootNode.rows.count());

        for (int pos = 0; pos < m_rootNode.rows.count(); pos ++)
            deleteSubtree(m_rootNode.rows.at(pos));
        m_rootNode.rows.clear();
        m_rootNode.poCount = 0;
        m_rootNode.translated = -1;
        m_rootNode.untranslated = -1;
        m_rootNode.fuzzy = -1;

        endRemoveRows();
    }

    //add trailing slashes to base URLs, needed for potToPo and poToPot
    m_poUrl = poUrl;
    m_potUrl = potUrl;
    m_poUrl.adjustPath(KUrl::AddTrailingSlash);
    m_potUrl.adjustPath(KUrl::AddTrailingSlash);

    if (!poUrl.isEmpty())
        m_poModel.dirLister()->openUrl(m_poUrl, KDirLister::Reload);
    if (!potUrl.isEmpty())
        m_potModel.dirLister()->openUrl(m_potUrl, KDirLister::Reload);
}


KUrl ProjectModel::beginEditing(const QModelIndex& index)
{
    Q_ASSERT(index.isValid());

    QModelIndex poIndex = poIndexForOuter(index);
    QModelIndex potIndex = potIndexForOuter(index);

    if (poIndex.isValid())
    {
        KFileItem item = m_poModel.itemForIndex(poIndex);
        return item.url();
    }
    else if (potIndex.isValid())
    {
        //TODO don't
        //copy over the file
        KUrl potFile = m_potModel.itemForIndex(potIndex).url();
        KUrl poFile = potToPo(potFile);

        //be careful, copy only if file does not exist already.
        if (!KIO::NetAccess::exists(poFile, KIO::NetAccess::DestinationSide, NULL))
            KIO::NetAccess::file_copy(potFile, poFile);

        return poFile;
    }
    else
    {
        Q_ASSERT(false);
        return KUrl();
    }
}

//Theese methds update the combined model from POT and PO model changes.
//Quite complex stuff here, better do not change anything.

void ProjectModel::po_dataChanged(const QModelIndex& po_topLeft, const QModelIndex& po_bottomRight)
{
    //nothing special here
    //map from source and propogate
    QModelIndex topLeft = indexForPoIndex(po_topLeft);
    QModelIndex bottomRight = indexForPoIndex(po_bottomRight);

    emit dataChanged(topLeft, bottomRight);

    enqueueNodeForMetadataUpdate(nodeForIndex(topLeft.parent()));
}

void ProjectModel::pot_dataChanged(const QModelIndex& pot_topLeft, const QModelIndex& pot_bottomRight)
{
    //tricky here - some of the pot items may be represented by po items
    //let's propogate that all subitems changed


    QModelIndex pot_parent = pot_topLeft.parent();
    QModelIndex parent = indexForPotIndex(pot_parent);

    ProjectNode* node = nodeForIndex(parent);
    int count = node->rows.count();

    QModelIndex topLeft = index(0, pot_topLeft.column(), parent);
    QModelIndex bottomRight = index(count-1, pot_bottomRight.column(), parent);

    emit dataChanged(topLeft, bottomRight);

    enqueueNodeForMetadataUpdate(nodeForIndex(topLeft.parent()));
}


void ProjectModel::po_rowsInserted(const QModelIndex& po_parent, int first, int last)
{
    QModelIndex parent = indexForPoIndex(po_parent);
    QModelIndex pot_parent = potIndexForOuter(parent);
    ProjectNode* node = nodeForIndex(parent);

    //insert po rows
    beginInsertRows(parent, first, last);

    for (int pos = first; pos <= last; pos ++)
    {
        ProjectNode * childNode = new ProjectNode(node, pos, pos, -1);
        node->rows.insert(pos, childNode);
    }

    node->poCount += last - first + 1;

    //update rowNumber
    for (int pos = last + 1; pos < node->rows.count(); pos++)
        node->rows[pos]->rowNumber = pos;

    endInsertRows();

    //remove unneeded pot rows, update PO rows
    if (pot_parent.isValid() || !parent.isValid())
    {
        QVector<int> pot2PoMapping;
        generatePOTMapping(pot2PoMapping, po_parent, pot_parent);

        for (int pos = node->poCount; pos < node->rows.count(); pos ++)
        {
            ProjectNode* potNode = node->rows.at(pos);
            int potIndex = potNode->potRowNumber;
            int poIndex = pot2PoMapping[potIndex];

            if (poIndex != -1)
            {
                //found pot node, that now has a PO index.
                //remove the pot node and change the corresponding PO node
                beginRemoveRows(parent, pos, pos);
                node->rows.removeAt(pos);
                deleteSubtree(potNode);
                endRemoveRows();

                node->rows[poIndex]->potRowNumber = potIndex;
                //This change does not need notification
                //dataChanged(index(poIndex, 0, parent), index(poIndex, ProjectModelColumnCount, parent));

                pos--;
            }
        }
    }

    enqueueNodeForMetadataUpdate(node);
}


void ProjectModel::pot_rowsInserted(const QModelIndex& pot_parent, int start, int end)
{
    QModelIndex parent = indexForPotIndex(pot_parent);
    QModelIndex po_parent = poIndexForOuter(parent);
    ProjectNode* node = nodeForIndex(parent);

    int insertedCount = end + 1 - start;
    QVector<int> newPotNodes;

    if (po_parent.isValid() || !parent.isValid())
    {
        //this node containts mixed items - add and merge the stuff

        QVector<int> pot2PoMapping;
        generatePOTMapping(pot2PoMapping, po_parent, pot_parent);

        //reassign affected PO row POT indices
        for (int pos = 0; pos < node->poCount;pos ++)
        {
            if (node->rows[pos]->potRowNumber >= start)
                node->rows[pos]->potRowNumber += insertedCount;
        }

        //assign new POT indices
        for (int potIndex = start; potIndex <= end; potIndex ++)
        {
            int poIndex = pot2PoMapping[potIndex];
            if (poIndex != -1)
            {
                //found pot node, that has a PO index.
                //change the corresponding PO node
                node->rows[poIndex]->potRowNumber = potIndex;
                //This change does not need notification
                //dataChanged(index(poIndex, 0, parent), index(poIndex, ProjectModelColumnCount, parent));
            }
            else
                newPotNodes.append(potIndex);
        }
    }
    else
    {
        for (int pos = start; pos < end; pos ++)
            newPotNodes.append(pos);
    }

    //insert standalone POT rows, preserving POT order

    int newNodesCount = newPotNodes.count();
    if (newNodesCount)
    {
        int insertionPoint = node->poCount;
        while ((insertionPoint < node->rows.count()) && (node->rows[insertionPoint]->potRowNumber < start))
            insertionPoint++;

        beginInsertRows(parent, insertionPoint, insertionPoint + newNodesCount - 1);

        for (int pos = 0; pos < newNodesCount; pos ++)
        {
            int potIndex = newPotNodes.at(pos);
            ProjectNode * childNode = new ProjectNode(node, insertionPoint, -1, potIndex);
            node->rows.insert(insertionPoint, childNode);
            insertionPoint++;
        }

        //renumber remaining POT rows
        for (int pos = insertionPoint; pos < node->rows.count(); pos ++)
        {
            node->rows[pos]->rowNumber = pos;
            node->rows[pos]->potRowNumber += insertedCount;
        }

        endInsertRows();
    }

    enqueueNodeForMetadataUpdate(node);
}

void ProjectModel::po_rowsRemoved(const QModelIndex& po_parent, int start, int end)
{
    QModelIndex parent = indexForPoIndex(po_parent);
    QModelIndex pot_parent = potIndexForOuter(parent);
    ProjectNode* node = nodeForIndex(parent);
    int removedCount = end + 1 - start;

    if ((!parent.isValid()) && (node->rows.count() == 0))
    {
        //events after removing entire contents
        return;
    }

    //remove PO rows
    QList<int> potRowsToInsert;

    beginRemoveRows(parent, start, end);

    for (int pos = end; pos >= start; pos --)
    {
        int potIndex = node->rows.at(pos)->potRowNumber;
        deleteSubtree(node->rows.at(pos));
        node->rows.removeAt(pos);

        if (potIndex != -1)
            potRowsToInsert.append(potIndex);
    }

    //renumber other rows
    for (int pos = end + 1; pos < node->rows.count(); pos ++)
    {
        ProjectNode* childNode = node->rows.at(pos);
        childNode->rowNumber = pos;

        if (childNode->poRowNumber > end)
            node->rows[pos]->poRowNumber -= removedCount;
    }

    node->poCount -= removedCount;

    endRemoveRows();

    //add back POT rows, preserving row order
    qSort(potRowsToInsert.begin(), potRowsToInsert.end());

    int insertionPoint = node->poCount;

    for (int pos = 0; pos < potRowsToInsert.count(); pos ++)
    {
        int potIndex = potRowsToInsert.at(pos);
        while (insertionPoint < node->rows.count() && node->rows[insertionPoint]->potRowNumber < potIndex)
        {
            node->rows[insertionPoint]->rowNumber = insertionPoint;
            insertionPoint ++;
        }

        beginInsertRows(parent, insertionPoint, insertionPoint);

        ProjectNode * childNode = new ProjectNode(node, insertionPoint, -1, potIndex);
        node->rows.insert(insertionPoint, childNode);
        insertionPoint++;
        endInsertRows();
    }

    //renumber remaining rows
    while (insertionPoint < node->rows.count())
    {
        node->rows[insertionPoint]->rowNumber = insertionPoint;
        insertionPoint++;
    }

    enqueueNodeForMetadataUpdate(node);
}


void ProjectModel::pot_rowsRemoved(const QModelIndex& pot_parent, int start, int end)
{
    QModelIndex parent = indexForPotIndex(pot_parent);
    QModelIndex po_parent = poIndexForOuter(parent);
    ProjectNode * node = nodeForIndex(parent);
    int removedCount = end + 1 - start;

    if ((!parent.isValid()) && (node->rows.count() == 0))
    {
        //events after removing entire contents
        return;
    }

    //First remove POT nodes

    int firstPOTToRemove = node->poCount;
    int lastPOTToRemove = node->rows.count() - 1;

    while (firstPOTToRemove <= lastPOTToRemove && node->rows[firstPOTToRemove]->potRowNumber < start)
        firstPOTToRemove ++;
    while (lastPOTToRemove >= firstPOTToRemove && node->rows[lastPOTToRemove]->potRowNumber > end)
        lastPOTToRemove --;

    if (firstPOTToRemove <= lastPOTToRemove)
    {
        beginRemoveRows(parent, firstPOTToRemove, lastPOTToRemove);

        for (int pos = lastPOTToRemove; pos >= firstPOTToRemove; pos --)
        {
            ProjectNode* childNode = node->rows.at(pos);
            Q_ASSERT(childNode->potRowNumber >= start);
            Q_ASSERT(childNode->potRowNumber <= end);
            deleteSubtree(childNode);
            node->rows.removeAt(pos);
        }

        //renumber remaining rows
        for (int pos = firstPOTToRemove; pos < node->rows.count(); pos ++)
        {
            node->rows[pos]->rowNumber = pos;
            node->rows[pos]->potRowNumber -= removedCount;
        }

        endRemoveRows();
    }

    //now remove POT indices form PO rows

    if (po_parent.isValid() || !parent.isValid())
    {
        for (int poIndex = 0; poIndex < node->poCount; poIndex ++)
        {
            ProjectNode * childNode = node->rows[poIndex];
            int potIndex = childNode->potRowNumber;

            if (potIndex >= start && potIndex <= end)
            {
                //found PO node, that has a POT index in range.
                //change the corresponding PO node
                node->rows[poIndex]->potRowNumber = -1;
                //this change does not affect the model
                //dataChanged(index(poIndex, 0, parent), index(poIndex, ProjectModelColumnCount, parent));
            }
            else if (childNode->potRowNumber > end)
            {
                //reassign POT indices
                childNode->potRowNumber -= removedCount;
            }
        }
    }

    enqueueNodeForMetadataUpdate(node);
}


int ProjectModel::columnCount(const QModelIndex& /*parent*/)const
{
    return ProjectModelColumnCount;
}


QVariant ProjectModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
        case FileName:          return i18nc("@title:column File name","Name");
        case Graph:             return i18nc("@title:column Graphical representation of Translated/Fuzzy/Untranslated counts","Graph");
        case TotalCount:        return i18nc("@title:column Number of entries","Total");
        case TranslatedCount:   return i18nc("@title:column Number of entries","Translated");
        case FuzzyCount:        return i18nc("@title:column Number of entries","Not ready");
        case UntranslatedCount: return i18nc("@title:column Number of entries","Untranslated");
        case TranslationDate:   return i18nc("@title:column","Last Translation");
        case SourceDate:        return i18nc("@title:column","Template Revision");
        case LastTranslator:    return i18nc("@title:column","Last Translator");
        default:                return QVariant();
    }
}


Qt::ItemFlags ProjectModel::flags( const QModelIndex & index ) const
{
    if (index.column() == FileName)
        return Qt::ItemIsSelectable|Qt::ItemIsEnabled;
    else
        return Qt::ItemIsSelectable;
}


int ProjectModel::rowCount ( const QModelIndex & parent /*= QModelIndex()*/ ) const
{
    return nodeForIndex(parent)->rows.size();
}


bool ProjectModel::hasChildren ( const QModelIndex & parent /*= QModelIndex()*/ ) const
{
    if (!parent.isValid())
        return true;

    QModelIndex poIndex = poIndexForOuter(parent);
    QModelIndex potIndex = potIndexForOuter(parent);

    return ((poIndex.isValid() && m_poModel.hasChildren(poIndex)) ||
            (potIndex.isValid() && m_potModel.hasChildren(potIndex)));
}

bool ProjectModel::canFetchMore ( const QModelIndex & parent ) const
{
    if (!parent.isValid())
        return m_poModel.canFetchMore(QModelIndex()) || m_potModel.canFetchMore(QModelIndex());

    QModelIndex poIndex = poIndexForOuter(parent);
    QModelIndex potIndex = potIndexForOuter(parent);

    return ((poIndex.isValid() && m_poModel.canFetchMore(poIndex)) ||
            (potIndex.isValid() && m_potModel.canFetchMore(potIndex)));
}

void ProjectModel::fetchMore ( const QModelIndex & parent )
{
    if (!parent.isValid())
    {
        if (m_poModel.canFetchMore(QModelIndex()))
            m_poModel.fetchMore(QModelIndex());

        if (m_potModel.canFetchMore(QModelIndex()))
            m_potModel.fetchMore(QModelIndex());
    }
    else
    {
        QModelIndex poIndex = poIndexForOuter(parent);
        QModelIndex potIndex = potIndexForOuter(parent);

        if (poIndex.isValid() && (m_poModel.canFetchMore(poIndex)))
            m_poModel.fetchMore(poIndex);

        if (potIndex.isValid() && (m_potModel.canFetchMore(potIndex)))
            m_potModel.fetchMore(potIndex);
    }
}



/**
 * we use QRect to pass data through QVariant tunnel
 *
 * order is tran,  untr, fuzzy
 *          left() top() width()
 *
 */
QVariant ProjectModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const ProjectModelColumns& column=(ProjectModelColumns)index.column();
    ProjectNode * node = nodeForIndex(index);
    QModelIndex internalIndex = poOrPotIndexForOuter(index);
    KFileItem item=itemForIndex(index);
    bool isDir = item.isDir();

    int translated = node->translated;
    int fuzzy = node->fuzzy;
    int untranslated = node->untranslated;
    bool hasStats = translated != -1;

    switch(role)
    {
    case Qt::DisplayRole:
        switch (column)
        {
            case FileName:      return item.text();
            case Graph:         return hasStats?QRect(translated, untranslated, fuzzy, 0):QVariant();
            case TotalCount:    return hasStats?(translated + untranslated + fuzzy):QVariant();
            case TranslatedCount:return hasStats?translated:QVariant();
            case FuzzyCount:    return hasStats?fuzzy:QVariant();
            case UntranslatedCount:return hasStats?untranslated:QVariant();
            case SourceDate:    return node->sourceDate;
            case TranslationDate:return node->translationDate;
            case LastTranslator:return node->lastTranslator;
            default:            return QVariant();
        }
    case Qt::ToolTipRole:
        switch (column)
        {
            case FileName: return item.text();
            default:       return QVariant();
        }
    case KDirModel::FileItemRole:
        return QVariant::fromValue(item);
    case Qt::DecorationRole:
        switch (column)
        {
            case FileName:
                if (isDir)
                    return m_dirIcon;
                if (hasStats && fuzzy == 0 && untranslated == 0)
                    return m_poComplIcon;
                else if (node->poRowNumber != -1)
                    return m_poIcon; 
                else if (node->potRowNumber != -1)
                    return m_potIcon;
            default:
                return QVariant();
        }
    default:
        return QVariant();
    }
}


QModelIndex ProjectModel::index(int row, int column, const QModelIndex& parent) const
{
    ProjectNode* parentNode = nodeForIndex(parent);
    //kWarning()<<(sizeof(ProjectNode))<<nodeCounter;
    if (row>=parentNode->rows.size())
    {
        kWarning()<<"SHIT HAPPENED WITH INDEXES"<<row<<parentNode->rows.size()<<itemForIndex(parent).url();
        return QModelIndex();
    }
    return createIndex(row, column, parentNode->rows.at(row));
}


KFileItem ProjectModel::itemForIndex(const QModelIndex& index) const
{
    if (!index.isValid())
    {
        //file item for root node.
        return m_poModel.itemForIndex(index);
    }
    QModelIndex poIndex = poIndexForOuter(index);
    QModelIndex potIndex = potIndexForOuter(index);

    if (poIndex.isValid())
        return m_poModel.itemForIndex(poIndex);
    else if (potIndex.isValid())
        return m_potModel.itemForIndex(potIndex);

    kWarning()<<"returning empty KFileItem()"<<index.row()<<index.column();
    kWarning()<<"returning empty KFileItem()"<<index.parent().isValid();
    kWarning()<<"returning empty KFileItem()"<<index.parent().internalPointer();
    kWarning()<<"returning empty KFileItem()"<<index.parent().data().toString();
    kWarning()<<"returning empty KFileItem()"<<index.internalPointer();
    kWarning()<<"returning empty KFileItem()"<<static_cast<ProjectNode*>(index.internalPointer())->untranslated<<static_cast<ProjectNode*>(index.internalPointer())->sourceDate;
    return KFileItem();
}


ProjectModel::ProjectNode* ProjectModel::nodeForIndex(const QModelIndex& index) const
{
    if (index.isValid())
    {
        ProjectNode * node = static_cast<ProjectNode *>(index.internalPointer());
        Q_ASSERT(node != NULL);
        return node;
    }
    else
    {
        ProjectNode * node = const_cast<ProjectNode *>(&m_rootNode);
        Q_ASSERT(node != NULL);
        return node;
    }
}


QModelIndex ProjectModel::indexForNode(const ProjectNode* node)
{
    if (node == &m_rootNode)
        return QModelIndex();

    int row = node->rowNumber;
    QModelIndex index = createIndex(row, 0, (void*)node);
    return index;
}

QModelIndex ProjectModel::indexForUrl(const KUrl& url)
{
    if (m_poUrl.isParentOf(url))
    {
        QModelIndex poIndex = m_poModel.indexForUrl(url);
        return indexForPoIndex(poIndex);
    }
    else if (m_potUrl.isParentOf(url))
    {
        QModelIndex potIndex = m_potModel.indexForUrl(url);
        return indexForPotIndex(potIndex);
    }

    return QModelIndex();
}

QModelIndex ProjectModel::parent(const QModelIndex& childIndex) const
{
    if (!childIndex.isValid())
        return QModelIndex();

    ProjectNode* childNode = nodeForIndex(childIndex);
    ProjectNode* parentNode = childNode->parent;

    if (!parentNode || (childNode == &m_rootNode) || (parentNode == &m_rootNode))
        return QModelIndex();

    return createIndex(parentNode->rowNumber, 0, parentNode);
}


/**
 * Theese methods map from project model indices to PO and POT model indices.
 * In each folder files form PO model comes first, and files from POT that do not exist in PO model come after.
 */
QModelIndex ProjectModel::indexForOuter(const QModelIndex& outerIndex, IndexType type) const
{
    if (!outerIndex.isValid())
        return QModelIndex();

    QModelIndex parent = outerIndex.parent();

    QModelIndex internalParent;
    if (parent.isValid())
    {
        internalParent = indexForOuter(parent, type);
        if (!internalParent.isValid())
            return QModelIndex();
    }

    ProjectNode* node = nodeForIndex(outerIndex);

    short rowNumber=(type==PoIndex?node->poRowNumber:node->potRowNumber);
    if (rowNumber == -1)
        return QModelIndex();
    return (type==PoIndex?m_poModel:m_potModel).index(rowNumber, outerIndex.column(), internalParent);
}

QModelIndex ProjectModel::poIndexForOuter(const QModelIndex& outerIndex) const
{
    return indexForOuter(outerIndex, PoIndex);
}


QModelIndex ProjectModel::potIndexForOuter(const QModelIndex& outerIndex) const
{
    return indexForOuter(outerIndex, PotIndex);
}


QModelIndex ProjectModel::poOrPotIndexForOuter(const QModelIndex& outerIndex) const
{
    if (!outerIndex.isValid())
        return QModelIndex();

    QModelIndex poIndex = poIndexForOuter(outerIndex);

    if (poIndex.isValid())
        return poIndex;

    QModelIndex potIndex = potIndexForOuter(outerIndex);

    if (!potIndex.isValid())
        kWarning()<<"error mapping index to PO or POT";

    return potIndex;
}

QModelIndex ProjectModel::indexForPoIndex(const QModelIndex& poIndex) const
{
    if (!poIndex.isValid())
        return QModelIndex();

    QModelIndex outerParent = indexForPoIndex(poIndex.parent());
    int row = poIndex.row(); //keep the same row, no changes

    return index(row, poIndex.column(), outerParent);
}

QModelIndex ProjectModel::indexForPotIndex(const QModelIndex& potIndex) const
{
    if (!potIndex.isValid())
        return QModelIndex();

    QModelIndex outerParent = indexForPotIndex(potIndex.parent());
    ProjectNode* node = nodeForIndex(outerParent);

    int potRow = potIndex.row();
    int row = 0;

    while(row<node->rows.count() && node->rows.at(row)->potRowNumber!=potRow)
        row++;

    if (row != node->rows.count())
        return index(row, potIndex.column(), outerParent);

    kWarning()<<"error mapping index from POT to outer";
    return QModelIndex();
}


/**
 * Makes a list of indices where pot items map to poItems.
 * result[potRow] = poRow or -1 if the pot entry is not found in po.
 * Does not use internal pot and po row number cache.
 */
void ProjectModel::generatePOTMapping(QVector<int> & result, const QModelIndex& poParent, const QModelIndex& potParent) const
{
    result.clear();

    int poRows = m_poModel.rowCount(poParent);
    int potRows = m_potModel.rowCount(potParent);

    if (potRows == 0)
        return;

    QList<KUrl> poOccupiedUrls;

    for (int poPos = 0; poPos < poRows; poPos ++)
    {
        KFileItem file = m_poModel.itemForIndex(m_poModel.index(poPos, 0, poParent));
        KUrl potUrl = poToPot(file.url());
        poOccupiedUrls.append(potUrl);
    }

    for  (int potPos = 0; potPos < potRows; potPos ++)
    {

        KUrl potUrl = m_potModel.itemForIndex(m_potModel.index(potPos, 0, potParent)).url();
        int occupiedPos = -1;

        //TODO: this is slow
        for (int poPos = 0; occupiedPos == -1 && poPos < poOccupiedUrls.count(); poPos ++)
        {
            KUrl& occupiedUrl = poOccupiedUrls[poPos];
            if (potUrl.equals(occupiedUrl))
                occupiedPos = poPos;
        }

        result.append(occupiedPos);
    }
}


KUrl ProjectModel::poToPot(const KUrl& poPath) const
{
    if (!m_poUrl.isParentOf(poPath))
    {
        kWarning()<<"PO path not in project: " << poPath.url();
        return KUrl();
    }

    QString pathToAdd = KUrl::relativeUrl(m_poUrl, poPath);

    //change ".po" into ".pot"
    if (pathToAdd.endsWith(".po")) //TODO: what about folders ??
        pathToAdd = pathToAdd + "t";

    KUrl potPath = m_potUrl;
    potPath.addPath(pathToAdd);

    //kDebug() << "ProjectModel::poToPot("<< poPath.pathOrUrl() << +") = " << potPath.pathOrUrl();
    return potPath;
}

KUrl ProjectModel::potToPo(const KUrl& potPath) const
{
    if (!m_potUrl.isParentOf(potPath))
    {
        kWarning()<<"POT path not in project: " << potPath.url();
        return KUrl();
    }

    QString pathToAdd = KUrl::relativeUrl(m_potUrl, potPath);

    //change ".pot" into ".po"
    if (pathToAdd.endsWith(".pot")) //TODO: what about folders ??
        pathToAdd = pathToAdd.left(pathToAdd.length() - 1);

    KUrl poPath = m_poUrl;
    poPath.addPath(pathToAdd);

    //kDebug() << "ProjectModel::potToPo("<< potPath.pathOrUrl() << +") = " << poPath.pathOrUrl();
    return poPath;
}


//Metadata stuff
//For updating translation stats

void ProjectModel::enqueueNodeForMetadataUpdate(ProjectNode * node)
{
    if (m_dirsWaitingForMetadata.contains(node))
    {
        if ((m_activeJob != NULL) && (m_activeNode == node))
            m_activeJob->setStatus(-1);

        return;
    }

    m_dirsWaitingForMetadata.insert(node);

    if (m_activeJob == NULL)
        startNewMetadataJob();
}


void ProjectModel::deleteSubtree(ProjectNode* node)
{
    for (int row = 0; row < node->rows.count(); row ++)
        deleteSubtree(node->rows.at(row));

    m_dirsWaitingForMetadata.remove(node);

    if ((m_activeJob != NULL) && (m_activeNode == node))
        m_activeJob->setStatus(-1);

    delete node;
}


void ProjectModel::startNewMetadataJob()
{
    m_activeJob = NULL;
    m_activeNode = NULL;

    if (m_dirsWaitingForMetadata.isEmpty())
        return;

    ProjectNode* node = *m_dirsWaitingForMetadata.begin();

    //prepare new work
    m_activeNode = node;

    QList<KFileItem> files;

    QModelIndex item = indexForNode(node);

    for (int row=0; row < node->rows.count(); row ++)
        files.append(itemForIndex(index(row, 0, item)));

    m_activeJob = new UpdateStatsJob(files, this);
    connect(
        m_activeJob,SIGNAL(done(ThreadWeaver::Job*)),
        this,SLOT(finishMetadataUpdate(ThreadWeaver::Job*)));

    ThreadWeaver::Weaver::instance()->enqueue(m_activeJob);
}

void ProjectModel::finishMetadataUpdate(ThreadWeaver::Job * _job)
{
    UpdateStatsJob* job = static_cast<UpdateStatsJob *>(_job);

    if (job->m_status == -2)
    {
        delete job;
        return;
    }

    if ((m_dirsWaitingForMetadata.contains(m_activeNode)) && (job->m_status == 0))
    {
        m_dirsWaitingForMetadata.remove(m_activeNode);
        //store the results

        setMetadataForDir(m_activeNode, m_activeJob->m_info);

        QModelIndex item = indexForNode(m_activeNode);

        //scan dubdirs - initiate data loading into the model.
        for (int row=0; row < m_activeNode->rows.count(); row++)
        {
            QModelIndex child = index(row, 0, item);

            if (canFetchMore(child))
                fetchMore(child);
        }
    }

    delete m_activeJob;

    startNewMetadataJob();
}


void ProjectModel::slotFileSaved(const KUrl& url)
{
    QModelIndex index = indexForUrl(url);

    if (!index.isValid())
        return;

    QList<KFileItem> files;
    files.append(itemForIndex(index));

    UpdateStatsJob* j = new UpdateStatsJob(files);
    connect(j,SIGNAL(done(ThreadWeaver::Job*)),
        this,SLOT(finishSingleMetadataUpdate(ThreadWeaver::Job*)));

    ThreadWeaver::Weaver::instance()->enqueue(j);
}

void ProjectModel::finishSingleMetadataUpdate(ThreadWeaver::Job* _job)
{
    UpdateStatsJob* job = static_cast<UpdateStatsJob*>(_job);

    if (job->m_status != 0)
    {
        delete job;
        return;
    }

    const KFileMetaInfo& info=job->m_info.first();
    QModelIndex index = indexForUrl(info.url());
    if (!index.isValid())
        return;

    ProjectNode* node = nodeForIndex(index);
    node->setFileStats(job->m_info.first());
    updateDirStats(nodeForIndex(index.parent()));

    QModelIndex topLeft = index.sibling(index.row(), Graph);
    QModelIndex bottomRight = index.sibling(index.row(), ProjectModelColumnCount - 1);
    emit dataChanged(topLeft, bottomRight);

    delete job;
}

void ProjectModel::setMetadataForDir(ProjectNode* node, const QList<KFileMetaInfo>& data)
{
    int dataCount = data.count();
    int rowsCount = node->rows.count();
    Q_ASSERT(dataCount == rowsCount);

    for (int row = 0; row < rowsCount; row ++)
        node->rows[row]->setFileStats(data.at(row));

    if (!dataCount)
        return;

    updateDirStats(node);

    QModelIndex item = indexForNode(node);
    QModelIndex topLeft = index(0, Graph, item);
    QModelIndex bottomRight = index(rowsCount - 1, ProjectModelColumnCount - 1, item);
    emit dataChanged(topLeft, bottomRight);
}

void ProjectModel::updateDirStats(ProjectNode* node)
{
    if (node == &m_rootNode)
        return;

    node->calculateDirStats();
    updateDirStats(node->parent);

    if (node->parent->rows.count()==0 || node->parent->rows.count()>=node->rowNumber)
        return;
    QModelIndex index = indexForNode(node);
    kWarning()<<index.row()<<node->parent->rows.count();
    if (index.row()>=node->parent->rows.count())
        return;
    QModelIndex topLeft = index.sibling(index.row(), Graph);
    QModelIndex bottomRight = index.sibling(index.row(), ProjectModelColumnCount - 1);
    emit dataChanged(topLeft, bottomRight);
}


//ProjectNode class

ProjectModel::ProjectNode::ProjectNode(ProjectNode* _parent, int _rowNum, int _poIndex, int _potIndex)
    : parent(_parent)
    , rowNumber(_rowNum)
    , poRowNumber(_poIndex)
    , potRowNumber(_potIndex)
    , poCount(0)
    , translated(-1)
    , untranslated(-10)
    , fuzzy(-1)
{
    ++nodeCounter;
}

ProjectModel::ProjectNode::~ProjectNode()
{
    --nodeCounter;
}

void ProjectModel::ProjectNode::calculateDirStats()
{
    fuzzy = 0;
    translated = 0;
    untranslated = 0;

    for (int pos = 0; pos < rows.count(); pos++)
    {
        ProjectNode* child = rows.at(pos);
        if (child->translated != -1)
        {
            fuzzy += child->fuzzy;
            translated += child->translated;
            untranslated += child->untranslated;
        }
    }
}


void ProjectModel::ProjectNode::setFileStats(const KFileMetaInfo& info)
{
    if (info.keys().count() == 0)
        return;

    translated = info.item("translation.translated").value().toInt();
    untranslated =info.item("translation.untranslated").value().toInt();
    fuzzy = info.item("translation.fuzzy").value().toInt();
    lastTranslator = info.item("translation.last_translator").value().toString();
    sourceDate = info.item("translation.source_date").value().toString();
    translationDate = info.item("translation.translation_date").value().toString();
}


UpdateStatsJob::UpdateStatsJob(QList<KFileItem> files, QObject* owner)
    : ThreadWeaver::Job(owner)
    , m_files(files)
    , m_status(0)
{
}

UpdateStatsJob::~UpdateStatsJob()
{
}

//runs in separate thread
void UpdateStatsJob::run ()
{
    for (int pos = 0; pos < m_files.count(); pos ++)
    {
        if (m_status != 0)
            return;

        const KFileItem & file = m_files.at(pos);
        KFileMetaInfo info;

        if ((!file.isNull()) && (!file.isDir()))
        {
            //force population of metainfo, do not use the KFileItem default behavior
            if (file.metaInfo(false).keys().isEmpty())
                info = KFileMetaInfo(file.url());
            else
                info = file.metaInfo(false);
        }

        m_info.append(info);
    }
}

void UpdateStatsJob::setStatus(int status)
{
    m_status = status;
}

#include "projectmodel.moc"

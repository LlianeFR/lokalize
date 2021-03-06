/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2008 by Nick Shaforostoff <shafff@ukr.net>
                2018-2019 by Simon Depiets <sdepiets@gmail.com>

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

#ifndef ALTTRANSVIEW_H
#define ALTTRANSVIEW_H

#define ALTTRANS_SHORTCUTS 9

#include "pos.h"
#include "alttrans.h"
#include <QDockWidget>
namespace TM
{
class TextBrowser;
}
class Catalog;
class QAction;

class AltTransView: public QDockWidget
{
    Q_OBJECT

public:
    explicit AltTransView(QWidget*, Catalog*, const QVector<QAction*>&);
    ~AltTransView() override;


public slots:
    void slotNewEntryDisplayed(const DocPosition&);
    void fileLoaded();
    void attachAltTransFile(const QString&);
    void addAlternateTranslation(int entry, const QString&);

private slots:
    //void contextMenu(const QPoint & pos);
    void process();
    void initLater();
    void slotUseSuggestion(int);

signals:
    void refreshRequested();
    void textInsertRequested(const QString&);

private:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent *event) override;
    bool event(QEvent *event) override;


private:
    TM::TextBrowser* m_browser;
    Catalog* m_catalog;
    QString m_normTitle;
    QString m_hasInfoTitle;
    bool m_hasInfo;
    bool m_everShown;
    DocPos m_entry;
    DocPos m_prevEntry;

    QVector<AltTrans> m_entries;
    QMap<int, int> m_entryPositions;
    QVector<QAction*> m_actions;//need them to get shortcuts
};

#endif

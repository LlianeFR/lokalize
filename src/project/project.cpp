/*****************************************************************************
  This file is part of KAider

  Copyright (C) 2007	  by Nick Shaforostoff <shafff@ukr.net>

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

#include "project.h"
#include "projectmodel.h"

#include "tbxparser.h"

#include <QXmlSimpleReader>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <kurl.h>
#include <kdirlister.h>

Project* Project::_instance=0;

Project* Project::instance()
{
    if (_instance==0)
        _instance=new Project();

    return _instance;
}

Project::Project(/*const QString &file*/)
    : ProjectBase()
    , m_model(0)
    , m_glossary(new Glossary)
{
}

Project::~Project()
{
    delete m_model;
    delete m_glossary;
    kWarning() << "--d "<< m_path << endl;
    writeConfig();
}

void Project::save()
{
//     kWarning() << "--s "<< m_path << endl;
//     setSharedConfig(KSharedConfig::openConfig(m_path, KConfig::NoGlobals));
// 
//     kWarning() << "--s "<< potBaseDir() << " " << poBaseDir()<< endl;
//     QString aa(potBaseDir());
//     readConfig();
//     setPotBaseDir(aa);
    writeConfig();
}


void Project::load(const QString &file)
{
    setSharedConfig(KSharedConfig::openConfig(file, KConfig::NoGlobals));
    readConfig();
    m_path=file;
//     kWarning() << "--l "<< m_path << endl;

    QTimer::singleShot(500,this,SLOT(populateDirModel()));
    QTimer::singleShot(400,this,SLOT(populateGlossary()));
}

void Project::populateDirModel()
{
    if (!m_model || m_path.isEmpty())
        return;

    KUrl url(m_path);
    url.setFileName(QString());
    url.cd(poBaseDir());

    if (QFile::exists(url.path()))
    {
        m_model->dirLister()->openUrl(url);
    }
}

QString Project::glossaryPath() const
{
    if (KUrl::isRelativeUrl(glossaryTbx()))
    {
        KUrl url(m_path);
        url.setFileName(QString());
        url.addPath(glossaryTbx());
        return url.path();
    }
    return glossaryTbx();
}

void Project::populateGlossary()
{
    m_glossary->clear();

    TbxParser parser(m_glossary);
    QXmlSimpleReader reader;
    reader.setContentHandler(&parser);

    QFile file(glossaryPath());
    if (!file.open(QFile::ReadOnly | QFile::Text))
         return;
    QXmlInputSource xmlInputSource(&file);
    if (!reader.parse(xmlInputSource))
    {
         kWarning() << "failed to load "<< glossaryPath()<< endl;
    }

}

/////////////////////////////at most 1 tag pair on the line
//int writeLine(int indent,const QString& str,QFile& out)
int writeLine(int indent,const QByteArray& str,QFile& out)
{
    kWarning() << str << /*" a1 "<< indent<< */endl;
    int balance=str.count('<')-str.count("</")*2;
    if (balance<0)
        indent+=balance;

    out.write(QByteArray(indent*4,' ')+str+'\n');

    if (balance>0)
        indent+=balance;


//kWarning() << "a3 "<< indent<< endl;

    return indent;
}
void Project::glossaryAdd(const TermEntry& term)
{
    QFile stream(glossaryPath());
    if (!stream.open(QFile::ReadWrite | QFile::Text))
         return;

//     QTextStream stream(&file);
//     stream.setCodec("UTF-8");
//     stream.setAutoDetectUnicode(true);
    QByteArray line(stream.readLine());
    int id=0;
    QList<int> idlist;
    QRegExp rx("<termEntry id=\"([0-9]*)\">");
    while(!stream.atEnd()&&!line.contains("</body>"))
//    while(!stream.atEnd()&&!stream.pos()+20>stream.size())
    {
        if (rx.indexIn(line)!=-1);
        {
            idlist<<rx.cap(1).toInt();
        }
        line=stream.readLine();
    }
//    stream.seek(stream.pos());
    if (!idlist.isEmpty())
    {
        qSort(idlist);
        while (idlist.contains(id))
            ++id;
    }
    stream.seek(stream.pos()-line.size());
    //kWarning() << "load "<< stream.readLine()<< endl;
    int indent=3;
    indent=writeLine(indent,QString("<termEntry id=\"%1\">").arg(id).toUtf8(),stream);
    indent=writeLine(indent,"<langSet xml:lang=\"en\">",stream);
    indent=writeLine(indent,"<ntig>",stream);
    indent=writeLine(indent,"<termGrp>",stream);
    indent=writeLine(indent,QString("<term>%1</term>").arg(term.english).toUtf8(),stream);
    //indent=writeLine(indent,"<termNote type="partOfSpeech">noun</termNote>
    indent=writeLine(indent,"</termGrp>",stream);
    //indent=writeLine(indent,"<descrip type="context"></descrip>
    indent=writeLine(indent,"</ntig>",stream);
    indent=writeLine(indent,"</langSet>",stream);
    indent=writeLine(indent,QString("<langSet xml:lang=\"%1\">").arg(langCode()).toUtf8(),stream);
    indent=writeLine(indent,"<ntig>",stream);
    indent=writeLine(indent,"<termGrp>",stream);
    indent=writeLine(indent,QString("<term>%1</term>").arg(term.target).toUtf8(),stream);
//     indent=writeLine(indent,"<termNote type="partOfSpeech">���.</termNote>",stream);
    indent=writeLine(indent,"</termGrp>",stream);
    indent=writeLine(indent,"</ntig>",stream);
    indent=writeLine(indent,"</langSet>",stream);
    indent=writeLine(indent,"</termEntry>",stream);
    indent=writeLine(indent,"</body>",stream);
    indent=writeLine(indent,"</text>",stream);
    indent=writeLine(indent,"</martif>",stream);

}



ProjectModel* Project::model()
{
    if (!m_model)
    {
        m_model=new ProjectModel;
        QTimer::singleShot(500,this,SLOT(populateDirModel()));
    }

    return m_model;

}

/*
Project::Project(const QString &file)
    : ProjectBase(KSharedConfig::openConfig(file, KConfig::NoGlobals))
{
    readConfig();
}
*/
// Project::~Project()
// {
// }



#include "project.moc"

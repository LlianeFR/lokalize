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

#include <glossary.h>

#include "tbxparser.h"
#include "project.h"
#include "prefs_lokalize.h"

#include <kdebug.h>

#include <QFile>
#include <QXmlSimpleReader>
#include <QXmlStreamReader>

//DISK

void Glossary::load(const QString& p)
{
    clear();
    path=p;

    TbxParser parser(this);
    QXmlSimpleReader reader;
    reader.setContentHandler(&parser);

    QFile file(p);
    if (!file.open(QFile::ReadOnly | QFile::Text))
         return;
    QXmlInputSource xmlInputSource(&file);
    if (!reader.parse(xmlInputSource))
         kWarning() << "failed to load "<< path;

    emit changed();
}

/**
 * reads glossary into buffer, changing and removing entries along the way
 * (if any -- the check is done by caller)
 */
static void saveChanged(const Glossary* glo)
{
    QFile in(glo->path);
    if (!in.open(QFile::ReadOnly|QFile::Text))
         return;

    QByteArray out;
    out.reserve(in.size()+256);
//     QFile out(glossaryPath()+".tmp");
//     if (!out.open(QFile::WriteOnly | QFile::Text))
//          return;


    QXmlStreamReader xmlIn(&in);
    QXmlStreamWriter xmlOut(&out);
    xmlOut.setAutoFormatting(true);
    while (!xmlIn.atEnd())
    {
        if (xmlIn.readNext()==QXmlStreamReader::StartElement
           &&xmlIn.name()=="termEntry")
        {
            //this is basically the same as the changing code, 
            //except that we don't write :)
            int i=glo->removedIds.size();
            while (--i>=0)
                if (xmlIn.attributes().value("id")==glo->removedIds.at(i))
                    break;
            if (i!=-1)
            {
                //xmlOut.writeCharacters("\nbegin remove\n");
                //_go through_ data from In, skipping data that we support
                const QString& langCode=Project::instance()->langCode();
                do
                {
                    if (xmlIn.tokenType()==QXmlStreamReader::StartElement)
                    {
                        if (xmlIn.name()=="descrip")
                        {
                            if (xmlIn.attributes().value("type")=="subjectField"
                            ||xmlIn.attributes().value("type")=="definition")
                            {
                                //skip this data in input stream
                                while (! (xmlIn.readNext()==QXmlStreamReader::EndElement
                                        &&xmlIn.name()=="descrip") )
                                    ;
                            }
                        }
                        else if (xmlIn.name()=="langSet"
                                &&(xmlIn.attributes().value("xml:lang")=="en"
                                ||xmlIn.attributes().value("xml:lang")==langCode))
                        {
                            while (! (xmlIn.readNext()==QXmlStreamReader::EndElement
                                    &&xmlIn.name()=="langSet") )
                                ;
                        }
                    }
                    if (xmlIn.readNext()==QXmlStreamReader::EndElement
                            &&xmlIn.name()=="termEntry")
                        break;
                } while(!(   xmlIn.readNext()==QXmlStreamReader::EndElement
                            &&xmlIn.name()=="termEntry"   ));
                            //xmlOut.writeCharacters("\nend remove\n");
            }
            else
            {
                i=glo->changedIds.size();
                while (--i>=0)
                    if (xmlIn.attributes().value("id")==glo->changedIds.at(i))
                        break;
                if (i!=-1)
                {
                    //find entry by its id
                    int j=glo->termList.size();
                    while (--j>=0)
                        if (glo->termList.at(j).id==glo->changedIds.at(i))
                            break;
                    if (j==-1)
                    {
                        kWarning()<<"should never happen";
                        continue;
                    }
                    const TermEntry& entry=glo->termList.at(j);

                    //we aint changing starting termEntry
                    xmlOut.writeCurrentToken(xmlIn);
                    //first, write _our_ meta data
        // #if 0
                    if (entry.subjectField/*!=-1*/)
                    {
                        xmlOut.writeStartElement("descrip");
                        xmlOut.writeAttribute("type","subjectField");
                        xmlOut.writeCharacters(glo->subjectFields.at(entry.subjectField));
                        xmlOut.writeEndElement();
                    }
                    if (!entry.definition.isEmpty())
                    {
                        xmlOut.writeStartElement("descrip");
                        xmlOut.writeAttribute("type","definition");
                        xmlOut.writeCharacters(entry.definition);
                        xmlOut.writeEndElement();
                    }
        // #endif
                    //write data from In, skipping data that we support
                    const QString& langCode=Project::instance()->langCode();
                    do
        //             while(!(   xmlIn.readNext()==QXmlStreamReader::EndElement
        //                          &&xmlIn.name()=="termEntry"   ));
                    {
                        if (xmlIn.tokenType()==QXmlStreamReader::StartElement)
                        {
                            if (xmlIn.name()=="descrip")
                            {
                                if (xmlIn.attributes().value("type")=="subjectField"
                                ||xmlIn.attributes().value("type")=="definition")
                                {
                                    //skip this data in input stream
                                    while (! (xmlIn.readNext()==QXmlStreamReader::EndElement
                                            &&xmlIn.name()=="descrip") )
                                        ;
        //                              xmlIn.readNext();
        //                              continue;
                                }
                            }
                            else if (xmlIn.name()=="langSet"
                                    &&(xmlIn.attributes().value("xml:lang")=="en"
                                    ||xmlIn.attributes().value("xml:lang")==langCode))
                            {
                                while (! (xmlIn.readNext()==QXmlStreamReader::EndElement
                                        &&xmlIn.name()=="langSet") )
                                    ;
        //                                 kWarning() << "text  "<< xmlIn.text().toString();;
        //                         xmlIn.readNext();
        //                         continue;
                            }
        //                     else
        //                         kWarning() << "text  "<< xmlIn.attributes().value("xml:lang").toString();;
                        }
        //                 kWarning() << "ff  "<< xmlIn.tokenString();
                        if (xmlIn.readNext()==QXmlStreamReader::EndElement
                                &&xmlIn.name()=="termEntry")
                            break;
                        xmlOut.writeCurrentToken(xmlIn);
        //             }
                    } while(!(   xmlIn.readNext()==QXmlStreamReader::EndElement
                                &&xmlIn.name()=="termEntry"   ));
        //             xmlOut.wri   teCurrentToken(xmlIn);
                    int k=0;
                    for (;k<entry.english.size();++k)
                    {
                        if (entry.english.at(k).isEmpty())
                            continue;
    
                        xmlOut.writeStartElement("langSet");
                        //xmlOut.writeAttribute("xml","lang","en");
                        xmlOut.writeAttribute("xml:lang","en");
                        xmlOut.writeStartElement("ntig");
                        xmlOut.writeStartElement("termGrp");
                        xmlOut.writeTextElement("term",entry.english.at(k));
                        xmlOut.writeEndElement();
                        xmlOut.writeEndElement();
                        xmlOut.writeEndElement();
                    }
                    for (k=0;k<entry.target.size();++k)
                    {
                        if (entry.target.at(k).isEmpty())
                            continue;
                        xmlOut.writeStartElement("langSet");
                        //xmlOut.writeAttribute("xml","lang",langCode());
                        xmlOut.writeAttribute("xml:lang",langCode);
                        xmlOut.writeStartElement("ntig");
                        xmlOut.writeStartElement("termGrp");
                        xmlOut.writeTextElement("term",entry.target.at(k));
                        xmlOut.writeEndElement();
                        xmlOut.writeEndElement();
                        xmlOut.writeEndElement();
                    }
                }
                xmlOut.writeCurrentToken(xmlIn);
            }
        }
        else
        //kDebug()<<action<<xmlIn.tokenString();
        //out+=action;
//        xmlOut.writeCharacters("\n000\n");
            xmlOut.writeCurrentToken(xmlIn);
    }

    if (!xmlIn.hasError())
    {
        in.close();
        if (!in.open(QFile::WriteOnly | QFile::Text))
            return;
        //HACK 
        out.replace("\n\n\n","\n");
        out.replace("\n\n","\n");
        out.replace("\n            <langSet","\n                <langSet");
        in.write(out);
    }

}

static void addTerms(Glossary* glo)
{
    QFile stream(glo->path);
    if (!stream.open(QFile::ReadWrite | QFile::Text))
         return;

    QByteArray line;
    if (stream.size()==0)
    {
        stream.write(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<!DOCTYPE martif PUBLIC \"ISO 12200:1999A//DTD MARTIF core (DXFcdV04)//EN\" \"TBXcdv04.dtd\">\n"
        "<martif type=\"TBX\" xml:lang=\"en\">\n"
        "    <martifHeader>\n"
        "        <fileDesc>\n"
        "            <titleStmt>\n"
        "                <title>Your Team Glossary</title>\n"
        "            </titleStmt>\n"
        "        </fileDesc>\n"
        "        <encodingDesc>\n"
        "            <p type=\"DCSName\">SYSTEM &quot;TBXDCSv05b.xml&quot;</p>\n"
        "        </encodingDesc>\n"
        "    </martifHeader>\n"
        "    <text>\n"
        "        <body>\n"
                    );
    }
    else
    {
        //This seeks to the end of file to add new items
        //This way file is never rewrited completely =>
        //this is fast and saves diskdrive's life


        //QByteArray line(stream.readLine());
        line=stream.readLine();
        while(!stream.atEnd()&&!line.contains("</body>"))
            line=stream.readLine();

        stream.seek(stream.pos()-line.size());
    }

    QString out;
    out+=QString(line).remove(QRegExp(" *</body>.*"));
    out+="\n";
    QXmlStreamWriter xmlOut(&out);

    int limit=glo->addedIds.size();
    int k=-1;
    while (++k<limit)
    {
        //find entry by its id
        int j=glo->termList.size();
        while (--j>=0)
            if (glo->termList.at(j).id==glo->addedIds.at(k))
                break;
        if (j==-1)
        {
            kWarning()<<"should never happen";
            continue;
        }
        const TermEntry& entry=glo->termList.at(j);
        if (entry.english.isEmpty()||entry.target.isEmpty())
        {
            kWarning()<<"Skippin non-complete entry";
            continue;
        }


        xmlOut.setAutoFormatting(true);
        xmlOut.writeStartElement("termEntry");
        xmlOut.writeAttribute("id",entry.id);

        if (entry.subjectField/*!=-1*/)
        {
            xmlOut.writeStartElement("descrip");
            xmlOut.writeAttribute("type","subjectField");
            xmlOut.writeCharacters(glo->subjectFields.at(entry.subjectField));
            xmlOut.writeEndElement();
        }

        if (!entry.definition.isEmpty())
        {
            xmlOut.writeStartElement("descrip");
            xmlOut.writeAttribute("type","definition");
            xmlOut.writeCharacters(entry.definition);
            xmlOut.writeEndElement();
        }

        int i=0;
        for (;i<entry.english.size();++i)
        {
            if (entry.english.at(i).isEmpty())
                continue;

            xmlOut.writeStartElement("langSet");
            //xmlOut.writeAttribute("xml","lang","en");
            xmlOut.writeAttribute("xml:lang","en");
            xmlOut.writeStartElement("ntig");
            xmlOut.writeStartElement("termGrp");
            xmlOut.writeTextElement("term",entry.english.at(i));
            xmlOut.writeEndElement();
            xmlOut.writeEndElement();
            xmlOut.writeEndElement();
        }
        for (i=0;i<entry.target.size();++i)
        {
            if (entry.target.at(i).isEmpty())
                continue;
            xmlOut.writeStartElement("langSet");
            //xmlOut.writeAttribute("xml","lang",langCode());
            xmlOut.writeAttribute("xml:lang",Project::instance()->langCode());
            xmlOut.writeStartElement("ntig");
            xmlOut.writeStartElement("termGrp");
            xmlOut.writeTextElement("term",entry.target.at(i));
            xmlOut.writeEndElement();
            xmlOut.writeEndElement();
            xmlOut.writeEndElement();
        }
        xmlOut.writeEndElement();
    }

    //Qt isn't perfect
    out.replace("\n\n","\n");
    out.replace("\n","\n            ");

    out+=
"\n        </body>"
"\n    </text>"
"\n</martif>\n";
    stream.write(out.toUtf8());
}

void Glossary::save()
{
    if (!changedIds.isEmpty()||!removedIds.isEmpty())
    {
        saveChanged(this);
        changedIds.clear();
        removedIds.clear();
    }
    if (!addedIds.isEmpty())
    {
        addTerms(this);
        addedIds.clear();
    }
}



//BEGIN MODEL

int GlossaryModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return Project::instance()->glossary()->termList.size();
}

QVariant GlossaryModel::headerData( int section, Qt::Orientation /*orientation*/, int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

    switch (section)
    {
//         case ID: return i18nc("@title:column","ID");
        case English: return i18nc("@title:column","English");
        case Target: return i18nc("@title:column","Target");
        case SubjectField: return i18nc("@title:column","Subject Field");
    }
    return QVariant();
}

QVariant GlossaryModel::data(const QModelIndex& index,int role) const
{
    if (role!=Qt::DisplayRole)
        return QVariant();

//     kDebug()<<index.row();
//     kDebug()<<Project::instance()->glossary()->termList.size();
    switch (index.column())
    {
//         case ID: return Project::instance()->glossary()->termList.at(index.row()).id/*index.row()+1*/;
        case English: return Project::instance()->glossary()->termList.at(index.row()).english.join("\n");
        case Target: return Project::instance()->glossary()->termList.at(index.row()).target.join("\n");
        case SubjectField: 
        {
            const Glossary* glo=Project::instance()->glossary();
            int field=glo->termList.at(index.row()).subjectField;

            return glo->subjectFields.at(field);
        }
    }
    return QVariant();
}

//END MODEL general (GlossaryModel continues below)

//BEGIN EDITING

QString Glossary::generateNewId()
{
    //generate uniq ID
    int id=0;
    QList<int> idlist;
    const QList<TermEntry>& termList=Project::instance()->glossary()->termList;
    QString authorId(Settings::authorName().toLower());
    authorId.replace(' ','_');
    QRegExp rx("^"+authorId+"\\-([0-9]*)$");

    int i=termList.size();
    while(--i>=0)
    {
        if (rx.exactMatch(termList.at(i).id))
        {
            kDebug()<<i
                    <<termList.at(i).id
                    <<rx.cap(1)
                    <<rx.cap(1).toInt();
            idlist.append(rx.cap(1).toInt());
        }
    }
    kDebug()<<idlist;
    i=removedIds.size();
    while(--i>=0)
    {
        if (rx.exactMatch(removedIds.at(i)))
            idlist.append(rx.cap(1).toInt());
    }
    kDebug()<<idlist;

    if (!idlist.isEmpty())
    {
        qSort(idlist);
        while (idlist.contains(id))
            ++id;
    }

    return QString(authorId+"-%1").arg(id);
}

//add words to the hash
// static void addToHash(const QMultiHash<QString,int>& wordHash,
//                       const QString& what,
//                       int index)
void Glossary::hashTermEntry(int index)
{
    Q_ASSERT(index<termList.size());
    int i=termList.at(index).english.size();
    while(--i>=0)
    {
        QStringList words=termList.at(index).english.at(i).split(' ',QString::SkipEmptyParts);
        if (!words.isEmpty())
        {
            int j=words.size();
            while(--j>=0)
                wordHash.insert(words.at(j),index);
        }
    }
}

void Glossary::unhashTermEntry(int index)
{
    Q_ASSERT(index<termList.size());
    int i=termList.at(index).english.size();
    while(--i>=0)
    {
        QStringList words=termList.at(index).english.at(i).split(' ',QString::SkipEmptyParts);
        if (!words.isEmpty())
        {
            int j=words.size();
            while(--j>=0)
                wordHash.remove(words.at(j),index);
        }
    }
}


void Glossary::remove(int i)
{
    unhashTermEntry(i);

    int pos;
    if ((pos=addedIds.indexOf(termList.at(i).id))!=-1)
        addedIds.removeAt(pos);
    removedIds.append(termList.at(i).id);
    termList.removeAt(i);

    emit changed();//may be emitted multiple times in a row. so what? :)
//     kDebug()<<"removedIds"<<removedIds.size();
//     kDebug()<<"termList"<<termList.size();
}

void Glossary::append(const QString& _english,const QString& _target)
{
    int index=termList.count();

    TermEntry a;
    a.english<<_english;
    a.target<<_target;
    a.id=generateNewId();
    termList.append(a);

    addedIds.append(a.id);

    hashTermEntry(index);

    emit changed();
}

bool GlossaryModel::removeRows(int row,int count,const QModelIndex& parent)
{
    beginRemoveRows(parent,row,row+count-1);

    Glossary* glo=Project::instance()->glossary();
    int i=row+count;
    while (--i>=row)
        glo->remove(i);

    endRemoveRows();
    return true;
}

// bool GlossaryModel::insertRows(int row,int count,const QModelIndex& parent)
// {
//     if (row!=rowCount())
//         return false;
bool GlossaryModel::appendRow(const QString& _english,const QString& _target)
{
    beginInsertRows(QModelIndex()/*parent*/,rowCount(),rowCount());

    Project::instance()->glossary()->append(_english,_target);

    endInsertRows();
    return true;
}

//END EDITING

/* ****************************************************************************
  This file is part of Lokalize

  Copyright (C) 2007-2009 by Nick Shaforostoff <shafff@ukr.net>

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

#include "syntaxhighlighter.h"

#include "project.h"
#include "prefs_lokalize.h"
#include "prefs.h"

#include <KDebug>
#include <KColorScheme>

#include <QApplication>

#define STATE_NORMAL 0
#define STATE_TAG 1


#define NUM_OF_RULES 5

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent/*, bool docbook*/)
    : QSyntaxHighlighter(parent)
    , tagBrush(KColorScheme::View,KColorScheme::VisitedText)
    , m_approved(true)
//     , fuzzyState(false)
//     , fromDocbook(docbook)
{
    highlightingRules.reserve(NUM_OF_RULES);
    HighlightingRule rule;
    //rule.format.setFontItalic(true);
//     tagFormat.setForeground(tagBrush.brush(QApplication::palette()));
    tagFormat.setForeground(tagBrush.brush(QApplication::palette()));
    //QTextCharFormat format;
    //tagFormat.setForeground(Qt::darkBlue);
//     if (!docbook) //support multiline tags
//     {
//         rule.format = tagFormat;
//         rule.pattern = QRegExp("<.+>");
//         rule.pattern.setMinimal(true);
//         highlightingRules.append(rule);
//     }

    //entity
    rule.format.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp("(&[A-Za-z_:][A-Za-z0-9_\\.:-]*;)");
    highlightingRules.append(rule);

    rule.format.setForeground(Qt::darkMagenta);
    rule.pattern = QRegExp(Project::instance()->accel());
    //rule.pattern = QRegExp("&[^;]*;");
/*    QString accelRx=Project::instance()->accel();
    int pos=accelRx.indexOf('(')+1;
    rule.pattern = QRegExp(  accelRx.mid( pos,accelRx.indexOf(')',pos)-1 )  );*/
    highlightingRules.append(rule);

    //\n \t \"
    rule.format.setForeground(Qt::darkGreen);
    rule.pattern = QRegExp("(\\\\[abfnrtv'\"\?\\\\])|(\\\\\\d+)|(\\\\x[\\dabcdef]+)");
    highlightingRules.append(rule);





    //spaces
    settingsChanged();
    connect(SettingsController::instance(),SIGNAL(generalSettingsChanged()),this, SLOT(settingsChanged()));

}

void SyntaxHighlighter::settingsChanged()
{
    QRegExp re(" +$|^ +");
    if (Settings::highlightSpaces() && highlightingRules.last().pattern!=re)
    {
        HighlightingRule rule;
        KColorScheme colorScheme(QPalette::Normal);
        rule.format.clearForeground();
        rule.format.setBackground(colorScheme.background(KColorScheme::ActiveBackground));
        rule.pattern = re;
        highlightingRules.append(rule);
        rehighlight();
    }
    else if (!Settings::highlightSpaces() && highlightingRules.last().pattern==re)
    {
        highlightingRules.resize(highlightingRules.size()-1);
        rehighlight();
    }
}

/*
void SyntaxHighlighter::setFuzzyState(bool fuzzy)
{
    return;
    int i=NUM_OF_RULES;
    while(--i>=0)
        highlightingRules[i].format.setFontItalic(fuzzy);

    tagFormat.setFontItalic(fuzzy);
}*/

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    QTextCharFormat f;
    f.setFontItalic(!m_approved);
    setFormat(0, text.length(), f);

    tagFormat.setFontItalic(!m_approved);
    //if (fromDocbook)
    {
        setCurrentBlockState(STATE_NORMAL);

        int startIndex = STATE_NORMAL;
        if (previousBlockState() != STATE_TAG)
            startIndex = text.indexOf('<');

        while (startIndex >= 0)
        {
            int endIndex = text.indexOf('>', startIndex);
            int commentLength;
            if (endIndex == -1)
            {
                setCurrentBlockState(STATE_TAG);
                commentLength = text.length() - startIndex;
            }
            else
            {
                commentLength = endIndex - startIndex
                                +1/*+ commentEndExpression.matchedLength()*/;
            }
            setFormat(startIndex, commentLength, tagFormat);
            startIndex = text.indexOf('<', startIndex + commentLength);
        }
    }

    foreach (const HighlightingRule &rule, highlightingRules)
    {
        QRegExp expression(rule.pattern);
        int index = expression.indexIn(text);
        while (index >= 0)
        {
            int length = expression.matchedLength();
            QTextCharFormat f=rule.format;
            f.setFontItalic(!m_approved);
            setFormat(index, length, f);
            index = expression.indexIn(text, index + length);
        }
    }
}


#include "syntaxhighlighter.moc"


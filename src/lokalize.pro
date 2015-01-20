#-------------------------------------------------
#
# Project created by QtCreator 2012-06-06T01:26:21
#
#-------------------------------------------------

QT       += core widgets xml sql

TARGET = lokalize
mac: TARGET = Lokalize
win32: TARGET = Lokalize
TEMPLATE = app


SOURCES += main.cpp\
##    lokalizemainwindow.cpp
    actionproxy.cpp\
    editortab.cpp\
#    editortab_findreplace.cpp
    editorview.cpp\
    xlifftextedit.cpp\
    syntaxhighlighter.cpp\
    completionstorage.cpp\
    phaseswindow.cpp\
    noteeditor.cpp\
    msgctxtview.cpp\
    binunitsview.cpp\
    cataloglistview/cataloglistview.cpp\
    cataloglistview/catalogmodel.cpp\
    common/headerviewmenu.cpp\
    common/domroutines.cpp\
    common/fastsizehintitemdelegate.cpp\
    common/flowlayout.cpp\
    common/termlabel.cpp\
    common/languagelistmodel.cpp\
    common/stemming.cpp\
    common/htmlhelpers.cpp\
    glossary/glossaryview.cpp\
    glossary/glossary.cpp\
    glossary/glossarywindow.cpp\
    mergemode/mergecatalog.cpp\
    mergemode/mergeview.cpp\
    alttransview.cpp\
    common/diff.cpp\
    project/project.cpp\
#    project/projectmodel.cpp
#    project/projectwidget.cpp
#    project/projecttab.cpp
#    project/poextractor.cpp
##    prefs/prefs.cpp
#    webquery/webqueryview.cpp
#    webquery/webquerycontroller.cpp
#    webquery/myactioncollectionview.cpp
#    tools/widgettextcaptureconfig.cpp
    filesearch/filesearchtab.cpp\
    tm/tmview.cpp\
    tm/tmscanapi.cpp\
    tm/jobs.cpp\
    tm/dbfilesmodel.cpp\
    tm/tmmanager.cpp\
    tm/tmtab.cpp\
    tm/qaview.cpp\
    tm/qamodel.cpp\
    catalog/phase.cpp\
    catalog/cmd.cpp\
    catalog/pos.cpp\
    catalog/catalog.cpp\
    catalog/catalogstring.cpp\
    catalog/gettextheader.cpp\
    catalog/gettext/gettextstorage.cpp\
    catalog/gettext/catalogitem.cpp\
    catalog/gettext/importplugin.cpp\
    catalog/gettext/gettextimport.cpp\
    catalog/gettext/gettextexport.cpp\
    catalog/xliff/xliffstorage.cpp\
    catalog/ts/tsstorage.cpp\
    nokde-stubs/prefs.cpp\
    nokde-stubs/lokalizesubwindowbase.cpp\
    nokde-stubs/welcometab.cpp

mac: CONFIG += objective_c
mac: OBJECTIVE_SOURCES += common/machelpers.mm
win32: SOURCES += common/winhelpers.cpp
unix:!mac: SOURCES += common/unixhelpers.cpp

HEADERS  += editortab.h\
    editorview.h\
    xlifftextedit.h\
    syntaxhighlighter.h\
##    completionstorage.h\
    phaseswindow.h\
    noteeditor.h\
    msgctxtview.h\
    binunitsview.h\
    cataloglistview/cataloglistview.h\
    cataloglistview/catalogmodel.h\
    common/headerviewmenu.h\
    common/fastsizehintitemdelegate.h\
    common/flowlayout.h\
    common/termlabel.h\
    common/languagelistmodel.h\
    common/stemming.h\
    glossary/glossaryview.h\
    glossary/glossary.h\
    glossary/glossarywindow.h\
    mergemode/mergecatalog.h\
    mergemode/mergeview.h\
    alttransview.h\
    project/project.h\
#    project/projectmodel.h
#    project/projectwidget.h
#    project/projecttab.h
#    project/poextractor.h
##    prefs/prefs.h
#    webquery/webqueryview.h
#    webquery/webquerycontroller.h
#    webquery/myactioncollectionview.h
#    tools/widgettextcaptureconfig.h
    filesearch/filesearchtab.h\
    tm/tmview.h\
    tm/tmscanapi.h\
    tm/jobs.h\
    tm/dbfilesmodel.h\
    tm/tmmanager.h\
    tm/tmtab.h\
    tm/qaview.h\
    tm/qamodel.h\
    catalog/phase.h\
    catalog/cmd.h\
    catalog/pos.h\
    catalog/catalog.h\
    catalog/catalogstring.h\
    nokde-stubs/prefs.h\
    nokde-stubs/prefs_lokalize.h\
    nokde-stubs/projectbase.h\
    nokde-stubs/projectlocal.h\
    nokde-stubs/kaboutdata.h\
    nokde-stubs/welcometab.h

FORMS    +=    glossary/termedit.ui\
    tm/queryoptions.ui\
    tm/managedatabases.ui\
    tm/dbparams.ui\
    filesearch/filesearchoptions.ui\
    filesearch/massreplaceoptions.ui\
    nokde-stubs/welcomewidget.ui

INCLUDEPATH += catalog catalog/gettext catalog/xliff catalog/ts cataloglistview mergemode glossary tm filesearch project common nokde-stubs

DEFINES += NOKDE

#unix: LIBS += -lhunspell

CONFIG += exceptions_off c++11 stl_off rtti_off

mac: QMAKE_LFLAGS += -dead_strip
mac: ICON = ../icons/osx/Lokalize.icns
mac: QMAKE_INFO_PLIST = ../icons/osx/Info.plist
mac: QMAKE_POST_LINK += cp -n ../icons/osx/LokalizePo*.icns ../icons/osx/LokalizeXliff.icns Lokalize.app/Contents/Resources/



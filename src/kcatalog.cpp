/* This file is part of the KDE libraries
   Copyright (c) 2001 Hans Petter Bieker <bieker@kde.org>
   Copyright (c) 2012, 2013 Chusslove Illich <caslav.ilic@gmx.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <stdlib.h>
#include <locale.h>
#include "gettext.h"

#include <qstandardpaths.h>
#include <QByteArray>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QSet>
#include <QStringList>

#include <kcatalog_p.h>

// not defined on win32 :(
#ifdef _WIN32
#ifndef LC_MESSAGES
#define LC_MESSAGES 42
#endif
#endif

#if defined(__USE_GNU_GETTEXT)
extern "C" int Q_DECL_IMPORT _nl_msg_cat_cntr;
#endif

static char *langenv = 0;
static const int langenvMaxlen = 42;
// = "LANGUAGE=" + 32 chars for language code + terminating zero

class KCatalogStaticData
{
public:
    KCatalogStaticData() {}

    QMutex mutex;
};

Q_GLOBAL_STATIC(KCatalogStaticData, catalogStaticData)

class KCatalogPrivate
{
public:
    KCatalogPrivate();

    QByteArray domain;
    QByteArray language;
    QByteArray localeDir;

    QByteArray systemLanguage;
    bool bindDone;

    static QByteArray currentLanguage;

    void setupGettextEnv();
    void resetSystemLanguage();
};

KCatalogPrivate::KCatalogPrivate()
    : bindDone(false)
{}

QByteArray KCatalogPrivate::currentLanguage;

KCatalog::KCatalog(const QByteArray &domain, const QString &language_)
    : d(new KCatalogPrivate)
{
    d->domain = domain;
    d->language = QFile::encodeName(language_);
    d->localeDir = QFile::encodeName(catalogLocaleDir(domain, language_));

    if (!d->localeDir.isEmpty()) {
        // Always get translations in UTF-8, regardless of user's environment.
        bind_textdomain_codeset(d->domain, "UTF-8");

        // Invalidate current language, to trigger binding at next translate call.
        KCatalogPrivate::currentLanguage.clear();

        if (!langenv) {
            // Call putenv only here, to initialize LANGUAGE variable.
            // Later only change langenv to what is currently needed.
            langenv = new char[langenvMaxlen];
            QByteArray baselang = qgetenv("LANGUAGE");
            qsnprintf(langenv, langenvMaxlen, "LANGUAGE=%s", baselang.constData());
            putenv(langenv);
        }
    }
}

KCatalog::~KCatalog()
{
    delete d;
}

QString KCatalog::catalogLocaleDir(const QByteArray &domain,
                                   const QString &language)
{
    QString relpath = QStringLiteral("%1/LC_MESSAGES/%2.mo")
                      .arg(language, QFile::decodeName(domain));
    QString file = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                          QStringLiteral("locale/") + relpath);
    QString localeDir;
    if (file.isEmpty()) {
        localeDir = QString();
    } else {
        // Path of the locale/ directory must be returned.
        localeDir = QFileInfo(file.left(file.size() - relpath.size())).absolutePath();
    }
    return localeDir;
}

QSet<QString> KCatalog::availableCatalogLanguages(const QByteArray &domain_)
{
    QString domain = QFile::decodeName(domain_);
    QStringList localeDirPaths = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                 QStringLiteral("locale"),
                                 QStandardPaths::LocateDirectory);
    QSet<QString> availableLanguages;
    foreach (const QString &localDirPath, localeDirPaths) {
        QDir localeDir(localDirPath);
        QStringList languages = localeDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
        foreach (const QString &language, languages) {
            QString relPath = QStringLiteral("%1/LC_MESSAGES/%2.mo")
                              .arg(language, domain);
            if (localeDir.exists(relPath)) {
                availableLanguages.insert(language);
            }
        }
    }
    return availableLanguages;
}

void KCatalogPrivate::setupGettextEnv()
{
    // Point Gettext to current language, recording system value for recovery.
    systemLanguage = qgetenv("LANGUAGE");
    if (systemLanguage != language) {
        // putenv has been called in the constructor,
        // it is enough to change the string set there.
        qsnprintf(langenv, langenvMaxlen, "LANGUAGE=%s", language.constData());
    }

    // Rebind text domain if language actually changed from the last time,
    // as locale directories may differ for different languages of same catalog.
    if (language != currentLanguage || !bindDone) {

        currentLanguage = language;
        bindDone = true;

        //qDebug() << "bindtextdomain" << domain << localeDir;
        bindtextdomain(domain, localeDir);

        // Magic to make sure GNU Gettext doesn't use stale cached translation
        // from previous language.
#if defined(__USE_GNU_GETTEXT)
        ++_nl_msg_cat_cntr;
#endif
    }
}

void KCatalogPrivate::resetSystemLanguage()
{
    if (language != systemLanguage) {
        qsnprintf(langenv, langenvMaxlen, "LANGUAGE=%s", systemLanguage.constData());
    }
}

QString KCatalog::translate(const QByteArray &msgid) const
{
    if (!d->localeDir.isEmpty()) {
        QMutexLocker locker(&catalogStaticData()->mutex);
        d->setupGettextEnv();
        const char *msgstr = dgettext(d->domain.constData(), msgid.constData());
        d->resetSystemLanguage();
        return msgstr != msgid
               ? QString::fromUtf8(msgstr)
               : QString();
    } else {
        return QString();
    }
}

QString KCatalog::translate(const QByteArray &msgctxt,
                            const QByteArray &msgid) const
{
    if (!d->localeDir.isEmpty()) {
        QMutexLocker locker(&catalogStaticData()->mutex);
        d->setupGettextEnv();
        const char *msgstr = dpgettext_expr(d->domain.constData(), msgctxt.constData(), msgid.constData());
        d->resetSystemLanguage();
        return   msgstr != msgid
               ? QString::fromUtf8(msgstr)
               : QString();
    } else {
        return QString();
    }
}

QString KCatalog::translate(const QByteArray &msgid,
                            const QByteArray &msgid_plural,
                            qulonglong n) const
{
    if (!d->localeDir.isEmpty()) {
        QMutexLocker locker(&catalogStaticData()->mutex);
        d->setupGettextEnv();
        const char *msgstr = dngettext(d->domain.constData(), msgid.constData(), msgid_plural.constData(), n);
        d->resetSystemLanguage();
        return   msgstr != msgid && msgstr != msgid_plural
               ? QString::fromUtf8(msgstr)
               : QString();
    } else {
        return QString();
    }
}

QString KCatalog::translate(const QByteArray &msgctxt,
                            const QByteArray &msgid,
                            const QByteArray &msgid_plural,
                            qulonglong n) const
{
    if (!d->localeDir.isEmpty()) {
        QMutexLocker locker(&catalogStaticData()->mutex);
        d->setupGettextEnv();
        const char *msgstr = dnpgettext_expr(d->domain.constData(), msgctxt.constData(), msgid.constData(), msgid_plural.constData(), n);
        d->resetSystemLanguage();
        return   msgstr != msgid && msgstr != msgid_plural
               ? QString::fromUtf8(msgstr)
               : QString();
    } else {
        return QString();
    }
}


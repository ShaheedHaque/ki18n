/*  This file is part of the KDE libraries
    Copyright (C) 2006 Chusslove Illich <caslav.ilic@gmx.net>

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

#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QTest>

#include <KLocalizedContext>

class KI18nDeclarativeTest : public QObject
{
Q_OBJECT

private Q_SLOTS:
    void testLocalizedContext() {
        KLocalizedContext ctx;
        QUrl input = QUrl::fromLocalFile(QFINDTESTDATA("test.qml"));

        QQmlEngine engine;
        engine.rootContext()->setContextObject(&ctx);
        QQmlComponent component(&engine, input, QQmlComponent::PreferSynchronous);
        QObject *object = component.create();

        QVERIFY(object);
        QVERIFY(!component.isLoading());
        QCOMPARE(object->property("testString").toString(), QStringLiteral("Awesome"));
    }
};

QTEST_MAIN(KI18nDeclarativeTest)

#include "ki18ndeclarativetest.moc"

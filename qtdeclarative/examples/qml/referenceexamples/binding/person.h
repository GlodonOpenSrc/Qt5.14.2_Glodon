/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef PERSON_H
#define PERSON_H

#include <QObject>
#include <QColor>

class ShoeDescription : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int size READ size WRITE setSize NOTIFY shoeChanged)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY shoeChanged)
    Q_PROPERTY(QString brand READ brand WRITE setBrand NOTIFY shoeChanged)
    Q_PROPERTY(qreal price READ price WRITE setPrice NOTIFY shoeChanged)
public:
    ShoeDescription(QObject *parent = nullptr);

    int size() const;
    void setSize(int);

    QColor color() const;
    void setColor(const QColor &);

    QString brand() const;
    void setBrand(const QString &);

    qreal price() const;
    void setPrice(qreal);
signals:
    void shoeChanged();

private:
    int m_size;
    QColor m_color;
    QString m_brand;
    qreal m_price;
};

class Person : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
// ![0]
    Q_PROPERTY(ShoeDescription *shoe READ shoe CONSTANT)
// ![0]
public:
    Person(QObject *parent = nullptr);

    QString name() const;
    void setName(const QString &);

    ShoeDescription *shoe();
signals:
    void nameChanged();

private:
    QString m_name;
    ShoeDescription m_shoe;
};

class Boy : public Person
{
    Q_OBJECT
public:
    Boy(QObject * parent = nullptr);
};

class Girl : public Person
{
    Q_OBJECT
public:
    Girl(QObject * parent = nullptr);
};

#endif // PERSON_H

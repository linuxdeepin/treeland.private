// Copyright (C) 2024 Yicheng Zhong <zhongyicheng@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QJSValue>
#include <QQuickItem>
#include <QSortFilterProxyModel>

class FilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(FilterProxyModel)
public:
    explicit FilterProxyModel(QObject *parent = nullptr);
    Q_PROPERTY(QJSValue filterAcceptsRow READ filterAcceptsRow WRITE setFilterAcceptsRow NOTIFY filterAcceptsRowChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    int count() const;

    void setFilterAcceptsRow(const QJSValue &val);

    QJSValue filterAcceptsRow() const { return m_filterAcceptsRow; }

    Q_INVOKABLE QVariantMap get(const int index) const;
    Q_INVOKABLE void invalidate();

Q_SIGNALS:
    void filterAcceptsRowChanged();
    void countChanged();

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    void initConnections();

    QJSValue m_filterAcceptsRow;
};

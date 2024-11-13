// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "itemselector.h"

#include <private/qquickitem_p.h>

#include <woutputitem.h>
#include <woutputrenderwindow.h>
#include <wsurfaceitem.h>

WAYLIB_SERVER_USE_NAMESPACE

ItemSelector::ItemSelector(QQuickItem *parent)
    : QQuickItem(parent)
{
    setAcceptHoverEvents(true);
    setFocus(false);
    setFocusPolicy(Qt::NoFocus);
    setKeepMouseGrab(true);
    QQuickItemPrivate::get(this)->anchors()->setFill(parentItem());
}

ItemSelector::~ItemSelector() { }

QRectF ItemSelector::selectionRegion() const
{
    return m_selectionRegion;
}

void ItemSelector::setSelectionRegion(const QRectF &newSelectionRegion)
{
    if (m_selectionRegion == newSelectionRegion)
        return;
    m_selectionRegion = newSelectionRegion;
    Q_EMIT selectionRegionChanged();
}

QQuickItem *ItemSelector::hoveredItem() const
{
    return m_hoveredItem;
}

WOutputItem *ItemSelector::outputItem() const
{
    return m_outputItem;
}

void ItemSelector::setHoveredItem(QQuickItem *newHoveredItem)
{
    if (m_hoveredItem == newHoveredItem)
        return;
    m_hoveredItem = newHoveredItem;
    Q_EMIT hoveredItemChanged();
}

ItemSelector::ItemTypes ItemSelector::selectionTypeHint() const
{
    return m_selectionTypeHint;
}

void ItemSelector::setSelectionTypeHint(ItemTypes newSelectionTypeHint)
{
    if (m_selectionTypeHint == newSelectionTypeHint)
        return;
    m_selectionTypeHint = newSelectionTypeHint;
    updateSelectableItems();
    Q_EMIT selectionTypeHintChanged();
}

void ItemSelector::updateSelectableItems()
{
    if (!window())
        return;
    auto renderWindow = qobject_cast<WOutputRenderWindow *>(window());
    m_selectableItems = WOutputRenderWindow::paintOrderItemList(
        renderWindow->contentItem(),
        [this](QQuickItem *item) {
            if (auto viewport = qobject_cast<WOutputItem *>(item)) {
                m_outputItems.append(viewport);
            }
            if (auto viewport = qobject_cast<WOutputItem *>(item)
                    && m_selectionTypeHint.testFlag(ItemType::Output)) {
                return true;
            } else if (auto surfaceItem = qobject_cast<WSurfaceItemContent *>(item)
                           && m_selectionTypeHint.testFlag(ItemType::Surface)) {
                return true;
            } else if (auto surfaceWrapper = qobject_cast<WSurfaceItem *>(item)
                           && m_selectionTypeHint.testFlag(ItemType::Window)) {
                return true;
            } else {
                return false;
            }
        });
}

void ItemSelector::hoverMoveEvent(QHoverEvent *event)
{
    checkHoveredItem(event->position());
}

void ItemSelector::checkHoveredItem(QPointF pos)
{
    decltype(m_selectableItems.crbegin()) it;
    for (it = m_selectableItems.crbegin(); it != m_selectableItems.crend(); it++) {
        auto itemRect = (*it)->mapRectToItem(this, (*it)->boundingRect());
        if (itemRect.contains(pos)) {
            setHoveredItem(*it);
            setSelectionRegion(itemRect);
            break;
        }
    }
    if (it == m_selectableItems.crend()) {
        setHoveredItem(nullptr);
        setSelectionRegion({});
    }
    for (const auto &item : std::as_const(m_outputItems)) {
        auto itemRect = item->mapRectToItem(this, item->boundingRect());
        if (itemRect.contains(pos)) {
            m_outputItem = item;
            break;
        }
    }
}

void ItemSelector::itemChange(ItemChange change, const ItemChangeData &data)
{
    switch (change) {
    case ItemParentHasChanged:
        QQuickItemPrivate::get(this)->anchors()->setFill(data.item);
        break;
    case ItemSceneChange:
        if (data.window)
            updateSelectableItems();
        break;
    default:
        break;
    }
    QQuickItem::itemChange(change, data);
}

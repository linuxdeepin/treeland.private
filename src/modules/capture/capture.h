// Copyright (C) 2024 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "impl/capturev1impl.h"
#include "itemselector.h"
#include "surfacecontainer.h"

#include <wglobal.h>
#include <woutput.h>
#include <wserver.h>
#include <wxdgsurface.h>
#include <wxdgsurfaceitem.h>

#include <QAbstractListModel>
#include <QPainter>
#include <QPointer>
#include <QQuickPaintedItem>
#include <QRect>

extern "C" {
#include <wlr/types/wlr_buffer.h>
}

WAYLIB_SERVER_BEGIN_NAMESPACE
class WOutputRenderWindow;
class WOutputViewport;
class WToplevelSurface;
WAYLIB_SERVER_END_NAMESPACE

WAYLIB_SERVER_USE_NAMESPACE
class SurfaceWrapper;
class ItemSelector;

class CaptureSource : public QObject
{
    Q_OBJECT
public:
    enum CaptureSourceType
    {
        Output = 0x1,
        Window = 0x2,
        Region = 0x4,
        Surface = 0x8,
    };
    Q_FLAG(CaptureSourceType)
    Q_DECLARE_FLAGS(CaptureSourceHint, CaptureSourceType)

    CaptureSource(WTextureProviderProvider *textureProvider, QObject *parent);

Q_SIGNALS:
    void ready();

public:
    bool valid() const;
    QImage image() const;

    void createImage();

    /**
     * @brief DMA buffer of source, there are three cases
     * 1. output - output's dma buffer
     * 2. window - window's dma buffer
     * 3. region - output's dma buffer
     *
     * @return QW_NAMESPACE::QWBuffer*
     */
    virtual QW_NAMESPACE::qw_buffer *sourceDMABuffer() = 0;

    /**
     * @brief copyBuffer render captured contents to a buffer
     * @param buffer buffer prepared by client
     */
    void copyBuffer(QW_NAMESPACE::qw_buffer *buffer);

    // Capture area relative to the whole viewport
    virtual QRect captureRegion() = 0;

    virtual CaptureSourceType sourceType() = 0;

protected:
    friend QDebug operator<<(QDebug debug, CaptureSource &captureSource);
    QImage m_image;
    WTextureProviderProvider *const m_provider;
};

#define CaptureSource_iid "org.deepin.treeland.CaptureSource"
Q_DECLARE_INTERFACE(CaptureSource, CaptureSource_iid)

class CaptureContextV1;

class CaptureContextModel : public QAbstractListModel
{
    Q_OBJECT
public:
    CaptureContextModel(QObject *parent = nullptr);

    enum CaptureContextRole
    {
        ContextRole = Qt::UserRole + 1
    };
    Q_ENUM(CaptureContextRole)
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void addContext(CaptureContextV1 *context);
    void removeContext(CaptureContextV1 *context);

private:
    QList<CaptureContextV1 *> m_captureContexts;
};

class CaptureContextV1 : public QObject
{
    Q_OBJECT
    QML_UNCREATABLE("Only created in c++")
    Q_PROPERTY(WSurface *mask READ mask NOTIFY selectInfoReady FINAL)
    Q_PROPERTY(bool freeze READ freeze NOTIFY selectInfoReady FINAL)
    Q_PROPERTY(bool withCursor READ withCursor NOTIFY selectInfoReady FINAL)
    Q_PROPERTY(CaptureSource::CaptureSourceHint sourceHint READ sourceHint NOTIFY selectInfoReady FINAL)
    Q_PROPERTY(CaptureSource *source READ source WRITE setSource NOTIFY sourceChanged FINAL)
public:
    CaptureSource *source() const;
    void setSource(CaptureSource *source);

    WSurface *mask() const;
    bool freeze() const;
    bool withCursor() const;
    CaptureSource::CaptureSourceHint sourceHint() const;
    QPointer<treeland_capture_session_v1> session() const;
    QPointer<CaptureSource> captureSource() const;
    QPointer<WOutputRenderWindow> outputRenderWindow() const;

public:
    enum SourceFailure
    {
        SelectorBusy,
        Other,
    };
    Q_ENUM(SourceFailure)

    CaptureContextV1(treeland_capture_context_v1 *h,
                     WOutputRenderWindow *outputRenderWindow,
                     QObject *parent = nullptr);
    void sendSourceFailed(SourceFailure failure);

    inline bool hintType(CaptureSource::CaptureSourceType type)
    {
        return sourceHint().testFlag(type);
    }

Q_SIGNALS:
    void sourceChanged();
    void finishSelect();
    void selectInfoReady();

private:
    void onSelectSource();
    void onCapture(treeland_capture_frame_v1 *frame);
    void onCreateSession(treeland_capture_session_v1 *session);
    void handleFrameCopy(QW_NAMESPACE::qw_buffer *buffer);
    void handleSessionStart();
    void handleRenderEnd();

    treeland_capture_context_v1 *const m_handle;
    CaptureSource *m_captureSource{ nullptr };
    QPointer<treeland_capture_frame_v1> m_frame{ nullptr };
    QPointer<treeland_capture_session_v1> m_session{ nullptr };
    const QPointer<WOutputRenderWindow> m_outputRenderWindow;
};
class CaptureSourceSelector;

class CaptureManagerV1
    : public QObject
    , public WServerInterface
{
    Q_OBJECT
    Q_PROPERTY(CaptureContextV1 *contextInSelection READ contextInSelection NOTIFY contextInSelectionChanged FINAL)

public:
    explicit CaptureManagerV1(QObject *parent = nullptr);

    CaptureContextModel *contextModel() const
    {
        return m_captureContextModel;
    }

    CaptureContextV1 *contextInSelection() const
    {
        return m_contextInSelection;
    }

    CaptureSourceSelector *selector() const
    {
        return m_selector;
    }

    void setSelector(CaptureSourceSelector *selector);

    WOutputRenderWindow *outputRenderWindow() const;
    void setOutputRenderWindow(WOutputRenderWindow *renderWindow);
    QByteArrayView interfaceName() const override;
    QPointer<WToplevelSurface> maskShellSurface() const;
    QPointer<SurfaceWrapper> maskSurfaceWrapper() const;

Q_SIGNALS:
    void contextInSelectionChanged();
    void newCaptureContext(CaptureContextV1 *context);
    void selectorChanged();

protected:
    void create(WServer *server) override;
    void destroy(WServer *server) override;
    wl_global *global() const override;

private Q_SLOTS:
    void onCaptureContextSelectSource();
    void clearContextInSelection(CaptureContextV1 *context);
    void freezeAllCapturedSurface(bool freeze, WAYLIB_SERVER_NAMESPACE::WSurface *maskItem);
    void handleContextBeforeDestroy(CaptureContextV1 *context);

private:
    treeland_capture_manager_v1 *m_manager;
    CaptureContextModel *m_captureContextModel;
    CaptureContextV1 *m_contextInSelection;
    WOutputRenderWindow *m_outputRenderWindow;
    QPointF m_frozenCursorPos;
    QPointer<WToplevelSurface> m_maskShellSurface;
    QPointer<SurfaceWrapper> m_maskSurfaceWrapper;
    CaptureSourceSelector *m_selector;
};

class CaptureSourceSurface : public CaptureSource
{
    Q_OBJECT
public:
    explicit CaptureSourceSurface(WSurfaceItemContent *surfaceItemContent);
    QW_NAMESPACE::qw_buffer *sourceDMABuffer() override;
    QRect captureRegion() override;
    CaptureSourceType sourceType() override;

private:
    WSurfaceItemContent *const m_surfaceItemContent;
};

class CaptureSourceOutput : public CaptureSource
{
    Q_OBJECT
public:
    explicit CaptureSourceOutput(WOutputViewport *viewport);
    QW_NAMESPACE::qw_buffer *sourceDMABuffer() override;
    QRect captureRegion() override;
    CaptureSourceType sourceType() override;

private:
    WOutputViewport *const m_outputViewport;
};

class CaptureSourceRegion : public CaptureSource
{
    Q_OBJECT
public:
    CaptureSourceRegion(WOutputViewport *viewport, const QRect &region);
    QW_NAMESPACE::qw_buffer *sourceDMABuffer() override;
    QRect captureRegion() override;
    CaptureSourceType sourceType() override;

private:
    WOutputViewport *const m_outputViewport;
    QRect m_region;
};
class ToolBarModel;

class CaptureSourceSelector : public SurfaceContainer
{
    Q_OBJECT
    Q_PROPERTY(CaptureManagerV1* captureManager READ captureManager WRITE setCaptureManager NOTIFY captureManagerChanged REQUIRED FINAL)
    Q_PROPERTY(QRectF selectionRegion READ selectionRegion NOTIFY selectionRegionChanged FINAL)
    Q_PROPERTY(SelectionMode selectionMode READ selectionMode WRITE setSelectionMode NOTIFY selectionModeChanged FINAL)
    Q_PROPERTY(QQmlListProperty<QObject> contents READ contents CONSTANT DESIGNABLE false)
    Q_PROPERTY(ToolBarModel* toolBarModel READ toolBarModel CONSTANT FINAL)
    Q_CLASSINFO("DefaultProperty", "contents")
    QML_ELEMENT

public:
    enum class SelectionMode
    {
        SelectOutput,
        SelectWindow,
        SelectRegion
    };
    Q_ENUM(SelectionMode);
    explicit CaptureSourceSelector(QQuickItem *parent = nullptr);
    ~CaptureSourceSelector() override;
    CaptureManagerV1 *captureManager() const;
    void setCaptureManager(CaptureManagerV1 *newCaptureManager);

    SelectionMode selectionMode() const;
    void setSelectionMode(const SelectionMode &newSelectionMode);
    void doSetSelectionMode(const SelectionMode &newSelectionMode);
    static CaptureSource::CaptureSourceHint selectionModeHint(const SelectionMode &selectionMode);
    ItemSelector::ItemTypes selectionModeToItemTypes(const SelectionMode &selectionMode) const;
    QQmlListProperty<QObject> contents() const;

    inline CaptureSource::CaptureSourceHint captureSourceHint() const
    {
        return captureManager() ? captureManager()->contextInSelection()->sourceHint()
                                : CaptureSource::CaptureSourceHint();
    }

    ToolBarModel *toolBarModel() const;
    void doneSelection();

Q_SIGNALS:
    void hoveredItemChanged();
    void selectedSourceChanged();
    void captureManagerChanged();
    void selectionRegionChanged();
    void selectionModeChanged();

protected:
    void componentComplete() override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChange(ItemChange change, const ItemChangeData &data) override;

private:
    QQuickItem *hoveredItem() const;
    QRectF selectionRegion() const;
    void setSelectionRegion(const QRectF &newSelectionRegion);
    bool itemSelectionMode() const;
    void setItemSelectionMode(bool itemSelection);
    CaptureSource *selectedSource() const;
    void setSelectedSource(CaptureSource *newSelectedSource);
    void handleItemSelectorSelectionRegionChanged();
    WOutputRenderWindow *renderWindow() const;

    void updateCursorShape();

    QPointer<QQuickItem> m_internalContentItem{};
    QPointer<ItemSelector> m_itemSelector{};
    QPointer<SurfaceContainer> m_canvasContainer{};
    CaptureSource *m_selectedSource{ nullptr };
    QList<QPointer<QQuickItem>> m_selectableItems{};
    QPointer<CaptureManagerV1> m_captureManager{ nullptr };
    QRectF m_selectionRegion{};
    QPointF m_selectionAnchor{};
    bool m_itemSelectionMode{ true };
    SelectionMode m_selectionMode = SelectionMode::SelectRegion;
    bool m_doNotFinish{ false };
    QPointer<SurfaceContainer> m_savedContainer{};
    QPointer<SurfaceWrapper> m_canvas{};
    ToolBarModel *m_toolBarModel{ nullptr };
};

class ToolBarModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(uint count READ rowCount NOTIFY countChanged FINAL)
public:
    enum ToolBarRole
    {
        IconNameRole,
        SelectionModeRole
    };
    Q_ENUM(ToolBarRole)
    explicit ToolBarModel(CaptureSourceSelector *selector);
    void updateModel();
    CaptureSourceSelector *selector() const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
Q_SIGNALS:
    void countChanged();

private:
    QList<QPair<QString, CaptureSourceSelector::SelectionMode>> m_data;
};

// Copyright (C) 2023 JiDe Zhang <zhangjide@deepin.org>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "helper.h"

#include "capture.h"
#include "ddeshellmanagerv1.h"
#include "inputdevice.h"
#include "layersurfacecontainer.h"
#include "lockscreen.h"
#include "multitaskview.h"
#include "output.h"
#include "outputmanagement.h"
#include "personalizationmanager.h"
#include "qmlengine.h"
#include "rootsurfacecontainer.h"
#include "shortcutmanager.h"
#include "surfacecontainer.h"
#include "surfacewrapper.h"
#include "treelandconfig.h"
#include "virtualoutputmanager.h"
#include "wallpapercolor.h"
#include "workspace.h"

#include <WBackend>
#include <WForeignToplevel>
#include <WOutput>
#include <WServer>
#include <WSurfaceItem>
#include <WXdgOutput>
#include <wcursorshapemanagerv1.h>
#include <winputmethodhelper.h>
#include <winputpopupsurface.h>
#include <wlayershell.h>
#include <wlayersurface.h>
#include <woutputitem.h>
#include <woutputlayout.h>
#include <woutputmanagerv1.h>
#include <woutputrenderwindow.h>
#include <woutputviewport.h>
#include <wqmlcreator.h>
#include <wquickcursor.h>
#include <wrenderhelper.h>
#include <wseat.h>
#include <wsocket.h>
#include <wtoplevelsurface.h>
#include <wxdgshell.h>
#include <wxdgsurface.h>
#include <wxwayland.h>
#include <wxwaylandsurface.h>

#include <qwallocator.h>
#include <qwbackend.h>
#include <qwbuffer.h>
#include <qwcompositor.h>
#include <qwdatacontrolv1.h>
#include <qwdisplay.h>
#include <qwfractionalscalemanagerv1.h>
#include <qwgammacontorlv1.h>
#include <qwlayershellv1.h>
#include <qwlogging.h>
#include <qwoutput.h>
#include <qwrenderer.h>
#include <qwscreencopyv1.h>
#include <qwsubcompositor.h>
#include <qwxwaylandsurface.h>

#include <QAction>
#include <QGuiApplication>
#include <QKeySequence>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>

#include <pwd.h>
#include <utility>

#define WLR_FRACTIONAL_SCALE_V1_VERSION 1

Helper *Helper::m_instance = nullptr;

Helper::Helper(QObject *parent)
    : WSeatEventFilter(parent)
    , m_renderWindow(new WOutputRenderWindow(this))
    , m_server(new WServer(this))
    , m_rootSurfaceContainer(new RootSurfaceContainer(m_renderWindow->contentItem()))
    , m_backgroundContainer(new LayerSurfaceContainer(m_rootSurfaceContainer))
    , m_bottomContainer(new LayerSurfaceContainer(m_rootSurfaceContainer))
    , m_workspace(new Workspace(m_rootSurfaceContainer))
    , m_lockScreen(new LockScreen(m_rootSurfaceContainer))
    , m_topContainer(new LayerSurfaceContainer(m_rootSurfaceContainer))
    , m_overlayContainer(new LayerSurfaceContainer(m_rootSurfaceContainer))
    , m_popupContainer(new SurfaceContainer(m_rootSurfaceContainer))
{
    setCurrentUserId(getuid());

    Q_ASSERT(!m_instance);
    m_instance = this;

    m_renderWindow->setColor(Qt::black);
    m_rootSurfaceContainer->setFlag(QQuickItem::ItemIsFocusScope, true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    m_rootSurfaceContainer->setFocusPolicy(Qt::StrongFocus);
#endif
    m_backgroundContainer->setZ(RootSurfaceContainer::BackgroundZOrder);
    m_bottomContainer->setZ(RootSurfaceContainer::BottomZOrder);
    m_workspace->setZ(RootSurfaceContainer::NormalZOrder);
    m_topContainer->setZ(RootSurfaceContainer::TopZOrder);
    m_overlayContainer->setZ(RootSurfaceContainer::OverlayZOrder);
    m_popupContainer->setZ(RootSurfaceContainer::PopupZOrder);
    m_lockScreen->setZ(RootSurfaceContainer::LockScreenZOrder);

    connect(m_renderWindow, &QQuickWindow::activeFocusItemChanged, this, [this]() {
        auto wrapper = keyboardFocusSurface();
        m_seat->setKeyboardFocusSurface(wrapper ? wrapper->surface() : nullptr);
    });

    connect(&TreelandConfig::ref(),
            &TreelandConfig::cursorThemeNameChanged,
            this,
            &Helper::cursorThemeChanged);
    connect(&TreelandConfig::ref(),
            &TreelandConfig::cursorSizeChanged,
            this,
            &Helper::cursorSizeChanged);
}

Helper::~Helper()
{
    for (auto s : m_rootSurfaceContainer->surfaces()) {
        if (auto c = s->container())
            c->removeSurface(s);
    }

    delete m_rootSurfaceContainer;
    Q_ASSERT(m_instance == this);
    m_instance = nullptr;
}

Helper *Helper::instance()
{
    return m_instance;
}

QmlEngine *Helper::qmlEngine() const
{
    return qobject_cast<QmlEngine *>(::qmlEngine(this));
}

WOutputRenderWindow *Helper::window() const
{
    return m_renderWindow;
}

Workspace *Helper::workspace() const
{
    return m_workspace;
}

void Helper::onOutputAdded(WOutput *output)
{
    allowNonDrmOutputAutoChangeMode(output);
    Output *o;
    if (m_mode == OutputMode::Extension || !m_rootSurfaceContainer->primaryOutput()) {
        o = Output::create(output, qmlEngine(), this);
        o->outputItem()->stackBefore(m_rootSurfaceContainer);
        // TODO: 应该让helper发出Output的信号，每个需要output的单元单独connect。
        m_rootSurfaceContainer->addOutput(o);
    } else if (m_mode == OutputMode::Copy) {
        o = Output::createCopy(output, m_rootSurfaceContainer->primaryOutput(), qmlEngine(), this);
    }
    m_outputList.append(o);
    enableOutput(output);
    m_outputManager->newOutput(output);
    m_lockScreen->addOutput(o);
}

void Helper::onOutputRemoved(WOutput *output)
{
    auto index = indexOfOutput(output);
    Q_ASSERT(index >= 0);
    const auto o = m_outputList.takeAt(index);
    m_outputManager->removeOutput(output);
    m_rootSurfaceContainer->removeOutput(o);
    m_lockScreen->removeOutput(o);
    delete o;
}

void Helper::setupSurfaceActiveWatcher(SurfaceWrapper *wrapper)
{
    Q_ASSERT_X(wrapper->container(), Q_FUNC_INFO, "Must setContainer at first!");

    connect(wrapper, &SurfaceWrapper::requestActive, this, [this, wrapper]() {
        if (wrapper->showOnWorkspace(m_workspace->currentIndex()))
            activateSurface(wrapper);
        else
            m_workspace->current()->pushActivedSurface(wrapper);
    });

    connect(wrapper, &SurfaceWrapper::requestDeactive, this, [this, wrapper]() {
        m_workspace->removeActivedSurface(wrapper);
        activateSurface(m_workspace->current()->latestActiveSurface());
    });

    connect(wrapper, &SurfaceWrapper::requestForceActive, this, [this, wrapper]() {
        if (wrapper->isMinimized())
            wrapper->requestCancelMinimize();

        if (!wrapper->surface()->mapped()) {
            qWarning() << "Can't activate unmapped surface: " << wrapper;
            return;
        }

        if (!wrapper->showOnWorkspace(m_workspace->currentIndex()))
            m_workspace->switchTo(wrapper->workspaceId());
        activateSurface(wrapper);
    });
}

void Helper::onXdgSurfaceAdded(WXdgSurface *surface)
{
    SurfaceWrapper *wrapper = nullptr;

    if (surface->isToplevel()) {
        wrapper = new SurfaceWrapper(qmlEngine(), surface, SurfaceWrapper::Type::XdgToplevel);
        m_foreignToplevel->addSurface(surface);
        m_treelandForeignToplevel->addSurface(wrapper);

        auto wSurface = surface->surface();
        if (m_ddeShellV1->isDdeShellSurface(wSurface)) {
            handleDdeShellSurfaceAdded(wSurface, wrapper);
        }
    } else {
        wrapper = new SurfaceWrapper(qmlEngine(), surface, SurfaceWrapper::Type::XdgPopup);
    }

    onSurfaceModeChanged(wrapper->surface(),
                         m_xdgDecorationManager->modeBySurface(surface->surface()));

    auto *attached =
        new PersonalizationAttached(wrapper->shellSurface(), m_personalization, wrapper);
    auto updateBlur = [wrapper, attached] {
        wrapper->setBlur(attached->backgroundType() == Personalization::BackgroundType::Blur);
    };
    connect(attached, &PersonalizationAttached::backgroundTypeChanged, wrapper, updateBlur);
    updateBlur();

    if (surface->isPopup()) {
        auto parent = surface->parentSurface();
        auto parentWrapper = m_rootSurfaceContainer->getSurface(parent);
        parentWrapper->addSubSurface(wrapper);
        m_popupContainer->addSurface(wrapper);
        wrapper->setOwnsOutput(parentWrapper->ownsOutput());
    } else {
        auto updateSurfaceWithParentContainer = [this, wrapper, surface] {
            if (wrapper->parentSurface())
                wrapper->parentSurface()->removeSubSurface(wrapper);
            if (wrapper->container())
                wrapper->container()->removeSurface(wrapper);

            if (auto parent = surface->parentSurface()) {
                auto parentWrapper = m_rootSurfaceContainer->getSurface(parent);
                auto container = qobject_cast<Workspace *>(parentWrapper->container());
                Q_ASSERT(container);
                parentWrapper->addSubSurface(wrapper);
                container->addSurface(wrapper, parentWrapper->workspaceId());
            } else {
                m_workspace->addSurface(wrapper);
            }
        };

        surface->safeConnect(&WXdgSurface::parentXdgSurfaceChanged,
                             this,
                             updateSurfaceWithParentContainer);
        updateSurfaceWithParentContainer();
        connect(wrapper,
                &SurfaceWrapper::requestShowWindowMenu,
                m_windowMenu,
                [this, wrapper](QPoint pos) {
                    QMetaObject::invokeMethod(m_windowMenu,
                                              "showWindowMenu",
                                              QVariant::fromValue(wrapper),
                                              QVariant::fromValue(pos));
                });
        setupSurfaceActiveWatcher(wrapper);
    }

    Q_ASSERT(wrapper->parentItem());
}

void Helper::onXdgSurfaceRemoved(WXdgSurface *surface)
{
    if (surface->isToplevel()) {
        m_foreignToplevel->removeSurface(surface);
        m_treelandForeignToplevel->removeSurface(m_rootSurfaceContainer->getSurface(surface));

        auto wSurface = surface->surface();
        if (m_ddeShellV1->isDdeShellSurface(wSurface)) {
            m_ddeShellV1->ddeShellSurfaceFromWSurface(wSurface)->destroy();
        }
    }

    m_rootSurfaceContainer->destroyForSurface(surface->surface());
}

void Helper::onLayerSurfaceAdded(WLayerSurface *surface)
{
    auto wrapper = new SurfaceWrapper(qmlEngine(), surface, SurfaceWrapper::Type::Layer);
    wrapper->setNoDecoration(true);
    wrapper->setSkipSwitcher(true);
    wrapper->setSkipDockPreView(true);
    wrapper->setSkipMutiTaskView(true);
    updateLayerSurfaceContainer(wrapper);

    auto *attached =
        new PersonalizationAttached(wrapper->shellSurface(), m_personalization, wrapper);
    auto updateBlur = [wrapper, attached] {
        wrapper->setBlur(attached->backgroundType() == Personalization::BackgroundType::Blur);
    };
    connect(attached, &PersonalizationAttached::backgroundTypeChanged, wrapper, updateBlur);
    updateBlur();

    connect(surface, &WLayerSurface::layerChanged, this, [this, wrapper] {
        updateLayerSurfaceContainer(wrapper);
    });

    setupSurfaceActiveWatcher(wrapper);
    Q_ASSERT(wrapper->parentItem());
}

void Helper::onLayerSurfaceRemoved(WLayerSurface *surface)
{
    m_rootSurfaceContainer->destroyForSurface(surface->surface());
}

void Helper::onInputPopupSurfaceV2Added(WInputPopupSurface *surface)
{
    auto wrapper = new SurfaceWrapper(qmlEngine(), surface, SurfaceWrapper::Type::InputPopup);
    auto parent = surface->parentSurface();
    auto parentWrapper = m_rootSurfaceContainer->getSurface(parent);
    parentWrapper->addSubSurface(wrapper);
    m_popupContainer->addSurface(wrapper);
    wrapper->setOwnsOutput(parentWrapper->ownsOutput());
    Q_ASSERT(wrapper->parentItem());
}

void Helper::onInputPopupSurfaceV2Removed(WInputPopupSurface *surface)
{
    m_rootSurfaceContainer->destroyForSurface(surface->surface());
}

void Helper::onSurfaceModeChanged(WSurface *surface, WXdgDecorationManager::DecorationMode mode)
{
    auto s = m_rootSurfaceContainer->getSurface(surface);
    if (!s)
        return;
    s->setNoDecoration(mode != WXdgDecorationManager::Server);
}

void Helper::setGamma(struct wlr_gamma_control_manager_v1_set_gamma_event *event)
{
    auto *qwOutput = qw_output::from(event->output);
    auto *wOutput = WOutput::fromHandle(qwOutput);
    size_t ramp_size = 0;
    uint16_t *r = nullptr, *g = nullptr, *b = nullptr;
    wlr_gamma_control_v1 *gamma_control = event->control;
    if (gamma_control) {
        ramp_size = gamma_control->ramp_size;
        r = gamma_control->table;
        g = gamma_control->table + gamma_control->ramp_size;
        b = gamma_control->table + 2 * gamma_control->ramp_size;
    }
    if (!wOutput->setGammaLut(ramp_size, r, g, b)) {
        qWarning() << "Failed to set gamma lut!";
        // TODO: use software impl it.
        qw_gamma_control_v1::from(gamma_control)->send_failed_and_destroy();
    }
}

void Helper::onOutputTestOrApply(qw_output_configuration_v1 *config, bool onlyTest)
{
    QList<WOutputState> states = m_outputManager->stateListPending();
    bool ok = true;
    for (auto state : std::as_const(states)) {
        WOutput *output = state.output;
        output->enable(state.enabled);
        if (state.enabled) {
            if (state.mode)
                output->setMode(state.mode);
            else
                output->setCustomMode(state.customModeSize, state.customModeRefresh);

            output->enableAdaptiveSync(state.adaptiveSyncEnabled);
            if (!onlyTest) {
                WOutputViewport *viewport = getOutput(output)->screenViewport();
                if (viewport) {
                    viewport->rotateOutput(state.transform);
                    viewport->setOutputScale(state.scale);
                    viewport->setX(state.x);
                    viewport->setY(state.y);
                }
            }
        }

        if (onlyTest)
            ok &= output->test();
        else
            ok &= output->commit();
    }
    m_outputManager->sendResult(config, ok);
}

void Helper::onDockPreview(std::vector<SurfaceWrapper *> surfaces,
                           WSurface *target,
                           QPoint pos,
                           ForeignToplevelV1::PreviewDirection direction)
{
    SurfaceWrapper *dockWrapper = m_rootSurfaceContainer->getSurface(target);
    Q_ASSERT(dockWrapper);

    QMetaObject::invokeMethod(m_dockPreview,
                              "show",
                              QVariant::fromValue(surfaces),
                              QVariant::fromValue(dockWrapper),
                              QVariant::fromValue(pos),
                              QVariant::fromValue(direction));
}

void Helper::onDockPreviewTooltip(QString tooltip,
                                  WSurface *target,
                                  QPoint pos,
                                  ForeignToplevelV1::PreviewDirection direction)
{
    SurfaceWrapper *dockWrapper = m_rootSurfaceContainer->getSurface(target);
    Q_ASSERT(dockWrapper);
    QMetaObject::invokeMethod(m_dockPreview,
                              "showTooltip",
                              QVariant::fromValue(tooltip),
                              QVariant::fromValue(dockWrapper),
                              QVariant::fromValue(pos),
                              QVariant::fromValue(direction));
}

void Helper::onShowDesktop()
{
    WindowManagementV1::DesktopState s = m_windowManagement->desktopState();
    if (m_showDesktop == s
        || (s != WindowManagementV1::DesktopState::Normal
            && s != WindowManagementV1::DesktopState::Show))
        return;

    m_showDesktop = s;
    const auto &surfaceList = workspace()->surfaces();
    for (auto surface : surfaceList) {
        if (s == WindowManagementV1::DesktopState::Normal && !surface->opacity()) {
            surface->setOpacity(1);
            surface->startShowAnimation(true);
        } else if (s == WindowManagementV1::DesktopState::Show && surface->opacity()) {
            surface->setOpacity(0);
            surface->startShowAnimation(false);
        }
    }
}

void Helper::init()
{
    auto engine = qmlEngine();
    engine->setContextForObject(m_renderWindow, engine->rootContext());
    engine->setContextForObject(m_renderWindow->contentItem(), engine->rootContext());
    m_rootSurfaceContainer->setQmlEngine(engine);

    m_seat = m_server->attach<WSeat>();
    m_seat->setEventFilter(this);
    m_seat->setCursor(m_rootSurfaceContainer->cursor());
    m_seat->setKeyboardFocusWindow(m_renderWindow);

    m_backend = m_server->attach<WBackend>();
    connect(m_backend, &WBackend::inputAdded, this, [this](WInputDevice *device) {
        m_seat->attachInputDevice(device);
        if (InputDevice::instance()->initTouchPad(device)) {
            if (m_windowGesture)
                m_windowGesture->addTouchpadSwipeGesture(SwipeGesture::Up, 3);

            if (m_multiTaskViewGesture) {
                m_multiTaskViewGesture->addTouchpadSwipeGesture(SwipeGesture::Up, 4);
                m_multiTaskViewGesture->addTouchpadSwipeGesture(SwipeGesture::Right, 4);
            }
        }
    });

    connect(m_backend, &WBackend::inputRemoved, this, [this](WInputDevice *device) {
        m_seat->detachInputDevice(device);
    });

    m_outputManager = m_server->attach<WOutputManagerV1>();
    connect(m_backend, &WBackend::outputAdded, this, &Helper::onOutputAdded);
    connect(m_backend, &WBackend::outputRemoved, this, &Helper::onOutputRemoved);

    auto *xdgShell = m_server->attach<WXdgShell>();
    m_foreignToplevel = m_server->attach<WForeignToplevel>(xdgShell);

    m_treelandForeignToplevel = m_server->attach<ForeignToplevelV1>();
    Q_ASSERT(m_treelandForeignToplevel);
    qmlRegisterSingletonInstance<ForeignToplevelV1>("Treeland.Protocols",
                                                    1,
                                                    0,
                                                    "ForeignToplevelV1",
                                                    m_treelandForeignToplevel);
    qRegisterMetaType<ForeignToplevelV1::PreviewDirection>();

    auto *layerShell = m_server->attach<WLayerShell>();
    auto *xdgOutputManager =
        m_server->attach<WXdgOutputManager>(m_rootSurfaceContainer->outputLayout());

    connect(xdgShell, &WXdgShell::surfaceAdded, this, &Helper::onXdgSurfaceAdded);
    connect(xdgShell, &WXdgShell::surfaceRemoved, this, &Helper::onXdgSurfaceRemoved);
    connect(layerShell, &WLayerShell::surfaceAdded, this, &Helper::onLayerSurfaceAdded);
    connect(layerShell, &WLayerShell::surfaceRemoved, this, &Helper::onLayerSurfaceRemoved);

    m_server->attach<PrimaryOutputV1>();
    m_wallpaperColorV1 = m_server->attach<WallpaperColorV1>();
    m_windowManagement = m_server->attach<WindowManagementV1>();
    m_server->attach<VirtualOutputV1>();
    m_shortcut = m_server->attach<ShortcutV1>();
    m_ddeShellV1 = m_server->attach<DDEShellManagerV1>();
    m_server->attach<CaptureManagerV1>();
    m_personalization = m_server->attach<PersonalizationV1>();
    m_personalization->setUserId(m_currentUserId);

    connect(
        this,
        &Helper::currentUserIdChanged,
        m_personalization,
        [this]() {
            m_personalization->setUserId(m_currentUserId);
        },
        Qt::QueuedConnection);

    connect(m_personalization,
            &PersonalizationV1::backgroundChanged,
            this,
            [this](const QString &output, bool isdark) {
                m_wallpaperColorV1->updateWallpaperColor(output, isdark);
            });

    connect(m_windowManagement,
            &WindowManagementV1::desktopStateChanged,
            this,
            &Helper::onShowDesktop);

    qmlRegisterUncreatableType<Personalization>("Treeland.Protocols",
                                                1,
                                                0,
                                                "Personalization",
                                                "Only for Enum");

    qmlRegisterUncreatableType<DDEShellHelper>("TreeLand.Protocols",
                                               1,
                                               0,
                                               "DDEShellHelper",
                                               "Only for attached");
    qmlRegisterUncreatableType<CaptureSource>("Treeland.Protocols",
                                              1,
                                              0,
                                              "CaptureSource",
                                              "An abstract class");
    qmlRegisterType<CaptureContextV1>("Treeland.Protocols", 1, 0, "CaptureContextV1");
    qmlRegisterType<CaptureSourceSelector>("Treeland.Protocols", 1, 0, "CaptureSourceSelector");

    m_server->start();
    m_renderer = WRenderHelper::createRenderer(m_backend->handle());
    if (!m_renderer) {
        qFatal("Failed to create renderer");
    }

    m_allocator = qw_allocator::autocreate(*m_backend->handle(), *m_renderer);
    m_renderer->init_wl_display(*m_server->handle());

    // free follow display
    m_compositor = qw_compositor::create(*m_server->handle(), 6, *m_renderer);
    qw_subcompositor::create(*m_server->handle());
    qw_screencopy_manager_v1::create(*m_server->handle());
    m_renderWindow->init(m_renderer, m_allocator);

    // for clipboard
    qw_data_control_manager_v1::create(*m_server->handle());

    // for xwayland
    auto *xwaylandOutputManager =
        m_server->attach<WXdgOutputManager>(m_rootSurfaceContainer->outputLayout());
    xwaylandOutputManager->setScaleOverride(1.0);

    m_xwayland = createXWayland();

    m_inputMethodHelper = new WInputMethodHelper(m_server, m_seat);

    connect(m_inputMethodHelper,
            &WInputMethodHelper::inputPopupSurfaceV2Added,
            this,
            &Helper::onInputPopupSurfaceV2Added);
    connect(m_inputMethodHelper,
            &WInputMethodHelper::inputPopupSurfaceV2Removed,
            this,
            &Helper::onInputPopupSurfaceV2Removed);

    m_xdgDecorationManager = m_server->attach<WXdgDecorationManager>();
    connect(m_xdgDecorationManager,
            &WXdgDecorationManager::surfaceModeChanged,
            this,
            &Helper::onSurfaceModeChanged);

    bool freezeClientWhenDisable = false;
    m_socket = new WSocket(freezeClientWhenDisable);
    if (m_socket->autoCreate()) {
        m_server->addSocket(m_socket);
        Q_EMIT socketFileChanged();
    } else {
        delete m_socket;
        qCritical("Failed to create socket");
        return;
    }

    auto gammaControlManager = qw_gamma_control_manager_v1::create(*m_server->handle());
    connect(gammaControlManager,
            &qw_gamma_control_manager_v1::notify_set_gamma,
            this,
            &Helper::setGamma);

    connect(m_outputManager,
            &WOutputManagerV1::requestTestOrApply,
            this,
            &Helper::onOutputTestOrApply);

    m_server->attach<WCursorShapeManagerV1>();
    qw_fractional_scale_manager_v1::create(*m_server->handle(), WLR_FRACTIONAL_SCALE_V1_VERSION);
    qw_data_control_manager_v1::create(*m_server->handle());

    m_windowMenu = engine->createWindowMenu(this);
    m_dockPreview = engine->createDockPreview(m_renderWindow->contentItem());

    connect(m_treelandForeignToplevel,
            &ForeignToplevelV1::requestDockPreview,
            this,
            &Helper::onDockPreview);
    connect(m_treelandForeignToplevel,
            &ForeignToplevelV1::requestDockPreviewTooltip,
            this,
            &Helper::onDockPreviewTooltip);

    connect(m_treelandForeignToplevel,
            &ForeignToplevelV1::requestDockClose,
            m_dockPreview,
            [this]() {
                QMetaObject::invokeMethod(m_dockPreview, "close");
            });

    m_backend->handle()->start();

    qInfo() << "Listing on:" << m_socket->fullServerName();
}

bool Helper::socketEnabled() const
{
    return m_socket->isEnabled();
}

void Helper::setSocketEnabled(bool newEnabled)
{
    if (m_socket)
        m_socket->setEnabled(newEnabled);
    else
        qWarning() << "Can't set enabled for empty socket!";
}

void Helper::activateSurface(SurfaceWrapper *wrapper, Qt::FocusReason reason)
{
    if (!wrapper || wrapper->shellSurface()->hasCapability(WToplevelSurface::Capability::Activate))
        setActivatedSurface(wrapper);
    if (!wrapper || wrapper->shellSurface()->hasCapability(WToplevelSurface::Capability::Focus))
        reuqestKeyboardFocusForSurface(wrapper, reason);
}

RootSurfaceContainer *Helper::rootContainer() const
{
    return m_rootSurfaceContainer;
}

void Helper::fakePressSurfaceBottomRightToReszie(SurfaceWrapper *surface)
{
    auto position = surface->geometry().bottomRight();
    m_fakelastPressedPosition = position;
    m_seat->setCursorPosition(position);
    Q_EMIT surface->requestResize(Qt::BottomEdge | Qt::RightEdge);
}

bool Helper::beforeDisposeEvent(WSeat *seat, QWindow *, QInputEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        auto kevent = static_cast<QKeyEvent *>(event);
        if (QKeySequence(kevent->keyCombination()) == QKeySequence::Quit) {
            qApp->quit();
            return true;
        } else if (event->modifiers() == Qt::MetaModifier) {
            if (kevent->key() == Qt::Key_Right) {
                m_workspace->switchToNext();
                return true;
            } else if (kevent->key() == Qt::Key_Left) {
                m_workspace->switchToPrev();
                return true;
            } else if (kevent->key() == Qt::Key_S) {
                toggleMultitaskview();
                return true;
            } else if (kevent->key() == Qt::Key_L) {
                for (auto &&output : m_rootSurfaceContainer->outputs()) {
                    m_lockScreen->addOutput(output);
                }
                return true;
            }
        } else if (kevent->key() == Qt::Key_Alt) {
            if (m_taskSwitch.isNull()) {
                auto contentItem = window()->contentItem();
                auto output = rootContainer()->primaryOutput();
                m_taskSwitch = qmlEngine()->createTaskSwitcher(output, contentItem);
                m_taskSwitch->setZ(RootSurfaceContainer::OverlayZOrder);
            }
        } else if (kevent->key() == Qt::Key_Tab || kevent->key() == Qt::Key_Backtab) {
            if (event->modifiers().testFlag(Qt::AltModifier)) {
                if (event->modifiers() == Qt::AltModifier) {
                    QMetaObject::invokeMethod(m_taskSwitch, "next");
                    return true;
                } else if (event->modifiers() == (Qt::AltModifier | Qt::ShiftModifier)) {
                    QMetaObject::invokeMethod(m_taskSwitch, "previous");
                    return true;
                }
            }
        } else if (kevent->key() == Qt::Key_Space) {
            if (event->modifiers().testFlag(Qt::AltModifier)) {
                Q_EMIT m_activatedSurface->requestShowWindowMenu({ 0, 0 });
            }
        } else if (event->modifiers() == Qt::NoModifier) {
            if (kevent->key() == Qt::Key_Escape) {
                if (m_multitaskview) {
                    m_multitaskview->exit(nullptr);
                }
            }
        }
    }

    // handle shortcut
    if (event->type() == QEvent::KeyRelease) {
        if (m_currentUserId != -1) {
            auto kevent = static_cast<QKeyEvent *>(event);
            QKeySequence sequence(kevent->modifiers() | kevent->key());
            bool isFind = false;
            for (auto *action : m_shortcut->actions(m_currentUserId)) {
                if (action->shortcut() == sequence) {
                    isFind = true;
                    action->activate(QAction::Trigger);
                }
            }
            if (isFind) {
                return true;
            }
        }
    }

    // ShowDesktop : Meta + D
    if (auto e = static_cast<QKeyEvent *>(event)) {
        if (e->type() == QKeyEvent::KeyPress && e->key() == Qt::Key_D
            && e->modifiers() == Qt::MetaModifier) {
            if (m_showDesktop == WindowManagementV1::DesktopState::Normal)
                m_windowManagement->setDesktopState(WindowManagementV1::DesktopState::Show);
            else if (m_showDesktop == WindowManagementV1::DesktopState::Show)
                m_windowManagement->setDesktopState(WindowManagementV1::DesktopState::Normal);
            return true;
        }
    }

    // FIXME: only one meta key
    // QEvent::ShortcutOverride QKeySequence("Meta")
    // QEvent::KeyPress QKeySequence("Meta+Meta")
    // QEvent::KeyRelease QKeySequence("Meta")
    static QFlags<ShortcutV1::MetaKeyCheck> metaKeyChecks;
    if (auto e = static_cast<QKeyEvent *>(event)) {
        if (e->type() == QKeyEvent::ShortcutOverride && e->key() == Qt::Key_Meta) {
            metaKeyChecks |= ShortcutV1::MetaKeyCheck::ShortcutOverride;
        } else if (metaKeyChecks.testFlags(ShortcutV1::MetaKeyCheck::ShortcutOverride)
                   && e->type() == QKeyEvent::KeyPress && e->modifiers() == Qt::MetaModifier
                   && e->key() == Qt::Key_Meta) {
            metaKeyChecks |= ShortcutV1::MetaKeyCheck::KeyPress;
        } else if (metaKeyChecks.testFlags(ShortcutV1::MetaKeyCheck::ShortcutOverride
                                           | ShortcutV1::MetaKeyCheck::KeyPress)
                   && e->type() == QKeyEvent::KeyRelease && e->key() == Qt::Key_Meta) {
            metaKeyChecks |= ShortcutV1::MetaKeyCheck::KeyRelease;
        } else {
            metaKeyChecks = {};
        }

        if (metaKeyChecks.testFlags(ShortcutV1::MetaKeyCheck::ShortcutOverride
                                    | ShortcutV1::MetaKeyCheck::KeyPress
                                    | ShortcutV1::MetaKeyCheck::KeyRelease)) {
            metaKeyChecks = {}; // reset
            m_shortcut->triggerMetaKey(m_currentUserId);
        }
    }
    // FIXME: end

    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonRelease) {
        handleLeftButtonStateChanged(event);
    }

    if (event->type() == QEvent::Wheel) {
        handleWhellValueChanged(event);
    }

    if (event->type() == QEvent::KeyRelease) {
        auto kevent = static_cast<QKeyEvent *>(event);
        if (kevent->key() == Qt::Key_Alt) {
            QMetaObject::invokeMethod(m_taskSwitch, "exit");
        }
    }

    if (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonPress) {
        seat->cursor()->setVisible(true);
    } else if (event->type() == QEvent::TouchBegin) {
        seat->cursor()->setVisible(false);
    }

    doGesture(event);

    if (auto surface = m_rootSurfaceContainer->moveResizeSurface()) {
        // for move resize
        if (Q_LIKELY(event->type() == QEvent::MouseMove || event->type() == QEvent::TouchUpdate)) {
            auto cursor = seat->cursor();
            Q_ASSERT(cursor);
            QMouseEvent *ev = static_cast<QMouseEvent *>(event);

            auto ownsOutput = surface->ownsOutput();
            if (!ownsOutput) {
                m_rootSurfaceContainer->endMoveResize();
                return false;
            }

            auto lastPosition =
                m_fakelastPressedPosition.value_or(cursor->lastPressedOrTouchDownPosition());
            auto increment_pos = ev->globalPosition() - lastPosition;
            m_rootSurfaceContainer->doMoveResize(increment_pos);

            return true;
        } else if (event->type() == QEvent::MouseButtonRelease
                   || event->type() == QEvent::TouchEnd) {
            m_rootSurfaceContainer->endMoveResize();
            m_fakelastPressedPosition.reset();
        }
    }

    return false;
}

bool Helper::afterHandleEvent([[maybe_unused]] WSeat *seat,
                              WSurface *watched,
                              QObject *surfaceItem,
                              QObject *,
                              QInputEvent *event)
{
    if (event->isSinglePointEvent() && static_cast<QSinglePointEvent *>(event)->isBeginEvent()) {
        // surfaceItem is qml type: XdgSurfaceItem or LayerSurfaceItem
        auto toplevelSurface = qobject_cast<WSurfaceItem *>(surfaceItem)->shellSurface();
        if (!toplevelSurface)
            return false;
        Q_ASSERT(toplevelSurface->surface() == watched);

        auto surface = m_rootSurfaceContainer->getSurface(watched);
        activateSurface(surface, Qt::MouseFocusReason);
    }

    return false;
}

bool Helper::unacceptedEvent(WSeat *, QWindow *, QInputEvent *event)
{
    if (event->isSinglePointEvent()) {
        if (static_cast<QSinglePointEvent *>(event)->isBeginEvent()) {
            activateSurface(nullptr, Qt::OtherFocusReason);
        }
    }

    return false;
}

bool Helper::doGesture(QInputEvent *event)
{
    if (event->type() == QEvent::NativeGesture) {
        auto e = static_cast<WGestureEvent *>(event);
        switch (e->gestureType()) {
        case Qt::BeginNativeGesture:
            if (e->libInputGestureType() == WGestureEvent::WLibInputGestureType::SwipeGesture)
                InputDevice::instance()->processSwipeStart(e->fingerCount());
            break;
        case Qt::EndNativeGesture:
            if (e->libInputGestureType() == WGestureEvent::WLibInputGestureType::SwipeGesture) {
                if (e->cancelled())
                    InputDevice::instance()->processSwipeCancel();
                else
                    InputDevice::instance()->processSwipeEnd();
            }
            break;
        case Qt::PanNativeGesture:
            if (e->libInputGestureType() == WGestureEvent::WLibInputGestureType::SwipeGesture)
                InputDevice::instance()->processSwipeUpdate(e->delta());
        case Qt::ZoomNativeGesture:
        case Qt::SmartZoomNativeGesture:
        case Qt::RotateNativeGesture:
        case Qt::SwipeNativeGesture:
        default:
            break;
        }
    }
    return false;
}

void Helper::toggleMultitaskview()
{
    if (!m_multitaskview) {
        toggleOutputMenuBar(false);
        m_workspace->setSwitcherEnabled(false);
        m_multitaskview =
            qobject_cast<Multitaskview *>(qmlEngine()->createMultitaskview(rootContainer()));
        connect(m_multitaskview.data(), &Multitaskview::visibleChanged, this, [this] {
            if (!m_multitaskview->isVisible()) {
                m_multitaskview->deleteLater();
                toggleOutputMenuBar(true);
                m_workspace->setSwitcherEnabled(true);
            }
        });
        m_multitaskview->enter(Multitaskview::ShortcutKey);
    } else if (m_multitaskview && m_multitaskview->status() == Multitaskview::Exited) {
        m_multitaskview->enter(Multitaskview::ShortcutKey);
    } else {
        m_multitaskview->exit(nullptr);
    }
}

SurfaceWrapper *Helper::keyboardFocusSurface() const
{
    auto item = m_renderWindow->activeFocusItem();
    if (!item)
        return nullptr;
    auto surface = qobject_cast<WSurfaceItem *>(item->parent());
    if (!surface)
        return nullptr;
    return qobject_cast<SurfaceWrapper *>(surface->parent());
}

void Helper::reuqestKeyboardFocusForSurface(SurfaceWrapper *newActivate, Qt::FocusReason reason)
{
    auto *nowKeyboardFocusSurface = keyboardFocusSurface();
    if (nowKeyboardFocusSurface == newActivate)
        return;

    Q_ASSERT(!newActivate
             || newActivate->shellSurface()->hasCapability(WToplevelSurface::Capability::Focus));

    if (nowKeyboardFocusSurface && nowKeyboardFocusSurface->hasActiveCapability()) {
        if (newActivate) {
            if (nowKeyboardFocusSurface->shellSurface()->keyboardFocusPriority()
                > newActivate->shellSurface()->keyboardFocusPriority())
                return;
        } else {
            if (nowKeyboardFocusSurface->shellSurface()->keyboardFocusPriority() > 0)
                return;
        }
    }

    if (nowKeyboardFocusSurface)
        nowKeyboardFocusSurface->setFocus(false, reason);

    if (newActivate)
        newActivate->setFocus(true, reason);
}

SurfaceWrapper *Helper::activatedSurface() const
{
    return m_activatedSurface;
}

void Helper::setActivatedSurface(SurfaceWrapper *newActivateSurface)
{
    if (m_activatedSurface == newActivateSurface)
        return;

    if (newActivateSurface) {
        Q_ASSERT(newActivateSurface->showOnWorkspace(m_workspace->currentIndex()));

        newActivateSurface->stackToLast();
    }

    if (m_activatedSurface)
        m_activatedSurface->setActivate(false);

    if (newActivateSurface) {
        if (m_showDesktop == WindowManagementV1::DesktopState::Show)
            m_showDesktop = WindowManagementV1::DesktopState::Normal;

        newActivateSurface->setActivate(true);
        m_workspace->pushActivedSurface(newActivateSurface);
    }
    m_activatedSurface = newActivateSurface;
    Q_EMIT activatedSurfaceChanged();
}

void Helper::setCursorPosition(const QPointF &position)
{
    m_rootSurfaceContainer->endMoveResize();
    m_seat->setCursorPosition(position);
}

void Helper::allowNonDrmOutputAutoChangeMode(WOutput *output)
{
    output->safeConnect(&qw_output::notify_request_state,
                        this,
                        [this](wlr_output_event_request_state *newState) {
                            if (newState->state->committed & WLR_OUTPUT_STATE_MODE) {
                                auto output = qobject_cast<qw_output *>(sender());

                                if (newState->state->mode_type == WLR_OUTPUT_STATE_MODE_CUSTOM) {
                                    output->set_custom_mode(newState->state->custom_mode.width,
                                                            newState->state->custom_mode.height,
                                                            newState->state->custom_mode.refresh);
                                } else {
                                    output->set_mode(newState->state->mode);
                                }

                                output->commit();
                            }
                        });
}

void Helper::enableOutput(WOutput *output)
{
    // Enable on default
    auto qwoutput = output->handle();
    // Don't care for WOutput::isEnabled, must do WOutput::commit here,
    // In order to ensure trigger QWOutput::frame signal, WOutputRenderWindow
    // needs this signal to render next frame. Because QWOutput::frame signal
    // maybe emit before WOutputRenderWindow::attach, if no commit here,
    // WOutputRenderWindow will ignore this output on render.
    if (!qwoutput->property("_Enabled").toBool()) {
        qwoutput->setProperty("_Enabled", true);

        if (!qwoutput->handle()->current_mode) {
            auto mode = qwoutput->preferred_mode();
            if (mode)
                output->setMode(mode);
        }
        output->enable(true);
        bool ok = output->commit();
        Q_ASSERT(ok);
    }
}

int Helper::indexOfOutput(WOutput *output) const
{
    for (int i = 0; i < m_outputList.size(); i++) {
        if (m_outputList.at(i)->output() == output)
            return i;
    }
    return -1;
}

Output *Helper::getOutput(WOutput *output) const
{
    for (auto o : std::as_const(m_outputList)) {
        if (o->output() == output)
            return o;
    }
    return nullptr;
}

void Helper::addOutput()
{
    qobject_cast<qw_multi_backend *>(m_backend->handle())
        ->for_each_backend(
            [](wlr_backend *backend, void *) {
                if (auto x11 = qw_x11_backend::from(backend)) {
                    qw_output::from(x11->output_create());
                } else if (auto wayland = qw_wayland_backend::from(backend)) {
                    qw_output::from(wayland->output_create());
                }
            },
            nullptr);
}

void Helper::setOutputMode(OutputMode mode)
{
    if (m_outputList.length() < 2 || m_mode == mode)
        return;
    m_mode = mode;
    Q_EMIT outputModeChanged();
    for (int i = 0; i < m_outputList.size(); i++) {
        if (m_outputList.at(i) == m_rootSurfaceContainer->primaryOutput())
            continue;
        Output *o;
        if (mode == OutputMode::Copy) {
            o = Output::createCopy(m_outputList.at(i)->output(),
                                   m_rootSurfaceContainer->primaryOutput(),
                                   qmlEngine(),
                                   this);
            m_rootSurfaceContainer->removeOutput(m_outputList.at(i));
        } else if (mode == OutputMode::Extension) {
            o = Output::create(m_outputList.at(i)->output(), qmlEngine(), this);
            o->outputItem()->stackBefore(m_rootSurfaceContainer);
            m_rootSurfaceContainer->addOutput(o);
            enableOutput(o->output());
        }
        m_outputList.at(i)->deleteLater();
        m_outputList.replace(i, o);
    }
}

void Helper::setOutputProxy(Output *output) { }

void Helper::updateLayerSurfaceContainer(SurfaceWrapper *surface)
{
    auto layer = qobject_cast<WLayerSurface *>(surface->shellSurface());
    Q_ASSERT(layer);

    if (auto oldContainer = surface->container())
        oldContainer->removeSurface(surface);

    switch (layer->layer()) {
    case WLayerSurface::LayerType::Background:
        m_backgroundContainer->addSurface(surface);
        break;
    case WLayerSurface::LayerType::Bottom:
        m_bottomContainer->addSurface(surface);
        break;
    case WLayerSurface::LayerType::Top:
        m_topContainer->addSurface(surface);
        break;
    case WLayerSurface::LayerType::Overlay:
        m_overlayContainer->addSurface(surface);
        break;
    default:
        Q_UNREACHABLE_RETURN();
    }
}

int Helper::currentUserId() const
{
    return m_currentUserId;
}

void Helper::setCurrentUserId(int uid)
{
    if (m_currentUserId == uid)
        return;
    m_currentUserId = uid;
    Q_EMIT currentUserIdChanged();
}

float Helper::animationSpeed() const
{
    return m_animationSpeed;
}

void Helper::setAnimationSpeed(float newAnimationSpeed)
{
    if (qFuzzyCompare(m_animationSpeed, newAnimationSpeed))
        return;
    m_animationSpeed = newAnimationSpeed;
    Q_EMIT animationSpeedChanged();
}

Helper::OutputMode Helper::outputMode() const
{
    return m_mode;
}

void Helper::addSocket(WSocket *socket)
{
    m_server->addSocket(socket);
}

WXWayland *Helper::createXWayland()
{
    auto *xwayland = m_server->attach<WXWayland>(m_compositor, false);
    m_xwaylands.append(xwayland);
    xwayland->setSeat(m_seat);

    connect(xwayland, &WXWayland::surfaceAdded, this, [this, xwayland](WXWaylandSurface *surface) {
        surface->safeConnect(&qw_xwayland_surface::notify_associate, this, [this, surface] {
            auto wrapper = new SurfaceWrapper(qmlEngine(), surface, SurfaceWrapper::Type::XWayland);
            wrapper->setNoDecoration(false);
            m_foreignToplevel->addSurface(surface);
            m_treelandForeignToplevel->addSurface(wrapper);
            m_workspace->addSurface(wrapper);
            Q_ASSERT(wrapper->parentItem());
            connect(wrapper,
                    &SurfaceWrapper::requestShowWindowMenu,
                    m_windowMenu,
                    [this, wrapper](QPoint pos) {
                        QMetaObject::invokeMethod(m_windowMenu,
                                                  "showWindowMenu",
                                                  QVariant::fromValue(wrapper),
                                                  QVariant::fromValue(pos));
                    });
            setupSurfaceActiveWatcher(wrapper);
        });
        surface->safeConnect(&qw_xwayland_surface::notify_dissociate, this, [this, surface] {
            m_foreignToplevel->removeSurface(surface);
            m_treelandForeignToplevel->removeSurface(m_rootSurfaceContainer->getSurface(surface));
            m_rootSurfaceContainer->destroyForSurface(surface->surface());
        });
    });

    return xwayland;
}

void Helper::removeXWayland(WXWayland *xwayland)
{
    m_xwaylands.removeOne(xwayland);
    xwayland->safeDeleteLater();
}

WSocket *Helper::defaultWaylandSocket() const
{
    return m_socket;
}

WXWayland *Helper::defaultXWaylandSocket() const
{
    return m_xwayland;
}

PersonalizationV1 *Helper::personalization() const
{
    return m_personalization;
}

void Helper::toggleOutputMenuBar(bool show)
{
#ifdef QT_DEBUG
    for (const auto &output : rootContainer()->outputs()) {
        output->outputMenuBar()->setVisible(show);
    }
#endif
}

QString Helper::cursorTheme() const
{
    return TreelandConfig::ref().cursorThemeName();
}

QSize Helper::cursorSize() const
{
    return TreelandConfig::ref().cursorSize();
}

void Helper::seatSendStartDrag(WSeat *seat)
{
    Q_ASSERT(m_ddeShellV1 && seat);
    m_ddeShellV1->sendStartDrag(seat);
}

WSeat *Helper::seat() const
{
    return m_seat;
}

void Helper::showAllWindows(WorkspaceModel* model, bool show)
{
    auto surfaces = model->surfaces();

    foreach (auto window, surfaces) {
        window->setOpacity(show ? 1.0 : 0.0);
    }
}

void Helper::handleLeftButtonStateChanged(const QInputEvent *event)
{
    Q_ASSERT(m_ddeShellV1 && m_seat);
    const QMouseEvent *me = static_cast<const QMouseEvent *>(event);
    if (me->button() == Qt::LeftButton) {
        if (event->type() == QEvent::MouseButtonPress) {
            m_ddeShellV1->sendActiveIn(TREELAND_DDE_ACTIVE_V1_REASON_MOUSE, m_seat);
        } else {
            m_ddeShellV1->sendActiveOut(TREELAND_DDE_ACTIVE_V1_REASON_MOUSE, m_seat);
        }
    }
}

void Helper::handleWhellValueChanged(const QInputEvent *event)
{
    Q_ASSERT(m_ddeShellV1 && m_seat);
    const QWheelEvent *we = static_cast<const QWheelEvent *>(event);
    QPoint delta = we->angleDelta();
    if (delta.x() + delta.y() < 0) {
        m_ddeShellV1->sendActiveOut(TREELAND_DDE_ACTIVE_V1_REASON_WHEEL, m_seat);
    }
    if (delta.x() + delta.y() > 0) {
        m_ddeShellV1->sendActiveIn(TREELAND_DDE_ACTIVE_V1_REASON_WHEEL, m_seat);
    }
}

void Helper::handleDdeShellSurfaceAdded(WSurface *surface, SurfaceWrapper *wrapper)
{
    wrapper->setIsDdeShellSurface(true);
    auto ddeShellSurface = m_ddeShellV1->ddeShellSurfaceFromWSurface(surface);
    Q_ASSERT(ddeShellSurface);
    auto updateLayer = [ddeShellSurface, wrapper] {
        if (ddeShellSurface->m_role.value() == treeland_dde_shell_surface::Role::OVERLAY)
            wrapper->setSurfaceRole(SurfaceWrapper::SurfaceRole::Overlay);
    };

    if (ddeShellSurface->m_role.has_value())
        updateLayer();

    connect(ddeShellSurface, &treeland_dde_shell_surface::roleChanged, this, [updateLayer] {
        updateLayer();
    });

    if (ddeShellSurface->m_yOffset.has_value())
        wrapper->setAutoPlaceYOffset(ddeShellSurface->m_yOffset.value());

    connect(ddeShellSurface,
            &treeland_dde_shell_surface::yOffsetChanged,
            this,
            [wrapper](uint32_t offset) {
                wrapper->setAutoPlaceYOffset(offset);
            });

    if (ddeShellSurface->m_surfacePos.has_value())
        wrapper->setClientRequstPos(ddeShellSurface->m_surfacePos.value());

    connect(ddeShellSurface,
            &treeland_dde_shell_surface::positionChanged,
            this,
            [wrapper](QPoint pos) {
                wrapper->setClientRequstPos(pos);
            });

    if (ddeShellSurface->m_skipSwitcher.has_value())
        wrapper->setSkipSwitcher(ddeShellSurface->m_skipSwitcher.value());

    if (ddeShellSurface->m_skipDockPreView.has_value())
        wrapper->setSkipDockPreView(ddeShellSurface->m_skipDockPreView.value());

    if (ddeShellSurface->m_skipMutiTaskView.has_value())
        wrapper->setSkipMutiTaskView(ddeShellSurface->m_skipMutiTaskView.value());

    connect(ddeShellSurface,
            &treeland_dde_shell_surface::skipSwitcherChanged,
            this,
            [wrapper](bool skip) {
                wrapper->setSkipSwitcher(skip);
            });
    connect(ddeShellSurface,
            &treeland_dde_shell_surface::skipDockPreViewChanged,
            this,
            [wrapper](bool skip) {
                wrapper->setSkipDockPreView(skip);
            });
    connect(ddeShellSurface,
            &treeland_dde_shell_surface::skipMutiTaskViewChanged,
            this,
            [wrapper](bool skip) {
                wrapper->setSkipMutiTaskView(skip);
            });
}


// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "appearance_impl.h"

#include "personalization_manager_impl.h"
#include "treeland-personalization-manager-protocol.h"
#include "util.h"

#include <wayland-server-core.h>
#include <wayland-server.h>

static const struct treeland_personalization_appearance_context_v1_interface
    personalization_appearance_context_impl = {
        .set_round_corner_radius = dispatch_member_function<
            &personalization_appearance_context_v1::setRoundCornerRadius>(),
        .get_round_corner_radius =
            [](struct wl_client *client, struct wl_resource *resource) {
                Q_EMIT personalization_appearance_context_v1::fromResource(resource)
                    ->requestRoundCornerRadius();
            },
        .set_icon_theme =
            dispatch_member_function<&personalization_appearance_context_v1::setIconTheme>(),
        .get_icon_theme =
            [](struct wl_client *client, struct wl_resource *resource) {
                Q_EMIT personalization_appearance_context_v1::fromResource(resource)->requestIconTheme();
            },
        .set_active_color =
            dispatch_member_function<&personalization_appearance_context_v1::setActiveColor>(),
        .get_active_color =
            [](struct wl_client *client, struct wl_resource *resource) {
                Q_EMIT personalization_appearance_context_v1::fromResource(resource)->requestActiveColor();
            },
        .set_window_opacity =
            dispatch_member_function<&personalization_appearance_context_v1::setWindowOpacity>(),
        .get_window_opacity =
            [](struct wl_client *client, struct wl_resource *resource) {
                Q_EMIT personalization_appearance_context_v1::fromResource(resource)
                    ->requestWindowOpacity();
            },
        .set_window_theme_type =
            dispatch_member_function<&personalization_appearance_context_v1::setWindowThemeType>(),
        .get_window_theme_type =
            [](struct wl_client *client, struct wl_resource *resource) {
                Q_EMIT personalization_appearance_context_v1::fromResource(resource)
                    ->requestWindowThemeType();
            },
        .set_window_titlebar_height = dispatch_member_function<
            &personalization_appearance_context_v1::setWindowTitlebarHeight>(),
        .get_window_titlebar_height =
            [](struct wl_client *client, struct wl_resource *resource) {
                Q_EMIT personalization_appearance_context_v1::fromResource(resource)
                    ->requestWindowTitlebarHeight();
            },
        .destroy =
            []([[maybe_unused]] struct wl_client *client, struct wl_resource *resource) {
                wl_resource_destroy(resource);
            }
    };

personalization_appearance_context_v1 *personalization_appearance_context_v1::fromResource(
    struct wl_resource *resource)
{
    assert(wl_resource_instance_of(resource,
                                   &treeland_personalization_appearance_context_v1_interface,
                                   &personalization_appearance_context_impl));

    return static_cast<struct personalization_appearance_context_v1 *>(
        wl_resource_get_user_data(resource));
}

personalization_appearance_context_v1::personalization_appearance_context_v1(
    struct wl_client *client,
    struct wl_resource *manager_resource,
    uint32_t id)
    : QObject()
{
    auto *manager = treeland_personalization_manager_v1::from_resource(manager_resource);
    Q_ASSERT(manager);

    m_manager = manager;

    uint32_t version = wl_resource_get_version(manager_resource);
    struct wl_resource *resource =
        wl_resource_create(client,
                           &treeland_personalization_appearance_context_v1_interface,
                           version,
                           id);
    if (resource == NULL) {
        wl_resource_post_no_memory(manager_resource);
    }

    m_resource = resource;

    wl_resource_set_implementation(
        resource,
        &personalization_appearance_context_impl,
        this,
        [](struct wl_resource *resource) {
            auto *p = personalization_appearance_context_v1::fromResource(resource);
            Q_EMIT p->beforeDestroy();
            delete p;
            wl_list_remove(wl_resource_get_link(resource));
        });

    wl_list_insert(&manager->resources, wl_resource_get_link(resource));

    Q_EMIT manager->appearanceContextCreated(this);
}

personalization_appearance_context_v1::~personalization_appearance_context_v1()
{
    // wl_list_remove(wl_resource_get_link(m_resource));
}

void personalization_appearance_context_v1::setRoundCornerRadius(int32_t radius)
{
    if (m_radius == radius) {
        return;
    }

    m_radius = radius;

    sendState([radius](struct wl_resource *resource) {
        treeland_personalization_appearance_context_v1_send_round_corner_radius(resource, radius);
    });

    Q_EMIT roundCornerRadiusChanged();
}

void personalization_appearance_context_v1::setIconTheme(const char *theme_name)
{
    if (m_iconTheme == theme_name) {
        return;
    }

    m_iconTheme = theme_name;

    sendState([theme_name](struct wl_resource *resource) {
        treeland_personalization_appearance_context_v1_send_icon_theme(resource, theme_name);
    });

    Q_EMIT iconThemeChanged();
}

void personalization_appearance_context_v1::sendRoundCornerRadius(int32_t radius)
{
    treeland_personalization_appearance_context_v1_send_round_corner_radius(m_resource, radius);
}

void personalization_appearance_context_v1::sendIconTheme(const char *icon_theme)
{
    treeland_personalization_appearance_context_v1_send_icon_theme(m_resource, icon_theme);
}

void personalization_appearance_context_v1::setActiveColor(const char *color)
{
    if (m_activeColor == color) {
        return;
    }

    m_activeColor = color;

    sendState([color](struct wl_resource *resource) {
        treeland_personalization_appearance_context_v1_send_active_color(resource, color);
    });

    Q_EMIT activeColorChanged();
}

void personalization_appearance_context_v1::sendActiveColor(const char *color)
{
    treeland_personalization_appearance_context_v1_send_active_color(m_resource, color);
}

void personalization_appearance_context_v1::setWindowOpacity(uint32_t opacity)
{
    if (m_windowOpacity == opacity) {
        return;
    }

    m_windowOpacity = opacity;

    sendState([opacity](struct wl_resource *resource) {
        treeland_personalization_appearance_context_v1_send_window_opacity(resource, opacity);
    });

    Q_EMIT windowOpacityChanged();
}

void personalization_appearance_context_v1::sendWindowOpacity(uint32_t opacity)
{
    treeland_personalization_appearance_context_v1_send_window_opacity(m_resource, opacity);
}

void personalization_appearance_context_v1::setWindowThemeType(uint32_t type)
{
    if (m_windowThemeType == type) {
        return;
    }

    m_windowThemeType = static_cast<ThemeType>(type);

    sendState([type](struct wl_resource *resource) {
        treeland_personalization_appearance_context_v1_send_window_theme_type(resource, type);
    });

    Q_EMIT windowThemeTypeChanged();
}

void personalization_appearance_context_v1::sendWindowThemeType(uint32_t type)
{
    treeland_personalization_appearance_context_v1_send_window_theme_type(m_resource, type);
}

void personalization_appearance_context_v1::setWindowTitlebarHeight(uint32_t height)
{
    if (m_titlebarHeight == height) {
        return;
    }

    m_titlebarHeight = height;

    sendState([height](struct wl_resource *resource) {
        treeland_personalization_appearance_context_v1_send_window_titlebar_height(resource,
                                                                                   height);
    });

    Q_EMIT titlebarHeightChanged();
}

void personalization_appearance_context_v1::sendWindowTitlebarHeight(uint32_t height)
{
    treeland_personalization_appearance_context_v1_send_window_titlebar_height(m_resource,
                                                                               m_titlebarHeight);
}

void personalization_appearance_context_v1::sendState(
    std::function<void(struct wl_resource *)> func)
{
    if (idle_source) {
        m_callbacks.push_back(func);
        return;
    }

    idle_source = wl_event_loop_add_idle(
        m_manager->event_loop,
        [](void *data) {
            auto *context = static_cast<personalization_appearance_context_v1 *>(data);
            struct wl_resource *resource;
            struct wl_resource *tmp;
            wl_resource_for_each_safe(resource, tmp, &context->m_manager->resources)
            {
                if (wl_resource_instance_of(
                        resource,
                        &treeland_personalization_appearance_context_v1_interface,
                        &personalization_appearance_context_impl)) {
                    for (auto &&func : std::as_const(context->m_callbacks)) {
                        func(resource);
                    }
                }
            }

            context->idle_source = nullptr;
        },
        this);
}

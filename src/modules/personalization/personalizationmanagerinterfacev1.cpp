#include "personalizationmanagerinterface.h"
#include "qwayland-server-treeland-personalization-manager-v1.h"

#include <qwdisplay.h>

#define TREELAND_PERSONALIZATION_MANAGER_V1_VERSION 1

namespace Protocol::Personalization {
class ManagerInterfacePrivate
    : public QObject
    , public QtWaylandServer::treeland_personalization_manager_v1
{
    Q_OBJECT
    Q_DECLARE_PUBLIC(ManagerInterface)

    ManagerInterface *const q_ptr;

public:
    ManagerInterfacePrivate(ManagerInterface *q)
        : QObject(q)
        , q_ptr(q)
    {
    }

    ~ManagerInterfacePrivate() override = default;

    wl_global *global() const
    {
        return m_global;
    }

protected:
    void treeland_personalization_manager_v1_get_window_context(Resource *resource,
                                                                uint32_t id,
                                                                struct ::wl_resource *surface)
    {
        wl_resource_post_error(
            surface,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_manager_v1.get_window_context has not been implemented yet");
    }

    void treeland_personalization_manager_v1_get_wallpaper_context(Resource *resource, uint32_t id)
    {
        wl_resource_post_error(resource->handle,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "treeland_personalization_manager_v1.get_wallpaper_context has not "
                               "been implemented yet");
    }

    void treeland_personalization_manager_v1_get_cursor_context(Resource *resource, uint32_t id)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_manager_v1.get_cursor_context has not been implemented yet");
    }

    void treeland_personalization_manager_v1_get_font_context(Resource *resource, uint32_t id)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_manager_v1.get_font_context has not been implemented yet");
    }

    void treeland_personalization_manager_v1_get_appearance_context(Resource *resource, uint32_t id)
    {
        wl_resource_post_error(resource->handle,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "treeland_personalization_manager_v1.get_appearance_context has not "
                               "been implemented yet");
    }
};

class WallpaperInterfacePrivate
    : public QtWaylandServer::treeland_personalization_wallpaper_context_v1
{
public:
    ~WallpaperInterfacePrivate() override = default;

protected:
    void treeland_personalization_wallpaper_context_v1_set_fd(Resource *resource,
                                                              int32_t fd,
                                                              const QString &metadata)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_wallpaper_context_v1.set_fd has not been implemented yet");
    }

    void treeland_personalization_wallpaper_context_v1_set_identifier(Resource *resource,
                                                                      const QString &identifier)
    {
        wl_resource_post_error(resource->handle,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "treeland_personalization_wallpaper_context_v1.set_identifier has "
                               "not been implemented yet");
    }

    void treeland_personalization_wallpaper_context_v1_set_output(Resource *resource,
                                                                  const QString &output)
    {
        wl_resource_post_error(resource->handle,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "treeland_personalization_wallpaper_context_v1.set_output has not "
                               "been implemented yet");
    }

    void treeland_personalization_wallpaper_context_v1_set_on(Resource *resource, uint32_t options)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_wallpaper_context_v1.set_on has not been implemented yet");
    }

    void treeland_personalization_wallpaper_context_v1_set_isdark(Resource *resource,
                                                                  uint32_t isdark)
    {
        wl_resource_post_error(resource->handle,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "treeland_personalization_wallpaper_context_v1.set_isdark has not "
                               "been implemented yet");
    }

    void treeland_personalization_wallpaper_context_v1_commit(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_wallpaper_context_v1.commit has not been implemented yet");
    }

    void treeland_personalization_wallpaper_context_v1_get_metadata(Resource *resource)
    {
        wl_resource_post_error(resource->handle,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "treeland_personalization_wallpaper_context_v1.get_metadata has not "
                               "been implemented yet");
    }

    void treeland_personalization_wallpaper_context_v1_destroy(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_wallpaper_context_v1.destroy has not been implemented yet");
    }
};

class CursorInterfacePrivate : public QtWaylandServer::treeland_personalization_cursor_context_v1
{
public:
    ~CursorInterfacePrivate() override = default;

protected:
    void treeland_personalization_cursor_context_v1_set_theme(Resource *resource,
                                                              const QString &name)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_cursor_context_v1.set_theme has not been implemented yet");
    }

    void treeland_personalization_cursor_context_v1_get_theme(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_cursor_context_v1.get_theme has not been implemented yet");
    }

    void treeland_personalization_cursor_context_v1_set_size(Resource *resource, uint32_t size)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_cursor_context_v1.set_size has not been implemented yet");
    }

    void treeland_personalization_cursor_context_v1_get_size(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_cursor_context_v1.get_size has not been implemented yet");
    }

    void treeland_personalization_cursor_context_v1_commit(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_cursor_context_v1.commit has not been implemented yet");
    }

    void treeland_personalization_cursor_context_v1_destroy(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_cursor_context_v1.destroy has not been implemented yet");
    }
};

class WindowInterfacePrivate : public QtWaylandServer::treeland_personalization_window_context_v1
{
public:
    ~WindowInterfacePrivate() override = default;

protected:
    void treeland_personalization_window_context_v1_set_position(Resource *resource,
                                                                 int32_t x,
                                                                 int32_t y)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_window_context_v1.set_position has not been implemented yet");
    }

    void treeland_personalization_window_context_v1_get_position(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_window_context_v1.get_position has not been implemented yet");
    }

    void treeland_personalization_window_context_v1_set_size(Resource *resource,
                                                             int32_t width,
                                                             int32_t height)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_window_context_v1.set_size has not been implemented yet");
    }

    void treeland_personalization_window_context_v1_get_size(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_window_context_v1.get_size has not been implemented yet");
    }

    void treeland_personalization_window_context_v1_destroy(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_window_context_v1.destroy has not been implemented yet");
    }
};

class FontInterfacePrivate : public QtWaylandServer::treeland_personalization_font_context_v1
{
public:
    ~FontInterfacePrivate() override = default;

protected:
    void treeland_personalization_font_context_v1_set_font_size(Resource *resource, uint32_t size)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_font_context_v1.set_font_size has not been implemented yet");
    }

    void treeland_personalization_font_context_v1_get_font_size(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_font_context_v1.get_font_size has not been implemented yet");
    }

    void treeland_personalization_font_context_v1_set_font(Resource *resource,
                                                           const QString &font_name)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_font_context_v1.set_font has not been implemented yet");
    }

    void treeland_personalization_font_context_v1_get_font(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_font_context_v1.get_font has not been implemented yet");
    }

    void treeland_personalization_font_context_v1_set_monospace_font(Resource *resource,
                                                                     const QString &font_name)
    {
        wl_resource_post_error(resource->handle,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "treeland_personalization_font_context_v1.set_monospace_font has "
                               "not been implemented yet");
    }

    void treeland_personalization_font_context_v1_get_monospace_font(Resource *resource)
    {
        wl_resource_post_error(resource->handle,
                               WL_DISPLAY_ERROR_INVALID_OBJECT,
                               "treeland_personalization_font_context_v1.get_monospace_font has "
                               "not been implemented yet");
    }

    void treeland_personalization_font_context_v1_destroy(Resource *resource)
    {
        wl_resource_post_error(
            resource->handle,
            WL_DISPLAY_ERROR_INVALID_OBJECT,
            "treeland_personalization_font_context_v1.destroy has not been implemented yet");
    }
};

ManagerInterface::ManagerInterface(QObject *parent)
    : QObject(parent)
    , d_ptr(new ManagerInterfacePrivate(this))
{
}

void ManagerInterface::create(WAYLIB_SERVER_NAMESPACE::WServer *server)
{
    Q_D(ManagerInterface);

    d->init(server->handle()->handle(), TREELAND_PERSONALIZATION_MANAGER_V1_VERSION);
}

void ManagerInterface::destroy(WAYLIB_SERVER_NAMESPACE::WServer *server)
{
    d_ptr = nullptr;
}

wl_global *ManagerInterface::global() const
{
    Q_D(const ManagerInterface);
    return d->global();
}
} // namespace Protocol::Personalization

#include "personalizationmanagerinterfacev1.moc"
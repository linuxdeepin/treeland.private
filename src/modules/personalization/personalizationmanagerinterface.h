#pragma once

#include <wglobal.h>
#include <wserver.h>

#include <QObject>

#include <memory>

namespace Protocol {
namespace Personalization {

class ManagerInterfacePrivate;

class ManagerInterface
    : public QObject
    , public WAYLIB_SERVER_NAMESPACE::WServerInterface
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(ManagerInterface)

public:
    explicit ManagerInterface(QObject *parent = nullptr);
    ~ManagerInterface() override = default;

protected:
    void create(WAYLIB_SERVER_NAMESPACE::WServer *server) override;
    void destroy(WAYLIB_SERVER_NAMESPACE::WServer *server) override;
    wl_global *global() const override;

private:
    std::unique_ptr<ManagerInterfacePrivate> d_ptr;
};

class WallpaperInterfacePrivate;

class WallpaperInterface : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(WallpaperInterface)

public:
    // Public members if any
    ~WallpaperInterface() override = default;

private:
    std::unique_ptr<WallpaperInterfacePrivate> d_ptr;
};

class CursorInterfacePrivate;

class CursorInterface : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(CursorInterface)

public:
    // Public members if any
    ~CursorInterface() override = default;

private:
    std::unique_ptr<CursorInterfacePrivate> d_ptr;
};

class WindowInterfacePrivate;

class WindowInterface : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(WindowInterface)

public:
    // Public members if any
    ~WindowInterface() override = default;

private:
    std::unique_ptr<WindowInterfacePrivate> d_ptr;
};

class FontInterfacePrivate;

class FontInterface : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(FontInterface)

public:
    // Public members if any
    ~FontInterface() override = default;

private:
    std::unique_ptr<FontInterfacePrivate> d_ptr;
};

} // namespace Personalization
} // namespace Protocol
// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <DSingleton>

#include <QObject>

#include <memory>
#include <optional>

class QCoreApplication;
class QCommandLineParser;
class QCommandLineOption;

DCORE_USE_NAMESPACE

class CmdLine
    : public QObject
    , public DSingleton<CmdLine>
{
    Q_OBJECT
    friend class DSingleton<CmdLine>;

public:
    std::optional<QString> socket() const;
    std::optional<QString> run() const;
    bool useLockScreen() const;

private:
    CmdLine();

private:
    std::unique_ptr<QCommandLineParser> m_parser;
    std::unique_ptr<QCommandLineOption> m_socket;
    std::unique_ptr<QCommandLineOption> m_run;
    std::unique_ptr<QCommandLineOption> m_lockScreen;
};

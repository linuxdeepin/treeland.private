// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "cmdline.h"

#include <DLog>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>

#include <optional>

DCORE_USE_NAMESPACE;

Q_LOGGING_CATEGORY(qLcCmdLine, "treeland.cmdline", QtInfoMsg)

CmdLine::CmdLine()
    : QObject()
    , m_parser(std::make_unique<QCommandLineParser>())
    , m_socket(std::make_unique<QCommandLineOption>(
          QStringList{ "s", "socket" }, "set ddm socket", "socket"))
    , m_run(std::make_unique<QCommandLineOption>(QStringList{ "r", "run" }, "run a process", "run"))
    , m_lockScreen(std::make_unique<QCommandLineOption>("lockscreen",
                                                        "use lockscreen, need DDM auth socket"))
    , m_logFile(std::make_unique<QCommandLineOption>("log", "log file path", "log file path"))
{
    m_parser->addHelpOption();
    m_parser->addOptions({ *m_socket.get(), *m_run.get(), *m_lockScreen.get(), *m_logFile.get() });
    m_parser->process(*QCoreApplication::instance());

    if (m_parser->isSet(*m_logFile.get())) {
        QFileInfo file(QDir::currentPath(), m_parser->value(*m_logFile.get()));
        DLogManager::setlogFilePath(file.absoluteFilePath());
        DLogManager::registerFileAppender();
    }
}

std::optional<QString> CmdLine::socket() const
{
    if (m_parser->isSet(*m_socket.get())) {
        return m_parser->value(*m_socket.get());
    }

    return std::nullopt;
}

std::optional<QString> CmdLine::run() const
{
    if (m_parser->isSet(*m_run.get())) {
        return m_parser->value(*m_run.get());
    }

    return std::nullopt;
}

bool CmdLine::useLockScreen() const
{
    return m_parser->isSet(*m_lockScreen.get());
}

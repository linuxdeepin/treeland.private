// Copyright (C) 2023 Dingyuan Zhang <lxz@mkacg.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "cmdline.h"

#include <wordexp.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLoggingCategory>

#include <optional>

Q_LOGGING_CATEGORY(cmdline, "treeland.cmdline", QtDebugMsg);

CmdLine::CmdLine()
    : QObject()
    , m_parser(std::make_unique<QCommandLineParser>())
    , m_socket(std::make_unique<QCommandLineOption>(QStringList{ "s", "socket" },
                                                    "set ddm socket",
                                                    "socket"))
    , m_run(std::make_unique<QCommandLineOption>(QStringList{ "r", "run" }, "run a process", "run"))
    , m_lockScreen(std::make_unique<QCommandLineOption>("lockscreen",
                                                        "use lockscreen, need DDM auth socket"))
{
    m_parser->addHelpOption();
    m_parser->addOptions({ *m_socket.get(), *m_run.get(), *m_lockScreen.get() });
    m_parser->process(*QCoreApplication::instance());
}

QString CmdLine::unescape(const QString &str) noexcept
{
    QString unescapedStr;
    for (qsizetype i = 0; i < str.size(); ++i) {
        auto c = str.at(i);
        if (c != '\\') {
            unescapedStr.append(c);
            continue;
        }

        switch (str.at(i + 1).toLatin1()) {
        default:
            unescapedStr.append(c);
            break;
        case 'n':
            unescapedStr.append('\n');
            ++i;
            break;
        case 't':
            unescapedStr.append('\t');
            ++i;
            break;
        case 'r':
            unescapedStr.append('\r');
            ++i;
            break;
        case '\\':
            unescapedStr.append('\\');
            ++i;
            break;
        case ';':
            unescapedStr.append(';');
            ++i;
            break;
        case 's': {
            unescapedStr.append(R"(\ )");
            ++i;
        } break;
        }
    }

    return unescapedStr;
}

std::optional<QStringList> CmdLine::unescapeExecArgs(const QString &str) noexcept
{
    auto unescapedStr = unescape(str);
    if (unescapedStr.isEmpty()) {
        qCWarning(cmdline) << "unescape Exec failed.";
        return std::nullopt;
    }

    auto deleter = [](wordexp_t *word) {
        wordfree(word);
        delete word;
    };
    std::unique_ptr<wordexp_t, decltype(deleter)> words{ new (std::nothrow)
                                                             wordexp_t{ 0, nullptr, 0 },
                                                         deleter };

    if (auto ret = wordexp(unescapedStr.toLocal8Bit(), words.get(), WRDE_SHOWERR); ret != 0) {
        QString errMessage;
        switch (ret) {
        case WRDE_BADCHAR:
            errMessage = "BADCHAR";
            break;
        case WRDE_BADVAL:
            errMessage = "BADVAL";
            break;
        case WRDE_CMDSUB:
            errMessage = "CMDSUB";
            break;
        case WRDE_NOSPACE:
            errMessage = "NOSPACE";
            break;
        case WRDE_SYNTAX:
            errMessage = "SYNTAX";
            break;
        default:
            errMessage = "unknown";
        }
        qCWarning(cmdline) << "wordexp error: " << errMessage;
        return std::nullopt;
    }

    QStringList execList;
    for (std::size_t i = 0; i < words->we_wordc; ++i) {
        execList.emplace_back(words->we_wordv[i]);
    }

    return execList;
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

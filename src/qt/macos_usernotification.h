// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#ifndef BITCOIN_QT_MACOS_USERNOTIFICATION_H
#define BITCOIN_QT_MACOS_USERNOTIFICATION_H

#include <QString>

/** macOS-specific notification handler (supports UserNotificationCenter).
 */
struct MacosUserNotificationHandler
{
    void showNotification(const QString& title, const QString& text);

    /** check if OS can handle UserNotifications */
    bool hasUserNotificationCenterSupport();
    static MacosUserNotificationHandler* instance();
};

// void RequestAuthorization();

#endif // BITCOIN_QT_MACOS_USERNOTIFICATION_H

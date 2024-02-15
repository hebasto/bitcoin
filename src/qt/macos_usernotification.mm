// Copyright (c) 2024-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit/.

#include "macos_usernotification.h"

#import <objc/runtime.h>
#include <Cocoa/Cocoa.h>


#import <UserNotifications/UserNotifications.h>
#include <logging.h>
// #import "AppDelegate.h"

// void RequestAuthorization()
// {
//     UNUserNotificationCenter *center = [UNUserNotificationCenter currentNotificationCenter];
//     UNAuthorizationOptions auth_options = (UNAuthorizationOptionAlert | UNAuthorizationOptionSound | UNAuthorizationOptionBadge);
//     [center requestAuthorizationWithOptions:auth_options
//         completionHandler:^(BOOL granted, NSError* error) {
//             if (error) {
//                 // tfm::format(std::cerr, "HEBASTO - %s:%s error=%s\n", __func__, __LINE__, messageAsString:error.localizedDescription);
//                 tfm::format(std::cerr, "HEBASTO - %s:%s Notifications permission ERROR.\n", __func__, __LINE__);
//             } else if (!granted) {
//                 tfm::format(std::cerr, "HEBASTO - %s:%s Notifications permission is NOT granted.\n", __func__, __LINE__);
//             } else {
//                 tfm::format(std::cerr, "HEBASTO - %s:%s Notifications permission is GRANTED.\n", __func__, __LINE__);
//             }
//     }];
// }

// Add an obj-c category (extension) to return the expected bundle identifier
@implementation NSBundle(returnCorrectIdentifier)
- (NSString*)__bundleIdentifier
{
    if (self == [NSBundle mainBundle]) {
        return @"org.bitcoinfoundation.Bitcoin-Qt";
    } else {
        return [self __bundleIdentifier];
    }
}
@end

void MacosUserNotificationHandler::showNotification(const QString& title, const QString& text)
{
    if (!this->hasUserNotificationCenterSupport()) return;

    UNMutableNotificationContent* content = [[UNMutableNotificationContent alloc] init];
    content.title = title.toNSString();
    content.body = text.toNSString();
    content.sound = [UNNotificationSound defaultSound];

    UNTimeIntervalNotificationTrigger* trigger = [UNTimeIntervalNotificationTrigger triggerWithTimeInterval:1 repeats:NO];

    UNNotificationRequest* request = [UNNotificationRequest requestWithIdentifier:@"NOTIFICATION" content:content trigger:trigger];

    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    [center addNotificationRequest:request withCompletionHandler:^(NSError* _Nullable error) {
        if (!error) {
            LogPrintf("HEBASTO - %s:%s Notifications permission ERROR.\n", __func__, __LINE__);
        }
    }];
}

bool MacosUserNotificationHandler::hasUserNotificationCenterSupport(void)
{
    UNUserNotificationCenter* center = [UNUserNotificationCenter currentNotificationCenter];
    UNAuthorizationOptions auth_options = (UNAuthorizationOptionAlert | UNAuthorizationOptionSound | UNAuthorizationOptionBadge);
    [center requestAuthorizationWithOptions:auth_options
        // The block to execute asynchronously with the results.
        // This block may execute on a background thread.
        // The block has no return value.
        completionHandler:^(BOOL granted, NSError* error) {
            if (error) {
                LogPrintf("HEBASTO - %s:%s Notifications permission ERROR.\n", __func__, __LINE__);
            } else if (!granted) {
                LogPrintf("HEBASTO - %s:%s Notifications permission is NOT granted.\n", __func__, __LINE__);
            } else {
                LogPrintf("HEBASTO - %s:%s Notifications permission is GRANTED.\n", __func__, __LINE__);
            }
    }];

    // Class possibleClass = NSClassFromString(@"NSUserNotificationCenter");

    // // check if users OS has support for NSUserNotification
    // if (possibleClass!=nil) {
    //     return true;
    // }
    return false;
}


MacosUserNotificationHandler* MacosUserNotificationHandler::instance()
{
    static MacosUserNotificationHandler* s_instance = nullptr;
    if (!s_instance) {
        s_instance = new MacosUserNotificationHandler();

        Class aPossibleClass = objc_getClass("NSBundle");
        if (aPossibleClass) {
            // change NSBundle -bundleIdentifier method to return a correct bundle identifier
            // a bundle identifier is required to use OSXs User Notification Center
            method_exchangeImplementations(class_getInstanceMethod(aPossibleClass, @selector(bundleIdentifier)),
                                           class_getInstanceMethod(aPossibleClass, @selector(__bundleIdentifier)));
        }
    }
    return s_instance;
}

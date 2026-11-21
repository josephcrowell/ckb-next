#include "xcbidletimer.h"
#include <stdint.h>
#include <QtGlobal>
#include <QByteArray>
#include <QString>

#ifdef USE_XCB_SCREENSAVER
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QX11Info>
#else
#include <QGuiApplication>
#endif
#include <xcb/xcb.h>
#include <xcb/screensaver.h>
#include <limits.h>

int XcbIdleTimer::getIdleTime() const
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0 ,0)
    xcb_connection_t* conn = QX11Info::connection();
#else
    xcb_connection_t* conn = nullptr;
    QNativeInterface::QX11Application* x11Application;
    if(!(x11Application = qGuiApp->nativeInterface<QNativeInterface::QX11Application>()))
        return 0;

    conn = x11Application->connection();
#endif

    // The time since the last input is the same for every screen, so the first root window will do
    const xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    if(!screen)
        return 0;

    xcb_screensaver_query_info_cookie_t replyCookie = xcb_screensaver_query_info(conn, screen->root);
    xcb_screensaver_query_info_reply_t* replyInfo = xcb_screensaver_query_info_reply(conn, replyCookie, nullptr);

    uint32_t userIdle = replyInfo->ms_since_user_input;
    free(replyInfo);

    // Clamp the value
    if(userIdle > INT_MAX)
        userIdle = INT_MAX;

    return (int)userIdle;
}
#else
int XcbIdleTimer::getIdleTime()
{
    return 0;
}
#endif

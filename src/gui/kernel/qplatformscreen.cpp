/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformscreen.h"
#include <QtCore/qdebug.h>
#include <QtGui/qguiapplication.h>
#include <qpa/qplatformcursor.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformscreen_p.h>
#include <qpa/qplatformintegration.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>
#include <private/qhighdpiscaling_p.h>

QT_BEGIN_NAMESPACE

QPlatformScreen::QPlatformScreen()
    : d_ptr(new QPlatformScreenPrivate)
{
    Q_D(QPlatformScreen);
    d->screen = nullptr;
}

QPlatformScreen::~QPlatformScreen()
{
    Q_D(QPlatformScreen);
    if (d->screen) {
        qWarning("Manually deleting a QPlatformScreen. Call QWindowSystemInterface::handleScreenRemoved instead.");
        delete d->screen;
    }
}

/*!
    This function is called when Qt needs to be able to grab the content of a window.

    Returns the content of the window specified with the WId handle within the boundaries of
    QRect(x,y,width,height).
*/
QPixmap QPlatformScreen::grabWindow(WId window, int x, int y, int width, int height) const
{
    Q_UNUSED(window);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(width);
    Q_UNUSED(height);
    return QPixmap();
}

/*!
    Return the given top level window for a given position.

    Default implementation retrieves a list of all top level windows and finds the first window
    which contains point \a pos
*/
QWindow *QPlatformScreen::topLevelAt(const QPoint & pos) const
{
    const QWindowList list = QGuiApplication::topLevelWindows();
    for (int i = list.size()-1; i >= 0; --i) {
        QWindow *w = list[i];
        if (w->isVisible() && QHighDpi::toNativePixels(w->geometry(), w).contains(pos))
            return w;
    }

    return nullptr;
}

/*!
    Return all windows residing on this screen.
*/
QWindowList QPlatformScreen::windows() const
{
    QWindowList windows;
    for (QWindow *window : QGuiApplication::allWindows()) {
        if (platformScreenForWindow(window) != this)
            continue;
        windows.append(window);
    }
    return windows;
}

/*!
  Find the sibling screen corresponding to \a globalPos.

  Returns this screen if no suitable screen is found at the position.
 */
const QPlatformScreen *QPlatformScreen::screenForPosition(const QPoint &point) const
{
    if (!geometry().contains(point)) {
        const auto screens = virtualSiblings();
        for (const QPlatformScreen *screen : screens) {
            if (screen->geometry().contains(point))
                return screen;
        }
    }
    return this;
}


/*!
    Returns a list of all the platform screens that are part of the same
    virtual desktop.

    Screens part of the same virtual desktop share a common coordinate system,
    and windows can be freely moved between them.
*/
QList<QPlatformScreen *> QPlatformScreen::virtualSiblings() const
{
    QList<QPlatformScreen *> list;
    list << const_cast<QPlatformScreen *>(this);
    return list;
}

QScreen *QPlatformScreen::screen() const
{
    Q_D(const QPlatformScreen);
    return d->screen;
}

/*!
    Reimplement this function in subclass to return the physical size of the
    screen, in millimeters. The physical size represents the actual physical
    dimensions of the display.

    The default implementation takes the pixel size of the screen, considers a
    resolution of 100 dots per inch, and returns the calculated physical size.
    A device with a screen that has different resolutions will need to be
    supported by a suitable reimplementation of this function.

    \sa logcalDpi
*/
QSizeF QPlatformScreen::physicalSize() const
{
    static const int dpi = 100;
    return QSizeF(geometry().size()) / dpi * qreal(25.4);
}

/*!
    Reimplement this function in subclass to return the logical horizontal
    and vertical dots per inch metrics of the screen.

    The logical dots per inch metrics are used by Qt to scale the user interface.

    The default implementation returns logicalBaseDpi(), which results in a
    UI scale factor of 1.0.

    \sa physicalSize
*/
QDpi QPlatformScreen::logicalDpi() const
{
    return logicalBaseDpi();
}

// Helper function for accessing the platform screen logical dpi
// which accounts for QT_FONT_DPI.
QPair<qreal, qreal> QPlatformScreen::overrideDpi(const QPair<qreal, qreal> &in)
{
    static const int overrideDpi = qEnvironmentVariableIntValue("QT_FONT_DPI");
    return overrideDpi > 0 ?  QDpi(overrideDpi, overrideDpi) : in;
}

/*!
    Reimplement to return the base logical DPI for the platform. This
    DPI value should correspond to a standard-DPI (1x) display. The
    default implementation returns 96.

    QtGui will use this value (together with logicalDpi) to compute
    the scale factor when high-DPI scaling is enabled, as follows:
        factor = logicalDPI / baseDPI
*/
QDpi QPlatformScreen::logicalBaseDpi() const
{
    return QDpi(96, 96);
}

/*!
    Reimplement this function in subclass to return the device pixel ratio
    for the screen. This is the ratio between physical pixels and the
    device-independent pixels of the windowing system. The default
    implementation returns 1.0.

    \sa QPlatformWindow::devicePixelRatio()
    \sa QPlatformScreen::pixelDensity()
*/
qreal QPlatformScreen::devicePixelRatio() const
{
    return 1.0;
}

/*!
    Reimplement this function in subclass to return the pixel density of the
    screen. This is the scale factor needed to make a low-dpi application
    usable on this screen. The default implementation returns 1.0.

    Returning something else than 1.0 from this function causes Qt to
    apply the scale factor to the application's coordinate system.
    This is different from devicePixelRatio, which reports a scale
    factor already applied by the windowing system. A platform plugin
    typically implements one (or none) of these two functions.

    \sa QPlatformWindow::devicePixelRatio()
*/
qreal QPlatformScreen::pixelDensity()  const
{
    return 1.0;
}

/*!
    Reimplement this function in subclass to return the vertical refresh rate
    of the screen, in Hz.

    The default returns 60, a sensible default for modern displays.
*/
qreal QPlatformScreen::refreshRate() const
{
    return 60;
}

/*!
    Reimplement this function in subclass to return the native orientation
    of the screen, e.g. the orientation where the logo sticker of the device
    appears the right way up.

    The default implementation returns Qt::PrimaryOrientation.
*/
Qt::ScreenOrientation QPlatformScreen::nativeOrientation() const
{
    return Qt::PrimaryOrientation;
}

/*!
    Reimplement this function in subclass to return the current orientation
    of the screen, for example based on accelerometer data to determine
    the device orientation.

    The default implementation returns Qt::PrimaryOrientation.
*/
Qt::ScreenOrientation QPlatformScreen::orientation() const
{
    return Qt::PrimaryOrientation;
}

QPlatformScreen * QPlatformScreen::platformScreenForWindow(const QWindow *window)
{
    // QTBUG 32681: It can happen during the transition between screens
    // when one screen is disconnected that the window doesn't have a screen.
    if (!window->screen())
        return nullptr;
    return window->screen()->handle();
}

/*!
    Reimplement this function in subclass to return the manufacturer
    of this screen.

    The default implementation returns an empty string.

    \since 5.9
*/
QString QPlatformScreen::manufacturer() const
{
    return QString();
}

/*!
    Reimplement this function in subclass to return the model
    of this screen.

    The default implementation returns an empty string.

    \since 5.9
*/
QString QPlatformScreen::model() const
{
    return QString();
}

/*!
    Reimplement this function in subclass to return the serial number
    of this screen.

    The default implementation returns an empty string.

    \since 5.9
*/
QString QPlatformScreen::serialNumber() const
{
    return QString();
}

/*!
    \class QPlatformScreen
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformScreen class provides an abstraction for visual displays.

    Many window systems has support for retrieving information on the attached displays. To be able
    to query the display QPA uses QPlatformScreen. Qt its self is most dependent on the
    physicalSize() function, since this is the function it uses to calculate the dpi to use when
    converting point sizes to pixels sizes. However, this is unfortunate on some systems, as the
    native system fakes its dpi size.
 */

/*! \fn QRect QPlatformScreen::geometry() const = 0
    Reimplement in subclass to return the pixel geometry of the screen
*/

/*! \fn QRect QPlatformScreen::availableGeometry() const
    Reimplement in subclass to return the pixel geometry of the available space
    This normally is the desktop screen minus the task manager, global menubar etc.
*/

/*! \fn int QPlatformScreen::depth() const = 0
    Reimplement in subclass to return current depth of the screen
*/

/*! \fn QImage::Format QPlatformScreen::format() const = 0
    Reimplement in subclass to return the image format which corresponds to the screen format
*/

/*!
    Reimplement this function in subclass to return the cursor of the screen.

    The default implementation returns \nullptr.
*/
QPlatformCursor *QPlatformScreen::cursor() const
{
    return nullptr;
}

/*!
  Convenience method to resize all the maximized and fullscreen windows
  of this platform screen.
*/
void QPlatformScreen::resizeMaximizedWindows()
{
    // 'screen()' still has the old geometry info while 'this' has the new geometry info
    const QRect oldGeometry = screen()->geometry();
    const QRect oldAvailableGeometry = screen()->availableGeometry();
    const QRect newGeometry = deviceIndependentGeometry();
    const QRect newAvailableGeometry = QHighDpi::fromNative(availableGeometry(), QHighDpiScaling::factor(this), newGeometry.topLeft());

    const bool supportsMaximizeUsingFullscreen = QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MaximizeUsingFullscreenGeometry);

    for (QWindow *w : windows()) {
        // Skip non-platform windows, e.g., offscreen windows.
        if (!w->handle())
            continue;

        if (supportsMaximizeUsingFullscreen
                && w->windowState() & Qt::WindowMaximized
                && w->flags() & Qt::MaximizeUsingFullscreenGeometryHint) {
            w->setGeometry(newGeometry);
        } else if (w->windowState() & Qt::WindowMaximized || w->geometry() == oldAvailableGeometry) {
            w->setGeometry(newAvailableGeometry);
        } else if (w->windowState() & Qt::WindowFullScreen || w->geometry() == oldGeometry) {
            w->setGeometry(newGeometry);
        }
    }
}

// i must be power of two
static int log2(uint i)
{
    if (i == 0)
        return -1;

    int result = 0;
    while (!(i & 1)) {
        ++result;
        i >>= 1;
    }
    return result;
}

int QPlatformScreen::angleBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b)
{
    if (a == Qt::PrimaryOrientation || b == Qt::PrimaryOrientation) {
        qWarning("Use QScreen version of %sBetween() when passing Qt::PrimaryOrientation", "angle");
        return 0;
    }

    if (a == b)
        return 0;

    int ia = log2(uint(a));
    int ib = log2(uint(b));

    int delta = ia - ib;

    if (delta < 0)
        delta = delta + 4;

    int angles[] = { 0, 90, 180, 270 };
    return angles[delta];
}

QTransform QPlatformScreen::transformBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b, const QRect &target)
{
    if (a == Qt::PrimaryOrientation || b == Qt::PrimaryOrientation) {
        qWarning("Use QScreen version of %sBetween() when passing Qt::PrimaryOrientation", "transform");
        return QTransform();
    }

    if (a == b)
        return QTransform();

    int angle = angleBetween(a, b);

    QTransform result;
    switch (angle) {
    case 90:
        result.translate(target.width(), 0);
        break;
    case 180:
        result.translate(target.width(), target.height());
        break;
    case 270:
        result.translate(0, target.height());
        break;
    default:
        Q_ASSERT(false);
    }
    result.rotate(angle);

    return result;
}

QRect QPlatformScreen::mapBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b, const QRect &rect)
{
    if (a == Qt::PrimaryOrientation || b == Qt::PrimaryOrientation) {
        qWarning("Use QScreen version of %sBetween() when passing Qt::PrimaryOrientation", "map");
        return rect;
    }

    if (a == b)
        return rect;

    if ((a == Qt::PortraitOrientation || a == Qt::InvertedPortraitOrientation)
        != (b == Qt::PortraitOrientation || b == Qt::InvertedPortraitOrientation))
    {
        return QRect(rect.y(), rect.x(), rect.height(), rect.width());
    }

    return rect;
}

QRect QPlatformScreen::deviceIndependentGeometry() const
{
    qreal scaleFactor = QHighDpiScaling::factor(this);
    QRect nativeGeometry = geometry();
    return QRect(nativeGeometry.topLeft(), QHighDpi::fromNative(nativeGeometry.size(), scaleFactor));
}

/*!
  Returns a hint about this screen's subpixel layout structure.

  The default implementation queries the \b{QT_SUBPIXEL_AA_TYPE} env variable.
  This is just a hint because most platforms don't have a way to retrieve the correct value from hardware
  and instead rely on font configurations.
*/
QPlatformScreen::SubpixelAntialiasingType QPlatformScreen::subpixelAntialiasingTypeHint() const
{
    static int type = -1;
    if (type == -1) {
        QByteArray env = qgetenv("QT_SUBPIXEL_AA_TYPE");
        if (env == "RGB")
            type = QPlatformScreen::Subpixel_RGB;
        else if (env == "BGR")
            type = QPlatformScreen::Subpixel_BGR;
        else if (env == "VRGB")
            type = QPlatformScreen::Subpixel_VRGB;
        else if (env == "VBGR")
            type = QPlatformScreen::Subpixel_VBGR;
        else
            type = QPlatformScreen::Subpixel_None;
    }

    return static_cast<QPlatformScreen::SubpixelAntialiasingType>(type);
}

/*!
  Returns the current power state.

  The default implementation always returns PowerStateOn.
*/
QPlatformScreen::PowerState QPlatformScreen::powerState() const
{
    return PowerStateOn;
}

/*!
  Sets the power state for this screen.
*/
void QPlatformScreen::setPowerState(PowerState state)
{
    Q_UNUSED(state);
}

/*!
    Reimplement this function in subclass to return the list
    of modes for this screen.

    The default implementation returns a list with
    only one mode from the current screen size and refresh rate.

    \sa QPlatformScreen::geometry
    \sa QPlatformScreen::refreshRate

    \since 5.9
*/
QList<QPlatformScreen::Mode> QPlatformScreen::modes() const
{
    QList<QPlatformScreen::Mode> list;
    list.append({geometry().size(), refreshRate()});
    return list;
}

/*!
    Reimplement this function in subclass to return the
    index of the current mode from the modes list.

    The default implementation returns 0.

    \sa QPlatformScreen::modes

    \since 5.9
*/
int QPlatformScreen::currentMode() const
{
    return 0;
}

/*!
    Reimplement this function in subclass to return the preferred
    mode index from the modes list.

    The default implementation returns 0.

    \sa QPlatformScreen::modes

    \since 5.9
*/
int QPlatformScreen::preferredMode() const
{
    return 0;
}

QList<QPlatformScreen *> QPlatformPlaceholderScreen::virtualSiblings() const
{
    QList<QPlatformScreen *> siblings;

    if (!m_virtualSibling)
        return siblings;

    for (QScreen *screen : QGuiApplication::screens()) {
        if (screen->handle() && screen->handle() != this)
            siblings << screen->handle();
    }
    return siblings;
}

QT_END_NAMESPACE

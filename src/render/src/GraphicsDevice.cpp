// The graphics device on macOS: Qt Quick makes its own Metal device for each window (QRhi), and decoding and
// capture keep their pictures in memory, so there is no device to make or share here. The object stays so that
// callers written for Windows' shared Direct3D 11 device build unchanged; device() and context() are null.
#include "GraphicsDevice.h"

#include <QQuickWindow>
#include <QSGRendererInterface>

namespace gs::render {

std::unique_ptr<GraphicsDevice> GraphicsDevice::create(bool debugLayer, bool)
{
    std::unique_ptr<GraphicsDevice> made(new GraphicsDevice());
    made->m_debugLayer = debugLayer;
    made->m_description = "Metal (Qt RHI)";
    return made;
}

GraphicsDevice::~GraphicsDevice() = default;

std::vector<std::string> GraphicsDevice::debugErrors(std::size_t) const
{
    return {};
}

void GraphicsDevice::share(QQuickWindow&) const
{
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Metal);
}

}

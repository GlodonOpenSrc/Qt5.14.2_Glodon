/****************************************************************************
**
** Copyright (C) 2017 Juan José Casafranca
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt3D module of the Qt Toolkit.
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

#include "sendbuffercapturejob_p.h"

#include <Qt3DRender/private/nodemanagers_p.h>
#include <Qt3DRender/private/job_common_p.h>
#include <Qt3DRender/private/buffer_p.h>
#include <Qt3DRender/private/buffermanager_p.h>
#include <Qt3DCore/private/qaspectmanager_p.h>
#include <Qt3DRender/private/qbuffer_p.h>

QT_BEGIN_NAMESPACE

namespace Qt3DRender {

namespace Render {

class SendBufferCaptureJobPrivate : public Qt3DCore::QAspectJobPrivate
{
public:
    SendBufferCaptureJobPrivate() {}
    ~SendBufferCaptureJobPrivate() {}

    void postFrame(Qt3DCore::QAspectManager *aspectManager) override;

    mutable QMutex m_mutex;
    QVector<QPair<Qt3DCore::QNodeId, QByteArray>> m_buffersToCapture;
    QVector<QPair<Qt3DCore::QNodeId, QByteArray>> m_buffersToNotify;
};

SendBufferCaptureJob::SendBufferCaptureJob()
    : Qt3DCore::QAspectJob(*new SendBufferCaptureJobPrivate)
    , m_nodeManagers(nullptr)
{
    SET_JOB_RUN_STAT_TYPE(this, JobTypes::SendBufferCapture, 0)
}

SendBufferCaptureJob::~SendBufferCaptureJob()
{
}

// Called from SubmitRenderView while rendering
void SendBufferCaptureJob::addRequest(QPair<Qt3DCore::QNodeId, QByteArray> request)
{
    Q_DJOB(SendBufferCaptureJob);
    QMutexLocker locker(&d->m_mutex);
    d->m_buffersToCapture.push_back(request);
}

// Called by aspect thread jobs to execute (we may still be rendering at this point)
bool SendBufferCaptureJob::hasRequests() const
{
    Q_DJOB(const SendBufferCaptureJob);
    QMutexLocker locker(&d->m_mutex);
    return d->m_buffersToCapture.size() > 0;
}

void SendBufferCaptureJob::run()
{
    Q_ASSERT(m_nodeManagers);
    Q_DJOB(SendBufferCaptureJob);
    QMutexLocker locker(&d->m_mutex);
    for (const QPair<Qt3DCore::QNodeId, QByteArray> &pendingCapture : qAsConst(d->m_buffersToCapture)) {
        Buffer *buffer = m_nodeManagers->bufferManager()->lookupResource(pendingCapture.first);
        // Buffer might have been destroyed between the time addRequest is made and this job gets run
        // If it exists however, it cannot be destroyed before this job is done running
        if (buffer != nullptr)
            buffer->updateDataFromGPUToCPU(pendingCapture.second);
    }
    d->m_buffersToNotify = std::move(d->m_buffersToCapture);
}

void SendBufferCaptureJobPrivate::postFrame(Qt3DCore::QAspectManager *aspectManager)
{
    QMutexLocker locker(&m_mutex);
    const QVector<QPair<Qt3DCore::QNodeId, QByteArray>> pendingSendBufferCaptures = std::move(m_buffersToNotify);
    for (const auto &bufferDataPair : pendingSendBufferCaptures) {
        QBuffer *frontendBuffer = static_cast<decltype(frontendBuffer)>(aspectManager->lookupNode(bufferDataPair.first));
        if (!frontendBuffer)
            continue;
        QBufferPrivate *dFrontend = static_cast<decltype(dFrontend)>(Qt3DCore::QNodePrivate::get(frontendBuffer));
        // Calling frontendBuffer->setData would result in forcing a sync against the backend
        // which isn't necessary
        dFrontend->setData(bufferDataPair.second);
        Q_EMIT frontendBuffer->dataAvailable();
    }
}

} // Render

} // Qt3DRender

QT_END_NAMESPACE

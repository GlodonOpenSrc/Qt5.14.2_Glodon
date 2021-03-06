/****************************************************************************
**
** Copyright (C) 2017 Klaralvdalens Datakonsult AB (KDAB).
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

#include "joint_p.h"
#include <Qt3DRender/private/managers_p.h>
#include <Qt3DCore/QJoint>
#include <Qt3DCore/private/qjoint_p.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

using namespace Qt3DCore;

namespace Qt3DRender {
namespace Render {

Joint::Joint()
    : BackendNode(Qt3DCore::QBackendNode::ReadOnly)
    , m_localPose()
    , m_jointManager(nullptr)
    , m_skeletonManager(nullptr)
{
}

void Joint::cleanup()
{
    m_inverseBindMatrix.setToIdentity();
    m_localPose = Sqt();
    m_childJointIds.clear();
    m_name.clear();
    m_owningSkeleton = HSkeleton();
    setEnabled(false);
}

void Joint::syncFromFrontEnd(const QNode *frontEnd, bool firstTime)
{
    const Qt3DCore::QJoint *joint = qobject_cast<const Qt3DCore::QJoint *>(frontEnd);
    if (!joint)
        return;

    bool jointDirty = firstTime;
    if (m_localPose.scale != joint->scale()) {
        m_localPose.scale = joint->scale();
        jointDirty = true;
    }
    if (m_localPose.rotation != joint->rotation()) {
        m_localPose.rotation = joint->rotation();
        jointDirty = true;
    }
    if (m_localPose.translation != joint->translation()) {
        m_localPose.translation = joint->translation();
        jointDirty = true;
    }
    if (m_inverseBindMatrix != joint->inverseBindMatrix()) {
        // Setting the inverse bind matrix should be a rare operation. Usually it is
        // set once and then remains constant for the duration of the skeleton. So just
        // trigger a rebuild of the skeleton's SkeletonData which will include obtaining
        // the inverse bind matrix.
        m_inverseBindMatrix = joint->inverseBindMatrix();
        m_skeletonManager->addDirtySkeleton(SkeletonManager::SkeletonDataDirty, m_owningSkeleton);
    }
    if (m_name != joint->name()) {
        // Joint name doesn't affect anything in the render aspect so no need
        // to mark anything as dirty.
        m_name = joint->name();

        // TODO: Notify other aspects (animation) about the name change.
    }

    Qt3DCore::QNodeIdVector childIds = qIdsForNodes(joint->childJoints());
    std::sort(std::begin(childIds), std::end(childIds));
    if (m_childJointIds != childIds)
        m_childJointIds = childIds;

    if (jointDirty) {
        markDirty(AbstractRenderer::JointDirty);
        m_jointManager->addDirtyJoint(peerId());
    }

    BackendNode::syncFromFrontEnd(frontEnd, firstTime);
}


JointFunctor::JointFunctor(AbstractRenderer *renderer,
                           JointManager *jointManager,
                           SkeletonManager *skeletonManager)
    : m_renderer(renderer)
    , m_jointManager(jointManager)
    , m_skeletonManager(skeletonManager)
{
}

Qt3DCore::QBackendNode *JointFunctor::create(const Qt3DCore::QNodeCreatedChangeBasePtr &change) const
{
    Joint *backend = m_jointManager->getOrCreateResource(change->subjectId());
    backend->setRenderer(m_renderer);
    backend->setJointManager(m_jointManager);
    backend->setSkeletonManager(m_skeletonManager);
    return backend;
}

Qt3DCore::QBackendNode *JointFunctor::get(Qt3DCore::QNodeId id) const
{
    return m_jointManager->lookupResource(id);
}

void JointFunctor::destroy(Qt3DCore::QNodeId id) const
{
    m_jointManager->removeDirtyJoint(id);
    m_jointManager->releaseResource(id);
}

} // namespace Render
} // namespace Qt3DRender

QT_END_NAMESPACE

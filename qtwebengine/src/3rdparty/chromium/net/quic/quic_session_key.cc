// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/quic_session_key.h"

#include "base/feature_list.h"
#include "net/base/features.h"

namespace net {

QuicSessionKey::QuicSessionKey(const HostPortPair& host_port_pair,
                               PrivacyMode privacy_mode,
                               const SocketTag& socket_tag,
                               const NetworkIsolationKey& network_isolation_key)
    : QuicSessionKey(host_port_pair.host(),
                     host_port_pair.port(),
                     privacy_mode,
                     socket_tag,
                     network_isolation_key) {}

QuicSessionKey::QuicSessionKey(const std::string& host,
                               uint16_t port,
                               PrivacyMode privacy_mode,
                               const SocketTag& socket_tag,
                               const NetworkIsolationKey& network_isolation_key)
    : QuicSessionKey(
          quic::QuicServerId(host, port, privacy_mode == PRIVACY_MODE_ENABLED),
          socket_tag,
          network_isolation_key) {}

QuicSessionKey::QuicSessionKey(const quic::QuicServerId& server_id,
                               const SocketTag& socket_tag,
                               const NetworkIsolationKey& network_isolation_key)
    : server_id_(server_id),
      socket_tag_(socket_tag),
      network_isolation_key_(
          base::FeatureList::IsEnabled(
              features::kPartitionConnectionsByNetworkIsolationKey)
              ? network_isolation_key
              : NetworkIsolationKey()) {}

bool QuicSessionKey::operator<(const QuicSessionKey& other) const {
  return std::tie(server_id_, socket_tag_, network_isolation_key_) <
         std::tie(other.server_id_, other.socket_tag_,
                  other.network_isolation_key_);
}
bool QuicSessionKey::operator==(const QuicSessionKey& other) const {
  return server_id_ == other.server_id_ && socket_tag_ == other.socket_tag_ &&
         network_isolation_key_ == other.network_isolation_key_;
}

size_t QuicSessionKey::EstimateMemoryUsage() const {
  return server_id_.EstimateMemoryUsage();
}

}  // namespace net

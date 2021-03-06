// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_UBER_QUIC_STREAM_ID_MANAGER_H_
#define QUICHE_QUIC_CORE_UBER_QUIC_STREAM_ID_MANAGER_H_

#include "net/third_party/quiche/src/quic/core/quic_stream_id_manager.h"

namespace quic {

namespace test {
class QuicSessionPeer;
class UberQuicStreamIdManagerPeer;
}  // namespace test

class QuicSession;

// This class comprises two QuicStreamIdManagers, which manage bidirectional and
// unidirectional stream IDs, respectively.
class QUIC_EXPORT_PRIVATE UberQuicStreamIdManager {
 public:
  UberQuicStreamIdManager(
      QuicSession* session,
      QuicStreamCount max_open_outgoing_bidirectional_streams,
      QuicStreamCount max_open_outgoing_unidirectional_streams,
      QuicStreamCount max_open_incoming_bidirectional_streams,
      QuicStreamCount max_open_incoming_unidirectional_streams);

  // Called when a stream with |stream_id| is registered as a static stream.
  // If |stream_already_counted| is true, the static stream is already counted
  // as an open stream earlier, so no need to count it again.
  void RegisterStaticStream(QuicStreamId id, bool stream_already_counted);

  // Sets the maximum outgoing stream count as a result of doing the transport
  // configuration negotiation. Forces the limit to max_streams, regardless of
  // static streams.
  void ConfigureMaxOpenOutgoingBidirectionalStreams(size_t max_streams);
  void ConfigureMaxOpenOutgoingUnidirectionalStreams(size_t max_streams);

  // Sets the limits to max_open_streams + number of static streams
  // in existence. SetMaxOpenOutgoingStreams will QUIC_BUG if it is called
  // after getting the first MAX_STREAMS frame or the transport configuration
  // was done.
  // TODO(fkastenholz): SetMax is cognizant of the number of static streams and
  // sets the maximum to be max_streams + number_of_statics. This should
  // eventually be removed from IETF QUIC.
  void SetMaxOpenOutgoingBidirectionalStreams(size_t max_open_streams);
  void SetMaxOpenOutgoingUnidirectionalStreams(size_t max_open_streams);
  void SetMaxOpenIncomingBidirectionalStreams(size_t max_open_streams);
  void SetMaxOpenIncomingUnidirectionalStreams(size_t max_open_streams);

  // Sets the outgoing stream count to the number of static streams + max
  // outgoing streams.  Unlike SetMaxOpenOutgoingStreams, this method will
  // not QUIC_BUG if called after getting  the first MAX_STREAMS frame.
  void AdjustMaxOpenOutgoingBidirectionalStreams(size_t max_streams);
  void AdjustMaxOpenOutgoingUnidirectionalStreams(size_t max_streams);

  // Returns true if next outgoing bidirectional stream ID can be allocated.
  bool CanOpenNextOutgoingBidirectionalStream();

  // Returns true if next outgoing unidirectional stream ID can be allocated.
  bool CanOpenNextOutgoingUnidirectionalStream();

  // Returns the next outgoing bidirectional stream id.
  QuicStreamId GetNextOutgoingBidirectionalStreamId();

  // Returns the next outgoing unidirectional stream id.
  QuicStreamId GetNextOutgoingUnidirectionalStreamId();

  // Returns true if the incoming |id| is within the limit.
  bool MaybeIncreaseLargestPeerStreamId(QuicStreamId id);

  // Called when |id| is released.
  void OnStreamClosed(QuicStreamId id);

  // Called when a MAX_STREAMS frame is received.
  bool OnMaxStreamsFrame(const QuicMaxStreamsFrame& frame);

  // Called when a STREAMS_BLOCKED frame is received.
  bool OnStreamsBlockedFrame(const QuicStreamsBlockedFrame& frame);

  // Return true if |id| is peer initiated.
  bool IsIncomingStream(QuicStreamId id) const;

  // Returns true if |id| is still available.
  bool IsAvailableStream(QuicStreamId id) const;

  size_t GetMaxAllowdIncomingBidirectionalStreams() const;

  size_t GetMaxAllowdIncomingUnidirectionalStreams() const;

  void SetLargestPeerCreatedStreamId(
      QuicStreamId largest_peer_created_stream_id);

  QuicStreamId next_outgoing_bidirectional_stream_id() const;
  QuicStreamId next_outgoing_unidirectional_stream_id() const;

  size_t max_allowed_outgoing_bidirectional_streams() const;
  size_t max_allowed_outgoing_unidirectional_streams() const;

  QuicStreamCount actual_max_allowed_incoming_bidirectional_streams() const;
  QuicStreamCount actual_max_allowed_incoming_unidirectional_streams() const;

  QuicStreamCount advertised_max_allowed_incoming_bidirectional_streams() const;
  QuicStreamCount advertised_max_allowed_incoming_unidirectional_streams()
      const;

 private:
  friend class test::QuicSessionPeer;
  friend class test::UberQuicStreamIdManagerPeer;

  // Manages stream IDs of bidirectional streams.
  QuicStreamIdManager bidirectional_stream_id_manager_;

  // Manages stream IDs of unidirectional streams.
  QuicStreamIdManager unidirectional_stream_id_manager_;
};

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_UBER_QUIC_STREAM_ID_MANAGER_H_

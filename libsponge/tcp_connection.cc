#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const {
    return _sender.stream_in().remaining_capacity();
}

size_t TCPConnection::bytes_in_flight() const {
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const {
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    return _time_since_last_segment_received_ms;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    _time_since_last_segment_received_ms = 0;
    _receiver.segment_received(seg);
    bool need_send_ack = seg.length_in_sequence_space();
    
    // If is RST, then close the connection
    if (seg.header().rst) {
        set_rst_state(false);
        return;
    }

    // Process ACK segments, update the sender's state
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);

        // If acknowledgment is needed but new segments are already queued, no need to send an empty ACK
        const bool sender_has_data_to_send = !_sender.segments_out().empty();
        if (need_send_ack && sender_has_data_to_send) {
            need_send_ack = false;
        }
    }


    // 如果是 LISEN 到了 SYN
    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::SYN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::CLOSED) {
        // 此时肯定是第一次调用 fill_window，因此会发送 SYN + ACK
        connect();
        _is_active = true;
        return;
    }

    // 判断 TCP 断开连接时是否时需要等待
    // CLOSE_WAIT
    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::SYN_ACKED)
        _linger_after_streams_finish = false;

    // 如果到了准备断开连接的时候。服务器端先断
    // CLOSED
    if (TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV &&
        TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED && !_linger_after_streams_finish) {
        _is_active = false;
        return;
    }

    // 如果收到的数据包里没有任何数据，则这个数据包可能只是为了 keep-alive
    if (need_send_ack)
        _sender.send_empty_segment();
        
    flush_segments_with_ack();
}

bool TCPConnection::active() const { return _is_active; }

//! Write data to the connection
size_t TCPConnection::write(const string &data) {
    size_t bytes_written = _sender.stream_in().write(data);
    connect();
    return bytes_written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _sender.tick(ms_since_last_tick);

    // Check retransmission limit and reset connection if exceeded
    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        // Clear any pending retransmissions before resetting
        if (!_sender.segments_out().empty()) {
            _sender.segments_out().pop();
        }
        set_rst_state(true);  // Reset connection
        return;
    }

    flush_segments_with_ack();
    _time_since_last_segment_received_ms += ms_since_last_tick;

    // Check for connection closure in TIME_WAIT state
    const bool receiver_closed = TCPState::state_summary(_receiver) == TCPReceiverStateSummary::FIN_RECV;
    const bool sender_closed = TCPState::state_summary(_sender) == TCPSenderStateSummary::FIN_ACKED;
    const bool time_wait_expired = _time_since_last_segment_received_ms >= 10 * _cfg.rt_timeout;
    if (receiver_closed && sender_closed && _linger_after_streams_finish && time_wait_expired) {
        _is_active = false;               // Mark the connection as inactive
        _linger_after_streams_finish = false;  // Disable lingering
    }
}


//! End the outbound stream
void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input(); // Close the outbound stream
    _sender.fill_window();
    flush_segments_with_ack();
}

//! Initiate connection by sending SYN
void TCPConnection::connect() {
    _sender.fill_window(); // Sends the initial SYN segment
    flush_segments_with_ack();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            set_rst_state(false);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

void TCPConnection::set_rst_state(const bool send_rst) {
    if (send_rst) {
        TCPSegment rst;
        rst.header().rst = true;
        _segments_out.push(rst);
    }
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _linger_after_streams_finish = false;
    _is_active = false;
}

void TCPConnection::flush_segments_with_ack() {
    // traverse all segments in the sender's queue
    while (!_sender.segments_out().empty()) {
        TCPSegment outgoing_segment = std::move(_sender.segments_out().front());
        _sender.segments_out().pop();

        // If the receiver has a valid ACK number, set the ACK flag, acknowledgment number, and window size in the segment header
        auto ack_number = _receiver.ackno(); 
        if (ack_number.has_value()) {
            outgoing_segment.header().ack = true;
            outgoing_segment.header().ackno = ack_number.value();
            outgoing_segment.header().win = _receiver.window_size();
        }

        _segments_out.push(outgoing_segment);
    }
}


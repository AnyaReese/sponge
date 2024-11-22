#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>
#include <algorithm>
#include <cassert>

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

//! \returns the number of bytes currently in flight
uint64_t TCPSender::bytes_in_flight() const {
    return _outgoing_bytes;
}

//! Fills the sending window with data to be sent
void TCPSender::fill_window() {
    // if the window is 0, regard it as 1, else do nothing
    size_t curr_window_size = (_last_window_size != 0) ? _last_window_size : 1;

    // fill the window with data
    while (_outgoing_bytes < curr_window_size) {
        TCPSegment segment;
        
        // send SYN
        if (!_syn_flag) {
            segment.header().syn = true;
            _syn_flag = true;
        }

        // set the sequence number
        segment.header().seqno = next_seqno();

        // set the payload
        size_t remaining_capacity = curr_window_size - _outgoing_bytes - segment.header().syn;
        size_t max_payload_size = min(TCPConfig::MAX_PAYLOAD_SIZE, remaining_capacity);
        Buffer payload = _stream.read(max_payload_size);

        // send FIN: if the stream is empty and the FIN flag is not set
        // eof equals to input_ended and buffer_empty
        if (_stream.eof() && !_fin_flag && (payload.size() + _outgoing_bytes < curr_window_size)) {
            segment.header().fin = true;
            _fin_flag = true;
        }

        segment.payload() = Buffer(payload);

        // if no data to send, break
        // if (payload.size() == 0) { // BUG CODE, if the payload is empty, the segment is still needed to send
        if (segment.length_in_sequence_space() == 0) {
            break;
        }

        // if no dgram awaiting ack, set timeout
        if (_outgoing_map.empty()) {
            _timeout = _initial_retransmission_timeout;
            _timecount = 0;
        }

        // add the segment to the outgoing map
        _segments_out.push(segment);
        _outgoing_map.insert(make_pair(_next_seqno, segment));
        _outgoing_bytes += segment.length_in_sequence_space();
        _next_seqno += segment.length_in_sequence_space();

        // if FIN is set, break
        if (segment.header().fin) {
            break;
        }
    }

}

//! Processes an ACK received from the remote receiver
//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    size_t abs_seqno;
    if (unwrap(ackno, _isn, _next_seqno) > _next_seqno) {
        return;
    } else {
        abs_seqno = unwrap(ackno, _isn, _next_seqno);
    }
    
    // traverse the outgoing map to remove the segments that have been acknowledged
    auto it = _outgoing_map.begin();
    while (it != _outgoing_map.end()) {
        const TCPSegment &seg = it->second;
        // if the segment is acknowledged
        if (it->first + seg.length_in_sequence_space() <= abs_seqno) {
            // Remove the segment from the outgoing map
            _outgoing_bytes -= seg.length_in_sequence_space();
            it = _outgoing_map.erase(it);

            // Reset the retransmission timeout
            _timeout = _initial_retransmission_timeout;
            _timecount = 0;
        } else {
            break; // No need to check the rest
        }
    }

    // reset the consecutive retransmissions count
    _consecutive_retransmissions_count = 0;
    // uodate the last window size and fill the window
    _last_window_size = window_size;
    fill_window();
}


//! Handles the passage of time for the sender
//! \param[in] ms_since_last_tick the number of milliseconds since the last tick
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timecount += ms_since_last_tick;

    auto it = _outgoing_map.begin();
    if (it != _outgoing_map.end() && _timecount >= _timeout) {
        // congestion control
        _timecount = 0;
        // Special case: if the window size is 0, not congestion control
        _timeout = _last_window_size ? _timeout * 2 : _initial_retransmission_timeout;
        _segments_out.push(it->second);
        _consecutive_retransmissions_count++;
    }
}

//! \returns the number of consecutive retransmissions
unsigned int TCPSender::consecutive_retransmissions() const {
    return _consecutive_retransmissions_count;
}

//! Sends an empty segment
void TCPSender::send_empty_segment() {
    TCPSegment segment;
    segment.header().seqno = next_seqno();
    _segments_out.push(segment);
}

#include "tcp_receiver.hh"
#include "wrapping_integers.hh"

using namespace std;

#define DEBUG 1

void TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &header = seg.header();

    // Handle SYN (synchronize) flag
    if (!_syn_flag) {
        if (!header.syn) {
            return; // Ignore segments before SYN
        }
        _isn = header.seqno;      // Save the initial sequence number (ISN)
        _syn_flag = true;     // Mark SYN as received
    }

    // get seqno
    uint64_t seqno = unwrap(header.seqno, _isn, _reassembler.stream_out().bytes_written());

    // get payload
    uint64_t index = seqno - 1 + header.syn;
    _reassembler.push_substring(seg.payload().copy(), index, header.fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    // If SYN has not been received, there is no valid acknowledgment number
    if (!_syn_flag) {
        return {};
    }

    // Calculate the acknowledgment number
    uint64_t ack_abs = _reassembler.stream_out().bytes_written() + 1;
    if (_reassembler.stream_out().input_ended()) {
        ack_abs += 1; // Include FIN if it has been received
    }
    return wrap(ack_abs, _isn);
}

size_t TCPReceiver::window_size() const {
    return _capacity - _reassembler.stream_out().buffer_size();
}

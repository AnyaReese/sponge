#include "byte_stream.hh"
#include <algorithm>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) 
    : _capacity(capacity), 
      _buffer(),
      _bytes_written(0), 
      _bytes_read(0), 
      _input_ended(false) {}

size_t ByteStream::write(const string &data) {
    size_t bytes_to_write = min(data.length(), remaining_capacity());
    _buffer.append(data.substr(0, bytes_to_write));
    _bytes_written += bytes_to_write;
    return bytes_to_write;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    return _buffer.substr(0, min(len, _buffer.length()));
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t pop_size = min(len, _buffer.length());
    _buffer.erase(0, pop_size);
    _bytes_read += pop_size;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string result = peek_output(len);
    pop_output(len);
    return result;
}

void ByteStream::end_input() {
    _input_ended = true;
}

bool ByteStream::input_ended() const {
    return _input_ended;
}

size_t ByteStream::buffer_size() const {
    return _buffer.length();
}

bool ByteStream::buffer_empty() const {
    return _buffer.empty();
}

bool ByteStream::eof() const {
    return _input_ended && buffer_empty();
}

size_t ByteStream::bytes_written() const {
    return _bytes_written;
}

size_t ByteStream::bytes_read() const {
    return _bytes_read;
}

size_t ByteStream::remaining_capacity() const {
    return _capacity - _buffer.length();
}

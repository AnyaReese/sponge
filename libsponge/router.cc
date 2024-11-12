#include "router.hh"

#include <iostream>

using namespace std;

#define DEBUG 1

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    // DUMMY_CODE(route_prefix, prefix_length, next_hop, interface_num);
    _router_table.push_back({route_prefix, prefix_length, next_hop, interface_num});
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // DUMMY_CODE(dgram);
    const uint32_t dest = dgram.header().dst;
    if (DEBUG) {
        cerr << "\033[1;33mDEBUG: Routing datagram with destination " << Address::from_ipv4_numeric(dest).ip() << "\033[0m\n";
    }

    
    // first check if the ttl is 0
    if (dgram.header().ttl == 0 || --dgram.header().ttl == 0) {
        if (DEBUG) {
            cerr << "\033[1;33mDEBUG: TTL is 0, dropping datagram\033[0m\n";
        }
        return;
    }

    // then check if the destination is in the router table
    auto max_matched_entry = _router_table.end();
    for (auto it = _router_table.begin(); it != _router_table.end(); ++it) {
        // invalid prefix length
        if (it->prefix_length > 32) {
            continue;
        }
        // mask is the prefix_length high-order bits of the route_prefix
        uint32_t mask = it->prefix_length == 0 ? 0 : (0xFFFFFFFF << (32 - it->prefix_length));
        if ((dest & mask) == (it->route_prefix & mask)) {
            if (max_matched_entry == _router_table.end() || it->prefix_length > max_matched_entry->prefix_length) {
                max_matched_entry = it;
            }
        }
    }

    // send the datagram
    if (max_matched_entry != _router_table.end()) {
        if (DEBUG) {
            cerr << "\033[1;33mDEBUG: Sending datagram to " << Address::from_ipv4_numeric(dest).ip() << " on interface "
                 << max_matched_entry->interface_num << "\033[0m\n";
        }
        // if the next hop is not empty, send the datagram to the next hop
        if (max_matched_entry->next_hop.has_value()) {
            interface(max_matched_entry->interface_num).send_datagram(dgram, max_matched_entry->next_hop.value());
        } else {
            interface(max_matched_entry->interface_num).send_datagram(dgram, Address::from_ipv4_numeric(dest));
        }
    } else {
        if (DEBUG) {
            cerr << "\033[1;33mDEBUG: No route found for destination " << Address::from_ipv4_numeric(dest).ip() << ", dropping datagram\033[0m\n";
        }
    }


}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}

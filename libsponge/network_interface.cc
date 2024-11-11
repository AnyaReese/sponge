#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

#define DEBUG 1

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "\033[1;33mDEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\033[0m\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    // create an ARP request for the next hop IP address
    const auto &arp_iter = _arp_table.find(next_hop_ip);
    // if the next hop IP address is not in the ARP table, send an ARP request
    if (arp_iter == _arp_table.end()) {
        ARPMessage arp_request;
        arp_request.opcode = ARPMessage::OPCODE_REQUEST;
        arp_request.sender_ethernet_address = _ethernet_address;
        arp_request.sender_ip_address = _ip_address.ipv4_numeric();
        arp_request.target_ethernet_address = {};
        arp_request.target_ip_address = next_hop_ip;
        if (DEBUG) {
            cerr << "\033[1;33mDEBUG: Sending ARP request for IP address " << next_hop_ip << "\033[0m\n";
        }

        // serialize the ARP request and create an Ethernet frame
        EthernetFrame frame;
        frame.header().dst = ETHERNET_BROADCAST; //! Ethernet broadcast address (ff:ff:ff:ff:ff:ff)
                                                 // constexpr EthernetAddress ETHERNET_BROADCAST = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        frame.header().src = _ethernet_address;
        frame.header().type = EthernetHeader::TYPE_ARP;
        frame.payload() = arp_request.serialize();

        // push the Ethernet frame onto the outbound queue
        _frames_out.push(frame);

        // remember that we're waiting for an ARP reply
        _waiting_arp_response[next_hop_ip] = 0;

        // remember the IP datagram we're waiting to send
        _waiting_ip_response.push_back({next_hop, dgram});
    } else {
        // create an Ethernet frame with the known Ethernet address
        EthernetFrame frame;
        frame.header().src = _ethernet_address;
        frame.header().dst = arp_iter->second.ethernet_address;
        frame.header().type = EthernetHeader::TYPE_IPv4;
        frame.payload() = dgram.serialize();

        // push the Ethernet frame onto the outbound queue
        _frames_out.push(frame);
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // DUMMY_CODE(frame);
    // filter out frames not addressed to us
    if (frame.header().dst != _ethernet_address && frame.header().dst != ETHERNET_BROADCAST) {
        if (DEBUG) {
            cerr << "\033[1;33mDEBUG: Frame not addressed to us\033[0m\n";
        }
        return {};
    }

    // handle ARP requests
    if (frame.header().type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp_request;
        arp_request.parse(frame.payload());

        const uint32_t &src_ip_addr = arp_request.sender_ip_address;
        const EthernetAddress &src_eth_addr = arp_request.sender_ethernet_address;
        const uint32_t &target_ip_addr = arp_request.target_ip_address;
        const EthernetAddress &target_eth_addr = arp_request.target_ethernet_address;

        bool ValidRequest = (target_ip_addr == _ip_address.ipv4_numeric()) && (arp_request.opcode == ARPMessage::OPCODE_REQUEST);
        bool ValidResponese = (target_eth_addr == _ethernet_address) && (arp_request.opcode == ARPMessage::OPCODE_REPLY);

        if (DEBUG) {
            cerr << "\033[1;32mDEBUG: ARP table updated with IP " << src_ip_addr
                << " -> Ethernet " << to_string(src_eth_addr) << "\033[0m\n";
        }

        if (DEBUG && ValidRequest) {
            cerr << "\033[1;34mDEBUG: Sending ARP reply to " << to_string(src_eth_addr) << "\033[0m\n";
        }

        if (DEBUG && ValidResponese) {
            cerr << "\033[1;33mDEBUG: Received ARP reply, sending waiting datagrams\033[0m\n";
        }

        if (ValidRequest) {
            ARPMessage arp_reply;
            arp_reply.opcode = ARPMessage::OPCODE_REPLY;
            arp_reply.sender_ethernet_address = _ethernet_address;
            arp_reply.sender_ip_address = _ip_address.ipv4_numeric();
            arp_reply.target_ethernet_address = src_eth_addr;
            arp_reply.target_ip_address = src_ip_addr;

            // serialize the ARP reply and create an Ethernet frame
            EthernetFrame eth_frame;
            eth_frame.header().dst = src_eth_addr;
            eth_frame.header().src = _ethernet_address;
            eth_frame.header().type = EthernetHeader::TYPE_ARP;
            eth_frame.payload() = arp_reply.serialize();

            // push the Ethernet frame onto the outbound queue
            _frames_out.push(eth_frame);
        }

        if (ValidRequest || ValidResponese) {
            // update the ARP table
            _arp_table[src_ip_addr] = {src_eth_addr, 0};

            // check if we have any IP datagrams waiting to be sent to this IP address
            for (auto it = _waiting_ip_response.begin(); it != _waiting_ip_response.end();) {
                if (it->first.ipv4_numeric() == src_ip_addr) {
                    send_datagram(it->second, it->first);
                    // remove the IP datagram from the waiting list
                    it = _waiting_ip_response.erase(it);
                } else {
                    ++it;
                }
                // erase the IP datagram from the waiting list
                _waiting_arp_response.erase(src_ip_addr);
            }
        }
    
    }

    // handle IPv4 datagrams
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {

        if (DEBUG) {
            cerr << "\033[1;33mDEBUG: Received IPv4 datagram\033[0m\n";
        }

        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) != ParseResult::NoError) {
            return {};
        }
        dgram.parse(frame.payload());
        return dgram;
    }

    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    
    // delete ARP entries that have timed out
    for (auto it = _arp_table.begin(); it != _arp_table.end();) {
        it->second.last_seen += ms_since_last_tick;
        if (it->second.last_seen >= ARP_TIMEOUT_MS) {
            it = _arp_table.erase(it);
        } else {
            ++it;
        }
    }

    // delete ARP requests that have timed out
    for (auto it = _waiting_arp_response.begin(); it != _waiting_arp_response.end();) {
        it->second += ms_since_last_tick;
        if (it->second >= ARP_CLEANUP_INTERVAL_MS) {
            // resend the ARP request
            ARPMessage arp_request;
            arp_request.opcode = ARPMessage::OPCODE_REQUEST;
            arp_request.sender_ethernet_address = _ethernet_address;
            arp_request.sender_ip_address = _ip_address.ipv4_numeric();
            arp_request.target_ethernet_address = {};
            arp_request.target_ip_address = it->first;

            // serialize the ARP request and create an Ethernet frame
            EthernetFrame frame;
            frame.header().dst = ETHERNET_BROADCAST;
            frame.header().src = _ethernet_address;
            frame.header().type = EthernetHeader::TYPE_ARP;
            frame.payload() = arp_request.serialize();

            // push the Ethernet frame onto the outbound queue
            _frames_out.push(frame);
            it->second = 0;
        } else {
            ++it;
        }
    }
}

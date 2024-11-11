# Lab3 the network interface and IP router

## 一、实验目的：

- 学习掌握网络接口的工作原理
- 学习掌握ARP地址解析协议相关知识
- 学习掌握IP路由的工作原理

## 二、实验内容

- 实现network interface，为每个下一跳地址查找（和缓存）以太网地址，即实现地址解析协议ARP。
- 实现简易路由器，对于给定的数据包，确认发送接口以及下一跳的IP地址。

## 三、主要仪器设备

- 联网的PC机
- Linux虚拟机

## 四、操作方法与实验步骤

(1) 地址解析协议：首先是实现网络接口，即完成ARP地址解析协议的代码编写。文件路径 sponge/libsponge/network_interface.cc。

1. 首先你需要阅读以下内容：
    - NetworkInterface 对象的公共接口。
    - 维基百科对 ARP 的总结以及原始的ARP规范（RFC 826）。
    - EthernetFrame 和 EthernetHeader 对象的文档和实现。
    - IPv4Datagram 和 IPv4Header 对象的文档和实现（这可以解析和序列化一个 Internet 数据报，当序列化时，可以作为以太网帧的有效负载）。、
    - ARPMessage 对象的文档和实现。（它知道如何解析和序列化 ARP 消息，也可以在序列化时作为以太网帧的有效负载）。

2. 实现 `network_interface.cc` 文件中的三个方法：自行在 `.hh` 文件里添加额外的成员变量。
    1. `NetworkInterface::send_datagram()`：当调用方希望向下一跳发送出IP数据包时，将调用此方法将此数据报转换为以太网帧并发送。
    - 如果目标以太网地址已经知道，立即发送。创建一个类型为    `EthernetHeader::TYPE_IPv4` 的以太网帧，将序列化的数据报设置为负载，并设置源地址和目的地址。
    - 如果目的以太网地址未知，则广播请求下一跳的以太网地址，并将IP数据报排队，以便在收到ARP应答后发送。
    2. `NetworkInterface::recv_frame()`：当以太网帧从网络到达时调用此方法，代码应该忽略任何不发送到该网络接口的帧。
    - 如果入站帧是 IPv4，将有效负载解析为 InternetDatagram（调用 `parse()`），如果成功（即，`parse()` 方法返回 `ParseResult::NoError`），将结果 InternetDatagram 返回给调用者。
    - 如果入站帧是 ARP，将负载解析为 ARPMessage。若成功，记住发送者的 IP 地址和以太网地址之间的映射关系 30 秒。另外，如果它是一个请求我们的 IP 地址的 ARP 请求，发送一个适当的 ARP 回复。
    3. `NetworkInterface::tick()`:此方法在到达时间点时调用，终止已过期的IP到Ethernet的映射。

3.  完成代码编写后，你可以运行 `ctest -V -R “^arp”` 命令来进行测试。


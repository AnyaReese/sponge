# Lab5: the summit (TCP in full)

> 3220103784 林子昕

## 一、实验背景
- 学习掌握TCP的工作原理

## 二、实验目的
- 学习掌握TCP的工作原理
- 学习掌握TCP connection的相关知识
- 学习掌握协议栈结构

## 三、实验内容
- 将TCPSender和TCPReceiver结合，实现一个TCP终端，同时收发数据。
- 实现TCP connection的状态管理，如连接和断开连接等。
- 整合网络接口、IP路由以及TCP并实现端到端的通信。

## 四、主要仪器设备
- 联网的PC机
- Linux虚拟机

## 五、操作方法与实验步骤
在开始整合代码之前，你需要了解以下TCPConnection的准则：

1. **接收端**，TCPConnection的segment_received()方法被调用时，它从网络中接收TCPSegment。当这种情况发生时，TCPConnection将查看segment并且：
   - 如果设置了RST标志，将入站流和出站流都设置为错误状态，并永久终止连接。
   - 把这个段交给TCPReceiver，这样它就可以在传入的段上检查它关心的字段：seqno、SYN、负载以及FIN。
   - 如果设置了ACK标志，则告诉TCPSender它关心的传入段的字段：ackno和window_size。
   - 如果传入的段包含一个有效的序列号，TCPConnection确保至少有一个段作为应答被发送，以反映ackno和window_size的更新。
   - 有一个特殊情况，你将不得不在TCPConnection的segment_received()方法中处理：响应一个"keep-alive"段。对方可能会选择发送一个带有无效序列号的段，以查看你的TCP是否仍然有效，你的TCPConnection应该回复这些"keep-alive"段。实现的代码如下：
     ```cpp
     if (_receiver.ackno().has_value() and (seg.length_in_sequence_space() == 0) and seg.header().seqno == _receiver.ackno().value() - 1) 
     { 
         _sender.send_empty_segment()
     }
     ```

2. **发送端**，TCPConnection将通过网络发送TCPSegment：
   - TCPSender通过调用tcp_sender.cc中的函数，将一个TCPSegment数据包添加到待发送队列中，并设置一些字段（seqno，SYN，FIN）。
   - 在发送当前数据包之前，TCPConnection 会获取当前它自己的 TCPReceiver 的 ackno 和 window size，将其放置进待发送 TCPSegment 中（设置window_size和ackno），并设置其 ACK 标志。

3. **tick**，当时间流逝，TCPConnection有一个tick()方法，该方法将被操作系统定期调用，当这种情况发生时，TCPConnection需要：
   - 告诉TCPSender时间的流逝。
   - 如果连续重传的次数超过上限TCPConfig::MAX_RETX_ATTEMPTS，则终止连接，并发送一个重置段给对端（设置了RST标志的空段）。
   - 如有必要，结束连接。

## 六、实验数据记录和处理
以下实验记录均需结合屏幕截图（截取源代码或运行结果），进行文字标注。

- 实现TCPConnection的关键代码截图
- 运行make check命令的运行结果截图
- 重新编写webget.cc的代码截图
- 重新测试webget的测试结果展示
- 最终测试中服务器的运行截图
- 最终测试中客户端的运行截图
- 【选做】在成功连接后端到端发送消息的运行截图
- 【选做】成功关闭连接的截图

## 七、实验数据记录和处理
根据你编写的程序运行效果，分别解答以下问题：

- ACK标志的目的是什么？ackno是经常存在的吗？
- 请描述TCPConnection的代码中是如何整合TCPReceiver和TCPSender的。
- 【选做】在最终测试中服务器和客户端能互连吗？如果不能，你分析是什么原因？
- 【选做】在关闭连接的时候是否两端都能正常关闭？如果不能，你分析是原因？

## 八、实验数据记录和处理
实验过程中遇到的困难，得到的经验教训，对本实验安排的更好建议。






# Lab2 Webget

## 一、实验目的：

- 学习掌握Linux虚拟机的用法
- 学习掌握网页的抓取方法
- 学习掌握ByteStream的相关知识
- 学习掌握C++的新特性

二、实验内容
- 安装配置Linux虚拟机并在其上完成本次实验。
- 编写小程序webget，通过网络获取web页面，类似于wget。
- 实现字节流ByteStream：
    - 字节流可以从写入端写入，并以相同的顺序，从读取端读取；
    - 字节流是有限的，写者可以终止写入。而读者可以在读取到字节流末尾时，产生EOF标志，不再读取；
    - 支持流量控制，以控制内存的使用；
    - 写入的字节流可能会很长，必须考虑到字节流大于缓冲区大小的情况。

## 三、实验步骤

### Task1：抓取网页

抓取网页: 在正式进行编码工作之前，你需要对本实验第一个任务，即抓取一个网页，有更深刻的理解。
- 在浏览器中，访问 http://cs144.keithw.org/hello 并观察结果。

![20240927214748](https://raw.githubusercontent.com/AnyaReese/PicGooo/main/images/20240927214748.png?token=A246ITWBZNNXLL4YFVPRPRTG6234I)

- 在你的虚拟中运行telnet cs144.keithw.org http命令，它告诉telnet程序在你的计算机与另一台计算机（名为cs144.keithw.org）之间打开一个可靠的字节流，并在这台计算机运行一个特定的服务：“http”服务。

```bash
telnet cs144.keithw.org http
Trying 104.196.238.229...
Connected to cs144.keithw.org.
Escape character is '^]'.
#     - 输入GET /hello HTTP/1.1以及回车键，这告诉服务器URL的路径部分。
GET /hello HTTP/1.1
# 输入Host: cs144.keithw.org以及回车键，这告诉服务器URL的主机部分。
Host: cs144.keithw.org

HTTP/1.1 200 OK
Date: Fri, 27 Sep 2024 11:56:24 GMT
Server: Apache
Last-Modified: Thu, 13 Dec 2018 15:45:29 GMT
ETag: "e-57ce93446cb64"
Accept-Ranges: bytes
Content-Length: 14
Content-Type: text/plain

Hello, CS144! # 网页内容

# 输入Connection: close以及回车键，这告诉服务器你已经完成了HTTP请求。
Connection: close
HTTP/1.1 400 Bad Request
Date: Fri, 27 Sep 2024 11:56:40 GMT
Server: Apache
Content-Length: 226
Connection: close
Content-Type: text/html; charset=iso-8859-1

# 需再次敲击回车键，将返回结果与浏览器的返回结果进行比较。
<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html><head>
<title>400 Bad Request</title>
</head><body>
<h1>Bad Request</h1>
<p>Your browser sent a request that this server could not understand.<br />
</p>
</body></html>
Connection closed by foreign host.
```

## Task2: 代码仓库准备工作

准备工作: 从github上抓取初始代码文件并完成环境搭建。
- 在虚拟机上，获取初始代码文件。运行

`git clone -b lab7-startercode https://github.com/anyareese/sponge`
- 运行 cd sponge 命令进入sponge目录。
- 运行mkdir build命令构建build目录来编译实验代码。
- 运行cd build命令进入build目录
- 运行cmake ..命令建立搭建系统
- 运行make命令编译源代码，需注意每次你对项目进行修改都需运行make命令。
- 实验代码的编写以现代C++风格完成，使用最新的2011特性尽可能安全地编程。（http://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines）。

## Task3: 阅读sponge文档

阅读sponge文档: sponge封装了操作系统函数，请务必在编写实验代码前仔细阅读相关的基础代码文件。
- 阅读入门代码文档（https://cs144.github.io/doc/lab0）。
- 阅读 FileDescriptor, Socket, TCPSocket以及Address这些类的文档。
- 阅读描述了这些类的头文件，请查阅libsponge/util目录：file_descriptor.hh, socket.hh以及address.hh。

## Task4: 编写webget

实现 webget: 完成 webget.cc 程序代码的编写以实现抓取网页的功能。要求实现get_URL函数，功能为向指定IP地址发送HTTP GET请求，然后输出所有响应。可参考配套Doc中TCPSocket的示例代码。
- 在build目录下，文本编辑器或IDE下打开../apps/webget.cc。
- 在get_URL函数中完成实现，实现代码使用HTTP（Web）请求的格式。使用TCPSocket类以及Address类。
- 运行 make 命令编译程序。
- 运行 ./apps/webget cs144.keithw.org /hello 命令进行程序的测试
- 通过上面的测试后运行 make check_webget命令进行自动测试。
注意点：
  - 在HTTP中，每行必须以 “\r\n” 结尾。
  -  Connection:close 这句代码必须包含在客户端的请求中。
  - 确保从服务器读取和打印所有的输出，即直到套接字到达 “EOF”（文件的末尾），才停止打印输出。

```cpp
    // 创建一个TCP套接字并连接到指定的主机和HTTP服务
    TCPSocket sock;
    sock.connect(Address(host, "http"));

    // 发送HTTP GET请求
    sock.write("GET " + path + " HTTP/1.1\r\n");
    sock.write("Host: " + host + "\r\n");
    sock.write("Connection: close\r\n");
    sock.write("\r\n");

    // 读取并打印服务器的响应
    while (!sock.eof()) {
        cout << sock.read();
    }

    // 关闭套接字连接
    sock.close();
```
![20240928023752](https://raw.githubusercontent.com/AnyaReese/PicGooo/main/images/20240928023752.png?token=A246ITRE5MOWBRPJWH5CGSLG6354A)

![20240928023808](https://raw.githubusercontent.com/AnyaReese/PicGooo/main/images/20240928023808.png?token=A246ITSVIEI6HLEXR2VT5NLG6355A)

## Task5: 实现ByteStream

可靠字节流：完成 ByteStream 的代码编写工作。
- 打开 libsponge/byte_stream.hh 以及 libsponge/byte_stream.cc 文件，并完成接口内的方法的实现，并按自己的需求增加 ByteStream 类中的私有成员变量。各个接口的描述可查看注释得到。
- 完成代码的编写后，运行 make 编译程序。
- 运行 make check_lab0 命令进行自动测试。
注意点：
- 若看完注释后仍然对接口的实现逻辑不清楚的，可以查看测试样例从而加深理解。路径：sponge/test/，其中 byte_stream_test_harness.hh 和 byte_stream_test_harness.cc 为测试所需方法的实现，其余 byte_stream_xxx.cc 为测试样例。

- ByteStream.hh 中添加类中的私有成员变量：

```cpp
  private:
    size_t _capacity;          // 流的总容量
    std::string _buffer;       // 用于存储数据的缓冲区
    size_t _bytes_written{0};  // 已写入的字节数
    size_t _bytes_read{0};     // 已读取的字节数
    bool _input_ended{false};  // 输入是否结束的标志
```

- ByteStream.cc 中实现构造函数：

```cpp
ByteStream::ByteStream(const size_t capacity) 
    : _capacity(capacity), 
      _buffer(),
      _bytes_written(0), 
      _bytes_read(0), 
      _input_ended(false) {}
```

```cpp
size_t ByteStream::write(const string &data) {
    size_t bytes_to_write = min(data.length(), remaining_capacity());
    _buffer.append(data.substr(0, bytes_to_write));
    _bytes_written += bytes_to_write;
    return bytes_to_write;
}
```

- 实现 peek_output 接口：

```cpp
string ByteStream::peek_output(const size_t max_len) const {
    size_t bytes_to_peek = min(max_len, _buffer.length());
    return _buffer.substr(0, bytes_to_peek);
}
```

- 实现 pop_output 接口：

```cpp
//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t pop_size = min(len, _buffer.length());
    _buffer.erase(0, pop_size);
    _bytes_read += pop_size;
}
```

- 实现 read 接口：

```cpp
//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string result = peek_output(len);
    pop_output(len);
    return result;
}
```

- 实现其他接口：

```cpp
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
```
- 运行 make check_lab0 命令进行自动测试。

![20240928025124](https://raw.githubusercontent.com/AnyaReese/PicGooo/main/images/20240928025124.png?token=A246ITUPLGNCK7DV6ITNXILG637O2)

## 实验处理和数据记录

- 第二步中抓取网页（通过浏览器和telnet）的运行结果
- 编写的webget关键代码，即get_URL中的代码
- 使用webget抓取网页运行结果
- 运行make check_webget的测试结果展示
- 实现ByteStream关键代码截图（描述总体，省略细节部分）
- 运行make check_lab0测试结果。

均已在实验报告中展示。

## Task6: 实验数据记录和处理

### Q1:

> 完成 webget 程序编写后的测试结果和 Fetch a Web page 步骤的运行结果一致吗？如果不一致的话你认为问题出在哪里？请描述一下所写的 webget 程序抓取网页的流程。

完成 webget 程序编写后的测试结果和 Fetch a Web page 步骤的运行结果应该是**一致**的。如果出现不一致,可能的原因包括:
  1. HTTP 请求头格式不正确。例如,没有正确使用 "\r\n" 作为行结束符。
  2. 没有正确处理服务器响应。比如没有读取所有数据直到 EOF。
  3. 网络连接问题导致数据传输不完整。
  4. 服务器端的变化,导致响应内容有所不同。

webget 程序抓取网页的流程:
  1. 创建 TCP 套接字并连接到指定的主机和 HTTP 服务端口。
  2. 构造 HTTP GET 请求,包括请求行、Host 头部和 Connection: close 头部。
  3. 发送 HTTP 请求到服务器。
  4. 循环读取服务器的响应数据,直到遇到 EOF。
  5. 将读取到的所有响应数据输出到标准输出。
  6. 关闭 TCP 连接。

### Q2:

> 请描述ByteStream是如何实现流控制的？

ByteStream 通过以下方式实现流控制：

1. **缓冲区大小限制**：ByteStream 类中有一个缓冲区，用于存储接收到的数据。缓冲区的大小由 _capacity 变量表示，当缓冲区满时，新的数据将无法写入。
2. **流量控制**：ByteStream 类中有一个 _bytes_read 变量，用于记录已经读取的字节数。当读取的字节数达到缓冲区大小时，将停止读取，直到缓冲区中有新的空间。
3. **EOF 标志**：当输入结束时，ByteStream 类中有一个 _input_ended 变量，用于标记输入是否结束。当 _input_ended 为 true 时，将停止读取，直到缓冲区中有新的空间。

### Q3:

> 当遇到超出capacity范围的数据流的时候，该如何进行处理？如果不限制流的长度的时候该如何处理？

当遇到超出capacity范围的数据流的时候，ByteStream 类中有一个 _capacity 变量，用于表示缓冲区的最大容量。当缓冲区满时，新的数据将无法写入。
如果不限制流的长度，可以将 _capacity 设置为一个很大的值，或者动态分配内存以存储数据。

### Q4:

> 你觉得这两种传参方式void Writer::push( string data )和void Writer::push( const string& data )有什么区别（注意：仅针对这一个函数而言）？尝试从性能和语法两方面说明。
Hint: Effective Modern C++   Item 41: Consider pass by value for copyable parameters that are cheap to move and always passed by value.

这两种传参方式 void Writer::push( string data ) 和 void Writer::push( const string& data ) 的区别如下：

1. **语法区别**：
    - void Writer::push( string data )：这种方式是按值传递，会将实参 data 的值复制到形参 data 中，然后在函数内部使用这个副本进行操作。
    - void Writer::push( const string& data )：这种方式是按引用传递，会将实参 data 的引用传递给形参 data，在函数内部使用这个引用进行操作。

2. **性能区别**：
    - void Writer::push( string data )：这种方式会涉及到一次字符串的复制操作，如果数据量较大，性能开销会比较大。
    - void Writer::push( const string& data )：这种方式不会涉及到字符串的复制操作，性能开销较小。

## 实验总结

在实验中，我完成了webget程序的编写，实现了抓取网页的功能；实现了ByteStream类，实现了字节流的相关操作。通过实验，我对网络编程和C++编程有了更深入的理解，提高了自己的编程能力。






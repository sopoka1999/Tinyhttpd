*Tinyhttpd fork 自 [Tinyhttpd](https://github.com/EZLippi/Tinyhttpd)，仅供学习*



## 项目描述

### 功能描述:

1. 处理GET文件请求：根据请求的文件类型自动设置Content-Type，并根据文件大小自动设置Content-Length。这使其能够充当一个静态Web服务器。

2. 处理带参数的GET请求：具备解析和处理带参数的GET请求的能力。

3. 处理POST请求：当前支持接收并解析传递的JSON格式数据。

4. 支持HTTP长连接：每个与客户端建立的socket都会在一个独立的线程中处理，通过select机制来实现请求的监听。如果某个socket在一段时间内没有接收到数据，会主动关闭该socket，以避免过多线程的创建和资源浪费。



## 运行环境

Linux  20.04

gcc  11.4.0

GNU Make 4.3



## 编译
```shell
cd Tinyhttpd-master
make all
```



## 运行
```shell
./httpd
```



## 效果
访问：`http://ip:4000/` 或者 `http://localhost:4000/`

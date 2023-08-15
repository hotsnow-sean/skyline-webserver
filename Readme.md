<h1 align="center">
🚀 Skyline WebServer 🚀
</h1>
<p align="center">
An Linux Web Server Framework Based on Reactor Developed Using Modern C++.
</p>

## 项目结构

```shell
skyline/
|
|- core/   # 核心库，使用主从反应堆，提供基本的Tcp服务器框架
|- http/   # HTTP库，提供 http 解析 和 路径分发的功能，实现基本的 HTTP 服务器
|- logger/ # 日志库，类 Log4J
```

## 框架使用方法

### 核心库使用方式

服务器开发者只需要继承 `TcpServer` 类，并重写 `AfterConnect` 和 `OnRecv` 方法，即可开发自己的服务器。

### HTTP库使用方式

见 `examples` 文件夹的 `http_test.cc`

### 链接方式

如果只需要核心库功能，链接 `skylien_core` 即可；如果要使用 HTTP 库，直接链接 `skyline_http` 即可。

## Performance

### test command

```shell
webbench -c 1000 -t 30 -2 http://127.0.0.1:8889/
```

### 1 main reactor

```shell
Speed=1623732 pages/min, 6900861 bytes/sec.
Requests: 811866 susceed, 0 failed.
```

avg QPS: **27062.2**

### 1 main reactor + 4 sub reactors

```shell
Speed=4565396 pages/min, 19402924 bytes/sec.
Requests: 2282698 susceed, 0 failed.
```

avg QPS: **76089.9**

### 1 main reactor + 8 sub reactors

```shell
Speed=4508036 pages/min, 19159162 bytes/sec.
Requests: 2254018 susceed, 0 failed.
```

avg QPS: **75133.9**

## 其它

`skyline/http` 目录下以 `.rl.cc` 结尾的文件为开源软件 `Ragel` 生成。

## 参考

+ https://github.com/chenshuo/muduo
+ https://github.com/sylar-yin/sylar
+ https://github.com/linyacool/WebServer

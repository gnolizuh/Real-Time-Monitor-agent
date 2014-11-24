Real-Time Monitor agent
==========================

### 功能简介
> 接收的媒体流为RTP协议封装
> 每个窗口分别包含一个音频和视频处理线程
> 每路RTP流都包含一个抖动延迟buffer

### 高性能

使用libevent提供的高性能IO, 内部实现了多线程

## 扩展性

c++11特性  XML配置文件  日志打印

## 参考、使用的开源项目
* [Libevent](https://github.com/nmathewson/Libevent) ([BSD License](https://github.com/nmathewson/Libevent/blob/master/LICENSE))
* [Pjsip](http://www.pjsip.org/) ([GPL v2 License](http://www.pjsip.org/licensing.htm))
* [ffmpeg](https://github.com/FFmpeg/FFmpeg) ([GPL v2 License](https://github.com/FFmpeg/FFmpeg/blob/master/LICENSE.md))

## 软件截图
![alt tag](https://github.com/gnolizuh/multi-media-monitor-client/blob/master/sinashow-monitor.jpg)

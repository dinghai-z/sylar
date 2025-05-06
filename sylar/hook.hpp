/**
 * @file hook.h
 * @brief hook函数封装
 * @author sylar.yin
 * @email 564628276@qq.com
 * @date 2019-06-02
 * @copyright Copyright (c) 2019年 sylar.yin All rights reserved (www.sylar.top)
 */

 #ifndef __SYLAR_HOOK_H__
 #define __SYLAR_HOOK_H__
 
 #include <fcntl.h>
 #include <sys/ioctl.h>
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <stdint.h>
 #include <time.h>
 #include <unistd.h>
 
 namespace sylar {
     /**
      * @brief 当前线程是否hook
      */
     bool is_hook_enable();
     /**
      * @brief 设置当前线程的hook状态
      */
     void set_hook_enable(bool flag);
 }
 
 extern "C" {
 
 //sleep
 typedef unsigned int (*sleep_fun)(unsigned int seconds);
 extern sleep_fun sleep_f;
 
 typedef int (*accept_fun)(int s, struct sockaddr *addr, socklen_t *addrlen);
 extern accept_fun accept_f;
 
 //read
 typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
 extern read_fun read_f;
 
 //write
 typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
 extern write_fun write_f;
 }
 
 #endif
 
#include "sylar/hook.hpp"
#include "sylar/log.hpp"
#include "sylar/iomanager.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>


sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void handler(int connect_fd, sockaddr_in client_sin){
    while(1){
        char buffer[256] = {0};
        char str[INET_ADDRSTRLEN];
        int ret = read(connect_fd, buffer, sizeof(buffer));
        if(ret > 0){
            SYLAR_LOG_DEBUG(g_logger) << inet_ntop(AF_INET, &client_sin.sin_addr, str, sizeof(str)) << ":" << client_sin.sin_port << ": " << buffer;
            int ret = write(connect_fd, buffer, strlen(buffer));
            if(ret == -1){
                SYLAR_LOG_DEBUG(g_logger) << "accept error";
                continue;
            }
        } else if(ret == 0){
            SYLAR_LOG_DEBUG(g_logger) << "disconnected: " << inet_ntop(AF_INET, &client_sin.sin_addr, str, sizeof(str)) << ":" << client_sin.sin_port;
            close(connect_fd);
            return;
        } else {
            SYLAR_LOG_DEBUG(g_logger) << "read error";
            return;
        }
    }
}

int main(int argc, char** argv) {
    g_logger->setLevel(sylar::LogLevel::ERROR);
    sylar::IOManager iom(8, "iomanager");

    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(8088);
    bind(listen_fd, (sockaddr*)&sin, sizeof(sin));
    listen(listen_fd, 1024);
    fcntl(listen_fd, F_SETFL, O_NONBLOCK);

    for(int i = 0; i != 1; i++){
        iom.schedule([=](){
            while(1){
                sleep(10);
                SYLAR_LOG_ERROR(g_logger) << std::to_string(i) << " sleep 10s";
            }
        });
    }

    while(1){
        sockaddr_in client_sin;
        socklen_t sock_addr_len = INET_ADDRSTRLEN;
        int connect_fd = accept(listen_fd, (sockaddr*)&client_sin, &sock_addr_len);
        if(connect_fd == -1){
            SYLAR_LOG_DEBUG(g_logger) << "accept error";
            continue;
        }
        char str[INET_ADDRSTRLEN];
        SYLAR_LOG_DEBUG(g_logger) << "connect from: " << inet_ntop(AF_INET, &client_sin.sin_addr, str, sizeof(str)) << ":" << client_sin.sin_port;
        
        fcntl(connect_fd, F_SETFL, O_NONBLOCK);
        sylar::IOManager::GetThis()->addEvent(connect_fd, sylar::IOManager::READ, [connect_fd, client_sin](){
            handler(connect_fd, client_sin);
        });
    }

    return 0;
}

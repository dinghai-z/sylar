#include "sylar/hook.hpp"
#include "sylar/log.hpp"
#include "sylar/iomanager.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <thread>
#include <sys/stat.h>
#include <curl/curl.h>
#include "application.hpp"

// Define static member variables
std::mutex Application::detectMutex;
std::condition_variable Application::detectCV;
std::mutex Application::writeDoneMutex;
std::condition_variable Application::writeDoneCV;
DoubleBuffer<std::vector<int>> Application::detect_buffer[2];
std::atomic<Application::device_state> Application::dev_state[2];
std::atomic<bool> Application::writeDone[2];

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void Application::upload_files(){
    CURL *curl;
    CURLcode res;
    FILE *hd_src;
    struct stat file_info;
    const char *local_file = "test.txt";  // 本地文件名
    const char *ftp_url = "ftp://192.168.1.22/Desktop/test.txt"; // 上传后路径

    // 获取文件大小
    if(stat(local_file, &file_info)) {
        SYLAR_LOG_DEBUG(g_logger) << "stat error";
        return;
    }

    // 打开文件用于读取
    hd_src = fopen(local_file, "rb");
    if(!hd_src) {
        SYLAR_LOG_DEBUG(g_logger) << "fopen error";
        return;
    }

    curl = curl_easy_init();
    if(curl) {
        // 设置FTP目标URL
        curl_easy_setopt(curl, CURLOPT_URL, ftp_url);

        // 设置用户名和密码
        curl_easy_setopt(curl, CURLOPT_USERPWD, "dinghai:2580");

        // 启用上传
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        // 传入文件指针
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);

        // 告诉libcurl要上传的文件大小
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);

        // 执行上传
        res = curl_easy_perform(curl);

        // 检查错误
        if(res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        // 清理
        curl_easy_cleanup(curl);
    }

    fclose(hd_src);
}

void Application::cap_detect_fiber(int dev_num){
    SYLAR_LOG_DEBUG(g_logger) << "cap_detect_fiber " << dev_num << " start";
    for(int i = 0; i < 10000; i++){
        {
            std::lock_guard<std::mutex> lk(detectMutex);
            std::vector<int> data(8, i);
            !detect_buffer[dev_num].write(data);
        }
        detectCV.notify_one();
        usleep(1000 * 33);
        if(dev_state[dev_num] != Capturing){
            SYLAR_LOG_DEBUG(g_logger) << "cap_detect_fiber " << dev_num << " stop capturing";
            break;
        }
    }
    //模拟文件写入完成
    if(dev_num == 0){
        sleep(1);
        writeDone[0].store(true);
    } else {
        sleep(5);
        writeDone[1].store(true);
    }
    SYLAR_LOG_DEBUG(g_logger) << "file " << dev_num << " write done";
    // Notify the main thread to stop capturing
    {
        std::lock_guard<std::mutex> lk(writeDoneMutex);
        writeDoneCV.notify_one();
    }
    SYLAR_LOG_DEBUG(g_logger) << "cap_detect_fiber " << dev_num << " end";
}

void Application::detect_thread(){
    SYLAR_LOG_DEBUG(g_logger) << "detect_thread start";
    while(1){
        std::unique_lock<std::mutex> lk(detectMutex);
        detectCV.wait(lk, [](){
            return detect_buffer[0].hasNew() && detect_buffer[1].hasNew();
        });
        std::vector<int> data[2];
        for(int i = 0; i < 2; i++){
            detect_buffer[i].read(data[i]);
        }
        lk.unlock();
        for(int i = 0; i < 2; i++){
            usleep(1000 * 30);
            char buf[4096]{0};
            for(std::size_t j = 0; j < data[i].size(); j++){
                sprintf(buf + j * 4, "%04d ", data[i][j]);
            }
            SYLAR_LOG_DEBUG(g_logger) << "detect_buffer[" << i << "] size: " << data[i].size() << ", data: " << buf;
        }
        if(dev_state[0] != Capturing && dev_state[1] != Capturing){
            SYLAR_LOG_DEBUG(g_logger) << "detect stop, start to upload files";
            break;
        }
    }
    //等待文件写入完成
    {
        std::unique_lock<std::mutex> lk(writeDoneMutex);
        writeDoneCV.wait(lk, [](){
            return writeDone[0].load() && writeDone[1].load();
        });
        writeDone[0].store(false);
        writeDone[1].store(false);
        upload_files();
        SYLAR_LOG_DEBUG(g_logger) << "files upload";
    }
    SYLAR_LOG_DEBUG(g_logger) << "detect_thread end";
}

void Application::cap_write_fiber(int dev_num){
    SYLAR_LOG_DEBUG(g_logger) << "cap_write_fiber " << dev_num << " start";
    
    SYLAR_LOG_DEBUG(g_logger) << "cap_write_fiber " << dev_num << " end";
}

void Application::calib_fiber(int dev_num){
    SYLAR_LOG_DEBUG(g_logger) << "calib_fiber " << dev_num << " start";

    SYLAR_LOG_DEBUG(g_logger) << "calib_fiber " << dev_num << " end";
}

int main(int argc, char** argv) {
    g_logger->setLevel(sylar::LogLevel::DEBUG);

    // 初始化libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);

    Application app;
    sylar::IOManager iom(1, "iomanager");

    for(int i = 0; i != 1; i++){
        iom.schedule([=](){
            while(1){
                // //try catch
                // try {
                //     sylar::Fiber::YieldToReady();
                //     throw std::runtime_error("Some runtime error occurred!");  // 手动抛出异常
                // } catch (const std::exception& e) {
                //     SYLAR_LOG_ERROR(g_logger) << "sleep fiber " << i << " exception: " << e.what();
                // } catch (...) {
                //     SYLAR_LOG_ERROR(g_logger) << "sleep fiber " << i << " unknown exception occurred!";
                // }
                int sleep_time = 10;
                sleep(sleep_time);
                SYLAR_LOG_INFO(g_logger) << std::to_string(i) << " sleep " << sleep_time << "s";
            }
        });
    }

    iom.schedule([&](){
        char buffer[256];
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
        while(1){
            int n = read(STDIN_FILENO, buffer, sizeof(buffer));
            if(n > 0){
                buffer[n - 1] = '\0';
                std::string cmd(buffer);
                // SYLAR_LOG_INFO(g_logger) << "read from stdin: " << cmd;
                if(cmd == "cap"){
                    SYLAR_LOG_INFO(g_logger) << "cap";
                    Application::dev_state[0] = Application::Capturing;
                    Application::dev_state[1] = Application::Capturing;
                    iom.schedule([=](){
                        app.cap_detect_fiber(0);
                    });
                    iom.schedule([=](){
                        app.cap_detect_fiber(1);
                    });
                    iom.schedule([=](){
                        app.cap_write_fiber(0);
                    });
                    iom.schedule([=](){
                        app.cap_write_fiber(1);
                    });
                    std::thread(Application::detect_thread).detach();
                } else if(cmd == "calib"){
                    SYLAR_LOG_INFO(g_logger) << "calib";
                    Application::dev_state[0] = Application::Calibrating;
                    Application::dev_state[1] = Application::Calibrating;
                    iom.schedule([=](){
                        app.calib_fiber(0);
                    });
                    iom.schedule([=](){
                        app.calib_fiber(1);
                    });
                } else if(cmd == "stop"){
                    SYLAR_LOG_INFO(g_logger) << "stop";
                    Application::dev_state[0] = Application::Idle;
                    Application::dev_state[1] = Application::Idle;
                } else {
                    // SYLAR_LOG_INFO(g_logger) << "unknown cmd: " << cmd;
                }
            } else if(n == 0){
                SYLAR_LOG_INFO(g_logger) << "stdin closed";
                break;
            } else {
                SYLAR_LOG_ERROR(g_logger) << "read error";
                break;
            }
        }
    });

    return 0;
}


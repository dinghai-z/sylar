#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <condition_variable>
#include <atomic>

template<typename T>
class DoubleBuffer {
    T buffer1_, buffer2_;
    T* write_buf_{&buffer1_};
    T* read_buf_ {&buffer2_};
    std::atomic<bool> has_new_{false};
    std::mutex mtx_;   // 一把锁即可

public:
    bool write(const T& in) {
        std::lock_guard<std::mutex> lk(mtx_);
        *write_buf_ = in;
        has_new_.store(true);
        return true;
    }

    bool read(T& out) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (!has_new_.load())
            return false;
        std::swap(write_buf_, read_buf_);
        out = *read_buf_;
        has_new_.store(false);
        return true;
    }

    bool hasNew() {
        return has_new_.load();
    }
};

class Application {
public:
    Application(){};
    enum device_state{
        Idle,
        Capturing,
        Calibrating,
    };

public:
    static void cap_detect_fiber(int dev_num);
    static void cap_write_fiber(int dev_num);
    static void calib_fiber(int dev_num);
    static void detect_thread();
    static void upload_files();
    static std::atomic<device_state> dev_state[2];

private:
    static std::mutex detectMutex;
    static std::condition_variable detectCV;
    static DoubleBuffer<std::vector<int>> detect_buffer[2];
    static std::mutex writeDoneMutex;
    static std::condition_variable writeDoneCV;
    static std::atomic<bool> writeDone[2];
};

#endif
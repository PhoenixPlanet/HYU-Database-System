#ifndef __LOG_MANAGER_H__
#define __LOG_MANAGER_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <string>
#include <thread>
#include <mutex>
#include <vector>

#include "enums.h"
#include "log.h"

struct LogBufferFrame {
    CommonLog log_main_;
    UpdateData update_log_;
    CompensateData compensate_log_;
};

#pragma pack(push, 1)
struct LogFileHeader {
    int64_t start_offset_;
};
#pragma pack(pop)

struct LogTrxObject {
    bool is_winner_;
    std::vector<int64_t> lsn_list_;
};

class LogManager {
public:
    static LogManager& instance() {
        static LogManager instance_;
        return instance_;
    }

    bool initLogManager(char* log_path);
    void endLogManager();
    int64_t addCommonLog(CommonLog log);
    int64_t addUpdateLog(CommonLog log, UpdateData update_data);
    int64_t addCompensateLog(CommonLog log, CompensateData compensate_data);

    void recovery();
    std::vector<LogTrxObject> analysis();

    bool isPageExistInLog(pagenum_t pagenum);
    void force();

private:
    LogManager() : initialized_(false) {  }
    bool initialized_;
    static constexpr int32_t HEADER_OFFSET = sizeof(LogFileHeader);

    std::string log_path_;
    int log_fd_;

    int64_t start_offset_;
    int64_t current_lsn_;

    std::vector<LogBufferFrame> buffer_;

    std::mutex log_manager_lock_;
};

#endif
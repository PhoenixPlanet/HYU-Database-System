#include "log_manager.h"

bool LogManager::initLogManager(char* log_path) {
    if (initialized_) {
        return false;
    }
    
    log_path_ = std::string(log_path_);

    int fd = open(log_path, O_RDWR | O_CREAT | O_SYNC, 0666);
    
    // 에러 발생 시 에러 출력
    if (fd < 0) {
        fprintf(stderr, "Open Failed - errno: %s\n", strerror(errno));
        return false;
    }
    
    log_fd_ = fd;
    // 파일 크기 확인 후 0인 경우 헤더페이지 생성
    off_t f_size = lseek(fd, 0, SEEK_END);
    if (f_size == 0) {
        LogFileHeader* header = new LogFileHeader();
        header->start_offset_ = HEADER_OFFSET;

        lseek(log_fd_, 0, SEEK_SET);
        ssize_t write_return = write(log_fd_, header, sizeof(LogFileHeader));
        fsync(log_fd_);

        delete header;
    }

    lseek(log_fd_, 0, SEEK_SET);
    LogFileHeader header;
    ssize_t read_return = read(log_fd_, &header, sizeof(LogFileHeader));
    start_offset_ = header.start_offset_;
    current_lsn_ = start_offset_ - HEADER_OFFSET;

    initialized_ = true;
    return true;
}

void LogManager::endLogManager() {
    LogFileHeader* header = new LogFileHeader();
    header->start_offset_ = current_lsn_;

    lseek(log_fd_, 0, SEEK_SET);
    ssize_t write_return = write(log_fd_, header, sizeof(LogFileHeader));
    fsync(log_fd_);
    
    close(log_fd_);
}

int64_t LogManager::addCommonLog(CommonLog log) {
    std::unique_lock<std::mutex> log_manager_lock;

    LogBufferFrame new_frame;
    new_frame.log_main_ = log;
    new_frame.log_main_.log_size_ = 28;
    new_frame.log_main_.lsn_ = current_lsn_;
    current_lsn_ += 28;

    buffer_.push_back(new_frame);
}

int64_t LogManager::addUpdateLog(CommonLog log, UpdateData update_data) {
    std::unique_lock<std::mutex> log_manager_lock;

    LogBufferFrame new_frame;
    new_frame.log_main_ = log;
    new_frame.log_main_.log_size_ = 288;
    new_frame.log_main_.lsn_ = current_lsn_;
    current_lsn_ += 288;

    buffer_.push_back(new_frame);
    return new_frame.log_main_.lsn_;
}

int64_t LogManager::addCompensateLog(CommonLog log, CompensateData compensate_data) {
    std::unique_lock<std::mutex> log_manager_lock;

    LogBufferFrame new_frame;
    new_frame.log_main_ = log;
    new_frame.log_main_.log_size_ = 296;
    new_frame.log_main_.lsn_ = current_lsn_;

    buffer_.push_back(new_frame);
    return new_frame.log_main_.lsn_;
}

bool LogManager::isPageExistInLog(pagenum_t pagenum) {
    for (auto& i : buffer_) {
        if ((i.log_main_.type_ == LogType::UPDATE && i.update_log_.pagenum_ == pagenum) ||
            (i.log_main_.type_ == LogType::COMPENSATE && i.compensate_log_.pagenum_ == pagenum)) {
            return true;
        }
    }
    return false;
}

void LogManager::force() {
    std::unique_lock<std::mutex> log_manager_lock;

    for (auto& i : buffer_) {
        lseek(log_fd_, i.log_main_.lsn_ + HEADER_OFFSET, SEEK_SET);
        write(log_fd_, &(i.log_main_), sizeof(CommonLog));
        fsync(log_fd_);
        if (i.log_main_.type_ == LogType::UPDATE) {
            lseek(log_fd_, i.log_main_.lsn_ + HEADER_OFFSET + sizeof(CommonLog), SEEK_SET);
            write(log_fd_, &(i.update_log_), sizeof(UpdateData));
            fsync(log_fd_);
        } else if (i.log_main_.type_ == LogType::COMPENSATE) {
            lseek(log_fd_, i.log_main_.lsn_ + HEADER_OFFSET + sizeof(CommonLog), SEEK_SET);
            write(log_fd_, &(i.compensate_log_), sizeof(CompensateData));
            fsync(log_fd_);
        }
    }

    buffer_.clear();
}
#ifndef __LOG_H__
#define __LOG_H__

#include <cstdint>

#include "enums.h"
#include "page.h"

#pragma pack(push, 1)
struct CommonLog {
    uint32_t log_size_;
    int64_t lsn_;
    int64_t prev_lsn_;
    int32_t trx_id_;
    LogType type_;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct UpdateData {
    uint32_t table_id_;
    pagenum_t pagenum_;
    uint32_t offset_;
    uint32_t data_length_;
    char old_image_[120];
    char new_image_[120];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct CompensateData {
    uint32_t table_id_;
    pagenum_t pagenum_;
    uint32_t offset_;
    uint32_t data_length_;
    char old_image_[120];
    char new_image_[120];
    int64_t next_undo_lsn;
};
#pragma pack(pop)

#endif
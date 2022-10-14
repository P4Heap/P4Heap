#pragma once
#ifndef __DEFS_H__

#define __DEFS_H__
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <deque>
#include <cstdint>
#include <string>
#include <chrono>
#include <cmath>
#include "logger.h"

#define NEW_FILE_PERM (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)

typedef uint32_t data_t;
typedef uint16_t partial_t;
typedef int32_t count_t;
typedef uint64_t seed_t;

typedef std::chrono::high_resolution_clock::time_point TP;

/**
 * @brief {item, cnt}
 */
struct slot_t
{
    data_t item;
    count_t cnt;
};

/**
 * @brief {item, cnt}
 */
struct record_t
{
    data_t item;
    count_t cnt;
    record_t() {};
    record_t(data_t _item, count_t _cnt) : item(_item), cnt(_cnt) {};
    bool operator<(const record_t& tp) const
    {
        return cnt > tp.cnt;
    }
};

/**
 * @brief {item, cnt}
 */
struct partial_record_t
{
    partial_t item;
    count_t cnt;
    bool operator<(const partial_record_t& tp) const
    {
        return cnt > tp.cnt;
    }
};


#endif
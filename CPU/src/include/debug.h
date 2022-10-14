#pragma once
#ifndef __DEBUG_H__

#define __DEBUG_H__
#include "defs.h"
#include "hash.h"
#include <vector>
namespace HASHPIPE
{
    void init();

    void insert(data_t item);

    count_t query(data_t item);

    std::vector<record_t> GetTopK();

    void TestTopK(std::vector<record_t>& ans, int K);
}

#endif

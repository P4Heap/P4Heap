#pragma once
#ifndef __TOPK_FRAMEWORK_H__

#define __TOPK_FRAMEWORK_H__
#include "defs.h"
#include "hash.h"
#include <vector>
#include <map>

class TopKFramework
{
public:

    virtual const char* GetName() = 0;

    /**
     * @brief Insert item into framework
     * 
     * @param item to be inserted
     * @return the output of framework (if not, return {0, 0} instead).
     */
    virtual slot_t insert(data_t item) = 0;

    /**
     * @brief query frequency of a particular item stored in the framework
     */
    virtual count_t query(data_t item) = 0;

    /**
     * @brief query frequency of a particular partial key stored in the framework
     */
    virtual count_t query(partial_t item) = 0;

    /**
     * @brief Get the Top K object
     * 
     * @return vector<record_t> containing {flows, cnt} in DESC order of frequency. 
     */
    virtual std::vector<record_t> GetTopK() = 0;

    /**
     * @brief Get the Top K object of partial keys
     * 
     * @return vector<partial_record_t> containing {flows, cnt} in DESC order of frequency. 
     */
    virtual std::vector<partial_record_t> GetPartialTopK() = 0;

    /**
     * @brief Test the accuracy of Top-K items detected by framework sketch.
     * 
     * @param ans vector containing ground truth {item, cnt} in DESC order of frequency.
     * @param K 
     */
    virtual void TestTopK(std::vector<record_t>& ans, int K) = 0;
};

#endif

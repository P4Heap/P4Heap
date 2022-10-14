#pragma once
#ifndef __DATASET_H__

#define __DATASET_H__
#include <map>
#include <unordered_map>
#include <string>
#include <vector>
#include "defs.h"
#include "util.h"
using namespace std;

class Dataset
{
public:
    count_t TOTAL_PACKETS;
    count_t TOTAL_FLOWS;
    data_t* raw_data = NULL; 
    unordered_map<data_t, count_t> counter;
    
    /**
     * @brief Construct a new Dataset object
     * 
     * @param PATH path of the dataset file
     * @param size_per_item size of one packet represented in the dataset
     */
    Dataset(string PATH, int size_per_item);

    ~Dataset()
    {
        if (raw_data)
            delete[] raw_data;
    }

    /**
     * @brief Get the Top K object
     * 
     * @return vector<record_t> containing {flows, cnt} in DESC order of frequency. 
     */
    vector<record_t> GetTopK();

    /**
     * @brief Get the Partial Top K object
     * 
     * @return vector<partial_record_t> containing {flows, cnt} in DESC order of frequency. 
     */
    vector<partial_record_t> GetPartialTopK();
};

#endif
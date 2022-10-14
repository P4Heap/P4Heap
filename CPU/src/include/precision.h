#pragma once
#ifndef __PRECISION_H__

#define __PRECISION_H__
#include "defs.h"
#include "hash.h"
#include "topkframework.h"
#include <vector>

class Precision : public TopKFramework
{
private:
    int TOTAL_MEM;
    const int NSTAGE;
    int LEN;
    int N_RECYC;
    slot_t** nt;
    seed_t* seed;
    std::map<partial_t, count_t> aggrst;

    /**
     * @brief aggregate frequcies of flows with different full key 
     * but the same partial key together. 
     */
    void aggregate();

public:

    Precision(int MEM_SZ = 60'000);

    ~Precision();

    virtual const char* GetName() override { return "PRECISION"; };

    /**
     * @brief Insert item into PRECISON sketch
     * 
     * @param item to be inserted
     * @return the output of PRECISION sketch (if not, return {0, 0} instead).
     */
    virtual slot_t insert(data_t item) override;

    /**
     * @brief query frequency of a particular item stored in the PRECISION
     */
    virtual count_t query(data_t item) override;

    /**
     * @brief query frequency of a particular partial key stored in the PRECISION
     */
    virtual count_t query(partial_t item) override;

    /**
     * @brief Get the Top K object
     * 
     * @return vector<record_t> containing {flows, cnt} in DESC order of frequency. 
     */
    virtual std::vector<record_t> GetTopK() override;

    /**
     * @brief Get the Top K object of partial keys
     * 
     * @return vector<partial_record_t> containing {flows, cnt} in DESC order of frequency. 
     */
    virtual std::vector<partial_record_t> GetPartialTopK() override;

    /**
     * @brief Test the accuracy of Top-K items detected by PRECISION sketch.
     * 
     * @param ans vector containing ground truth {item, cnt} in DESC order of frequency.
     * @param K 
     */
    virtual void TestTopK(std::vector<record_t>& ans, int K) override;
};

#endif

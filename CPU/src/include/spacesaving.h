#pragma once
#ifndef __SPACESAVING_H__

#define __SPACESAVING_H__
#include "defs.h"
#include "hash.h"
#include "topkframework.h"
#include <vector>
#include <map>
#include <set>

namespace SS
{
    /**
     * @brief {item, cnt}
     */
    struct SS_bucket_t
    {
        data_t item;
        count_t cnt;

        bool operator<(const SS_bucket_t& tp) const
        {
            if (cnt != tp.cnt)
                return cnt < tp.cnt;
            else
                return item < tp.item;
        }
    };
    
} // namespace SS


class SpaceSaving : public TopKFramework
{
private:
    const int TOTAL_MEM;
    const int MAX_BUCKET;
    std::map<data_t, SS::SS_bucket_t> counter;
    std::set<SS::SS_bucket_t> heap;
    std::map<partial_t, count_t> aggrst;

    /**
     * @brief aggregate frequcies of flows with different full key 
     * but the same partial key together. 
     */
    void aggregate();

public:

    SpaceSaving(int MEM_SZ = 60'000);

    ~SpaceSaving() = default;

    virtual const char* GetName() override { return "SpaceSaving"; };

    /**
     * @brief Insert item into SS
     * 
     * @param item to be inserted
     * @return the output of hashpipe (if not, return {0, 0} instead).
     */
    virtual slot_t insert(data_t item) override;

    /**
     * @brief query frequency of a particular item stored in the SS
     */
    virtual count_t query(data_t item) override;

    /**
     * @brief query frequency of a particular partial key stored in the SS
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
     * @brief Test the accuracy of Top-K items detected by SS sketch.
     * 
     * @param ans vector containing ground truth {item, cnt} in DESC order of frequency.
     * @param K 
     */
    virtual void TestTopK(std::vector<record_t>& ans, int K) override;
};

#endif

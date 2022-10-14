#pragma once
#ifndef __P4HEAP_H__

#define __P4HEAP_H__
#include "defs.h"
#include "hash.h"
#include "topkframework.h"
#include <vector>
#include <map>

namespace P4HEAP
{
    struct elastic_slot_t
    {
        data_t item;
        count_t cnt;
        int32_t vote;
    };

    /**
     * @brief Prototype of each stage of Funnel Sketch
     */
    class Stage
    {
    public:
        virtual ~Stage() {};
        virtual void init(int len, seed_t seed) = 0;
        virtual slot_t insert(slot_t cur) = 0;
        virtual count_t query(data_t item) = 0;
        virtual std::map<data_t, count_t> GetRecord() = 0;
    };

    class Elastic : public Stage
    {
    public:
        int32_t lambda_;
        int len_;
        elastic_slot_t* nt_ = NULL;
        seed_t seed_;

        Elastic(int32_t lambda) : lambda_(lambda) {};

        virtual ~Elastic() override
        {
            if (nt_ != NULL)
                delete[] nt_;
        }

        virtual void init(int len, seed_t seed) override
        {
            len_ = len;
            seed_ = seed;
            nt_ = new elastic_slot_t[len_];
            memset(nt_, 0, sizeof(elastic_slot_t)*len_);
        }

        virtual slot_t insert(slot_t cur) override
        {
            int pos = HASH::hash(cur.item, seed_) % len_;
            if (nt_[pos].cnt == 0)
            {
                nt_[pos] = elastic_slot_t{cur.item, cur.cnt, lambda_};
                return slot_t{0, 0};
            }
            else if (nt_[pos].item == cur.item)
            {
                nt_[pos].cnt += cur.cnt;
                nt_[pos].vote += lambda_;
                return slot_t{0, 0};
            }

            nt_[pos].vote -= 1;
            if (nt_[pos].vote <= 0)
            {
                slot_t victim = slot_t{nt_[pos].item, nt_[pos].cnt};
                nt_[pos] = elastic_slot_t{cur.item, cur.cnt, lambda_};
                return victim;
            }
            else
                return cur;
        }

        virtual count_t query(data_t item) override
        {
            int pos = HASH::hash(item, seed_) % len_;
            if (nt_[pos].item == item)
                return nt_[pos].cnt;
            else
                return 0;
        }

        virtual std::map<data_t, count_t> GetRecord() override
        {
            std::map<data_t, count_t> rst;
            for (int i=0;i<len_;i++)
            {
                if (nt_[i].cnt != 0)
                {
                    rst.insert(std::make_pair(nt_[i].item, nt_[i].cnt));
                }
            }
            return rst;
        }
    };

    class Basic : public Stage
    {
    public:
        int len_;
        int sum_;
        double C_;
        slot_t* nt_ = NULL;
        seed_t seed_;

        Basic(double C) : C_(C) {};

        virtual ~Basic() override
        {
            if (nt_ != NULL)
                delete[] nt_;
        }

        virtual void init(int len, seed_t seed) override
        {
            len_ = len;
            sum_ = 0;
            seed_ = seed;
            nt_ = new slot_t[len_];
            memset(nt_, 0, sizeof(slot_t)*len);
        }

        virtual slot_t insert(slot_t cur) override
        {
            int pos = HASH::hash(cur.item, seed_) % len_;
            if (nt_[pos].cnt == 0)
            {
                nt_[pos] = cur;
                sum_ += cur.cnt;
                return slot_t{0, 0};
            }
            else if (nt_[pos].item == cur.item)
            {
                nt_[pos].cnt += cur.cnt;
                sum_ += cur.cnt;
                return slot_t{0, 0};
            }
            else if (cur.cnt > nt_[pos].cnt)
            // else if (cur.cnt > C_*sum_/len_)
            {
                sum_ += cur.cnt;
                slot_t victim = nt_[pos];
                nt_[pos] = cur;
                return victim;
            }

            return cur;
        }

        virtual count_t query(data_t item) override
        {
            int pos = HASH::hash(item, seed_) % len_;
            if (nt_[pos].item == item)
                return nt_[pos].cnt;
            else
                return 0;
        }

        virtual std::map<data_t, count_t> GetRecord() override
        {
            std::map<data_t, count_t> rst;
            for (int i=0;i<len_;i++)
            {
                if (nt_[i].cnt != 0)
                {
                    rst.insert(std::make_pair(nt_[i].item, nt_[i].cnt));
                }
            }
            return rst;
        }
    };
} // namespece P4HEAP

class P4Heap : public TopKFramework
{
private:

    static const double ratio = 0.5;
    int TOTAL_MEM;
    static const int NSTAGE = 6;
    int len[NSTAGE] = {};

    P4HEAP::Stage* stages[NSTAGE] = {
        new P4HEAP::Elastic(8), 
        new P4HEAP::Elastic(8), 
        new P4HEAP::Elastic(8), 
        new P4HEAP::Elastic(8), 
        new P4HEAP::Basic(1.0), 
        new P4HEAP::Basic(1.0), 
    };
    std::map<partial_t, count_t> aggrst;

    /**
     * @brief aggregate frequcies of flows with different full key 
     * but the same partial key together. 
     */
    void aggregate();

public:

    /**
     * @brief Construct a new P4Heap object
     * 
     * @param MEM_SIZE memory size (B)
     */
    P4Heap(int MEM_SIZE = 60'000);

    ~P4Heap();

    virtual const char* GetName() override { return "P4Heap"; };

    /**
     * @brief Insert item into P4Heap sketch
     * 
     * @param item to be inserted
     * @return the output of P4Heap sketch (if not, return {0, 0} instead).
     */
    virtual slot_t insert(data_t item) override;

    /**
     * @brief query frequency of a particular item stored in the P4Heap
     */
    virtual count_t query(data_t item) override;

    /**
     * @brief query frequency of a particular partial key stored in the P4Heap
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
     * @brief Test the accuracy of Top-K items detected by P4Heap sketch.
     * 
     * @param ans vector containing ground truth {item, cnt} in DESC order of frequency.
     * @param K 
     */
    virtual void TestTopK(std::vector<record_t>& ans, int K) override;
    
};

#endif
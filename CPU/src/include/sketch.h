#pragma once
#ifndef __SKETCH_H__

#define __SKETCH_H__
#include "defs.h"
#include "hash.h"
#include "dataset.h"
#include "p4heap.h"
#include "heap.h"
#include "topkframework.h"
#include <vector>
#include <map>
#include <set>
#include <queue>

class BaseSketch
{
public:

    virtual ~BaseSketch() = default;

    virtual void insert(data_t item, count_t freq = 1) = 0;

    virtual count_t query(data_t item) = 0;

    virtual std::deque<record_t> GetTopK() {LOG_ERROR("Unimplemented or NOT supported"); exit(-1);}

    virtual void test(int K, Dataset& stream) = 0;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) = 0;
};

class CM : public BaseSketch
{
private:

    const int TOTAL_MEM;
    const int NSTAGE;
    const int LEN;

    count_t** nt;
    seed_t* seed;

public:

    CM(int _TOTAL_MEM, int _NSTAGE, int _SLOT_SZ = sizeof(count_t));

    ~CM();

    virtual void insert(data_t item, count_t freq = 1) override;

    virtual count_t query(data_t item) override;

    virtual void test(int K, Dataset& stream) override;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) override;
};

class Count : public BaseSketch
{
private:

    int TOTAL_MEM;
    int NSTAGE;
    int LEN;

    count_t** nt;
    seed_t* seed;
    seed_t* sseed;

    inline int get_sign(data_t item, int stage);

public:

    Count(int _TOTAL_MEM, int _NSTAGE, int _SLOT_SZ = sizeof(count_t));

    ~Count();

    virtual void insert(data_t item, count_t freq = 1) override;

    virtual count_t query(data_t item) override;

    virtual void test(int K, Dataset& stream) override;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) override;
};

class CountHeap : public BaseSketch
{
private:

    int TOTAL_MEM;
    int NSTAGE;
    int LEN;
    int HEAPSIZE;

    count_t** nt;
    seed_t* seed;
    seed_t* sseed;
    Heap* heap;

    inline int get_sign(data_t item, int stage);

public:

    CountHeap(int _TOTAL_MEM, int _NSTAGE, int _SLOT_SZ = sizeof(count_t));

    virtual ~CountHeap() override;

    virtual void insert(data_t item, count_t freq = 1) override;

    virtual count_t query(data_t item) override;

    virtual std::deque<record_t> GetTopK() override;

    virtual void test(int K, Dataset& stream) override;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) override;
};

class Coco : public BaseSketch
{
private:

    int TOTAL_MEM;
    int NSTAGE;
    int LEN;

    slot_t** nt;
    seed_t* seed;
    std::map<partial_t, count_t> aggrst;

    void aggregate();

public:

    Coco(int _TOTAL_MEM, int _NSTAGE);

    ~Coco();

    virtual void insert(data_t item, count_t freq = 1) override;

    virtual count_t query(data_t item) override;

    std::deque<record_t> GetTopK() override;

    virtual void test(int K, Dataset& stream) override;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) override;
};

class NitroCM : public BaseSketch
{
private:

    const int TOTAL_MEM;
    const int NSTAGE;
    const int LEN;
    const double SAMPLE_RATE;

    count_t** nt;
    seed_t* seed;

public:

    NitroCM(int _TOTAL_MEM, double _SAMPLE_RATE, int _NSTAGE, int _SLOT_SZ = sizeof(count_t));

    ~NitroCM();

    virtual void insert(data_t item, count_t freq = 1) override;

    virtual count_t query(data_t item) override;

    virtual void test(int K, Dataset& stream) override;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) override;
};

class HalfCU : public BaseSketch
{
private:

    const int TOTAL_MEM;
    const int NSTAGE;
    const int LEN;

    count_t** nt;
    seed_t* seed;

public:

    HalfCU(int _TOTAL_MEM, int _NSTAGE, int _SLOT_SZ = sizeof(count_t));

    ~HalfCU();

    virtual void insert(data_t item, count_t freq = 1) override;

    virtual count_t query(data_t item) override;

    virtual void test(int K, Dataset& stream) override;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) override;
};

class Univmon : public BaseSketch
{
private:

    int NSKETCH;

    CountHeap** sketches;
    seed_t seed;

public:

    Univmon(int _TOTAL_MEM, int _SLOT_SZ = sizeof(count_t));

    ~Univmon();

    virtual void insert(data_t item, count_t freq = 1) override;

    virtual count_t query(data_t item) override;

    virtual std::deque<record_t> GetTopK() override;

    virtual void test(int K, Dataset& stream) override;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) override;
};

class FCM : public BaseSketch
{
private:

    const int TOTAL_MEM;
    const int NTREES = 2;
    const int HEIGHT;
    const int LEN;
    const uint32_t THRESHOLD[3] = {UINT8_MAX, UINT16_MAX, UINT32_MAX};

    uint64_t*** nt;
    seed_t* seed;

public:

    FCM(int _TOTAL_MEM, int _SLOT_SZ = sizeof(uint32_t));

    ~FCM();

    virtual void insert(data_t item, count_t freq = 1) override;

    virtual count_t query(data_t item) override;

    virtual void test(int K, Dataset& stream) override;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) override;
};

class Elastic : public BaseSketch
{
struct elastic_slot_t
    {
        data_t item;
        count_t vote_p;
        count_t vote_all;
    };

private:

    int TOTAL_MEM;
    static const int lambda = 32;
    static const int NSTAGE = 4;
    int LEN;
    elastic_slot_t** nt;
    seed_t* seed;

public:

    Elastic(int _TOTAL_MEM, int _SLOT_SZ = sizeof(uint32_t));

    ~Elastic();

    virtual void insert(data_t item, count_t freq = 1) override;

    virtual count_t query(data_t item) override;

    virtual std::deque<record_t> GetTopK() override;

    virtual void test(int K, Dataset& stream) override;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) override;

};

class RHHH : public BaseSketch
{
private:

    const double alpha;
    static const int NHEAP = 4;
    seed_t seed;
    const data_t MASK[NHEAP] = {0xff000000U, 0xffff0000U, 0xffffff00U, 0xffffffffU};
    TopKFramework* nt[NHEAP] = {};

    count_t query(data_t item, int stage);

    std::vector<record_t> Filtered_TopK(int stage, std::vector<record_t> topk);

public:

    /**
     * @brief Construct a new RHHH object
     * 
     * @param _TOTAL_MEM total memory size (B)
     * @param framework 0-SpaceSaving, 1-Funnel
     */
    RHHH(int _TOTAL_MEM, int framework);

    ~RHHH() = default;

    virtual void insert(data_t item, count_t freq = 1) override;

    virtual count_t query(data_t item) override;

    virtual std::deque<record_t> GetTopK() override;

    virtual void test(int K, Dataset& stream) override;

    virtual void test(int K, Dataset& stream, TopKFramework& topk) override;
};

#endif

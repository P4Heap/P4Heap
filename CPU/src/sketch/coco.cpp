#include "sketch.h"
#include "defs.h"
#include "util.h"
#include <set>

Coco::Coco(int _TOTAL_MEM, int _NSTAGE) : 
    TOTAL_MEM(_TOTAL_MEM), NSTAGE(_NSTAGE), LEN(_TOTAL_MEM/(NSTAGE * sizeof(slot_t)))
{
    nt = new slot_t*[NSTAGE];
    seed = new seed_t[NSTAGE];

    for (int i=0;i<NSTAGE;i++)
    {
        seed[i] = clock();
        sleep(1);
        nt[i] = new slot_t[LEN];
        memset(nt[i], 0, sizeof(slot_t)*LEN);
    }
}

Coco::~Coco()
{
    delete[] seed;
    for (int i=0;i<NSTAGE;i++)
    {
        delete[] nt[i];
    }
    delete[] nt;
}

void Coco::insert(data_t item, count_t freq)
{
    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(item, seed[i]) % LEN;
        nt[i][pos].cnt += freq;
        if (RandP() < double(freq)/nt[i][pos].cnt)
            nt[i][pos].item = item;
    }
}

count_t Coco::query(data_t item)
{
    int rst[NSTAGE];
    memset(rst, 0, sizeof(rst));
    for (int i=0;i<NSTAGE;i++)
    {
        int pos = HASH::hash(item, seed[i]) % LEN;
        if (nt[i][pos].item == item)
            rst[i] = nt[i][pos].cnt;
    }
    std::sort(rst, rst+NSTAGE);
    if (NSTAGE % 2)
    {
        return rst[NSTAGE/2];
    }
    else
    {
        return (rst[NSTAGE/2] + rst[NSTAGE/2 - 1]) / 2;
    }
}

std::deque<record_t> Coco::GetTopK()
{
    std::map<data_t, std::vector<count_t> > tparr;
    for (int i=0;i<NSTAGE;i++)
    {
        for (int j=0;j<LEN;j++)
        {
            if (nt[i][j].cnt != 0)
            {
                auto it = tparr.find(nt[i][j].item);
                if (it == tparr.end())
                {
                    std::vector<count_t> tp(NSTAGE, 0);
                    tp[i]=nt[i][j].cnt;
                    tparr.insert(std::make_pair(nt[i][j].item, tp));
                }
                else
                {
                    it->second[i] = nt[i][j].cnt;
                }
            }
        }
    }

    std::map<data_t, count_t> tpcnt;
    for (auto t : tparr)
    {
        std::sort(t.second.begin(), t.second.end());
        int curst;
        if (t.second.size() % 2)
            curst = t.second[t.second.size()/2];
        else
            curst = (t.second[t.second.size()/2] + t.second[t.second.size()/2 - 1]) / 2;
        
        auto it = tpcnt.find(t.first);
        if (it==tpcnt.end())
            tpcnt.insert(std::make_pair(t.first, curst));
        else
            it->second += curst;
    }

    std::deque<record_t> rst;
    for (auto t : tpcnt)
    {
        rst.push_back(record_t{t.first, t.second});
    }
    std::sort(rst.begin(), rst.end());
    return rst;
}

void Coco::aggregate()
{
    if (!aggrst.empty())
        return;

    std::map<partial_t, std::vector<count_t> > tparr;
    for (int i=0;i<NSTAGE;i++)
    {
        for (int j=0;j<LEN;j++)
        {
            if (nt[i][j].cnt != 0)
            {
                partial_t curip = GetPartialKey(nt[i][j].item);
                auto it = tparr.find(curip);
                if (it == tparr.end())
                {
                    std::vector<count_t> tp(NSTAGE, 0);
                    tp[i]=nt[i][j].cnt;
                    tparr.insert(std::make_pair(curip, tp));
                }
                else
                {
                    it->second[i] = nt[i][j].cnt;
                }
            }
        }
    }

    for (auto t : tparr)
    {
        count_t curcnt;
        std::sort(t.second.begin(), t.second.end());
        if (NSTAGE % 2)
            curcnt = t.second[NSTAGE/2];
        else
            curcnt = (t.second[NSTAGE/2] + t.second[NSTAGE/2 - 1]) / 2;

        aggrst.insert(std::make_pair(t.first, curcnt));
    }
}

void Coco::test(int K, Dataset& stream)
{
    LOG_INFO("Test Coco:");

    // test on full keys
    {
        auto ans = stream.GetTopK();
        K = min(K, int(ans.size()));
        LOG_INFO("Test top-%d items on full key:", K);

        auto rst = GetTopK();
        std::map<data_t, count_t> tpcnt;
        for (auto a : rst)
        {
            assert(tpcnt.insert(std::make_pair(a.item, a.cnt)).second);
        }

        // Test AAE, ARE
        double aae=0, are=0;
        for (int i=0;i<K;i++)
        {
            auto it=tpcnt.find(ans[i].item);
            count_t cur=0;
            if (it != tpcnt.end())
                cur=it->second;
            aae += abs(ans[i].cnt - cur);
            are += double(abs(ans[i].cnt - cur)) / ans[i].cnt;
        }
        aae /= K; are /= K;
        LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);

        // Test PR
        std::set<data_t> anset;
        for (int i=0;i<K;i++)
            anset.insert(ans[i].item);

        double pr=0;
        for (int i=0; i<K && i<rst.size(); i++)
        {
            if (anset.find(rst[i].item) != anset.end())
                pr++;
        }
        pr /= K;
        LOG_RESULT("Precision Rate (PR): %lf", pr);

        // Test RR
        double rr=0;
        for (int i=0; i<rst.size(); i++)
        {
            if (anset.find(rst[i].item) != anset.end())
                rr++;
        }
        rr /= K;
        LOG_RESULT("Recall Rate (RR): %lf", rr);
    }

    // test on partial keys
    {
        aggregate();
        auto ans = stream.GetPartialTopK();
        K = min(K, int(ans.size()));
        LOG_INFO("Test top-%d items on partial key:", K);

        double aae = 0, are = 0;
        for (int i=0;i<K;i++)
        {
            auto it = aggrst.find(ans[i].item);
            count_t rst = 0;
            if (it != aggrst.end())
                rst = it->second;
            aae += abs(rst - ans[i].cnt);
            are += double(abs(rst - ans[i].cnt)) / ans[i].cnt;
        }
        aae /= K;
        are /= K;
        LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);

        double pr = 0;

        vector<partial_record_t> rst;
        for (auto t : aggrst)
        {
            rst.push_back(partial_record_t{t.first, t.second});
        }
        sort(rst.begin(), rst.end());

        set<partial_t> anset;
        for (int i=0;i<K;i++)
        {
            anset.insert(ans[i].item);
        }

        for (int i=0;i<K && i<rst.size();i++)
        {
            if (anset.find(rst[i].item) != anset.end())
                pr++;
        }
        pr /= K;
        LOG_RESULT("Precision Rate (PR): %lf", pr);

        // Test RR
        double rr=0;
        for (int i=0; i<rst.size(); i++)
        {
            if (anset.find(rst[i].item) != anset.end())
                rr++;
        }
        rr /= K;
        LOG_RESULT("Recall Rate (RR): %lf", rr);
    }
}

void Coco::test(int K, Dataset& stream, TopKFramework& topk)
{
    LOG_INFO("Test %s+Coco:", topk.GetName());

    // test on full keys
    {
        auto ans = stream.GetTopK();
        K = min(K, int(ans.size()));
        LOG_INFO("Test top-%d items on full key:", K);

        auto r1 = topk.GetTopK();
        auto r2 = GetTopK();
        std::map<data_t, count_t> tpcnt;
        for (auto a : r1)
        {
            auto it = tpcnt.find(a.item);
            if (it == tpcnt.end())
                tpcnt.insert(std::make_pair(a.item, a.cnt));
            else
                it->second += a.cnt;
        }
        for (auto a : r2)
        {
            auto it = tpcnt.find(a.item);
            if (it == tpcnt.end())
                tpcnt.insert(std::make_pair(a.item, a.cnt));
            else
                it->second += a.cnt;
        }
        std::vector<record_t> rst;
        for (auto a : tpcnt)
            rst.push_back(record_t{a.first, a.second});
        std::sort(rst.begin(), rst.end());

        // Test AAE, ARE
        double aae=0, are=0;
        for (int i=0;i<K;i++)
        {
            auto it=tpcnt.find(ans[i].item);
            count_t cur=0;
            if (it != tpcnt.end())
                cur=it->second;
            aae += abs(ans[i].cnt - cur);
            are += double(abs(ans[i].cnt - cur)) / ans[i].cnt;
        }
        aae /= K; are /= K;
        LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);

        // Test PR
        std::set<data_t> anset;
        for (int i=0;i<K;i++)
            anset.insert(ans[i].item);

        double pr=0;
        for (int i=0; i<K && i<rst.size(); i++)
        {
            if (anset.find(rst[i].item) != anset.end())
                pr++;
        }
        pr /= K;
        LOG_RESULT("Precision Rate (PR): %lf", pr);

        // Test RR
        double rr=0;
        for (int i=0; i<rst.size(); i++)
        {
            if (anset.find(rst[i].item) != anset.end())
                rr++;
        }
        rr /= K;
        LOG_RESULT("Recall Rate (RR): %lf", rr);
    }

    // test on partial keys
    {
        aggregate();
        auto ans = stream.GetPartialTopK();
        K = min(K, int(ans.size()));
        LOG_INFO("Test top-%d items on partial key:", K);

        double aae = 0, are = 0;
        for (int i=0;i<K;i++)
        {
            auto it = aggrst.find(ans[i].item);
            count_t rst = topk.query(ans[i].item);
            if (it != aggrst.end())
                rst += it->second;
            aae += abs(rst - ans[i].cnt);
            are += double(abs(rst - ans[i].cnt)) / ans[i].cnt;
        }
        aae /= K;
        are /= K;
        LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);

        double pr = 0;

        map<partial_t, count_t> cntr = aggrst;
        auto topkrst = topk.GetPartialTopK();
        for (auto t : topkrst)
        {
            auto it = cntr.find(t.item);
            if (it == cntr.end())
            {
                cntr.insert(std::make_pair(t.item, t.cnt));
            }
            else
            {
                it->second += t.cnt;
            }
        }

        vector<partial_record_t> rst;
        for (auto t : cntr)
        {
            rst.push_back(partial_record_t{t.first, t.second});
        }
        sort(rst.begin(), rst.end());

        set<partial_t> anset;
        for (int i=0;i<K;i++)
        {
            anset.insert(ans[i].item);
        }

        for (int i=0;i<K && i<rst.size();i++)
        {
            if (anset.find(rst[i].item) != anset.end())
                pr++;
        }
        pr /= K;
        LOG_RESULT("Precision Rate (PR): %lf", pr);

        // Test RR
        double rr=0;
        for (int i=0; i<rst.size(); i++)
        {
            if (anset.find(rst[i].item) != anset.end())
                rr++;
        }
        rr /= K;
        LOG_RESULT("Recall Rate (RR): %lf", rr);
    }
}

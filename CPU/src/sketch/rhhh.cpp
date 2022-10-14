#include "sketch.h"
#include "spacesaving.h"
#include "p4heap.h"
#include "defs.h"
#include "util.h"

RHHH::RHHH(int _TOTAL_MEM, int framework) : alpha(1.0)
{
    seed = clock();
    double sum = 0;
    for (int i=1;i<=NHEAP;i++)
        sum += pow(alpha, i);

    int curmem = _TOTAL_MEM/sum;
    for (int i=0;i<NHEAP;i++)
    {
        switch (framework)
        {
        case 0:
            nt[i] = new SpaceSaving(curmem);
            break;
        
        case 1:
            nt[i] = new P4Heap(curmem);
            break;

        default:
            LOG_ERROR("Unrecognized framework %d", framework);
            break;
        }

        curmem *= alpha;
    }
}

std::vector<record_t> RHHH::Filtered_TopK(int stage, std::vector<record_t> topk)
{
    map<data_t, count_t> cntr;
    for (auto t : topk)
    {
        data_t cur = t.item & MASK[stage];
        auto it = cntr.find(cur);
        if (it == cntr.end())
            cntr.insert(make_pair(cur, t.cnt));
        else
            it->second += t.cnt;
    }

    vector<record_t> ans;
    for (auto t : cntr)
        ans.push_back(record_t{t.first, t.second});
    sort(ans.begin(), ans.end());
    return ans;
}

void RHHH::insert(data_t item, count_t freq)
{
    assert(freq == 1);
    int pos = clock() % NHEAP;
        int cur = item & MASK[pos];
        nt[pos]->insert(cur);
}

count_t RHHH::query(data_t item)
{
    LOG_ERROR("Unsupported");
    return 0;
}

count_t RHHH::query(data_t item, int stage)
{
    data_t cur = item & MASK[stage];
    return nt[stage]->query(cur);
}

std::deque<record_t> RHHH::GetTopK()
{
    LOG_ERROR("Unimplemented");
    exit(-1);
}

void RHHH::test(int K, Dataset& stream)
{
    auto ans = stream.GetTopK();

    for (int i=0;i<NHEAP;i++)
    {
        auto curans = Filtered_TopK(i, ans);
        int curK = std::min(K, int(curans.size()));
        LOG_INFO("Test RHHH(%s) on highest %d bytes of top-%d items:", nt[0]->GetName(), i+1, curK);

        // aae, are
        double aae = 0, are = 0;
        for (int j=0;j<curK;j++)
        {
            count_t cur = query(curans[j].item, i);
            aae += abs(cur-curans[j].cnt);
            are += double(abs(cur-curans[j].cnt))/curans[j].cnt;
        }

        // PR
        double pr = 0;
        map<data_t, count_t> anscnt;
        for (int i=0;i<curK;i++)
        {
            assert(anscnt.insert(make_pair(curans[i].item, curans[i].cnt)).second);
        }

        auto rst = nt[i]->GetTopK();
        for (int i=0;i<curK && i<rst.size();i++)
        {
            if (anscnt.find(rst[i].item) != anscnt.end())
                pr++;
        }
        pr /= curK;
        LOG_RESULT("Precision Rate (PR): %lf", pr);
        aae /= K;
        are /= K;
        LOG_RESULT("AAE = %lf, ARE = %lf", aae, are);

        // RR
        double rr = 0;
        map<data_t, count_t> cntr;
        for (auto t : rst)
        {
            assert(cntr.insert(make_pair(t.item, t.cnt)).second);
        }

        for (int i=0;i<curK;i++)
        {
            if (cntr.find(curans[i].item) != cntr.end())
                rr++;
        }
        rr /= curK;
        LOG_RESULT("Recall Rate (RR): %lf", rr);
    }
}

void RHHH::test(int K, Dataset& stream, TopKFramework& topk)
{
    LOG_ERROR("Unsupported");
}

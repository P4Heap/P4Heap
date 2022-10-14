#include "dataset.h"
#include "logger.h"
#include <algorithm>
#include <map>
using namespace std;

Dataset::Dataset(string PATH, int size_per_item)
{
    struct stat buf;
    LOG_DEBUG("Opening file %s", PATH.c_str());
    int fd=Open(PATH.c_str(),O_RDONLY);
    fstat(fd,&buf);
    int n_elements = buf.st_size / size_per_item;
    TOTAL_PACKETS=n_elements;
    LOG_DEBUG("\tcnt=%d", n_elements);
    LOG_DEBUG("Mmap..."); 
    void* addr=mmap(NULL,buf.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    raw_data = new data_t[n_elements];
    close(fd);
    if (addr==MAP_FAILED)
    {
        LOG_ERROR("MMAP FAILED!");
        exit(-1);
    }

    char *ptr = reinterpret_cast<char *>(addr);
    for (int i = 0; i < n_elements; i++)
    {
        raw_data[i] = *reinterpret_cast<data_t *>(ptr);
        ptr += size_per_item;
    }
    munmap(addr, buf.st_size);

    for (count_t i = 0; i < n_elements;i++)
    {
        auto it = counter.find(raw_data[i]);
        if (it==counter.end())
        {
            counter.insert(std::make_pair(raw_data[i], 1));
        }
        else
        {
            it->second++;
        }
    }
    TOTAL_FLOWS = counter.size();
    LOG_INFO("Total packets: %d, Total flows: %d", TOTAL_PACKETS, TOTAL_FLOWS);
}

vector<record_t> Dataset::GetTopK()
{
    vector<record_t> rst;
    for (auto& it : counter)
        rst.push_back(record_t(it.first, it.second));
    std::sort(rst.begin(), rst.end());
    return rst;
}

vector<partial_record_t> Dataset::GetPartialTopK()
{
    std::map<partial_t, count_t> tpcnt;
    for (auto t : counter)
    {
        partial_t curip = GetPartialKey(t.first);
        auto it = tpcnt.find(curip);
        if (it == tpcnt.end())
            tpcnt.insert(std::make_pair(curip, t.second));
        else
            it->second += t.second;
    }

    std::vector<partial_record_t> rst;
    for (auto t : tpcnt)
        rst.push_back(partial_record_t{t.first, t.second});
    std::sort(rst.begin(), rst.end());
    return rst;
}

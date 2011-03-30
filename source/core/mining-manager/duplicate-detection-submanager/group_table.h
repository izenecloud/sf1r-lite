#ifndef SF1R_DD_GROUP_TABLE_H
#define SF1R_DD_GROUP_TABLE_H

#include <am/sequence_file/SimpleSequenceFile.hpp>
#include <am/3rdparty/rde_hash.h>

namespace sf1r
{
class GroupTable
{
    typedef izenelib::am::SSFType<uint32_t, uint32_t, uint32_t> SSFType;
public:
    GroupTable(const std::string& file);

    ~GroupTable();

    bool Load();

    bool Flush();

    void AddDoc(uint32_t docid1, uint32_t docid2);

    bool IsSameGroup(uint32_t docid1, uint32_t docid2);

    /**
       @return a reference of vector of docids that are in the same group.
     */
    void Find(uint32_t docid, std::vector<uint32_t>& list);

    void PrintStat() const;

private:
    std::string file_;
    izenelib::am::rde_hash<uint32_t, uint32_t> docid_group_;
    std::vector<std::vector<uint32_t> > group_info_;
    std::deque<uint32_t> valid_groupid_;
};
}
#endif

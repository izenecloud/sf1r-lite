/// product similarity algorithm for Electronic commerce manager

#ifndef SF1R_EC_SIMALGORITHM_H_
#define SF1R_EC_SIMALGORITHM_H_


#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "ec_types.h"

namespace izenelib{
namespace ir{
namespace indexmanager{
class IndexReader;
}
}
}

namespace sf1r { 
    
class DocumentManager;

namespace ec {

class EcManager;
    
class SimAlgorithm
{
//     typedef boost::function<void (const std::vector<uint32_t>&) > DocDelCallback;
//     typedef boost::function<void (const GroupIdType&, uint32_t docid) > FindCallback;
    
    public:
        SimAlgorithm(const std::string& working_dir, EcManager* ec_manager, uint32_t start_docid, double sim_threshold);
        
        bool ProcessCollection(const boost::shared_ptr<DocumentManager>& document_manager, izenelib::ir::indexmanager::IndexReader* index_reader, uint32_t property_id, const std::string& property_name);
        
        void FindSim_(uint32_t docid1, uint32_t docid2, double score);
        
        void AddTo_(uint32_t docid, const std::vector<GroupIdType>& gid_list);

        
    private:
        std::string working_dir_;
        EcManager* ec_manager_;
        uint32_t start_docid_;
        double sim_threshold_;
//         DocDelCallback docdel_callback_;
//         FindCallback find_callback_;

};




}
}

#endif

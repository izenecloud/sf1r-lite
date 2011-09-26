/// product similarity algorithm for Electronic commerce manager

#ifndef SF1R_EC_SIMALGORITHM_H_
#define SF1R_EC_SIMALGORITHM_H_


#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "ec_types.h"

#include <am/sequence_file/ssfr.h>
#include <idmlib/util/idm_analyzer.h>

namespace izenelib{
namespace ir{
namespace indexmanager{
class IndexReader;
}
}
}

namespace idmlib{
namespace kpe{
class DocKpItem;
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
        SimAlgorithm(const std::string& working_dir, EcManager* ec_manager, uint32_t start_docid, double sim_threshold
        , idmlib::util::IDMAnalyzer* analyzer, idmlib::util::IDMAnalyzer* cma_analyzer, const std::string& kpe_resource_path);
        
        bool ProcessCollection(const boost::shared_ptr<DocumentManager>& document_manager, izenelib::ir::indexmanager::IndexReader* index_reader, const std::pair<uint32_t, std::string>& title_property, const std::pair<uint32_t, std::string>& content_property );
        
        
    private:
        
        void KpeTask_(const boost::shared_ptr<DocumentManager>& document_manager, const std::pair<uint32_t, std::string>& title_property, const std::pair<uint32_t, std::string>& content_property );
        
        void FindTfidfSim_(uint32_t docid1, uint32_t docid2, double score);
        
        void KpeCallback_(uint32_t docid, const std::vector<idmlib::kpe::DocKpItem>& kp_list);
        
        void FindKpeSim_(uint32_t docid1, uint32_t docid2, double score);
        
        void FindSim_(uint32_t docid1, uint32_t docid2, double score);
        
        void CombineSim_();
        
        void AddTo_(uint32_t docid, const std::vector<GroupIdType>& gid_list);
        
        void Continue_();
        
    private:
        std::string working_dir_;
        EcManager* ec_manager_;
        idmlib::util::IDMAnalyzer* analyzer_;
        idmlib::util::IDMAnalyzer* cma_analyzer_;
        std::string kpe_resource_path_;
        uint32_t start_docid_;
        double sim_threshold_;
        double tfidf_ratio_;
        double tfidf_threshold_;
        double kpe_ratio_;
        double kpe_threshold_;
        
        izenelib::am::ssf::Writer<>* writer_;
        izenelib::am::ssf::Writer<>* kpe_writer_;
        uint32_t max_kpid_;
        boost::unordered_map<std::string, uint32_t> kpid_map_;
//         DocDelCallback docdel_callback_;
//         FindCallback find_callback_;

};




}
}

#endif

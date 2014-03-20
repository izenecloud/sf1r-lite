#ifndef SF1R_B5MMANAGER_B5MOCHECKER_H_
#define SF1R_B5MMANAGER_B5MOCHECKER_H_

#include <common/ScdParser.h>
#include <string>
#include <vector>
#include "b5m_types.h"
#include "offer_db.h"
//#include "brand_db.h"
#include "b5m_m.h"
#include "scd_doc_processor.h"

NS_SF1R_B5M_BEGIN
class B5moChecker {
    struct Error
    {
        std::string oid;
        std::string pid;
        std::string odb_pid;
        SCD_TYPE type;
    };
public:
    B5moChecker(const B5mM& b5mm): b5mm_(b5mm), odb_(NULL)
    {
    }
    ~B5moChecker()
    {
        if(odb_!=NULL) delete odb_;
    }


    bool Check(const std::string& mdb_instance)
    {
        LOG(INFO)<<"ignore b5mo checker now"<<std::endl;
        return true;
        if(b5mm_.mode>0)
        {
            LOG(INFO)<<"rebuild does not required to check"<<std::endl;
            return true;
        }
        std::string odb_path = B5MHelper::GetOdbPath(mdb_instance);
        if(!boost::filesystem::exists(odb_path))
        {
            LOG(ERROR)<<"odb does not exist"<<std::endl;
            return false;
        }
        odb_ = new OfferDb(odb_path);
        if(!odb_->is_open())
        {
            LOG(INFO)<<"open odb..."<<std::endl;
            if(!odb_->open())
            {
                LOG(ERROR)<<"odb open fail"<<std::endl;
                return false;
            }
            LOG(INFO)<<"odb open successfully"<<std::endl;
        }
        ScdDocProcessor::ProcessorType p = boost::bind(&B5moChecker::Process_, this, _1);
        ScdDocProcessor sd_processor(p, 1);
        sd_processor.AddInput(b5mm_.b5mo_path);
        LOG(INFO)<<"start to check "<<b5mm_.b5mo_path<<std::endl;
        sd_processor.Process();
        if(!err_list_.empty())
        {
            LOG(INFO)<<"error list not empty, size : "<<err_list_.size()<<std::endl;
            for(std::size_t i=0;i<err_list_.size();i++)
            {
                const Error& err = err_list_[i];
                LOG(INFO)<<err.oid<<","<<err.pid<<","<<err.odb_pid<<","<<err.type<<std::endl;
            }
            return false;
        }
        return true;
    }

private:
    void Process_(ScdDocument& doc)
    {
        Error err;
        doc.getString("DOCID", err.oid);
        doc.getString("uuid", err.pid);
        odb_->get(err.oid, err.odb_pid);
        if(err.pid!=err.odb_pid)
        {
            err.type = doc.type;
            err_list_.push_back(err);
        }
    }


private:
    B5mM b5mm_;
    OfferDb* odb_;
    std::vector<Error> err_list_;
    boost::shared_mutex mutex_;
};

NS_SF1R_B5M_END

#endif



#ifndef SF1R_B5MMANAGER_ORDEREDWRITER_H_
#define SF1R_B5MMANAGER_ORDEREDWRITER_H_

#include "b5m_types.h"
#include <document-manager/Document.h>
#include <document-manager/ScdDocument.h>
#include <document-manager/JsonDocument.h>
#include <common/ScdTypeWriter.h>
#include <3rdparty/json/json.h>
#include <string>
#include <vector>
#include <boost/thread.hpp>
#include <boost/atomic.hpp>
#include "b5m_helper.h"
#include "product_db.h"
#include <util/functional.h>

NS_SF1R_B5M_BEGIN
using izenelib::util::UString;
class OrderedWriter {
    typedef std::pair<uint32_t, std::vector<std::string> > Value;
    typedef std::vector<Value> Vector;
public:
    OrderedWriter()//invalid use
    {
    }
    OrderedWriter(const std::string& file): ofs_(file.c_str()), capacity_(10000), lastid_(0), thread_(NULL)
    {
    }

    void Insert(uint32_t id, const std::vector<std::string>& texts)
    {
        {
            boost::unique_lock<boost::mutex> lock(mutex_);
            if(data_.size()==capacity_)
            {
                Work_();
            }
            data_.push_back(std::make_pair(id, texts));
        }
    }

    void Finish()
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        Work_();
        Wait_();
        ofs_.close();
    }
private:
    void Wait_()
    {
        if(thread_!=NULL)
        {
            thread_->join();
            delete thread_;
            thread_ = NULL;
        }
    }
    void Work_()
    {
        Wait_();
        wdata_.insert(wdata_.end(), data_.begin(), data_.end());
        data_.resize(0);
        thread_ = new boost::thread(boost::bind(&OrderedWriter::RealWork_, this));
    }
    void RealWork_()
    {
        typedef izenelib::util::first_less<Value> Compare;
        std::sort(wdata_.begin(), wdata_.end(), Compare());
        uint32_t id = lastid_;
        uint32_t i=0;
        for(;i<wdata_.size();i++)
        {
            ++id;
            //LOG(INFO)<<"id "<<id<<","<<wdata_[i].first<<std::endl;
            if(wdata_[i].first!=id) break;
            lastid_ = id;
            for(uint32_t l=0;l<wdata_[i].second.size();l++)
            {
                ofs_<<wdata_[i].second[l]<<std::endl;
            }
        }
        Vector ndata(wdata_.begin()+i, wdata_.end());
        wdata_ = ndata;
    }
private:
    std::ofstream ofs_;
    std::size_t capacity_;
    uint32_t lastid_;
    //std::size_t size_;
    Vector data_;
    Vector wdata_;//working data
    boost::mutex mutex_;
    boost::thread* thread_;
};

NS_SF1R_B5M_END
#endif


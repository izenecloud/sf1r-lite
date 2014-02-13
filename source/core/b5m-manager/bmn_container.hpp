#ifndef SF1R_B5MMANAGER_BMNCONTAINER_HPP_
#define SF1R_B5MMANAGER_BMNCONTAINER_HPP_

#include "b5m_threadpool.h"
#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/shared_ptr.hpp>
#include <am/sequence_file/ssfr.h>
#include <product-manager/product_price.h>
#include "b5m_types.h"
NS_SF1R_B5M_BEGIN

class BmnContainer
{
public:
    struct Value
    {
        Value():gid(0), key(0)
        {
        }
        Value(const std::string& p1, const std::string& p2, const std::string& pc, const std::vector<std::string>& pk, const Document& p3, const std::vector<uint32_t>& p4)
        : brand(p1), model(p2), category(pc), keywords(pk), doc(p3), word(p4), gid(0), key(0)
        {
        }
        std::string brand;
        std::string model;
        std::string category;
        std::vector<std::string> keywords;
        Document doc;
        std::vector<uint32_t> word;
        std::size_t gid;//group_id
        std::size_t key;//not ser
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & brand & model & category & keywords & doc & gid & word;
        }
        double GetPrice() const
        {
            return ProductPrice::ParseDocPrice(doc);
        }
        static bool GidCompare(const Value& x, const Value& y)
        {
            return x.gid<y.gid;
        }
    };
    typedef std::vector<Value> ValueArray;
    typedef izenelib::am::ssf::Writer<std::size_t> Writer;
    typedef izenelib::am::ssf::Reader<std::size_t> Reader;
    typedef izenelib::am::ssf::Sorter<std::size_t, std::size_t> Sorter;
    typedef boost::function<void (ValueArray&)> Processor;
    BmnContainer(const std::string& path)
    : path_(path), file_(path+"/file"), file1_(path+"/file1"), file2_(path+"/file2")
    {
        if(!boost::filesystem::exists(path))
        {
            boost::filesystem::create_directories(path);
        }
        if(boost::filesystem::exists(file1_))
        {
            boost::filesystem::remove_all(file1_);
        }
        writer_.reset(new Writer(file1_));
        writer_->Open();
    }
    void Append(const Value& v)
    {
        std::string skey = v.category+"|"+v.model;
        std::size_t key = izenelib::util::HashFunction<std::string>::generateHash64(skey);
        writer_->Append(key, v);
    }
    void Finish(const Processor& func, int thread_num)
    {
        func_ = func;
        std::size_t write_count = writer_->Count();
        writer_->Close();
        if(write_count==0) return;
        namespace bfs = boost::filesystem;
        if(bfs::exists(file2_))
        {
            bfs::remove_all(file2_);
        }
        Writer writer(file2_);
        writer.Open();
        {
            Reader reader(writer_->GetPath());
            reader.Open();
            std::size_t key=0;
            Value value;
            while(reader.Next(key, value))
            {
                writer.Append(key, value);
            }
            reader.Close();
            bfs::remove_all(writer_->GetPath());
        }
        if(bfs::exists(file_))
        {
            Reader reader(file_);
            reader.Open();
            std::size_t key=0;
            Value value;
            while(reader.Next(key, value))
            {
                writer.Append(key, value);
            }
            reader.Close();
            bfs::remove_all(file_);
        }
        writer.Close();
        Sorter sorter;
        sorter.Sort(file2_);
        fwriter_.reset(new Writer(file_));
        fwriter_->Open();
        Reader reader(file2_);
        reader.Open();
        LOG(INFO)<<"bmn count "<<reader.Count()<<std::endl;
        std::size_t key=0;
        Value value;
        std::size_t lkey=0;
        ValueArray* va = new ValueArray;
        std::size_t p=0;
        B5mThreadPool<ValueArray> pool(thread_num, boost::bind(&BmnContainer::ProcessVA_, this, _1));
        while(reader.Next(key, value))
        {
            ++p;
            if(p%10000==0)
            {
                LOG(INFO)<<"bmn processing "<<p<<std::endl;
            }
            value.key = key;
            if(key!=lkey&&!va->empty())
            {
                pool.schedule(va);
                va = new ValueArray;
            }
            lkey = key;
            va->push_back(value);
        }
        if(!va->empty()) 
        {
            pool.schedule(va);
        }
        else
        {
            delete va;
        }
        LOG(INFO)<<"wait pool finish"<<std::endl;
        pool.wait();
        LOG(INFO)<<"pool finished"<<std::endl;
        reader.Close();
        fwriter_->Close();
        bfs::remove_all(file2_);
    }
    void Finish2(const Processor& func, int thread_num)
    {
        func_ = func;
        writer_->Close();
        namespace bfs = boost::filesystem;
        Reader reader(file_);
        reader.Open();
        LOG(INFO)<<"bmn count "<<reader.Count()<<std::endl;
        std::size_t key=0;
        Value value;
        std::size_t lkey=0;
        ValueArray* va = new ValueArray;
        std::size_t p=0;
        B5mThreadPool<ValueArray> pool(thread_num, boost::bind(&BmnContainer::ProcessVA_, this, _1));
        while(reader.Next(key, value))
        {
            ++p;
            if(p%100==0)
            {
                LOG(INFO)<<"bmn processing "<<p<<std::endl;
            }
            if(key!=lkey&&!va->empty())
            {
                pool.schedule(va);
                va = new ValueArray;
            }
            lkey = key;
            va->push_back(value);
        }
        if(!va->empty()) 
        {
            pool.schedule(va);
        }
        else
        {
            delete va;
        }
        LOG(INFO)<<"wait pool finish"<<std::endl;
        pool.wait();
        LOG(INFO)<<"pool finished"<<std::endl;
        reader.Close();
    }
private:
    void ProcessVA_(ValueArray& va)
    {
        func_(va);
        if(fwriter_)
        {
            boost::unique_lock<boost::mutex> lock(mutex_);
            for(std::size_t i=0;i<va.size();i++)
            {
                fwriter_->Append(va[i].key, va[i]);
            }
        }
    }
private:
    std::string path_;
    std::string file_;
    std::string file1_;
    std::string file2_;
    boost::shared_ptr<Writer> writer_;
    boost::shared_ptr<Writer> fwriter_;
    Processor func_;
    boost::mutex mutex_;
};

NS_SF1R_B5M_END

#endif


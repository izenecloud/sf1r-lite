#ifndef SF1R_B5MMANAGER_B5MASCDGENERATOR_H_
#define SF1R_B5MMANAGER_B5MASCDGENERATOR_H_

#include "b5m_types.h"
#include <string>
#include <vector>
#include "b5m_m.h"
#include "b5m_helper.h"
#include "b5mo_sorter.h"
#include <document-manager/ScdDocument.h>

NS_SF1R_B5M_BEGIN
class B5maScdGenerator {
    struct Value
    {
        std::string pid;
        std::string line;
    };
    typedef std::vector<Value> ValueArray;
    typedef std::vector<std::string> StringArray;
public:
    B5maScdGenerator(const B5mM& b5mm): b5mm_(b5mm), min_(4)
    {
    }
    ~B5maScdGenerator()
    {
    }

    bool Generate(const std::string& mdb_instance)
    {
        std::string mirror_dir = B5MHelper::GetB5moMirrorPath(mdb_instance);
        std::string mirror_file = mirror_dir+"/block";
        if(!boost::filesystem::exists(mirror_file))
        {
            LOG(ERROR)<<"mirror file does not exists"<<std::endl;
            return false;
        }
        const std::string& output_dir = b5mm_.b5ma_path;
        B5MHelper::PrepareEmptyDir(output_dir);
        writer_.reset(new ScdWriter(output_dir, UPDATE_SCD));
        B5mThreadPool<StringArray> pool(b5mm_.thread_num, boost::bind(&B5maScdGenerator::Process_, this, _1));
        std::ifstream ifs(mirror_file.c_str());
        std::string line;
        std::size_t p=0;
        StringArray* array = new StringArray;
        std::string lpid;
        while(std::getline(ifs, line))
        {
            ++p;
            if(p%10000==0)
            {
                LOG(INFO)<<"b5ma processing line "<<p<<std::endl;
            }
            if(line.length()<33) continue;
            std::string pid = line.substr(0, 32);
            if(pid!=lpid&&!array->empty())
            {
                pool.schedule(array);
                array = new StringArray;
            }
            array->push_back(line);
            lpid = pid;
        }
        if(!array->empty()) 
        {
            pool.schedule(array);
        }
        else
        {
            delete array;
        }
        ifs.close();
        pool.wait();
        return true;
    }


private:
    void Process_(StringArray& array)
    {
        if(array.size()<min_) return;
        Json::Reader json_reader;
        B5moSorter::Value value;
        std::vector<ScdDocument> odocs;
        for(std::size_t i=0;i<array.size();i++)
        {
            value.Parse(array[i], &json_reader);
            odocs.push_back(value.doc);
        }
        ScdDocument pdoc;
        pgen_.Gen(odocs, pdoc);
        std::string doctext;
        ScdWriter::DocToString(pdoc, doctext);
        boost::unique_lock<boost::mutex> lock(mutex_);
        writer_->Append(doctext);
    }


private:
    //BrandDb* bdb_;
    B5mM b5mm_;
    std::size_t min_;
    boost::shared_ptr<ScdWriter> writer_;
    B5mpDocGenerator pgen_;
    boost::mutex mutex_;
};

NS_SF1R_B5M_END

#endif



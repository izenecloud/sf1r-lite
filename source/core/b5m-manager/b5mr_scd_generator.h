#ifndef SF1R_B5MMANAGER_B5MRSCDGENERATOR_H_
#define SF1R_B5MMANAGER_B5MRSCDGENERATOR_H_

#include "b5m_types.h"
#include <string>
#include <vector>
#include "b5m_m.h"
#include "b5m_helper.h"
#include "b5mo_sorter.h"
#include "b5m_threadpool.h"
#include <document-manager/ScdDocument.h>

NS_SF1R_B5M_BEGIN
class B5mrScdGenerator {
    typedef std::vector<ScdDocument> DocArray;
public:
    B5mrScdGenerator()
    {
    }
    ~B5mrScdGenerator()
    {
    }

    bool Generate(const std::string& input_path, const std::string& output_dir, int thread_num=1)
    {
        std::vector<std::string> scd_list;
        ScdParser::getScdList(input_path, scd_list);
        if(scd_list.empty()) return false;
        B5MHelper::PrepareEmptyDir(output_dir);
        writer_.reset(new ScdWriter(output_dir, UPDATE_SCD));
        B5mThreadPool<DocArray> pool(thread_num, boost::bind(&B5mrScdGenerator::Process_, this, _1));
        DocArray* array = new DocArray;
        std::string lpid;
        for(uint32_t i=0;i<scd_list.size();i++)
        {
            std::string scd_file = scd_list[i];
            //SCD_TYPE scd_type = ScdParser::checkSCDType(scd_file);
            LOG(INFO)<<"Processing "<<scd_file<<std::endl;
            ScdParser parser(izenelib::util::UString::UTF_8);
            parser.load(scd_file);
            uint32_t n=0;
            for( ScdParser::iterator doc_iter = parser.begin();
              doc_iter!= parser.end(); ++doc_iter, ++n)
            {
                ScdDocument doc(UPDATE_SCD);
                std::string pid;
                SCDDoc& scddoc = *(*doc_iter);
                SCDDoc::iterator p = scddoc.begin();
                for(; p!=scddoc.end(); ++p)
                {
                    const std::string& property_name = p->first;
                    doc.property(property_name) = p->second;
                    if(property_name=="uuid") pid = p->second;
                }
                if(pid.length()!=32) continue;
                if(pid!=lpid&&!array->empty())
                {
                    pool.schedule(array);
                    array = new DocArray;
                }
                array->push_back(doc);
                lpid = pid;
            }
        }
        if(!array->empty()) 
        {
            pool.schedule(array);
        }
        else
        {
            delete array;
        }
        pool.wait();
        writer_->Close();
        return true;
    }

    void Process_(DocArray& odocs)
    {
        ScdDocument pdoc;
        pgen_.Gen(odocs, pdoc);
        std::vector<std::string> text_list;
        for(std::size_t i=0;i<odocs.size();i++)
        {
            std::string doctext;
            ScdWriter::DocToString(odocs[i], doctext);
            text_list.push_back(doctext);
        }
        std::string doctext;
        ScdWriter::DocToString(pdoc, doctext);
        text_list.push_back(doctext);
        boost::unique_lock<boost::mutex> lock(mutex_);
        for(std::size_t i=0;i<text_list.size();i++)
        {
            writer_->Append(text_list[i]);
        }
    }


private:


private:
    //BrandDb* bdb_;
    boost::shared_ptr<ScdWriter> writer_;
    B5mpDocGenerator pgen_;
    boost::mutex mutex_;
};

NS_SF1R_B5M_END

#endif




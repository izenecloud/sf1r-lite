#ifndef SF1R_B5MMANAGER_SCDDOCPROCESSOR_H_
#define SF1R_B5MMANAGER_SCDDOCPROCESSOR_H_

#include <string>
#include <vector>
#include <common/ScdParser.h>
#include <common/ScdTypeWriter.h>
#include <document-manager/ScdDocument.h>
#include <boost/threadpool.hpp>
#include <glog/logging.h>
#include "b5m_types.h"
#include "b5m_helper.h"
#include "b5m_threadpool.h"

NS_SF1R_B5M_BEGIN
class ScdDocProcessor {
public:
    typedef boost::function<void (ScdDocument&) > ProcessorType;
    ScdDocProcessor(const ProcessorType& p, int thread_num=1):p_(p), self_writer_(false), thread_num_(thread_num), pool_(thread_num, boost::bind(&ScdDocProcessor::DoProcessDoc_, this, _1)), count_(0), debug_count_(100000), limit_(0)
    {
    }

    void SetDebugCount(std::size_t c) {debug_count_ = c;}
    void SetLimit(std::size_t l) {limit_ = l;}

    void AddInput(const std::string& scd_path)
    {
        input_.push_back(scd_path);
    }
    void AddInput(const std::vector<std::string>& scds)
    {
        input_.insert(input_.end(), scds.begin(), scds.end());
    }
    void SetOutput(const boost::shared_ptr<ScdTypeWriter>& writer)
    {
        writer_ = writer;
    }

    void SetOutput(const std::string& output_path)
    {
        writer_.reset(new ScdTypeWriter(output_path));
        self_writer_ = true;
        //output_ = output_path;
    }

    void Process()
    {
        std::size_t p=0;
        for(uint32_t i=0;i<input_.size();i++)
        {
            std::vector<std::string> scd_list;
            B5MHelper::GetScdList(input_[i], scd_list);
            for(uint32_t i=0;i<scd_list.size();i++)
            {
                std::string scd_file = scd_list[i];
                SCD_TYPE scd_type = ScdParser::checkSCDType(scd_file);
                LOG(INFO)<<"Processing "<<scd_file<<std::endl;
                ScdParser parser(izenelib::util::UString::UTF_8);
                parser.load(scd_file);
                uint32_t n=0;
                for( ScdParser::iterator doc_iter = parser.begin();
                  doc_iter!= parser.end(); ++doc_iter, ++n)
                {
                    ++p;
                    if(limit_>0&&p>limit_) break;
                    if(n%debug_count_==0)
                    {
                        LOG(INFO)<<"Find Documents "<<n<<std::endl;
                    }
                    ScdDocument* doc = new ScdDocument;
                    doc->type = scd_type;
                    SCDDoc& scddoc = *(*doc_iter);
                    SCDDoc::iterator p = scddoc.begin();
                    for(; p!=scddoc.end(); ++p)
                    {
                        const std::string& property_name = p->first;
                        doc->property(property_name) = p->second;
                    }
                    ProcessDoc(doc);
                }
            }
        }
        FinishDocs();
    }

    void ProcessDoc(ScdDocument* doc)
    {
        //static const std::size_t num_wait = 100000;
        //count_++;
        //if(count_%num_wait==0) pool_.wait();
        pool_.schedule(doc);
        //pool_.schedule(boost::bind(&ScdDocProcessor::DoProcessDoc_, this, doc));
        //DoProcessDoc_(doc);
    }

    void FinishDocs()
    {
        pool_.wait();
        if(writer_&&self_writer_)
        {
            writer_->Close();
        }
    }


private:
    void DoProcessDoc_(ScdDocument& doc)
    {
        p_(doc);
        if(writer_)
        {
            std::string doctext;
            ScdWriter::DocToString(doc, doctext);
            boost::unique_lock<boost::mutex> lock(mutex_);
            writer_->Append(doctext, doc.type);
        }
    }


private:
    ProcessorType p_;
    std::vector<std::string> input_;
    boost::shared_ptr<ScdTypeWriter> writer_;
    bool self_writer_;
    int thread_num_;
    //boost::threadpool::thread_pool<> pool_;
    B5mThreadPool<ScdDocument> pool_;
    boost::mutex mutex_;
    std::size_t count_;
    std::size_t debug_count_;
    std::size_t limit_;
    //std::string output_;
};

NS_SF1R_B5M_END

#endif


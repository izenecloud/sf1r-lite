#ifndef SF1R_B5MMANAGER_SCDDOCPROCESSOR_H_
#define SF1R_B5MMANAGER_SCDDOCPROCESSOR_H_

#include <string>
#include <vector>
#include <common/ScdParser.h>
#include <common/ScdTypeWriter.h>
#include <boost/threadpool.hpp>

namespace sf1r {
    class ScdDocProcessor {
    public:
        typedef boost::function<void (ScdDocument&) > ProcessorType;
        ScdDocProcessor(const ProcessorType& p, int thread_num=1):p_(p), thread_num_(thread_num), pool_(thread_num)
        {
        }

        void AddInput(const std::string& scd_path)
        {
            input_.push_back(scd_path);
        }
        void SetOutput(const boost::shared_ptr<ScdTypeWriter>& writer)
        {
            writer_ = writer;
        }

        void SetOutput(const std::string& output_path)
        {
            writer_.reset(new ScdTypeWriter(output_path));
            //output_ = output_path;
        }

        void Process()
        {
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
                        if(n%100000==0)
                        {
                            LOG(INFO)<<"Find Documents "<<n<<std::endl;
                        }
                        ScdDocument doc;
                        doc.type = scd_type;
                        SCDDoc& scddoc = *(*doc_iter);
                        SCDDoc::iterator p = scddoc.begin();
                        for(; p!=scddoc.end(); ++p)
                        {
                            const std::string& property_name = p->first;
                            doc.property(property_name) = p->second;
                        }
                        ProcessDoc(doc);
                    }
                }
            }
            FinishDocs();
        }

        void ProcessDoc(ScdDocument& doc)
        {
            pool_.schedule(boost::bind(&ScdDocProcessor::DoProcessDoc_, this, doc));
        }

        void FinishDocs()
        {
            pool_.wait();
            if(writer_)
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
                boost::unique_lock<boost::mutex> lock(mutex_);
                writer_->Append(doc);
            }
        }


    private:
        ProcessorType p_;
        std::vector<std::string> input_;
        boost::shared_ptr<ScdTypeWriter> writer_;
        int thread_num_;
        boost::threadpool::thread_pool<> pool_;
        boost::mutex mutex_;
        //std::string output_;
    };

}

#endif


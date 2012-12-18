#ifndef SF1R_B5MMANAGER_SCDDOCPROCESSOR_H_
#define SF1R_B5MMANAGER_SCDDOCPROCESSOR_H_

#include <string>
#include <vector>
#include <common/ScdParser.h>
#include <common/ScdTypeWriter.h>

namespace sf1r {
    class ScdDocProcessor {
    public:
        typedef boost::function<void (Document&, int&) > ProcessorType;
        ScdDocProcessor(const ProcessorType& p):p_(p)
        {
        }

        void AddInput(const std::string& scd_path)
        {
            input_.push_back(scd_path);
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
                    int type = ScdParser::checkSCDType(scd_file);
                    //if(type==DELETE_SCD) continue;
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
                        Document doc;
                        SCDDoc& scddoc = *(*doc_iter);
                        SCDDoc::iterator p = scddoc.begin();
                        for(; p!=scddoc.end(); ++p)
                        {
                            const std::string& property_name = p->first;
                            doc.property(property_name) = p->second;
                        }
                        p_(doc, type);
                        if(writer_)
                        {
                            writer_->Append(doc, type);
                        }
                    }
                }
            }
            if(writer_)
            {
                writer_->Close();
            }
        }


    private:


    private:
        ProcessorType p_;
        std::vector<std::string> input_;
        boost::shared_ptr<ScdTypeWriter> writer_;
        //std::string output_;
    };

}

#endif


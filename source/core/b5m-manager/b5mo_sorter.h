#ifndef SF1R_B5MMANAGER_B5MOSORTER_H_
#define SF1R_B5MMANAGER_B5MOSORTER_H_

#include <document-manager/Document.h>
#include <document-manager/ScdDocument.h>
#include <document-manager/JsonDocument.h>
#include <common/ScdTypeWriter.h>
#include <3rdparty/json/json.h>
#include <string>
#include <vector>
#include <boost/thread.hpp>
#include "b5m_helper.h"
#include "b5m_types.h"
#include "product_db.h"

namespace sf1r {
    using izenelib::util::UString;
    class B5moSorter {
    public:
        struct Value
        {
            std::string spid;
            ScdDocument doc;
            //Document doc;
            //bool is_update;//else delete
            std::string ts;
            Value(){}
            Value(const ScdDocument& d, const std::string& t):doc(d), ts(t)
            {
            }
            bool Parse(const std::string& str)
            {
                std::size_t begp=0;
                std::size_t endp=str.find('\t', begp);
                if(endp==std::string::npos) return false;
                spid = str.substr(begp, endp-begp);
                begp=endp+1;
                endp=str.find('\t', begp);
                if(endp==std::string::npos) return false;
                int inttype = boost::lexical_cast<int>(str.substr(begp, endp-begp));
                doc.type=(SCD_TYPE)inttype;
                begp=endp+1;
                endp=str.find('\t', begp);
                if(endp==std::string::npos) return false;
                ts = str.substr(begp, endp-begp);
                begp=endp+1;
                std::string json_str = str.substr(begp);
                if(json_str.empty()) return false;
                Json::Value value;
                static Json::Reader json_reader;
                json_reader.parse(json_str, value);
                JsonDocument::ToDocument(value, doc);
                return true;
            }
        };
        B5moSorter(const std::string& m, uint32_t mcount=100000);

        void Append(const ScdDocument& doc, const std::string& ts);

        bool StageOne();
        bool StageTwo(const std::string& last_m);


    private:
        void WriteValue_(std::ofstream& ofs, const ScdDocument& doc, const std::string& ts);
        static bool PidCompare_(const Value& doc1, const Value& doc2)
        {
            UString pid1;
            UString pid2;
            doc1.doc.getProperty("uuid", pid1);
            doc2.doc.getProperty("uuid", pid2);
            return pid1<pid2;
        }
        static bool OCompare_(const Value& doc1, const Value& doc2)
        {
            UString oid1;
            UString oid2;
            doc1.doc.getProperty("DOCID", oid1);
            doc2.doc.getProperty("DOCID", oid2);
            int c = oid1.compare(oid2);
            if(c<0) return true;
            else if(c>0) return false;
            else
            {
                return doc1.ts<doc2.ts;
            }
        }
        void WaitUntilSortFinish_();
        void DoSort_();
        void Sort_(std::vector<Value>& docs);
        void OBag_(std::vector<Value>& docs);
        static void ODocMerge_(std::vector<ScdDocument>& vec, const ScdDocument& doc);
        bool GenMirrorBlock_(const std::string& mirror_path);

    private:
        std::string m_;
        std::string ts_;
        uint32_t mcount_;
        uint32_t index_;
        std::vector<Value> buffer_;
        boost::thread* sort_thread_;
        std::ofstream mirror_ofs_;
        boost::shared_ptr<ScdTypeWriter> pwriter_;
        B5mpDocGenerator pgenerator_;
    };

}

#endif



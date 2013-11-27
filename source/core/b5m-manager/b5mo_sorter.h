#ifndef SF1R_B5MMANAGER_B5MOSORTER_H_
#define SF1R_B5MMANAGER_B5MOSORTER_H_

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
#include "ordered_writer.h"
#include "b5m_m.h"

NS_SF1R_B5M_BEGIN
using izenelib::util::UString;
class B5moSorter {
typedef boost::mutex MutexType;
public:
    struct Value
    {
        std::string spid;
        ScdDocument doc;
        ScdDocument diff_doc;
        //Document doc;
        //bool is_update;//else delete
        std::string ts;
        int flag;
        std::string text;
        Value(): flag(0){}
        Value(const ScdDocument& d, const std::string& t, int f=0):doc(d), ts(t), flag(f)
        {
        }
        void merge(const Value& value)
        {
            doc.merge(value.doc);
            ts = value.ts;
            flag = value.flag;
        }
        bool ParsePid(const std::string& str)
        {
            if(str.length()<32) return false;
            spid = str.substr(0, 32);
            text = str;
            return true;
        }
        bool Parse(const std::string& str, Json::Reader* json_reader)
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
            endp=str.find('\t', begp);
            if(endp==std::string::npos) return false;
            flag = boost::lexical_cast<int>(str.substr(begp, endp-begp));
            begp=endp+1;
            std::string json_str = str.substr(begp);
            if(json_str.empty()) return false;
            Json::Value value;
            //static Json::Reader json_reader;
            json_reader->parse(json_str, value);
            JsonDocument::ToDocument(value, doc);
            return true;
        }
    };

    struct PItem{
        PItem():id(0), pdoc(NOT_SCD)
        {
        }
        uint32_t id;
        std::vector<Value> odocs;
        std::vector<ScdDocument> diff_odocs;
        ScdDocument pdoc;
    };
    B5moSorter(const std::string& m, uint32_t mcount=100000);

    void SetB5mM(const B5mM& b5mm) { b5mm_ = b5mm; }

    void SetBufferSize(const std::string& bs)
    {
        buffer_size_ = bs;
    }
    void SetSorterBin(const std::string& bin)
    {
        sorter_bin_ = bin;
    }

    void Append(const ScdDocument& doc, const std::string& ts, int flag=0);

    bool StageOne();
    bool StageTwo(const std::string& last_m, int thread_num=1);

private:
    void WriteValue_(std::ofstream& ofs, const Value& value);
    static bool PidCompare_(const Value& doc1, const Value& doc2)
    {
        Document::doc_prop_value_strtype pid1;
        Document::doc_prop_value_strtype pid2;
        doc1.doc.getProperty("uuid", pid1);
        doc2.doc.getProperty("uuid", pid2);
        return pid1<pid2;
    }
    static bool OCompare_(const Value& doc1, const Value& doc2)
    {
        Document::doc_prop_value_strtype oid1;
        Document::doc_prop_value_strtype oid2;
        doc1.doc.getProperty("DOCID", oid1);
        doc2.doc.getProperty("DOCID", oid2);
        int c = oid1.compare(oid2);
        if(c<0) return true;
        else if(c>0) return false;
        else
        {
            c = doc1.ts.compare(doc2.ts);
            if(c<0) return true;
            else if(c>0) return false;
            else 
            {
                //always keep -D to be in the last, as omapper changing will output -U.
                return doc1.doc.type<doc2.doc.type;
            }
        }
    }
    void WaitUntilSortFinish_();
    void DoSort_();
    void Sort_(std::vector<Value>& docs);
    void OBag_(PItem& pitem);
    static void ODocMerge_(std::vector<ScdDocument>& vec, const ScdDocument& doc);
    static void ODocMerge_(std::vector<Value>& vec, const Value& doc);
    static void SetAttributes_(std::vector<Value>& values, const ScdDocument& pdoc);
    bool GenMirrorBlock_(const std::string& mirror_path);
    bool GenMBlock_();
    void WritePItem_(PItem& pitem);

private:
    B5mM b5mm_;
    std::string m_;
    std::string ts_;
    std::ofstream ofs_;
    std::string sorter_bin_;
    uint32_t mcount_;
    uint32_t index_;
    std::string buffer_size_;
    //boost::atomic<uint32_t> last_pitemid_;
    //uint32_t last_pitemid_;
    std::vector<Value> buffer_;
    boost::thread* sort_thread_;
    //std::ofstream mirror_ofs_;
    boost::shared_ptr<OrderedWriter> ordered_writer_;
    boost::shared_ptr<ScdTypeWriter> pwriter_;
    B5mpDocGenerator pgenerator_;
    Json::Reader json_reader_;
    MutexType mutex_;
};

NS_SF1R_B5M_END

#endif



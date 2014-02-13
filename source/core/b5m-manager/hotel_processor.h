#ifndef SF1R_B5MMANAGER_HOTELPROCESSOR_H
#define SF1R_B5MMANAGER_HOTELPROCESSOR_H 
#include <util/ustring/UString.h>
#include <product-manager/product_price.h>
#include <document-manager/ScdDocument.h>
#include "b5m_helper.h"
#include "b5m_types.h"
#include "b5m_m.h"
#include <am/sequence_file/ssfr.h>
#include <boost/unordered_map.hpp>
#include <common/ScdWriter.h>
NS_SF1R_B5M_BEGIN


class HotelProcessor
{
typedef uint32_t term_t;
typedef std::vector<term_t> word_t;
typedef std::vector<std::size_t> posting_t;
typedef std::map<word_t, posting_t> map_t;
typedef std::pair<std::size_t, std::size_t> DocPair;
typedef std::vector<word_t> WordList;
typedef boost::unordered_map<DocPair, WordList> MCS;
struct Item {
    Document doc;
    word_t word;
    std::size_t index;
};
typedef izenelib::am::ssf::Writer<std::size_t> Writer;
typedef izenelib::am::ssf::Reader<std::size_t> Reader;
typedef izenelib::am::ssf::Sorter<std::size_t, std::size_t> Sorter;
public:
    HotelProcessor(const B5mM& b5mm);
    ~HotelProcessor();
    bool Generate(const std::string& mdb_instance);

private:
    void GetWord_(const std::string& name, word_t& word);
    void DoClustering_(std::vector<Document>& docs);
    double NameSim_(const Item& x, const Item& y, const MCS& mcs);
    double Sim_(const Item& x, const Item& y, const MCS& mcs);
    void ProcessDoc_(ScdDocument& doc);
    void GenP_(const std::vector<Document>& docs, Document& pdoc);


private:
    B5mM b5mm_;
    idmlib::util::IDMAnalyzer* analyzer_;
    boost::shared_ptr<Writer> writer_;
    boost::shared_ptr<ScdWriter> owriter_;
    boost::shared_ptr<ScdWriter> pwriter_;

};

NS_SF1R_B5M_END

#endif


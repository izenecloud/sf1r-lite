/**
 * @file KNlpWrapper.h
 * @brief a wrapper class for ilplib::knlp functions.
 */

#ifndef SF1R_ATTRIBUTE_TOKENIZE_WRAPPER_H
#define SF1R_ATTRIBUTE_TOKENIZE_WRAPPER_H

#include <vector>
#include <map>
#include <string>
#include <boost/scoped_ptr.hpp>

namespace ilplib
{
namespace knlp
{
    class AttributeTokenize;
    class VectorDictionary;
}
}

namespace sf1r
{

class AttrTokenizeWrapper
{
public:

    static AttrTokenizeWrapper* get();

    AttrTokenizeWrapper();
    ~AttrTokenizeWrapper();

    bool loadDictFiles(const std::string& dictDir);

    /**
     * Attribute Tokenize
     */
    void attr_tokenize_index(
        const std::string& title, 
        const std::string& attr, 
        const std::string& cate, 
        const std::string& ocate,
        const std::string& source,
        std::vector<std::pair<std::string, double> >& tokenScoreList);

    void attr_tokenize(
        const std::string& Q,
        std::vector<std::pair<std::string, int> >& tokenList,
        bool do_subtok = false);

    /**
     * @return true if @p tokenList is not equal with @p tks.
     */
    //bool attr_subtokenize(
      //  const std::vector<std::pair<std::string, int> >& tks,
        //std::vector<std::pair<std::string, int> >& tokenList);

    double att_name_weight(
        const std::string& attr_name,
        const std::string& cate);

    double att_value_weight(
        const std::string& attr_name,
        const std::string& attr_value,
        const std::string& cate);

    std::vector<char*>** get_TermCategory(const std::string& query);

private:
    std::string dictDir_;
    bool isDictLoaded_;
    boost::scoped_ptr<ilplib::knlp::VectorDictionary> queryMultiCatesDict_;

    boost::scoped_ptr<ilplib::knlp::AttributeTokenize> attr_tokenizer_;
};

} // namespace sf1r

#endif // SF1R_ATTRIBUTE_TOKENIZE_WRAPPER_H

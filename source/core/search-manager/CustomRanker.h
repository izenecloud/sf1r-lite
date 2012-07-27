/**
 * @file CustomRanker.h
 * @author Zhongxia Li
 * @date Apr 13, 2011
 * @brief Support Custom Ranking feature
 */
#ifndef CUSTOM_RANKER_H_
#define CUSTOM_RANKER_H_

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>

#include <search-manager/CustomRankTreeParser.h>

#include <common/inttypes.h>
#include <util/ustring/UString.h>

using namespace BOOST_SPIRIT_CLASSIC_NS;

namespace sf1r {

class SortPropertyCache;

class CustomRanker
{
public:
    //typedef std::vector<> d;
private:
    typedef tree_match<char const*>::container_t ast_info_trees;
    typedef node_val_data<>::iterator_t ast_info_tree_val_iter;

private:
    double score_;

    // parameters
    std::map<std::string, double> paramConstValueMap_;
    std::map<std::string, std::string> paramPropertyValueMap_;

    std::map<std::string, PropertyDataType> propertyDataTypeMap_;

    // expression
    std::string strExp_;
    izenelib::util::UString ustrExp_;

    // expression tree parser
    CustomRankTreeParser customRankTreeParser_;
    ExpSyntaxTreePtr ESTree_;

    // error info
    std::string errorInfo_;

public:
    CustomRanker()
    : score_(0)
    , strExp_()
    , errorInfo_()
    {
        ESTree_.reset(new ExpSyntaxTree(ExpSyntaxTree::ROOT));
    }

    CustomRanker(std::string& strExp)
    : score_(0)
    , strExp_(strExp)
    , errorInfo_()
    {
        ESTree_.reset(new ExpSyntaxTree(ExpSyntaxTree::ROOT)); // root
    }

    ~CustomRanker()
    {

    }

    /**
     * @brief error information
     */
    std::string& getErrorInfo()
    {
        return errorInfo_;
    }

    /**
     * @brief convert string to real number
     */
    static bool str2real(const std::string& str, double& ret)
    {
        try {
            ret = boost::lexical_cast<double>(str);
            return true;
        }
        catch(const boost::bad_lexical_cast&)
        {
            ret = 0;
            return false;
        }
    }

    /**
     * @brief Validate name string
     * include alphabets, numbers or '_', start with an alphabet or '_'.
     * consistent with rule of CustomRankTreeParser:param
     */
    static bool validateName(std::string& name)
    {
        rule<phrase_scanner_t> name_parser =
                (alpha_p|ch_p('_')) >> *(alnum_p|ch_p('_'));

        parse_info<> info = BOOST_SPIRIT_CLASSIC_NS::parse(name.c_str(), name_parser);

        if (info.full)
            return true;
        else
            return false;
    }

public:
    /**
     * @brief set parameters
     * value of parameters can be double or string(property name)
     */
    void addConstantParam(const std::string& name, double& value)
    {
        paramConstValueMap_.insert(std::make_pair(name, value));
    }

    std::map<std::string, double>& getConstParamMap()
    {
        return paramConstValueMap_;
    }

    void addPropertyParam(const std::string& name, const std::string& value)
    {
        paramPropertyValueMap_.insert(std::make_pair(name, value));
    }

    std::map<std::string, std::string>& getPropertyParamMap()
    {
        return paramPropertyValueMap_;
    }

    /**
     * @brief get properties occurred in custom ranking expression
     */
    std::map<std::string, PropertyDataType>& getPropertyDataTypeMap()
    {
        return propertyDataTypeMap_;
    }

    std::string& getExpression()
    {
        return strExp_;
    }

    /**
     * @brief Parse the arithmetic expression for evaluating custom ranking score
     * @param ustrExp string of arithmetic expression
     * @return true if success, or false
     */
    bool parse()
    {
        return parse(strExp_);
    }

    bool parse(std::string& strExp);

    bool parse(izenelib::util::UString& ustrExp);

    /**
     * @brief Build expression syntax tree with sort property data.
     */
    bool setPropertyData(SortPropertyCache* pPropertyData)
    {
        cout << "[CustomRanker] setting PropertyData ..." << endl;
        return setInnerPropertyData(ESTree_, pPropertyData);
    }

    /**
     * @brief Evaluate the custom ranking score for a document
     * @param docid the id of document to be evaluated
     * @return score
     */
    double evaluate(docid_t& docid)
    {
        sub_evaluate(ESTree_, docid);
        //cout << "CustomRanker::evaluate(" << docid << ") = " <<  ESTree_->value_ <<endl; //test
        return ESTree_->value_;
    }

private:
    /**
     * brief Build expression syntax tree for custom ranking
     */
    bool buildExpSyntaxTree(const ast_info_trees& astrees, ExpSyntaxTreePtr& estree);

    /**
     * @brief Set property data to Expression Syntax Tree recursively.
     */
    bool setInnerPropertyData(ExpSyntaxTreePtr& estree, SortPropertyCache* pPropertyData);

    /**
     * @brief evaluate custom ranking score recursively from EST
     * the return value may be meaningless currently.
     */
    bool sub_evaluate(ExpSyntaxTreePtr& estree, docid_t& docid);

    /**
     * @brief remove front and trailing space
     */
    void trim(string& s)
    {
        string whitespaces (" \t\f\v\n\r");

        size_t begin = s.find_first_not_of(whitespaces);
        size_t end = s.find_last_not_of(whitespaces);

        if (begin == string::npos)
        {
            string().swap(s); // only whitespaces in string
            return;
        }

        string tmp(s, begin, end-begin+1);
        s.swap(tmp);
    }

    /// for testing
    void showBoostAST(const ast_info_trees& trees, int level=0);

    void showEST(est_trees& trees, int level=0);

public:
    void printESTree();

};

typedef boost::shared_ptr<CustomRanker> CustomRankerPtr;

} // namespace sf1r

#endif /* CUSTOM_RANKER_H_ */

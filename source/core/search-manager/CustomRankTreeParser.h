/**
 * @file CustomRankingParser.h
 * @author Zhongxia Li
 * @date Apr 13, 2011
 * @brief Custom Ranking Parser
 * Parser for parsing arithmetic expression which is used to evaluating custom ranking score
 */
#ifndef CUSTOM_RANK_TREE_PARSER_H_
#define CUSTOM_RANK_TREE_PARSER_H_

#define BOOST_SPIRIT_THREADSAFE
#include <boost/spirit/include/classic.hpp>
#include <boost/spirit/include/classic_ast.hpp>

#include "PropertyData.h"
#include <common/type_defs.h>

namespace sf1r {

using namespace std;
using namespace BOOST_SPIRIT_CLASSIC_NS;

class ExpSyntaxTree // Node
{
public:
    enum NodeType {
        UNKNOWN,   // unknown
        ROOT,      // root
        /* variable */
        CONSTANT,  // constant value
        PARAMETER, // property name which indicating where to get value,
                   // or variable who's value is set by user(set as constant after parsing)
        /* operators */
        SUM,       // +
        SUB,       // -
        PRODUCT,   // *
        DIV,       // /
        SQRT,      // sqrt(x), square root
        LOG,       // log(x)
        POW,       // pow(x,y)
    };

    static string getTypeString(int typeValue)
    {
        switch (typeValue)
        {
        case CONSTANT:
            return string("CONSTANT");
        case PARAMETER:
            return string("PARAMETER");
        case SUM:
            return string("SUM");
        case SUB:
            return string("SUB");
        case PRODUCT:
            return string("PRODUCT");
        case DIV:
            return string("DIV");
        case LOG:
            return string("LOG");
        case POW:
            return string("POW");
        case SQRT:
            return string("SQRT");
        case ROOT:
            return string("ROOT");
        default:
            return string("UNKNOWN");
        }
    }

public:
    NodeType type_;
    string name_; // param name or property name
    double value_;

    boost::shared_ptr<PropertyData> propertyData_;
    std::vector<boost::shared_ptr<ExpSyntaxTree> > children_;

    ExpSyntaxTree()
    :type_(UNKNOWN), name_(), value_(0)
    {

    }

    ExpSyntaxTree(NodeType type)
    :type_(type), name_(), value_(0)
    {

    }
};

typedef boost::shared_ptr<ExpSyntaxTree> ExpSyntaxTreePtr;
typedef std::vector<boost::shared_ptr<ExpSyntaxTree> > est_trees;
typedef std::vector<boost::shared_ptr<ExpSyntaxTree> >::iterator est_iterator;


/**
 * @brief CustomRankingParser grammar
 */
class CustomRankTreeParser : public grammar<CustomRankTreeParser>
{
public:
    template <typename ScannerT>
    struct definition
    {
        real_parser<double> real_db_p;
        rule<ScannerT, parser_context<>, parser_tag<ExpSyntaxTree::CONSTANT> > constant;
        rule<ScannerT, parser_context<>, parser_tag<ExpSyntaxTree::PARAMETER> > param;
        rule<ScannerT, parser_context<>, parser_tag<ExpSyntaxTree::LOG> > op_pre_log;
        rule<ScannerT, parser_context<>, parser_tag<ExpSyntaxTree::SQRT> > op_pre_sqrt;
        rule<ScannerT, parser_context<>, parser_tag<ExpSyntaxTree::POW> > op_pre_pow;
        rule<ScannerT, parser_context<>, parser_tag<ExpSyntaxTree::PRODUCT> > op_product;
        rule<ScannerT, parser_context<>, parser_tag<ExpSyntaxTree::DIV> > op_div;
        rule<ScannerT, parser_context<>, parser_tag<ExpSyntaxTree::SUM> > op_sum;
        rule<ScannerT, parser_context<>, parser_tag<ExpSyntaxTree::SUB> > op_sub;

        rule<ScannerT> op_pre_1, op_pre_2, op_1, op_2, op_3;
        rule<ScannerT> factor_pre_1, factor_pre_2, factor_0, factor_1, factor_2;
        rule<ScannerT> exp_;

        const rule<ScannerT>& start() const { return exp_; }

        definition(const CustomRankTreeParser&)
        {
            constant = real_db_p;
            param = leaf_node_d[lexeme_d[(alpha_p|ch_p('_')) >> *(alnum_p|ch_p('_'))]]; // include alphabets, numbers or '_', start with an alphabet or '_'.
            op_pre_log = as_lower_d[str_p("log")];
            op_pre_sqrt = as_lower_d[str_p("sqrt")];
            op_pre_pow = as_lower_d[str_p("pow")];
            op_product = ch_p('*');
            op_div = ch_p('/');
            op_sum = ch_p('+');
            op_sub = ch_p('-');

            // group operators by priority: pre > 1 > 2 ..
            op_pre_1 = op_pre_log | op_pre_sqrt; // prefix unary operator
            op_pre_2 = op_pre_pow;               // prefix binary operator
            op_1 = op_product | op_div;          // *, /
            op_2 = op_sum | op_sub;              // +, -

            factor_pre_1 = root_node_d[op_pre_1] >> inner_node_d[('(' >> exp_  >> ')')];
            factor_pre_2 = root_node_d[op_pre_2] >>
                           inner_node_d[('(' >> (exp_ >> no_node_d[ch_p(',')] >> exp_) >> ')')];
            // !! param parser should behind factor_pre_x parser, if param is the first candidate parser
            // for "log(..)" (or other name indicated operator parser), "log" will be full matched as a
            // param name, and there is no following rule for "(..)". In this case, boost spirit will not
            // backtrack and retry other parser(rule), therefore cause an error.
            factor_0 = factor_pre_1 | factor_pre_2 |
                       param | // param as candidate parser behind factor_pre_x (name indicated operator)
                       constant |
                       inner_node_d[('(' >> exp_ >> ')')];  // () operator
            factor_1 = factor_0 >> *(root_node_d[op_1] >> factor_0);
            factor_2 = factor_1 >> *(root_node_d[op_2] >> factor_1);
            exp_ = factor_2;
        }
    };
};

} // namespace sf1r

#endif /* CUSTOM_RANK_TREE_PARSER_H_ */

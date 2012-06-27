#include "PropertyConfig.h"


using namespace std;

namespace sf1r
{

bool PropertyConfig::isAnalyzed() const
{
    if ( propertyType_ == sf1r::STRING_PROPERTY_TYPE && !analysisInfo_.analyzerId_.empty() )
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool PropertyConfig::isAliasProperty() const
{
    if (originalName_.empty() || originalName_ == propertyName_)
        return false;

    return true;
}

std::string PropertyConfig::toString() const
{
    stringstream ss;
    ss << "[PropertyConfigData] @id=" << propertyId_
    << " @name=" << propertyName_
    << " @originalName=" << originalName_
    << " @type=";

    switch ( propertyType_ )
    {
    case sf1r::STRING_PROPERTY_TYPE:
        ss << "string_type";
        break;
    case sf1r::INT_PROPERTY_TYPE:
        ss << "int_type";
        break;
    case sf1r::FLOAT_PROPERTY_TYPE:
        ss << "float_type";
        break;
    case sf1r::NOMINAL_PROPERTY_TYPE:
        ss << "nominal_type";
        break;
	case sf1r::DATETIME_PROPERTY_TYPE:
		ss << "datetime_type";
		break;
    default:
        break;
    };

    ss << " @index=" << ( bIndex_ ? "yes" : "no" )
    << " @snippet=" << ( bSnippet_ ? "yes" : "no" )
    << " @summary=" << ( bSummary_ ? "yes" : "no" )
    << " @highlight=" << ( bHighlight_ ? "yes" : "no" )
    << " @displayLength=" << displayLength_
    << " @summaryNum=" << summaryNum_ << "]";

    ss << " @filter=" << ( bFilter_ ? "yes" : "no" )
    << " @analysisInfo=("
    << " @methodID=" << analysisInfo_.analyzerId_;

    set<string>::const_iterator it;
    for ( it = analysisInfo_.tokenizerNameList_.begin(); it != analysisInfo_.tokenizerNameList_.end(); it++ )
    {
        ss << " @regulatorID=" << *it;
    }

    ss << ") @rankWeight=" << rankWeight_;

    return ss.str();
}

} // namespace


// eof

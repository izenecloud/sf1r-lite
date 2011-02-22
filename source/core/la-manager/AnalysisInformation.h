/// @file AnalysisInformation.h
/// @brief header file of AnalysisInformation
/// @author JunHui
/// @date 2008-09-03

#if !defined(_Analysis_Information_)
#define _Analysis_Information_

#include <string>
#include <sstream>
#include <list>

#include <util/izene_serialization.h>
#include <boost/serialization/list.hpp>
#include <boost/functional/hash.hpp>

namespace sf1r
{

    ///
    /// @brief Information of Lanaguage Analyzer options
    /// This class(structure) includes regulator list and language analysis identifier in the configuration file.
    ///
    class AnalysisInfo
    {
    public:
        AnalysisInfo()
        {
        }

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar& tokenizerNameList_;
            ar& analyzerId_;
        }

        void clear()
        {
            tokenizerNameList_.clear();
            analyzerId_.clear();
        }

        std::string toString() const
        {
            std::stringstream ss;
            ss << "analyzer:[" << analyzerId_ 
                << "] tokenizer(s):[";

            for( std::set<std::string>::const_iterator i = tokenizerNameList_.begin();
                    i != tokenizerNameList_.end(); i++)
                ss << *i << " ";
            ss << "] ";

            /*
            ss << std::endl;
            ss << "AnalysisInformation class" << std::endl;
            ss << "------------------------------------------------" << std::endl;
            ss << "RegulatorNameList_ : ";
            for( std::set<std::string>::const_iterator i = tokenizerNameList_.begin();
                    i != tokenizerNameList_.end(); i++)
                ss << *i << " ";
            ss << std::endl;
            ss << "Method ID : " << analyzerId_ << std::endl;
            */

            return ss.str();
        } // end - toString()

        DATA_IO_LOAD_SAVE(AnalysisInfo, &tokenizerNameList_&analyzerId_)

        bool operator==( const AnalysisInfo & obj ) const
        {
            return analyzerId_ == obj.analyzerId_ && tokenizerNameList_ == obj.tokenizerNameList_;
        }

        /// @brief  creating hash key for the instance.
        friend std::size_t hash_value( const AnalysisInfo & obj )
        {
            std::size_t seed =0;
            boost::hash_combine( seed, obj.analyzerId_ );

            std::set<std::string>::const_iterator it;
            for(it = obj.tokenizerNameList_.begin(); it != obj.tokenizerNameList_.end(); it++ )
            {
                boost::hash_combine( seed, *it );
            }

            return seed;
        }
    public:
        std::set<std::string> tokenizerNameList_; ///< list of regulator names in the configuration file
        std::string analyzerId_; ///< language analysis identifier
    };

} // namespace sf1r

#endif // _Analysis_Information_

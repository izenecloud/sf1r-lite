/// @file LaInformation.h
/// @brief header file of LaInformation
/// @author JunHui
/// @date 2008-07-02

#if !defined(_LA_Information_)
#define _LA_Information_

#include <list>

#include <boost/serialization/list.hpp>
#include <util/izene_serialization.h>

#include <util/ustring/UString.h>

#include <sstream>

namespace sf1r
{

    ///
    /// @brief interface of LAInformation
    /// This class contains information of a term.
    ///
    class LAInformation
    {
    public:
        LAInformation():termId_(0),morphemeInformation_(0),wordOffset_(0),byteOffset_(0),subOffset_(0)
        {}

        ~LAInformation()
        {
        }
        
        void operator=(const LAInformation& laInfo)
        {
            termId_              = laInfo.termId_;
            termString_          = laInfo.termString_;
            morphemeInformation_ = laInfo.morphemeInformation_;
            wordOffset_          = laInfo.wordOffset_;
            byteOffset_          = laInfo.byteOffset_;
            subOffset_           = laInfo.subOffset_;
        }
        
        friend class boost::serialization::access;
        
        template<class Archive>
        void serialize(Archive& ar, const unsigned int version)
        {
            ar& termId_;
            ar& termString_;
            ar& morphemeInformation_;
            ar& wordOffset_;
            ar& byteOffset_;
            ar& subOffset_;
        }

        void clear()
        {
            termId_ = 0;
            termString_.clear();
            morphemeInformation_ = 0;
            wordOffset_ = 0;
            byteOffset_ = 0;
            subOffset_ = 0;
        }

        std::string toString()
        {
            std::string tmp;
            termString_.convertString(tmp, izenelib::util::UString::CP949);
            std::stringstream ss;
            ss << "LAInformation object"                    << std::endl;
            ss << "--------------------------"              << std::endl;
            ss << "termId_     : " << termId_               << std::endl;
            ss << "termString_ : " << tmp                   << std::endl;
            ss << "morpheme    : " << morphemeInformation_  << std::endl;
            ss << "wordOffset_ : " << wordOffset_           << std::endl;
            ss << "byteOffset_ : " << byteOffset_           << std::endl;
            ss << "subOffset_  : " << subOffset_            << std::endl;

            return ss.str();

        } // end - toString()
        
        DATA_IO_LOAD_SAVE(LAInformation, &termId_&termString_&morphemeInformation_&wordOffset_&byteOffset_&subOffset_)

    public:
        unsigned int termId_;               /// identifier of the term
        izenelib::util::UString termString_;                ///< string data of the term
        unsigned char morphemeInformation_; ///< morpheme information of the term
        unsigned int wordOffset_;           ///< word offset of the term
        unsigned int byteOffset_;           ///< byte offset of the term // TODO: will save the end offset for those terms that have a range
        unsigned int subOffset_;            ///< sub offset which is used for compound word
    };

#define LAInformationList std::list<LAInformation>

} // namespace sf1r

#endif // _LA_Information_

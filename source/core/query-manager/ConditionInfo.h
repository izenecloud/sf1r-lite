///
/// @file   ConditionInfo.h
/// @brief  Header file of structures which are used by ActionItem classes
/// @author Dohyun Yun
/// @date   2008-06-05
/// @dtails
/// - Log
///     - 2009.08.10 Remain only PageInfo and all the classes are commented.
///     - 2009.09.08 Add serialize() function to LanguageAnalyzer for using it to the MLRUCacue.
///

#ifndef _CONDITION_INFO_
#define _CONDITION_INFO_

#include "SearchingEnumerator.h"
#include <common/sf1_msgpack_serialization_types.h>
#include <common/Utilities.h>

#include <util/izene_serialization.h>
#include <3rdparty/msgpack/msgpack.hpp>

#include <vector>
#include <string>

namespace sf1r {

///
/// @brief This class contains page information of result.
///
class PageInfo
{
public:

    ///
    /// @brief start index of result documents to send
    ///
    unsigned start_;

    ///
    /// @brief document count to send
    ///
    unsigned count_;

    ///
    /// @brief a constructor
    ///
    explicit PageInfo()
        : start_(0), count_(0)
    {}

    ///
    /// @brief clear member variables
    ///
    void clear()
    {
        start_ = 0;
        count_ = 0;
    }

    unsigned topKStart(unsigned topKNum, bool topKFromConfig = false) const
    {
        return topKFromConfig ? Utilities::roundDown(start_, topKNum) : 0;
    }

    DATA_IO_LOAD_SAVE(PageInfo, &start_&count_);
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & start_;
        ar & count_;
    }

    MSGPACK_DEFINE(start_,count_);
};

inline bool operator==(const PageInfo& a, const PageInfo& b)
{
    return a.start_ == b.start_ && a.count_ == b.count_;
}

///
/// @brief structure which includes conditions related with language analyzer
///
class LanguageAnalyzerInfo
{
public:
    ///
    /// @brief a flag if appling LA.
    ///
    bool applyLA_;

    ///
    /// @brief a flag if using original keyword.
    ///
    bool useOriginalKeyword_;

    ///
    /// @brief a flag if processing synonym extention while la.
    ///
    bool synonymExtension_;

    /// @brief a constructor
    LanguageAnalyzerInfo(void);

    /// @brief clear member variables
    void clear(void);

    DATA_IO_LOAD_SAVE(LanguageAnalyzerInfo, &applyLA_&useOriginalKeyword_&synonymExtension_)

    MSGPACK_DEFINE(applyLA_,useOriginalKeyword_,synonymExtension_);

private:
    // Log : 2009.09.08
    // ---------------------------------------
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & applyLA_;
        ar & useOriginalKeyword_;
        ar & synonymExtension_;
    }
};

inline bool operator==(const LanguageAnalyzerInfo& a,
                       const LanguageAnalyzerInfo& b)
{
    return a.applyLA_ == b.applyLA_
        && a.useOriginalKeyword_ == b.useOriginalKeyword_
        && a.synonymExtension_ == b.synonymExtension_;
}

///
/// @brief structure SearchingModeInfo including search mode type and corresponding threshold
///
class SearchingModeInfo
{
public:
    ///
    /// @brief search mode type.
    ///
    SearchingMode::SearchingModeType mode_;

    float threshold_;

    uint32_t lucky_;

    bool useOriginalQuery_;

    bool usefuzzy_;

    SearchingMode::SuffixMatchFilterMode filtermode_;

    bool useQueryFrune_;

    /// @brief a constructor
    SearchingModeInfo(void);

    /// @brief clear member variables
    void clear(void);

    DATA_IO_LOAD_SAVE(SearchingModeInfo, & mode_ & threshold_ & lucky_ & useOriginalQuery_ & usefuzzy_ & filtermode_ & useQueryFrune_)

    MSGPACK_DEFINE(mode_, threshold_, lucky_, useOriginalQuery_, usefuzzy_, filtermode_, useQueryFrune_);

private:
    // Log : 2009.09.08
    // ---------------------------------------
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & mode_;
        ar & threshold_;
        ar & lucky_;
        ar & useOriginalQuery_;
        ar & usefuzzy_;
        ar & filtermode_;
        ar & useQueryFrune_;
    }
};

inline bool operator==(const SearchingModeInfo& a,
                       const SearchingModeInfo& b)
{
    return a.mode_ == b.mode_
        && a.threshold_ == b.threshold_
        && a.lucky_ == b.lucky_
        && a.useOriginalQuery_ == b.useOriginalQuery_
        && a.usefuzzy_ == b.usefuzzy_ 
        && a.filtermode_ == b.filtermode_ 
        && a.useQueryFrune_ == b.useQueryFrune_ ;
}

} // end - namespace sf1r
#endif // _CONDITION_INFO_

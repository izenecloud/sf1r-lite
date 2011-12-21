///
/// @file PropertyExpr.h
/// @brief header file of PropertyExpr
/// @author Dohyun Yun 
/// @date 2008-12-10
///

#if !defined(_PROPERTY_TERM_INFO_)
#define _PROPERTY_TERM_INFO_

#include "type_defs.h"

#include <util/ustring/UString.h>

#include <util/izene_serialization.h>

#include <map>
#include <list>
#include <string>
#include <vector>


namespace sf1r {

    ///
    /// @brief This class represents proeprty query information container 
    ///        which contains property query term position and frequency.
    /// @details
    /// This object is built by the method postProcess() in QueryTree
    /// @see QueryTree
    ///
    class PropertyTermInfo
    {
        public:
            typedef std::map<termid_t , std::list<unsigned int> > id_uint_list_map_t;

        public:

            PropertyTermInfo(void);
            ~PropertyTermInfo(void);

            PropertyTermInfo& operator=(const PropertyTermInfo& obj);

            ///
            /// @brief Sets target search property name.
            /// @param[searchProperty] search property name
            ///
            void setSearchProperty(const std::string& searchProperty);

            ///
            /// @brief return search property name of current property term info.
            /// @return string object of search property name
            ///
            const std::string& getSearchProperty(void) const;

            ///
            /// @brief processes term position, term frequency of given term
            /// @param[keyword] one query term.
            /// @param[termId]  term id of given keyword.
            /// @param[nPos]    position of keyword
            ///
            void dealWithTerm(izenelib::util::UString& keyword, termid_t termId, unsigned int nPos);

            ///
            /// @brief returns term id position map.
            ///
            const id_uint_list_map_t&   getTermIdPositionMap() const    { return termIdPositionMap_;  }

            ///
            /// @brief returns term id frequency map.
            ///
            const ID_FREQ_MAP_T& getTermIdFrequencyMap() const   { return termIdFrequencyMap_; }

            ///
            /// @brief returns string of current property term information.
            ///
            std::string toString(void) const;

            ///
            /// @brief serves position ordered term id list.
            /// @param[termIdList] vector of termid_t which is ordered by position.
            ///
            void getPositionedTermIdList(std::vector<termid_t>& termIdList) const;

            ///
            /// @brief serves position ordered term string list.
            /// @param[termStringList] vector of ustring which is ordered by position.
            ///
            void getPositionedTermStringList(std::vector<izenelib::util::UString>& termStringList) const;

            ///
            /// @brief serves one string object which is ordered by position.
            /// @param[termString] is a position ordered ustring object.
            ///
            void getPositionedFullTermString(izenelib::util::UString& termString) const;

            ///
            /// @brief sets wildcard flag of current property term info class.
            ///
            void setWildCard()      { isWildCardQuery_ = true; }

            ///
            /// @brief clear member variables
            ///
            void clear(void);

        private:
            ///
            /// @brief contains target search property.
            ///
            std::string searchProperty_;

            ///
            /// @brief contains term position info.
            ///
            id_uint_list_map_t termIdPositionMap_;

            ///
            /// @brief contains term frequency info.
            ///
            ID_FREQ_MAP_T termIdFrequencyMap_;

            ///
            /// @brief contains term id - term string map.
            ///
            std::map<termid_t, izenelib::util::UString> termIdKeywordMap_;

            ///
            /// @brief determines if current property term info contains wildcard query.
            ///
            bool isWildCardQuery_;

        private:
            // friend class boost::serialization::access;
            // template<class Archive>
            // void serialize( Archive & ar, const unsigned int version )
            // {
            //     ar & searchProperty_;
            //     ar & termIdPositionMap_;
            //     ar & termIdFrequencyMap_;
            //     ar & isWildCardQuery_;
            // }
            DATA_IO_LOAD_SAVE(PropertyTermInfo, &searchProperty_&termIdPositionMap_&termIdFrequencyMap_&isWildCardQuery_)

    }; // end - class PropertyTermInfo

} // end - namespace sf1r

#endif //_PROPERTY_TERM_INFO_

/** 
 * @file LAConfigUnit.h
 * @brief Defines and implements LAConfigUnit class
 * @author MyungHyun (Kent)
 * @date 2008-06-05
 */


#if !defined(_LA_CONFIG_UNIT_H_)
#define _LA_CONFIG_UNIT_H_

#include <boost/serialization/access.hpp>
#include <boost/serialization/map.hpp>

#include <string>
#include <sstream>
#include <map>


namespace sf1r
{

    /**
     * @brief LAUnit represents a single language analyzing setting("<Method>") from the configuration file. 
     */
    class LAConfigUnit {

        public:
            //------------------------  PUBLIC MEMBER FUNCTIONS ------------------------

            LAConfigUnit();

            /**
             * @brief Sets the member variables
             *
             * @param id            The identifier of this unit.
             * @param analysis      The language analyzing method used by this unit
             */
            LAConfigUnit( const std::string & id, const std::string & analysis );


            /**
            */
            ~LAConfigUnit(){}


            /**
             * Reset all the setting to initial state
             */
            void clear();


            /**
             * @brief       Sets the id of this unit
             *
             * @param id    The identifier of this unit.
             */
            void setId( const std::string & id ) { id_ = id; }

            /**
             * @brief   Gets the id of this unit
             *
             * @return  The ID of this unit
             */
            std::string getId() const { return id_; }



            /**
             * @brief           Sets the analysis method of this unit
             * @param analysis  The language analyzing method used by this unit
             */
            void setAnalysis( const std::string & analysis ) { analysis_ = analysis; }

            /**
             * @brief   Gets the analysis method of this unit
             *
             * @return  The analyzing method of this unit
             */
            std::string getAnalysis() const { return analysis_; }



            /**
             * @brief           
             * @param analysis  
             */
            void addReferenceMethod( const LAConfigUnit & method );

            /**
             * @brief   
             *
             * @return  
             */
            bool getMethodIdByLang( const std::string & language, std::string & method_id  ) const;



            /**
             * @brief           Set the setting for the level of the ngram. 
             *
             * @param level     The level of ngram to apply
             */
            void setLevel(const unsigned int level) 
            { 
                level_ = level; 
            }

            /**
             * @brief   Get the setting for the level of the ngram. 
             *
             * @return  The level of ngram that is assigned to this unit
             */
            unsigned int getLevel() const 
            { 
                return level_; 
            }



            /**
             * @brief       Returns whether Koreans, English, and Number words should be analyzed separately
             *
             * @param flag  True of false
             */
            void setApart(const bool flag) 
            { 
                apart_ = flag; 
            }

            /**
             * @brief  Returns whether Koreans, English, and Number should be considered separately
             *
             * @return The "apart" setting of this unit
             */
            bool getApart() const 
            { 
                return apart_; 
            }



            /**
             * @brief           Set the minimum level of ngram to apply
             *
             * @param level     The level of the minimum ngram
             */
            void setMinLevel( const unsigned int level ) 
            { 
                minLevel_ = level; 
            }

            /**
             * @brief   Get the minimum level of ngram to apply
             *
             * @return  The minimum level of ngram that this unit uses
             */
            unsigned int getMinLevel() const 
            { 
                return minLevel_; 
            }



            /**
             * @brief           Set the maximum level of ngram to apply
             *
             * @param level     The level of the maximum ngram
             */
            void setMaxLevel( const unsigned int level ) 
            { 
                maxLevel_ = level; 
            }

            /**
             * @brief   Get the maximum level of ngram to apply
             *
             * @return  The maximum level of ngram that this unit uses
             */
            unsigned int getMaxLevel() const 
            {   
                return maxLevel_; 
            }



            /**
             * @brief           Set the maximum number of tokens to extract by applying the "like" analyzing method.
             *
             * @param number    The maximum number of tokens to generate 
             */
            void setMaxNo( const unsigned int number ) 
            { 
                maxNo_ = number; 
            }

            /**
             * @brief   Get the maximum number of tokens to extract by applying the "like" method.
             *
             * @return  The maximum number of tokens to generate 
             */
            unsigned int getMaxNo() const 
            { 
                return maxNo_; 
            }

            /**
             * @brief       Returns whether Koreans, English, and Number words should be analyzed separately
             *
             * @param flag  True of false
             */
            void setPrefix(const bool flag) 
            { 
                prefix_ = flag; 
            }

            /**
             * @brief  Returns whether Koreans, English, and Number should be considered separately
             *
             * @return The "apart" setting of this unit
             */
            bool getPrefix() const 
            { 
                return prefix_;
            }

            /**
             * @brief       Returns whether Koreans, English, and Number words should be analyzed separately
             *
             * @param flag  True of false
             */
            void setSuffix(const bool flag) 
            { 
                suffix_ = flag; 
            }

            /**
             * @brief  Returns whether Koreans, English, and Number should be considered separately
             *
             * @return The "apart" setting of this unit
             */
            bool getSuffix() const 
            { 
                return suffix_; 
            }



            /**
             * @brief           Set the language of this unit 
             *
             * @param language  The language name 
             */
            void setLanguage( const std::string & language ) 
            { 
                language_ = language; 
            }

            /**
             * @brief   Get the language of this unit 
             *
             * @return  The language name 
             */
            std::string getLanguage() const 
            { 
                return language_; 
            }

            void setMode( const std::string & mode ) 
            { 
                mode_ = mode;
            }

            std::string getMode() const 
            { 
                return mode_;
            }



            /**
             * @brief           Set the option that this language analyzer unit refers to 
             *
             * @param option    The option string 
             */
            void setOption( const std::string & option ) 
            { 
                option_ = option; 
            }

            /**
             * @brief   Get the option that this language analyzer unit refers to 
             *
             * @return  The option string 
             */
            std::string getOption() const 
            { 
                return option_; 
            }



            /**
             * @brief               Set special character value used in this language analyzer unit
             *
             * @param specialChar   The special character string
             */
            void setSpecialChar( const std::string & specialChar ) 
            { 
                specialChar_ = specialChar ; 
            }

            /**
             * @brief   Get special character value used in this language analyzer unit
             *
             * @return  The special character string
             */
            std::string getSpecialChar() const  
            { 
                return specialChar_; 
            }



            /**
             * @brief                   Set a dictionary that will be used by this unit
             *
             * @param dictionaryPath    The dictionary path
             */
            void setDictionaryPath( const std::string & dictionaryPath ) 
            { 
                dictionaryPath_ = dictionaryPath; 
            }

            /**
             * @brief   Get the dictionary used for this Language Analyze setting.
             *
             * @return  The dictionary path
             */
            std::string getDictionaryPath() const  
            { 
                return dictionaryPath_; 
            }

            /**
             * @brief       Returns whether to be case sensitive
             *
             * @param flag  True of false
             */
            void setCaseSensitive(const bool flag)
            {
                caseSensitive_ = flag;
            }

            /**
             * @brief  Returns whether to be case sensitive
             *
             * @return The "caseSensitive" setting of this unit
             */
            bool getCaseSensitive() const
            {
                return caseSensitive_;
            }


            /**
             * @brief              Set the advanced option that will be used by this unit
             *
             * @param advOption    The advanced option
             */
            void setAdvOption( const std::string & advOption )
            {
                advOption_ = advOption;
            }

            /**
             * @brief   Get the advanced option used for this Language Analyze setting.
             *
             * @return  The advanced option
             */
            std::string getAdvOption() const
            {
                return advOption_;
            }

            /**
             * @brief            Set the index return flag that will be used by this unit
             *
             * @param idxFlag    The index return flag
             */
            void setIdxFlag( unsigned char idxFlag )
            {
                idxFlag_ = idxFlag;
            }

            /**
             * @brief   Get the index return flag used for this Language Analyze setting.
             *
             * @return  The index return flag
             */
            unsigned char getIdxFlag() const
            {
                return idxFlag_;
            }

            /**
             * @brief            Set the search return flag that will be used by this unit
             *
             * @param schFlag    The search return flag
             */
            void setSchFlag( unsigned char schFlag )
            {
                schFlag_ = schFlag;
            }

            /**
             * @brief   Get the search return flag used for this Language Analyze setting.
             *
             * @return  The search return flag
             */
            unsigned char getSchFlag() const
            {
                return schFlag_;
            }


            /**
             * @brief       Set whether contain English stemming word
             *
             * @param flag  True of false
             */
            void setStem(const bool flag)
            {
                stem_ = flag;
            }

            /**
             * @brief  Returns whether contain English stemming word
             *
             * @return The "stem" setting of this unit
             */
            bool getStem() const
            {
                return stem_;
            }

            /**
             * @brief Whether contain lower form of English
             * @param flag True of false
             */
            void setLower( bool flag )
            {
                lower_ = flag;
            }

            /**
             * @brief get whether the Analyzer is case sensitive
             * @return the "lower_" setting of this unit
             */
            bool getLower() const
            {
                return lower_;
            }


            /**
             * @brief   Puts the configuration into string
             */
            std::string toString()
            {
                std::stringstream ss;
                ss << *this;
                return ss.str();
            }



            /**
             * @brief
             */
            friend std::ostream & operator<<( std::ostream & out, const LAConfigUnit & unit );



        private:
            //------------------------  PRIVATE MEMBER FUNCTIONS  ------------------------

            friend class boost::serialization::access;

            template <class Archive>
                void serialize( Archive & ar, const unsigned int version ) 
                {
                    ar & id_;
                    ar & analysis_;
                    ar & langRefMap_;
                    ar & level_;
                    ar & apart_;
                    ar & minLevel_;
                    ar & maxLevel_;
                    ar & maxNo_;
                    ar & prefix_;
                    ar & suffix_;
                    ar & language_;
                    ar & mode_;
                    ar & option_;
                    ar & specialChar_;
                    ar & dictionaryPath_;
                    ar & caseSensitive_;
                    ar & advOption_;
                    ar & idxFlag_;
                    ar & schFlag_;
                    ar & stem_;
                    ar & lower_;
                }


        private:
            //------------------------  PRIVATE MEMBER VARIABLES ------------------------


            /// @brief  The identifier of this unit. Used to identify a unit in property setting of documents
            std::string id_;

            /// @brief  The language analyzing method used by this unit
            std::string analysis_;

            //-- AUTO --
            /**
             * @brief  
             * @details    
             * maps <language> to <method id>. 
             * Only one method id for one language is allowed to be registered as reference to "auto" analysis
             */
            std::map<std::string, std::string> langRefMap_;


            //-- TOKEN --
            /// @brief  Level of ngram that this analyzer is to use (optional)
            unsigned int level_;

            /// @brief  Whether Korean, English, and Number are analyzed separately (optional)
            bool apart_;


            //-- LIKE --
            /// @brief The minimum level of ngram to apply (optional)
            int minLevel_;

            /// @brief The maximum level of ngram to apply (optional)
            int maxLevel_;

            /// @brief Get the maximum number of tokens to extract by applying the analyzer (optional)
            int maxNo_;


            /// @brief Matrix analysis: Starts from the beginning
            bool prefix_;

            /// @brief Matrix analysis: Starts from the end
            bool suffix_;

            //-- ALL & NOUN --
            /// @brief  The language of this unit  (optional)
            std::string language_;

            std::string mode_;

            /// @brief  The analyzer option of this unit (optional)
            std::string option_;

            /// @brief  The special character used in this unit (optional)
            std::string specialChar_;

            /// @brief  The path to a additional dictionary used by this unit (optional)
            std::string dictionaryPath_;

            /// @brief Whether to be case-sensitive ( process Upper and Lower
            /// cases separately )
            bool caseSensitive_;

            /// @brief Advanced Options for some Analyzers
            std::string advOption_;

            /// @brief index return flag of Indexing
            unsigned char idxFlag_;

            /// @brief search return flag of Searching
            unsigned char schFlag_;

            /// @brief whether include stemming word
            bool stem_;

            /// @brief Whether contain lower form of English
            bool lower_;

    };

} // namespace

#endif  //_LA_CONFIG_UNIT_H_

// eof

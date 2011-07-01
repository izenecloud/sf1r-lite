#include <common/SFLogger.h>
#include "LAPool.h"
#include "AnalysisInformation.h"

#include <boost/tokenizer.hpp>

using namespace std;
using namespace la;
using namespace boost;
using namespace izenelib::util;
using namespace izenelib::util;

typedef boost::tokenizer <boost::char_separator<char>,
    string::const_iterator, string> LAStringTokenizer;

const boost::char_separator<char> COMMA_SEP(",");
const boost::char_separator<char> SEMICOLON_SEP(";");

void poolsize( const unordered_map<sf1r::AnalysisInfo, deque<LA*> > & pool )
{
    unordered_map<sf1r::AnalysisInfo, deque<LA*> >::const_iterator it;

    for( it = pool.begin(); it != pool.end(); it++ )
    {
        cout << it->first.analyzerId_ << ":" << it->second.size() << endl;
    }
}


void setOptions( const std::string & option, EnglishAnalyzer * ka, bool outputLog = true )
{
    const char* o = option.c_str();
    set<char> hisSet;
    while (*o)
    {
        char origChar = *o;
        char upperType = toupper( origChar );

        bool checkDupl = true;
        bool errorVal = false;
        switch (*o)
        {
            case 'S': case 's':
                ++o;
                if (*o == '+')
                {
                    ka->setExtractEngStem( true );
                }
                else if (*o == '-')
                {
                    ka->setExtractEngStem( false );
                }
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'S': %c", *o);
                    errorVal = true;
                }
                break;
            case ' ':
                checkDupl = false;
                break;

            default:
                if( outputLog )
                    sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA Option \"%c\"", *o );
                checkDupl = false;
                break;
        }
        if( checkDupl )
        {
            if( hisSet.find( upperType ) != hisSet.end() )
            {
                if( !errorVal && outputLog )
                    sflog->warn( SFL_LA, "SF-070104: Duplicated LanguageLA option \"%c\", it would overwrite the previous setting.", origChar );
            }
            else
                hisSet.insert( upperType );
        }

        if( *o == 0 )
            break;
        o++;
    }
}

void setOptions( const std::string & option, ChineseAnalyzer * ka, bool outputLog = true )
{
    const char* o = option.c_str();
    set<char> hisSet;
    while (*o)
    {
        char origChar = *o;
        char upperType = toupper( origChar );

        bool checkDupl = true;
        bool errorVal = false;
        switch (*o)
        {
            case 'R': case 'r':
                ++o;
                if (*o == '+')
                {
                    ka->setNBest( 2 );
                }
                else if (*o == '0' || *o == '-')
                {
                    ka->setNBest( 0 );
                }
                else if ((*o >= '1') && (*o <= '9'))
                {
                    ka->setNBest( atoi(o) );
                }
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'R': %c", *o);
                    errorVal = true;
                }
                break;
            case 'S': case 's':
                ++o;
                if (*o == '+')
                {
                    ka->setExtractEngStem( true );
                }
                else if (*o == '-')
                {
                    ka->setExtractEngStem( false );
                }
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'S': %c", *o);
                    errorVal = true;
                }
                break;
            case 'T': case 't': // ignore analysis type here
                ++o;
                if ((*o >= '1') && (*o <= '9'))
                    ka->setAnalysisType( (ChineseAnalyzer::ChineseAnalysisType)atoi(o) );
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'T': %c", *o);
                    errorVal = true;
                }
                break;

            case ' ':
                checkDupl = false;
                break;

            default:
                if( outputLog )
                    sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA Option \"%c\"", *o );
                checkDupl = false;
                break;
        }
        if( checkDupl )
        {
            if( hisSet.find( upperType ) != hisSet.end() )
            {
                if( !errorVal && outputLog )
                    sflog->warn( SFL_LA, "SF-070104: Duplicated LanguageLA option \"%c\", it would overwrite the previous setting.", origChar );
            }
            else
                hisSet.insert( upperType );
        }

        if( *o == 0 )
            break;
        o++;
    }
}

void setOptions( const std::string & option, JapaneseAnalyzer * ka, bool outputLog = true )
{
    const char* o = option.c_str();
    set<char> hisSet;
    while (*o)
    {
        char origChar = *o;
        char upperType = toupper( origChar );

        bool checkDupl = true;
        bool errorVal = false;
        switch (*o)
        {
            case 'R': case 'r':
                ++o;
                if (*o == '+')
                {
                    ka->setNBest( 2 );
                }
                else if (*o == '0' || *o == '-')
                {
                    ka->setNBest( 0 );
                }
                else if ((*o >= '1') && (*o <= '9'))
                {
                    ka->setNBest( atoi(o) );
                }
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'R': %c", *o);
                    errorVal = true;
                }
                break;
            case 'S': case 's':
                ++o;
                if (*o == '+')
                {
                    ka->setExtractEngStem( true );
                }
                else if (*o == '-')
                {
                    ka->setExtractEngStem( false );
                }
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'S': %c", *o);
                    errorVal = true;
                }
                break;
            case ' ':
                checkDupl = false;
                break;

            default:
                if( outputLog )
                    sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA Option \"%c\"", *o );
                checkDupl = false;
                break;
        }
        if( checkDupl )
        {
            if( hisSet.find( upperType ) != hisSet.end() )
            {
                if( !errorVal && outputLog )
                    sflog->warn( SFL_LA, "SF-070104: Duplicated LanguageLA option \"%c\", it would overwrite the previous setting.", origChar );
            }
            else
                hisSet.insert( upperType );
        }

        if( *o == 0 )
            break;
        o++;
    }
}

// std::map<char, char> parse_lasetting_options(const std::string& option)
// {
//   std::map<char, char> result;
//   result['R'] = "";
//   result['S'] = "";
//   result['T'] = "";
//   const char* o = option.c_str();
//   set<char> hisSet;
//   while (*o)
//   {
//       char origChar = *o;
//       char upperType = toupper( origChar );
//
//       bool checkDupl = true;
//       bool errorVal = false;
//       switch (upperType)
//       {
//           case 'R':
//             ++o;
//             result['R'] = *o;
//             break;
//           case 'S':
//             ++o;
//             result['S'] = *o;
//             break;
//           case 'T':
//             ++o;
//             result['T'] = *o;
//             break;
//
//           default:
//               if( outputLog )
//                   sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA Option \"%c\"", *o );
//               break;
//       }
//       if( *o == 0 )
//           break;
//       o++;
//   }
//   return result;
// }

void setOptions( const std::string & option, KoreanAnalyzer * ka, bool outputLog = true )
{
    const char* o = option.c_str();
    set<char> hisSet;
    while (*o)
    {
        char origChar = *o;
        char upperType = toupper( origChar );

        bool checkDupl = true;
        bool errorVal = false;
        switch (*o)
        {
            case 'R': case 'r':
                ++o;
                if (*o == '+')
                {
                    ka->setNBest( 2 );
                }
                else if (*o == '0' || *o == '-')
                {
                    ka->setNBest( 0 );
                }
                else if (*o == '1')
                {
                  ka->setNBest( 1 );
                }
                else if ((*o >= '2') && (*o <= '9'))
                {
                    ka->setNBest( atoi(o) );
                }
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'R': %c", *o);
                    errorVal = true;
                }
                break;
            case 'N': case 'n':
                ++o;
                if (*o == '0')
                {
                    ka->setLowDigitBound( 0 );
                }
                else if ((*o >= '1') && (*o <= '9'))
                {
                    ka->setLowDigitBound( atoi(o) );
                }
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'N': %c", *o);
                    errorVal = true;
                }
                break;
            case 'B': case 'b':
                ++o;
                if (*o == '+')
                {
                    ka->setCombineBoundNoun( true );
                }
                else if (*o == '-')
                {
                    ka->setCombineBoundNoun( false );
                }
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'B': %c", *o);
                    errorVal = true;
                }
                break;

            case 'V': case 'v':
                ++o;
                if (*o == '+')
                {
                    ka->setVerbAdjStems( true );
                }
                else if (*o == '-')
                {
                    ka->setVerbAdjStems( false );
                }
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'V': %c", *o);
                    errorVal = true;
                }
                break;
            case 'S': case 's':
                ++o;
                if (*o == '+')
                {
                    ka->setExtractEngStem( true );
                }
                else if (*o == '-')
                {
                    ka->setExtractEngStem( false );
                }
                else
                {
                    if( outputLog )
                        sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA option value for Option 'S': %c", *o);
                    errorVal = true;
                }
                break;
            case ' ':
                checkDupl = false;
                break;
            default:
                if( outputLog )
                    sflog->warn(SFL_LA, "SF-070105: Invalid LanguageLA Option \"%c\"", *o );
                checkDupl = false;
                break;
        }
        if( checkDupl )
        {
            if( hisSet.find( upperType ) != hisSet.end() )
            {
                if( !errorVal && outputLog )
                    sflog->warn( SFL_LA, "SF-070104: Duplicated LanguageLA option \"%c\", it would overwrite the previous setting.", origChar );
            }
            else
                hisSet.insert( upperType );
        }

        if( *o == 0 )
            break;
        o++;
    }

} // end - setOptions( const std::string & option, LanguageAnalyzer * ka )


#ifdef DEBUG_LAPOOL_ANALYZER

#define ANALYSIS_ID_TO_TEST "la_sia"

std::ostream & printTerm( const la::Term & term, std::ostream & out )
{
    std::string tmp;
    //term.text_.convertString(tmp, SYSTEM_ENCODING );
    term.text_.convertString(tmp, izenelib::util::UString::UTF_8 );

    out << "len=" 		    << (term.text_.length()) 	<< "\t";
    out << "pos=" 		    << term.pos_ 		            << "\t";
    out << "morpheme=" 		<< bitset<32>(term.morpheme_) 	<< "\t";
    out << "woffset=" 	    << term.wordOffset_			    << "\t";
    out << "\ttext=[" 		<< tmp			                << "]";
    out << std::endl;
    return out;
}
std::ostream & printTermList( const la::TermList & termList, std::ostream & out )
{
    la::TermList::const_iterator it;
    for( it = termList.begin(); it != termList.end(); it++ )
    {
        printTerm( *it, out );
    }
    return out;
}
#endif

namespace sf1r
{

    typedef std::vector<AnalysisInfo>                   AnalysisInfoVec;
    typedef std::map<std::string, LAConfigUnit>         LaConfigUnitMap;
    typedef std::map<std::string, TokenizerConfigUnit>  TokenizerConfigMap;

    class TokCharUnit{
    public:
        enum TokType{
            Divide,
            Unite,
            Allow,
            Num
        };


        TokCharUnit()
        {
            for( int i = 0; i < Num; ++i )
                types_[i] = false;
        }

        /**
         * Only contains one type
         */
        bool isSingleType()
        {
            bool init = false;
            for( int i = 0; i < Num; ++i )
            {
                if( types_[i] )
                {
                    if( init )
                        return false;
                    init = true;
                }
            }

            return true;
        }

        TokType getBestType()
        {
            if( types_[Allow] )
                return Allow;
            else if( types_[Unite] )
                return Unite;
            else if( types_[Divide] )
                return Divide;
            return Num;
        }

        bool setType( TokType type )
        {
            if( type == Num )
                return false;
            types_[ type ] = true;
            return true;
        }

        void display( stringstream& stream )
        {
            vector<string> typesName;
            // the if-statements' order can't be changed
            if( types_[Divide] )
                typesName.push_back("divide");
            if( types_[Unite] )
                typesName.push_back("unite");
            if( types_[Allow] )
                typesName.push_back("allow");
            int num = static_cast<int>(typesName.size());
            if( num == 0 )
                return;
            stream << "contains (";
            for( int i = 0; i < (num - 1); ++i )
            {
                stream << typesName[i] << ", ";
            }
            stream << typesName[num-1] << "), uses \"" <<typesName[num-1]<<"\"";
        }

    private:
        bool types_[Num];
    };


    LAPool::LAPool()
    {
    }

    LAPool::~LAPool()
    {
        unordered_map<AnalysisInfo, deque<LA*> >::iterator it;
        deque<LA*>::iterator la_it;

        for( it = laSearchPool_.begin(); it != laSearchPool_.end(); it++ )
        {
            for( la_it = it->second.begin(); la_it != it->second.end(); la_it++ )
            {
                if( *la_it != NULL )
                    delete *la_it;
            }
        }

        boost::unordered_map<AnalysisInfo, la::LA*>::iterator it2;

        for( it2 = laIndexMap_.begin(); it2 != laIndexMap_.end(); it2++ )
        {
            delete it2->second;
        }
    }

    bool LAPool::init(const sf1r::LAManagerConfig & laManagerConfig)
    {
        //bool useCache = laManagerConfig.getUseCache();
#ifdef USE_MF_LIGHT
         ScopedWriteLock<ReadWriteLock> lock (lock_ );
#endif

        // Get LAConfigMap
        laManagerConfig.getLAConfigMap( laConfigUnitMap_ );
        laManagerConfig.getTokenizerConfigMap( tokenizerConfigUnitMap_ );

        if(laConfigUnitMap_.size() < 1)
        {
            sflog->warn(SFL_LA, "SF-070101: The size of config unit mapt is zero\n");
            //cout << "The size of config unit Map is 0" << endl;
            return false;
        }

        AnalysisInfoVec analysisInfoVec;
        laManagerConfig.getAnalysisPairList( analysisInfoVec );
        deque<LA*> laQueue;
        LA * pLA = NULL;


        // 1. create default LA which performs only Tokenizing

        AnalysisInfo dummy;

        if( laSearchPool_.find(dummy) == laSearchPool_.end() )
        {
            for( unsigned int i = 0; i < LA_THREAD_NUM; i++ )
            {
                if( (pLA = createDefaultLA()) == NULL )
                {
                    return false;
                }
                laQueue.push_back( pLA );
            }
            laSearchPool_.insert( make_pair(dummy, laQueue) );
        }

        if( laIndexMap_.find(dummy) == laIndexMap_.end() )
        {
            if( (pLA = createDefaultLA()) == NULL )
            {
                return false;
            }
            laIndexMap_.insert( make_pair(dummy, pLA) );
        }


        // 2. create LAs

        AnalysisInfoVec::const_iterator it;
        for( it = analysisInfoVec.begin(); it != analysisInfoVec.end(); it++)
        {
            laQueue.clear();

            if( laSearchPool_.find(*it) == laSearchPool_.end() )
            {
                // create LA for searching
                for( unsigned int i = 0; i < LA_THREAD_NUM; i++ )
                {
                    //only output log for first searching LA
                    bool outputLog = (i == 0);
                    if( (pLA = createLA(*it, outputLog, false)) == NULL )
                    {
                        return false;
                    }
                    laQueue.push_back( pLA );
                }
                laSearchPool_.insert( make_pair(*it, laQueue) );
            }

            if( laIndexMap_.find(*it) == laIndexMap_.end() )
            {
                //always false/don't output log for indexing LA
                if( (pLA = createLA(*it, false, true )) == NULL )
                {
                    return false;
                }
                laIndexMap_.insert( make_pair(*it, pLA) );
            }
        }

        return true;
    } // end - init()


    LA * LAPool::createDefaultLA()
    {
        LA * la = new LA();
        la->setAnalyzer(boost::shared_ptr<Analyzer>(new TokenAnalyzer()));
        return la;
    } // end - createDefaultLA()

    /// @param mode true is indexing, false is searching
    LA * LAPool::createLA( const AnalysisInfo & analysisInfo, bool outputLog, bool mode )
    {
        la::TokenizeConfig tokenConfig;        //tokenizer config

        // get the tokenze configuration and set them
        for( set<string>::const_iterator tokenizerNameIter = analysisInfo.tokenizerNameList_.begin();
                tokenizerNameIter != analysisInfo.tokenizerNameList_.end();
                tokenizerNameIter++ )
        {
            TokenizerConfigMap::const_iterator tokenizerConfigIter = tokenizerConfigUnitMap_.find( *tokenizerNameIter );
            if ( tokenizerConfigIter == tokenizerConfigUnitMap_.end() )
            {
                if( outputLog )
                {
                    sflog->warn( SFL_LA, "SF-070101: Cannot find tokenizer id: %s.",
                            tokenizerNameIter->c_str() );
                }
                return NULL;
            }

            string methodName = tokenizerConfigIter->second.method_;
            UString tokChars;
            if( methodName == "divide" || methodName == "unite" || methodName == "allow" )
            {
                UString tmpTokchars = tokenizerConfigIter->second.getChars();
                if( tmpTokchars.empty() )
                {
                    if( outputLog )
                    {
                        sflog->warn( SFL_LA, "SF-070101: Can't gain any valid character from \"value\" and \"code\" of tokenizer \"%s\".",
                                tokenizerNameIter->c_str() );
                    }
                    continue;
                }

                // remove the space character
                bool containSpace = false;

                for( size_t i = 0; i < tmpTokchars.length(); ++i )
                {
                    if( tmpTokchars.isSpaceChar( i ) )
                        containSpace = true;
                    else
                        tokChars += tmpTokchars[i];
                }

                if( containSpace )
                {
                    if( outputLog )
                        sflog->warn( SFL_LA, "SF-070101: Tokenizer \"%s\" contains space characters.", tokenizerNameIter->c_str() );
                }
            }


            if ( methodName == "divide" )
                tokenConfig.addDivides( tokChars );
            else if ( methodName == "unite" )
                tokenConfig.addUnites( tokChars );
            else if ( methodName == "allow" )
                tokenConfig.addAllows( tokChars );
            else if ( methodName != "" )
            {
                if( outputLog )
                {
                    sflog->warn( SFL_LA, "SF-070101: Invalid tokenizer method \"%s\" for tokenizer \"%s\".",
                        methodName.c_str(), tokenizerNameIter->c_str() );
                }
                return NULL;
            }
            else
            {
                if( outputLog )
                {
                    sflog->warn( SFL_LA, "SF-070101: Empty tokenizer method for tokenizer \"%s\".",
                        tokenizerNameIter->c_str() );
                }
            }
        } // end - for


        LaConfigUnitMap::const_iterator laConfigUnitIter = laConfigUnitMap_.find( analysisInfo.analyzerId_ );
        if ( laConfigUnitIter == laConfigUnitMap_.end() )
        {
            if( outputLog )
                sflog->error(SFL_LA, "SF-070102: Cannot Find LA analyzer. AnalyzerID %d\n", (analysisInfo.analyzerId_).c_str());
            return NULL;
        }

        const string & analysis  = laConfigUnitIter->second.getAnalysis();
        shared_ptr<la::Analyzer> analyzer;

        LA * la = NULL;
        la = new LA();

        if( analysis == "token" )
        {
            analyzer.reset(new TokenAnalyzer());
        }
        else if(analysis == "char")
        {
            CharAnalyzer* pChAnalyzer = new CharAnalyzer();

            string option = laConfigUnitIter->second.getAdvOption();
            if (option == "part") {
                pChAnalyzer->setSeparateAll(false);
            }
            else {
                pChAnalyzer->setSeparateAll(true);
            }

            pChAnalyzer->setCaseSensitive(laConfigUnitIter->second.getCaseSensitive());

            analyzer.reset(pChAnalyzer);
        }
        else if(analysis == "ngram")
        {
            analyzer.reset(new NGramAnalyzer(  laConfigUnitIter->second.getMinLevel(),
                                        laConfigUnitIter->second.getMaxLevel(),
                                        laConfigUnitIter->second.getMaxNo(),
                                        ((laConfigUnitIter->second.getApart()) ?
                                            NGramAnalyzer::NGRAM_APART_ALL_:NGramAnalyzer::NGRAM_APART_NON_) ) );
        }
        else if (analysis == "matrix")
        {
            analyzer.reset(new MatrixAnalyzer( laConfigUnitIter->second.getPrefix(), laConfigUnitIter->second.getSuffix()) );
        }
        else if( analysis == "english" )
        {
            analyzer.reset(new EnglishAnalyzer() );
            if(mode) {
                static_cast<EnglishAnalyzer*>(analyzer.get())->setCaseSensitive(
                    laConfigUnitIter->second.getCaseSensitive(), laConfigUnitIter->second.getLower());
            } else {
                static_cast<EnglishAnalyzer*>(analyzer.get())->setCaseSensitive(
                    laConfigUnitIter->second.getCaseSensitive(), false);
            }
            setOptions( laConfigUnitIter->second.getOption(), static_cast<EnglishAnalyzer*>(analyzer.get()) );
        }
#ifdef USE_WISEKMA
        else if( analysis == "korean" )
        {
            analyzer.reset( new NKoreanAnalyzer( laConfigUnitIter->second.getDictionaryPath() ) );
            //static_cast<NKoreanAnalyzer*>(analyzer.get())->setGenerateCompNoun( true );


            if( laConfigUnitIter->second.getMode() == "all" )
            {
//                static_cast<NKoreanAnalyzer*>(analyzer.get())->setRetFlag_index( Analyzer::ANALYZE_ALL_ );
            }
            else if( laConfigUnitIter->second.getMode() == "noun" )
            {
//                static_cast<NKoreanAnalyzer*>(analyzer.get())->setRetFlag_index( Analyzer::ANALYZE_SECOND_ );
            }
            else if( laConfigUnitIter->second.getMode() == "label" )
            {
                static_cast<NKoreanAnalyzer*>(analyzer.get())->setLabelMode();
//                static_cast<NKoreanAnalyzer*>(analyzer.get())->setRetFlag_index( Analyzer::ANALYZE_ALL_ );
            }

            if(mode) {
                static_cast<NChineseAnalyzer*>(analyzer.get())->setCaseSensitive(
                    laConfigUnitIter->second.getCaseSensitive(), laConfigUnitIter->second.getLower());
            } else {
                static_cast<NChineseAnalyzer*>(analyzer.get())->setCaseSensitive(
                    laConfigUnitIter->second.getCaseSensitive(), false);
            }

            setOptions( laConfigUnitIter->second.getOption(), static_cast<NKoreanAnalyzer*>(analyzer.get()) );

            if(mode) {
                static_cast<NKoreanAnalyzer*>(analyzer.get())->setAnalyzePrime(true);
            } else {
                static_cast<NKoreanAnalyzer*>(analyzer.get())->setAnalyzePrime(false);
                static_cast<NKoreanAnalyzer*>(analyzer.get())->setNBest(1);
            }

            //check the special char here, only accept 0x0000 to 0x007F in ucs encoding
            UString::EncodingType defEncoding = UString::UTF_8;
            UString origSpeU( laConfigUnitIter->second.getSpecialChar().c_str(), defEncoding );
            string speU; // after remove invalid characters
            for( size_t i = 0; i < origSpeU.length(); ++i )
            {
                unsigned int c = static_cast<unsigned int>( origSpeU.at(i) );
                if( c > 0x007F )
                {
                    string invalidStr;
                    origSpeU.substr( i, 1 ).convertString( invalidStr, defEncoding );
                    if( outputLog )
                    {
                        sflog->warn( SFL_LA, "SF-070105: Invalid special char \"%s\" for analyzer \"%s\" (accept 0x0000 to 0x007F).",
                                invalidStr.c_str(), analysisInfo.analyzerId_.c_str() );
                    }
                }
                else
                    speU.append( 1, static_cast<char>(c) );
            }

            // need to adjust tokenizer settings if there is "specialchar" settings
            tokenConfig.addAllows( speU );
//            static_cast<NKoreanAnalyzer*>(analyzer.get())->setSpecialChars( speU );
        }
#endif

#ifdef USE_IZENECMA
        else if( analysis == "chinese" )
        {
            //check whether loadModel
            const char* o = laConfigUnitIter->second.getOption().c_str();
            unsigned int type = 1;
            while (*o)
            {
                switch( *o )
                {
                    case 'T': case 't':
                        ++o;
                        if ((*o >= '1') && (*o <= '9'))
                            type = static_cast<unsigned int>( atoi(o) );
                        break;
                    case ' ':
                        break;

                    default: //ignore other option
                        ++o;
                        break;
                }
                o++;
            }
            bool loadModel = ( type == 1 );

            analyzer.reset( new NChineseAnalyzer( laConfigUnitIter->second.getDictionaryPath(), loadModel ) );

            if( laConfigUnitIter->second.getMode() == "all" )
            {
                static_cast<NChineseAnalyzer*>(analyzer.get())->setIndexMode();
            }
            else if( laConfigUnitIter->second.getMode() == "noun" )
            {
                static_cast<NChineseAnalyzer*>(analyzer.get())->setIndexMode();
            }
            else if( laConfigUnitIter->second.getMode() == "label" )
            {
                static_cast<NChineseAnalyzer*>(analyzer.get())->setLabelMode();
            }

            if(mode) {
                static_cast<NChineseAnalyzer*>(analyzer.get())->setCaseSensitive(
                    laConfigUnitIter->second.getCaseSensitive(), laConfigUnitIter->second.getLower());
            } else {
                static_cast<NChineseAnalyzer*>(analyzer.get())->setCaseSensitive(
                    laConfigUnitIter->second.getCaseSensitive(), false);
            }

            setOptions( laConfigUnitIter->second.getOption(), static_cast<NChineseAnalyzer*>(analyzer.get()) );

            if(!mode) {
                static_cast<NChineseAnalyzer*>(analyzer.get())->setNBest(1);
            }

            // Fix some setting
//            static_cast<NChineseAnalyzer*>(analyzer.get())->setGenerateCompNoun( false );
//            static_cast<NChineseAnalyzer*>(analyzer.get())->setExtractChinese( false );
            //static_cast<NChineseAnalyzer*>(analyzer.get())->setRetFlag_index( Analyzer::ANALYZE_SECOND_ );

            //check the special char here, only accept 0x0000 to 0x007F in ucs encoding
            UString::EncodingType defEncoding = UString::UTF_8;
            UString origSpeU( laConfigUnitIter->second.getSpecialChar().c_str(), defEncoding );
            string speU; // after remove invalid characters
            for( size_t i = 0; i < origSpeU.length(); ++i )
            {
                unsigned int c = static_cast<unsigned int>( origSpeU.at(i) );
                if( c > 0x007F )
                {
                    string invalidStr;
                    origSpeU.substr( i, 1 ).convertString( invalidStr, defEncoding );
                    if( outputLog )
                    {
                        sflog->warn( SFL_LA, "SF-070105: Invalid special char \"%s\" for analyzer \"%s\" (accept 0x0000 to 0x007F).",
                                invalidStr.c_str(), analysisInfo.analyzerId_.c_str() );
                    }
                }
                else
                    speU.append( 1, static_cast<char>(c) );
            }

            // need to adjust tokenizer settings if there is "specialchar" settings
            tokenConfig.addAllows( speU );
            // ignore special char for Chinese now as Chinese don't have such POS
            //static_cast<NChineseAnalyzer*>(analyzer.get())->setSpecialChars(laConfigUnitIter->second.getSpecialChar());
        }
#endif

#ifdef USE_IZENEJMA
        else if( analysis == "japanese" )
        {
            analyzer.reset( new NJapaneseAnalyzer( laConfigUnitIter->second.getDictionaryPath()) );

            if( laConfigUnitIter->second.getMode() == "all" )
            {
                static_cast<NJapaneseAnalyzer*>(analyzer.get())->setIndexMode();
            }
            else if( laConfigUnitIter->second.getMode() == "noun" )
            {
                static_cast<NJapaneseAnalyzer*>(analyzer.get())->setIndexMode();
            }
            else if( laConfigUnitIter->second.getMode() == "label" )
            {
                static_cast<NJapaneseAnalyzer*>(analyzer.get())->setLabelMode();
            }

            if(mode) {
                static_cast<NJapaneseAnalyzer*>(analyzer.get())->setCaseSensitive(
                    laConfigUnitIter->second.getCaseSensitive(), laConfigUnitIter->second.getLower());
            } else {
                static_cast<NJapaneseAnalyzer*>(analyzer.get())->setCaseSensitive(
                    laConfigUnitIter->second.getCaseSensitive(), false);
            }

            setOptions( laConfigUnitIter->second.getOption(), static_cast<NJapaneseAnalyzer*>(analyzer.get()) );

            if(!mode) {
                static_cast<NJapaneseAnalyzer*>(analyzer.get())->setNBest(1);
            }

            //check the special char here, only accept 0x0000 to 0x007F in ucs encoding
            UString::EncodingType defEncoding = UString::UTF_8;
            UString origSpeU( laConfigUnitIter->second.getSpecialChar().c_str(), defEncoding );
            string speU; // after remove invalid characters
            for( size_t i = 0; i < origSpeU.length(); ++i )
            {
                unsigned int c = static_cast<unsigned int>( origSpeU.at(i) );
                if( c > 0x007F )
                {
                    string invalidStr;
                    origSpeU.substr( i, 1 ).convertString( invalidStr, defEncoding );
                    if( outputLog )
                    {
                        sflog->warn( SFL_LA, "SF-070105: Invalid special char \"%s\" for analyzer \"%s\" (accept 0x0000 to 0x007F).",
                                invalidStr.c_str(), analysisInfo.analyzerId_.c_str() );
                    }
                }
                else
                    speU.append( 1, static_cast<char>(c) );
            }

            // need to adjust tokenizer settings if there is "specialchar" settings
            tokenConfig.addAllows( speU );
            // ignore special char for Chinese now as Chinese don't have such POS
            //static_cast<NJapaneseAnalyzer*>(analyzer.get())->setSpecialChars(laConfigUnitIter->second.getSpecialChar());
        }
#endif

//        else if( analysis == "danish"
//                || analysis == "dutch"
//                || analysis == "english"
//                || analysis == "finnish"
//                || analysis == "french"
//                || analysis == "german"
//                || analysis == "hungarian"
//                || analysis == "italian"
//                || analysis == "norwegian"
//                || analysis == "portuguese"
//                || analysis == "romanian"
//                || analysis == "russian"
//                || analysis == "spanish"
//                || analysis == "swedish"
//                || analysis == "turkish"
//               )
//        {
//            analyzer.reset( new StemAnalyzer(analysis) );
//        }
        else if( analysis == "multilang" )
        {
            analyzer.reset( new MultiLanguageAnalyzer() );
            MultiLanguageAnalyzer* mla = static_cast<MultiLanguageAnalyzer*>(analyzer.get());
//            mla->setExtractEngStem( laConfigUnitIter->second.getStem() );
            string option = laConfigUnitIter->second.getAdvOption();
            bool multi_label = false;
            if( laConfigUnitIter->second.getMode() == "label" )
            {
              multi_label = true;
              mla->setExtractSpecialChar(false, false);
            }
            bool setDef = false;
            LAStringTokenizer lst( option, SEMICOLON_SEP );
            for( LAStringTokenizer::iterator itr = lst.begin(); itr != lst.end(); ++itr )
            {
                if( itr->empty() )
                    continue;
                LAStringTokenizer langop( *itr, COMMA_SEP );
                LAStringTokenizer::iterator laitr = langop.begin();
                if( laitr == langop.end() )
                {
                    stringstream message;
                    message << "LAPool::createLA() analysis \"" << analysis << "\"'s option is invalid: "<< option << ".";
                    if( outputLog )
                        sflog->error(SFL_LA, "SF-070103: Analysis option is invalid: %s\n", (message.str()).c_str());
                    throw std::logic_error( message.str() );
                }

                // other represents default language
                MultiLanguageAnalyzer::Language language;
//                MultiLanguageAnalyzer::ProcessMode processMode = MultiLanguageAnalyzer::NONE_PM;
                string langname = *laitr;

                if( langname == "default" )
                    language = MultiLanguageAnalyzer::OTHER;
                else if( langname == "cn" )
                    language = MultiLanguageAnalyzer::CHINESE;
                else if( langname == "jp" )
                    language = MultiLanguageAnalyzer::JAPANESE;
                else if( langname == "kr" )
                    language = MultiLanguageAnalyzer::KOREAN;
                else if( langname == "en" )
                    language = MultiLanguageAnalyzer::ENGLISH;
                else
                {
                    stringstream message;
                    message << "LAPool::createLA() analysis \"" << analysis << "\"'s option, the language name is invalid: "<< langname <<
                            " (include default/cn/jp/kr/en ).";
                    if( outputLog )
                        sflog->error(SFL_LA, "SF-070103: Analysis option is invalid: %s\n", (message.str()).c_str());
                    throw std::logic_error( message.str() );
                }

                ++laitr;
//
//                if( language != MultiLanguageAnalyzer::OTHER)
//                {
//                    if( laitr == langop.end() )
//                    {
//                        stringstream message;
//                        message << "LAPool::createLA() analysis \"" << analysis << "\"'s option, requires process mode " <<
//                                "(ma/char/string/none) after language "<< langname << ".";
//                        if( outputLog )
//                            sflog->error(SFL_LA, "SF-070103: Analysis option is invalid: %s\n", (message.str()).c_str());
//                        throw std::logic_error( message.str() );
//                    }
//
//                    string pmname = *laitr;
//
//                    if( pmname == "ma" )
//                        processMode = MultiLanguageAnalyzer::MA_PM;
//                    else if( pmname == "char" )
//                        processMode = MultiLanguageAnalyzer::CHARACTER_PM;
//                    else if( pmname == "string" )
//                        processMode = MultiLanguageAnalyzer::STRING_PM;
//                    else if( pmname == "none" )
//                        processMode = MultiLanguageAnalyzer::NONE_PM;
//                    else
//                    {
//                        stringstream message;
//                        message << "LAPool::createLA() analysis \"" << analysis << "\"'s option, the process mode is invalid: "<< pmname <<
//                                " (include ma/char/string/none ).";
//                        if( outputLog )
//                            sflog->error(SFL_LA, "SF-070103: Analysis option is invalid: %s\n", (message.str()).c_str());
//                        throw std::logic_error( message.str() );
//                    }
//
//                    // set the process mode, it will be fail to set MA_PM as associated Analyzer isn't created
//                    mla->setProcessMode( language, processMode );
//                    ++laitr;
//                }

//                if( language == MultiLanguageAnalyzer::OTHER ||
//                        processMode == MultiLanguageAnalyzer::MA_PM )
                {
                    if( laitr == langop.end() )
                    {
                        stringstream message;
                        message << "LAPool::createLA() analysis \"" << analysis << "\"'s option, requires analyer id when language " <<
                                "is default or process mode is ma after language "<< langname << ".";
                        if( outputLog )
                            sflog->error(SFL_LA, "SF-070103: Analysis option is invalid: %s\n", (message.str()).c_str());
                        throw std::logic_error( message.str() );
                    }

                    string analyzerId = *laitr;

                    bool foundInnerAN = false;
                    for(LaConfigUnitMap::const_iterator lacii = laConfigUnitMap_.begin();
                            lacii != laConfigUnitMap_.end(); ++ lacii )
                    {
                        if( lacii->first == analyzerId )
                        {
                            foundInnerAN = true;
                            break;
                        }
                    }

                    if( !foundInnerAN )
                    {
                        stringstream message;
                        message << "LAPool::createLA() analysis \"" << analysis << "\"'s option, cannot find analyer id " <<
                                   analyzerId << " after language "<< langname << "(be declared before this Method and inner_ as prefix).";
                        if( outputLog )
                            sflog->error(SFL_LA, "SF-070103: Analysis option is invalid: %s\n", (message.str()).c_str());
                        throw std::logic_error( message.str() );
                    }

                    AnalysisInfo innerInfo;
                    innerInfo.analyzerId_ = analyzerId;
                    innerInfo.tokenizerNameList_ = analysisInfo.tokenizerNameList_;
                    LA* laan = createLA( innerInfo, outputLog, mode );
                    if( laan == NULL )
                        return NULL;
                    boost::shared_ptr<la::Analyzer> innerAN = laan->getAnalyzer();
                    if( language == MultiLanguageAnalyzer::OTHER )
                    {
                        if( multi_label )
                        {
                          static_cast<NKoreanAnalyzer*>(innerAN.get())->setAnalyzePrime(true);
                        }
                        mla->setDefaultAnalyzer( innerAN );
                        setDef = true;
                    }
                    else
                    {
                        mla->setAnalyzer( language, innerAN );
                        // set the ProcessMode again if ProcessMode is MA_PM
//                        mla->setProcessMode( language, processMode );
                    }
                    delete laan;
                }

                if( !setDef )
                {
                    stringstream message;
                    message << "LAPool::createLA() analysis \"" << analysis << "\"'s option, requires default language " << ".";
                    if( outputLog )
                        sflog->error(SFL_LA, "SF-070103: Analysis option is invalid: %s\n", (message.str()).c_str());
                    throw std::logic_error( message.str() );
                }

            }
        }
        else
        {
            stringstream message;
            message << "LAPool::createLA() analysis type \"" << analysis << "\" not available. ";
            if( outputLog )
                sflog->error(SFL_LA, "SF-070103: Analysis type is not available. Type %s\n", (message.str()).c_str());
            throw std::logic_error( message.str() );
        }

//        if( analyzer.get() != NULL )
//        {
//            analyzer.get()->setCaseSensitive( laConfigUnitIter->second.getCaseSensitive() );
//            analyzer.get()->setContainLower( laConfigUnitIter->second.getLower() );
//            if( analysis != "chinese" )
//            {
//                if( laConfigUnitIter->second.getIdxFlag() != (unsigned char)0x77 )
//                    analyzer.get()->setRetFlag_index( laConfigUnitIter->second.getIdxFlag() );
//                if( laConfigUnitIter->second.getSchFlag() != (unsigned char)0x77 )
//                    analyzer.get()->setRetFlag_search( laConfigUnitIter->second.getSchFlag() );
//            }
//            else
//            {
//                analyzer.get()->setRetFlag_index( 0x02 );
//                analyzer.get()->setRetFlag_search( 0x02 );
//            }
//        }

        // check duplicated characters in the tokenConfig
        map< UCS2Char, TokCharUnit > tokMap;
        TokCharUnit::TokType typesEnum[] = { TokCharUnit::Divide, TokCharUnit::Unite, TokCharUnit::Allow };
        UString ustrArray[] = { tokenConfig.divides_, tokenConfig.unites_, tokenConfig.allows_ };
        for( int i = 0; i< TokCharUnit::Num; ++i )
        {
            TokCharUnit::TokType tokType = typesEnum[i];
            UString& tokUStr = ustrArray[i];
            for( size_t j = 0; j < tokUStr.length(); ++j )
                tokMap[ tokUStr[j] ].setType( tokType );
        }

        map< UCS2Char, TokCharUnit >::iterator tokItr;
        // remove un-duplicated chars
        for( tokItr = tokMap.begin(); tokItr != tokMap.end(); )
        {
            if( tokItr->second.isSingleType() )
            {
                map< UCS2Char, TokCharUnit >::iterator tmpTokItr = tokItr;
                ++tokItr;
                tokMap.erase( tmpTokItr );
            }
            else
                ++tokItr;
        }

        // print out message
        if( !tokMap.empty() )
        {
            stringstream tokanaNamesStream;

            for( set<string>::const_iterator tokNameItr = analysisInfo.tokenizerNameList_.begin();
                    tokNameItr != analysisInfo.tokenizerNameList_.end(); ++tokNameItr )
            {
                tokanaNamesStream << *tokNameItr << ",";
            }
            tokanaNamesStream << analysisInfo.analyzerId_;
            string tokanaNamesStr = tokanaNamesStream.str();

            for( tokItr = tokMap.begin(); tokItr != tokMap.end(); ++tokItr )
            {
                stringstream tokDuplChar;
                UString ustrtmp;
                ustrtmp += tokItr->first;
                tokDuplChar << "'";
                ustrtmp.displayStringInfo( UString::UTF_8, tokDuplChar );
                tokDuplChar << "'(0x" << hex << tokItr->first << ")";
                if( outputLog )
                {
                    sflog->warn( SFL_LA, "SF-070101: Duplicate Tokenizers (or specialchar) setting for character %s, in %s.",
                            tokDuplChar.str().c_str(), tokanaNamesStr.c_str() );
                }
            }
        }
        // end checking tokenConfig

//        la->setTokenizerConfig( tokenConfig );
        if( analyzer.get() != NULL )
            analyzer->setTokenizerConfig(tokenConfig);
        la->setAnalyzer( analyzer );

#ifdef DEBUG_LAPOOL_ANALYZER
        if( analysisInfo.analyzerId_ == ANALYSIS_ID_TO_TEST )
        {
            cout<<"Test analyzer with id "<<analysisInfo.analyzerId_<<endl;
            char buf[1024];
            UString query;
            TermList termList;
            while( true )
            {
                cout << "Enter query, or enter \"x(X)\" to exit" << endl;
                cin.getline( buf, 1024 );

                if( buf[0]  == 'X' || buf[0] == 'x' )
                {
                    break;
                }


                query.assign( buf, izenelib::util::UString::UTF_8 );
                cout << "Query: "; query.displayStringValue( izenelib::util::UString::UTF_8 ); cout << endl;
                cout << endl;

                cout << "============ process_index ===========" << endl;
                la->process_index( query, termList );
                cout << "Terms:" << endl;
                printTermList( termList, cout );
                cout << endl;

                cout << "============ process_search ===========" << endl;
                la->process_search( query, termList );
                cout << "Terms:" << endl;
                printTermList( termList, cout );
                cout << endl;
            }
        }
#endif

        return la;

    } // end - createLA( const AnalysisInfo & analysisInfo )

    la::LA* LAPool::getIndexLA(const AnalysisInfo& analysisInfo )
    {
        unordered_map<AnalysisInfo, LA*>::iterator it = laIndexMap_.find(analysisInfo);
        if(it == laIndexMap_.end())
        {
            return NULL;
        }
        else
        {
            return it->second;
        }
    }

    LA * LAPool::popSearchLA(const AnalysisInfo & analysisInfo )
    {
        ScopedWriteLock<ReadWriteLock> swl( lock_ );

        unordered_map<AnalysisInfo, deque<LA*> >::iterator it = laSearchPool_.find(analysisInfo);
        if(it == laSearchPool_.end())
        {
            return NULL;
        }

        if( it->second.empty() )
        {
            // create new instance when Pool is empty
            return createLA( analysisInfo, false, false );
            //return NULL;
        }

        LA * temp = it->second.front();
        it->second.pop_front();

        return temp;
    } //end - popSearchLA(const AnalysisInfo & analysisInfo )



    void LAPool::pushSearchLA(const AnalysisInfo & analysisInfo, LA * laThread )
    {
        ScopedWriteLock<ReadWriteLock> swl( lock_ );

        unordered_map<AnalysisInfo, deque<LA*> >::iterator it = laSearchPool_.find(analysisInfo);

        // DO NOT limit the Pool size
        // the pool is already full. wrong release() call
        /*if( it->second.size() == LA_THREAD_NUM )
        {
            delete laThread;
            return;
        }*/

        it->second.push_back( laThread );
    } // end  - pushSearchLA(const AnalysisInfo & analysisInfo, LA * laThread )


    LA * LAPool::topSearchLA(const AnalysisInfo & analysisInfo )
    {
        unordered_map<AnalysisInfo, deque<LA*> >::iterator it = laSearchPool_.find(analysisInfo);
        if(it == laSearchPool_.end())
        {
            return NULL;
        }

        if( it->second.empty() )
            return NULL;

        return it->second.front();
    } //end - topSearchLA(const AnalysisInfo & analysisInfo )


    bool LAPool::getLAConfigUnit( const string & laConfigId, LAConfigUnit & laConfigUnit ) const
    {
        std::map<std::string, LAConfigUnit>::const_iterator it = laConfigUnitMap_.find( laConfigId );
        if( it != laConfigUnitMap_.end() )
        {
            laConfigUnit = it->second;
            return true;
        }
        else
        {
            return false;
        }
    }

} // namespace sf1r

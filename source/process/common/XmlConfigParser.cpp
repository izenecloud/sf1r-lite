/**
 * @file XmlConfigParser.cpp
 * @brief Implements SF1Config class, which is a XML configuration file parser for SF-1 v5.0
 * @author MyungHyun (Kent)
 * @date 2008-09-05
 */

#include "XmlConfigParser.h"
#include "XmlSchema.h"
#include <util/ustring/UString.h>

#include <common/type_defs.h>
#include <common/SFLogger.h>

#include <boost/filesystem.hpp>

#include <iostream>
#include <sstream>

using namespace std;
using namespace izenelib::util::ticpp;

namespace sf1r
{

//------------------------- HELPER FUNCTIONS -------------------------

bool checkIntFormat( const std::string & str )
{
    std::size_t pos = str.find_first_not_of( "0123456789", 0 );
    if( pos != std::string::npos )
    {
        /*
        // the value is a negative number
        if( pos == 0 && str[pos] == '-' )
        {
            pos = str.find_first_not_of( "0123456789", pos+1 );
            if( pos == string::npos )
                return true;
            else
                return false;
        }
        */

        return false;
    }
    return true;
}

void downCase( std::string & str )
{
    for ( string::iterator it = str.begin(); it != str.end(); it++ )
    {
        *it = tolower(*it);
    }
}

int parseTruth( const string & str )
{
    std::string temp = str;
    downCase( temp );

    if( temp == "y" || temp == "yes" )
        return 1;
    else if( temp == "n" || temp == "no" )
        return 0;
    else
        return -1;
}

void parseByComma( const string & str, vector<string> & subStrList )
{
    subStrList.clear();

    std::size_t startIndex=0, endIndex=0;


    while ( (startIndex = str.find_first_not_of( " ,\t", startIndex )) != std::string::npos )
    {
        endIndex = str.find_first_of( " ,\t", startIndex + 1 );

        if ( endIndex == string::npos )
        {
            endIndex = str.length();
        }

        std::string substr = str.substr( startIndex, endIndex - startIndex );
        startIndex = endIndex + 1;

        subStrList.push_back( substr );
    }
}

// ------------------------- HELPER MEMBER FUNCTIONS of XmlConfigParser -------------------------

ticpp::Element * XmlConfigParser::getUniqChildElement(
        const ticpp::Element * ele, const std::string & name, bool throwIfNoElement ) const
{
    ticpp::Element * temp = NULL;
    temp = ele->FirstChildElement( name, false );


    if( temp == NULL )
    {
        if( throwIfNoElement )
            throw_NoElement( name );
        else
            return NULL;
    }

    if ( temp->NextSibling( name, false ) != NULL )
    {
        throw_MultipleElement( name );
    }

    return temp;
}

inline bool XmlConfigParser::getAttribute(
        const ticpp::Element * ele,
        const std::string & name,
        std::string & val,
        bool throwIfNoAttribute ) const
{
    val = ele->GetAttribute( name );

    if( val.empty() )
    {
        if( throwIfNoAttribute )
            throw_NoAttribute( ele, name );
        else
            return false;
    }

    return true;
}


bool XmlConfigParser::getAttribute(
        const ticpp::Element * ele,
        const std::string & name,
        bool & val,
        bool throwIfNoAttribute ) const
{
    std::string temp;

    if( !getAttribute( ele, name, temp, throwIfNoAttribute ) )
        return false;

    switch( parseTruth(temp) )
    {
        case 1:
            val = true;
            break;
        case 0:
            val = false;
            break;
        case -1:
            throw_TypeMismatch( ele, name, temp );
            break;
    }

    return true;
}

izenelib::util::UString::EncodingType XmlConfigParser::parseEncodingType(const std::string& encoding)
{
    izenelib::util::UString::EncodingType eType = izenelib::util::UString::UTF_8;
    if ( encoding == "utf-8" || encoding == "utf8" )
        eType = izenelib::util::UString::UTF_8;
    else if ( encoding == "euc-kr" || encoding == "euckr" )
        eType = izenelib::util::UString::EUC_KR;
    else if ( encoding == "cp949" )
        eType = izenelib::util::UString::CP949;
    else if ( encoding == "euc-jp" || encoding == "eucjp" )
        eType = izenelib::util::UString::EUC_JP;
    else if ( encoding == "sjis" )
        eType = izenelib::util::UString::SJIS;
    else if ( encoding == "gb2312" )
        eType = izenelib::util::UString::GB2312;
    else if ( encoding == "big5" )
        eType = izenelib::util::UString::BIG5;
    else if ( encoding == "iso8859-15" )
        eType = izenelib::util::UString::ISO8859_15;
    return eType;
}


// ------------------------- SF1Config-------------------------

SF1Config::SF1Config()
{
}

SF1Config::~SF1Config()
{
}

bool SF1Config::parseConfigFile( const string & fileName ) throw( XmlConfigParserException  )
{
    namespace bf=boost::filesystem;
    
    try
    {
        if( !boost::filesystem::exists(fileName) )
        {
            std::cerr << "[SF1Config] Config File doesn't exist." << std::endl;
            return false;
        }
        
        /** schema validate begin */
         bf::path config_file(fileName);
         bf::path config_dir = config_file.parent_path();
         bf::path schema_file = config_dir/"sf1config.xsd";
         std::string schema_file_string = schema_file.file_string();
         std::cout<<"XML Schema File: "<<schema_file_string<<std::endl;
         if( !boost::filesystem::exists(schema_file_string) )
         {
             std::cerr << "[SF1Config] Schema File doesn't exist." << std::endl;
             return false;
         }
         
         XmlSchema schema(schema_file_string);
         bool schema_valid = schema.validate(fileName);
         std::list<std::string> schema_warning = schema.getSchemaValidityWarnings();
         if(schema_warning.size()>0)
         {
           std::list<std::string>::iterator it = schema_warning.begin();
           while( it!= schema_warning.end() )
           {
             std::cout<<"[Schema-Waring] "<<*it<<std::endl;
             it++;
           }
         }
         if( !schema_valid )
         {
           //output schema errors
           std::list<std::string> schema_error = schema.getSchemaValidityErrors();
           if(schema_error.size()>0)
           {
             std::list<std::string>::iterator it = schema_error.begin();
             while( it!= schema_error.end() )
             {
               std::cerr<<"[Schema-Error] "<<*it<<std::endl;
               it++;
             }
           }
           return false;
         }
        /** schema validate end */

        ticpp::Document configDocument(fileName.c_str());
        configDocument.LoadFile();

        // make sure the top level element is "SF1Config"; if it isn't, an exception is thrown
        Element * sf1config = NULL;
        if( (sf1config =  configDocument.FirstChildElement( "SF1Config", false )) == NULL )
        {
            throw_NoElement( "SF1Config" );
        }

        //#1 System

        parseSystemSettings( getUniqChildElement( sf1config, "System" ) );

        ticpp::Element *cobra = getUniqChildElement( sf1config, "Deployment" ) ;
        if( cobra == NULL )
        	cobra = getUniqChildElement( sf1config, "Cobra" ) ;
        assert(cobra != NULL );
        parseDeploymentSettings( cobra );

    }
    catch( ticpp::Exception err )
    {
        size_t substart = err.m_details.find( "\nDescription: " );
        substart = substart + strlen("\nDescription: ");


        string msg = err.m_details.substr( substart);
        size_t pos = 0;

        while( (pos = msg.find("\n", pos )) != string::npos )
        {
            msg.replace( pos, 1, " ");
            pos++;
        }

        msg.insert( 0, "Exception occured while parsing file: " );

        throw XmlConfigParserException( msg );
    }

    return true;
} // END - SF1Config::parseConfigFile()


// 1. SYSTEM SETTINGS  -------------------------------------

void SF1Config::parseSystemSettings( const ticpp::Element * system )
{
  parseBrokerAgent( getUniqChildElement( system, "BrokerAgent" ) );
  //get resource dir
  getAttribute( getUniqChildElement( system, "Resource" ), "path", resource_dir_);
  
  parseQuerySupport( getUniqChildElement( system, "QuerySupport" ) );
  
  //mining switch, for test
  getAttribute( getUniqChildElement( system, "MiningSwitch" ), "enable", mining_switch_, true);
  
  parseParametersDefault( getUniqChildElement( system, "ParametersDefault" ) );
  
  parseFirewall( getUniqChildElement( system, "Firewall" ) );
  
  parseTokenizer( getUniqChildElement( system, "Tokenizing" ) );

  parseLanguageAnalyzer( getUniqChildElement( system, "LanguageAnalyzer" ) );

  parseRanking( getUniqChildElement( system, "Ranking" ) );

  parseLanguageIdentifier( getUniqChildElement( system, "LanguageIdentifier" ) );
  
  //parse crontab
  {
    ticpp::Element* cron = getUniqChildElement( system, "CronIndexRecommend" );
    if(cron!=NULL)
    {
      std::string value;
      if(getAttribute(cron, "value", value))
      {
        cronIndexRecommend_ = value;
      }
    }
  }
  {
    ticpp::Element* cron = getUniqChildElement( system, "CronIndexer" );
    if(cron!=NULL)
    {
      std::string value;
      if(getAttribute(cron, "value", value))
      {
        cronIndexer_ = value;
      }
    }
  }

}



// 1.3. BrokerAgent  -------------------------------------
void SF1Config::parseBrokerAgent( const ticpp::Element * brokerAgent )
{
    const size_t MAX_THREAD_NUM = 200;
    bool bUseCache = false;
    size_t threadNum = 1;
    bool bEnableTest = false;

    getAttribute( brokerAgent, "usecache", bUseCache, false );
    getAttribute( brokerAgent, "enabletest", bEnableTest, false );
    brokerAgentConfig_.setUseCache( bUseCache );
    if ( !getAttribute( brokerAgent, "threadnum", threadNum, false ) ||
            threadNum > MAX_THREAD_NUM || threadNum == 0 )
    {
        stringstream ss;
        ss << "Thread Num is invalid format or out of valid range(1 < THREAD_NUM < " << MAX_THREAD_NUM << "). Change the value to " << MAX_THREAD_NUM;
        cerr << "[Warning] : " << ss.str() << endl;
        sflog->warn( SFL_INIT, ss.str().c_str() );
        threadNum = MAX_THREAD_NUM;
    }
    brokerAgentConfig_.setThreadNums( threadNum );
    brokerAgentConfig_.setEnableTest( bEnableTest );

} // end - parseBrokerAgent()


// 1.4. QuerySupport -------------------------------------
void SF1Config::parseQuerySupport( const ticpp::Element * querySupport )
{

    getAttribute( querySupport, "updatetime", query_support_config_.update_time );
    getAttribute( querySupport, "logdays", query_support_config_.log_days );
    

    Element * settings = NULL;

    settings = getUniqChildElement( querySupport, "Path" );
    getAttribute( settings, "basepath", query_support_config_.basepath );
    settings = getUniqChildElement( querySupport, "QueryCorrectionPara" );
    getAttribute( settings, "enableEK", query_support_config_.query_correction_enableEK );
    getAttribute( settings, "enableCN", query_support_config_.query_correction_enableCN );
    settings = getUniqChildElement( querySupport, "AutoFillPara" );
    getAttribute( settings, "num", query_support_config_.autofill_num );

}

// 1.5. FireWall -------------------------------------
void SF1Config::parseFirewall( const ticpp::Element * fireElement )
{
    Iterator<Element> allow_it("Allow");
    for ( allow_it = allow_it.begin( fireElement ); allow_it != allow_it.end(); allow_it++ )
    {
        string ipAddress;

        getAttribute( allow_it.Get(), "value", ipAddress, false );

        if( !ipAddress.empty() )
            firewallConfig_.allowIPList_.push_back( ipAddress );
        else
            throw XmlConfigParserException( "Firewall - \"Allow\" : Need to provide value.");
    }

    Iterator<Element> deny_it("Deny");
    for ( deny_it = deny_it.begin( fireElement ); deny_it != deny_it.end(); deny_it++ )
    {
        string ipAddress;

        getAttribute( deny_it.Get(), "value", ipAddress, false );

        if (!ipAddress.empty())
            firewallConfig_.denyIPList_.push_back( ipAddress );
        else
            throw XmlConfigParserException( "Firewall - \"Deny\" : Need to provide value.");
    }
}




// 1.5. FireWall -------------------------------------
void SF1Config::parseParametersDefault( const ticpp::Element * element )
{
    default_parameter_.LoadXML(element, false);
}

// 2.1 Tokenizer -------------------------------------
void SF1Config::parseTokenizer( const ticpp::Element * tokenizing )
{
    string id, method, value, code;

    Iterator<Element> tokenizer_it( "Tokenizer" );
    for ( tokenizer_it = tokenizer_it.begin( tokenizing ); tokenizer_it != tokenizer_it.end(); tokenizer_it++ )
    {
        getAttribute( tokenizer_it.Get(), "id", id );
        getAttribute( tokenizer_it.Get(), "method", method );
        getAttribute( tokenizer_it.Get(), "value", value, false );
        getAttribute( tokenizer_it.Get(), "code", code, false );

        downCase( method );

        if( !validateID(id) ) throw_TypeMismatch( tokenizer_it.Get(), "id", id, "Alphabets, Numbers, Dot(.), Dash(-) and Underscore(_)");

        if( method != "allow" && method != "divide" && method != "unite" )
        {
            stringstream message;
            message << "<Tokenizer> \"" << id << "\": method must one of 'allow', 'divide', or 'unite'.";
            throw XmlConfigParserException( message.str() );
        }

        // check if both "value" and "code" are empty
        if( value.empty() && code.empty() )
        {
            stringstream message;
            message << "<Tokenizer> \"" << id << "\": Need to provide either \"value\" or \"code\".";
            throw XmlConfigParserException( message.str() );
        }


        TokenizerConfigUnit tkUnit(id, method, value, code);

        if ( tokenizerConfigNameMap_.insert( pair<string, TokenizerConfigUnit>(id, tkUnit) ).second == false )
        {
            throw XmlConfigParserException( "Duplicate <Tokenizer> IDs" );
        }

        laManagerConfig_.addTokenizerConfig( tkUnit );
    }
}


// 2.2 LanguageAnalyzer -------------------------------------
void SF1Config::parseLanguageAnalyzer( const ticpp::Element * languageAnalyzer )
{
    // 1. <LanguageAnalyzer>
    string dictionaryPath;

    getAttribute( languageAnalyzer, "dictionarypath", dictionaryPath );

    laManagerConfig_.kma_path_ = dictionaryPath;
    std::cout<<"set KMA path: "<<dictionaryPath<<std::endl;


    // 2. <Method>


    // common settings among analyzers
    string id;
    string analysis;


    Iterator<Element> analyzer_it( "Method" );
    for ( analyzer_it = analyzer_it.begin( languageAnalyzer ); analyzer_it != analyzer_it.end(); analyzer_it++ )
    {
        getAttribute( analyzer_it.Get(), "id", id );
        getAttribute( analyzer_it.Get(), "analysis", analysis );

        downCase(analysis);


        if( !validateID(id) ) throw_TypeMismatch( analyzer_it.Get(), "id", id, "Alphabets, Numbers, Dot(.), Dash(-) and Underscore(_)");

        // 1.1. Initializes the container unit
        LAConfigUnit laUnit( id, analysis );

        bool bCaseSensitive = false;
        if( getAttribute( analyzer_it.Get(), "casesensitive", bCaseSensitive, false ) == true )
        {
            laUnit.setCaseSensitive( bCaseSensitive );
        }


        for( int flagIdx = 0; flagIdx < 2; ++flagIdx )
        {
            string flagName = flagIdx == 0 ? "idxflag" : "schflag";
            string flagVal;
            if( getAttribute( analyzer_it.Get(), flagName, flagVal, false ) == false )
                continue;
            downCase( flagVal );

            unsigned char flagCharVal;
            if( flagVal == "none" )
            {
                flagCharVal = 0x0;
            }
            else if( flagVal == "prime" )
            {
                flagCharVal = 0x01;
            }
            else if( flagVal == "second" )
            {
                flagCharVal = 0x02;
            }
            else if( flagVal == "all" )
            {
                flagCharVal = 0x03;
            }
            else
            {
                stringstream message;
                message << "\"" << id << "\": "<<flagName<<"'s value should be one of none/prime/second/all.";
                throw XmlConfigParserException( message.str() );
                continue;
            }
            if( flagIdx == 0 )
                laUnit.setIdxFlag( flagCharVal );
            else
                laUnit.setSchFlag( flagCharVal );
        }

        // set configurations by different analyzer types

        if( analysis == "token" )
        {
        }
        else if( analysis == "char" )
        {
        }
        else if( analysis == "ngram" )
        {
            unsigned int minLevel = 0, maxLevel = 0, maxNo = 0;
            bool bApart = false;

            getAttribute( analyzer_it.Get(), "min", minLevel );
            getAttribute( analyzer_it.Get(), "max", maxLevel );
            getAttribute( analyzer_it.Get(), "maxno", maxNo );
            getAttribute( analyzer_it.Get(), "apart", bApart );

            if( minLevel < 0) throw_TypeMismatch( analyzer_it.Get(), "min", minLevel, "Positive integer");
            if( maxLevel < 0) throw_TypeMismatch( analyzer_it.Get(), "max", maxLevel, "Positive integer");
            if( maxNo < 0) throw_TypeMismatch( analyzer_it.Get(), "maxno", maxNo, "Positive integer");
            if( maxLevel < minLevel) throw_TypeMismatch( analyzer_it.Get(), "min, max", 0, "Max level must be greater than equals to Min level");

            laUnit.setMinLevel( minLevel );
            laUnit.setMaxLevel( maxLevel );
            laUnit.setMaxNo( maxNo );
            laUnit.setApart( bApart );
        }
        else if( analysis == "matrix" )
        {
            bool bPrefix=false, bSuffix=false;

            getAttribute( analyzer_it.Get(), "prefix", bPrefix );
            getAttribute( analyzer_it.Get(), "suffix", bSuffix );

            laUnit.setPrefix(bPrefix);
            laUnit.setSuffix(bSuffix);
        }
        else if( analysis == "english"
#ifdef USE_WISEKMA
                || analysis == "korean"
#endif
#ifdef USE_IZENECMA
                || analysis == "chinese"
#endif
            )
        {
            Element * settings = NULL;
            // dictionary path set here can overwrite the global one set in <LanguageAnalyzer>
            string mode, option, specialChar, dictionaryPath_inner;

            settings = getUniqChildElement( analyzer_it.Get(), "settings", false );
            if ( settings != NULL )
            {
                getAttribute( settings, "mode", mode, false );
                getAttribute( settings, "option", option, false );
                getAttribute( settings, "specialchar", specialChar, false );
                getAttribute( settings, "dictionarypath", dictionaryPath_inner, false );

                downCase( mode );
                downCase( option );

                if(mode != "all" && mode != "noun" && mode != "label") throw_TypeMismatch( settings, "mode", mode, "\"all\", \"noun\" or \"label\"" );

                laUnit.setMode( mode );
                laUnit.setOption( option );
                laUnit.setSpecialChar( specialChar );

                // TODO: always set global dictionary path first
                if ( !dictionaryPath_inner.empty() )
                {
                    laUnit.setDictionaryPath( dictionaryPath_inner );
                }
                else if ( !dictionaryPath.empty() && dictionaryPath_inner.empty() )
                {
                    laUnit.setDictionaryPath( dictionaryPath );
                }
                else
                {
                    stringstream message;
                    message << "\"" << id << "\": Need to provide a dictionary path";
                    throw XmlConfigParserException( message.str() );
                }
            }
        }
        else if( analysis == "multilang" )
        {
            string advOption;
            getAttribute( analyzer_it.Get(), "advoption", advOption );
            laUnit.setAdvOption( advOption );

            bool stem = true;
            if( getAttribute( analyzer_it.Get(), "stem", stem, false ) == true )
            {
                laUnit.setStem( stem );
            }

            bool lower = true;
            if( getAttribute( analyzer_it.Get(), "lower", lower, false ) == true )
            {
                laUnit.setLower( lower );
            }
            
            Element * settings = NULL;
            string mode;

            settings = getUniqChildElement( analyzer_it.Get(), "settings", false );
            if ( settings != NULL )
            {
                getAttribute( settings, "mode", mode, false );
                downCase( mode );
                

                if(mode != "all" && mode != "noun" && mode != "label") throw_TypeMismatch( settings, "mode", mode, "\"all\", \"noun\" or \"label\"" );

                laUnit.setMode( mode );
                
            }
        }
        else
        {
            stringstream  message;
            message << "\"" << id << "\": Wrong analysis name. Should be one of "
#ifdef USE_IZENECMA
                << "\"chinese\", "
#endif
#ifdef USE_WISEKMA
                << "\"korean\", "
#endif
                << "\"token\", \"char\", \"ngram\", \"matrix\", or \"english\". (currently:"
                <<  analysis << ")";
            throw XmlConfigParserException( message.str() );
        }


        if ( laConfigIdNameMap_.insert( pair<string, LAConfigUnit>( id, laUnit ) ).second ==  false )
        {
            throw XmlConfigParserException( "Duplicate LanguageAnalyzer Method IDs" );
        }

        laManagerConfig_.addLAConfig( laUnit );

    }// for all the <Method> elements. (the LAConfigUnits)

} // END - SF1Config::parseLanguageAnalyzer()

// 2.3 Ranking -------------------------------------
void SF1Config::parseRanking( const ticpp::Element * ranking )
{
    //string variables to parse the config file
    string id;
    string qd_text_model;
    string ignore;

    //enum variables to contain the actual settings of their string representatives above
    TextRankingType e_qd_text_model = NotUseTextRanker;

    Iterator<Element> ranker_it( "RankingModel" );

    //iterate through <RankingModel> elements
    for ( ranker_it = ranker_it.begin(ranking); ranker_it!= ranker_it.end(); ranker_it++ )
    {
        //parse values
        getAttribute( ranker_it.Get(), "id", id );
        getAttribute( ranker_it.Get(), "qd_text_model", qd_text_model  );

        if( !validateID(id) ) throw_TypeMismatch( ranker_it.Get(), "id", id, "Alphabets, Numbers, Dot(.), Dash(-) and Underscore(_)");
        //create one RankingConfigUnit
        RankingConfigUnit rankingUnit( id );

        //query dependent text model
        if ( qd_text_model == "default" )
        {
            e_qd_text_model = RankingType::DefaultTextRanker;
        }
        else if ( qd_text_model == "bm25" )
        {
            e_qd_text_model = RankingType::BM25;
        }
        else if ( qd_text_model == "kl" )
        {
            e_qd_text_model = RankingType::KL;
        }
        else if ( qd_text_model == "plm" )
        {
            e_qd_text_model = RankingType::PLM;
        }
        else if ( qd_text_model == "none" )
        {
            e_qd_text_model = NotUseTextRanker;
        }
        else
        {
            throw_TypeMismatch( ranker_it.Get(), "qd_text_model", e_qd_text_model, "\"default\", \"bm25\", \"kl\", \"plm\", or \"none\"" );
        }


        //set the enum variables
        rankingUnit.setTextRankingModel( e_qd_text_model );

        if ( rankingConfigNameMap_.insert( pair<string, RankingConfigUnit>(id, rankingUnit) ).second == false )
        {
            throw XmlConfigParserException( "Duplicate <RankingModel> IDs" );
        }


    }//for all the ranking units that are set

} // END - SF1Config::parseRanking()


// 2.4 LanguageIdentifier -------------------------------------
void SF1Config::parseLanguageIdentifier( const ticpp::Element * langid )
{
    string dbpath;
    getAttribute( langid, "dbpath", dbpath );

    docManagerConfig_.languageIdentifierDbPath_ = dbpath;
} // END - SF1Config::parseLanguageIdentifier()

void SF1Config::parseDeploymentSettings(const ticpp::Element * deploy) 
{
    Element * ba_node = NULL;
    ba_node = getUniqChildElement(deploy, "BA");
    getPortNumber(ba_node, "port", cobraConfig_.baPort, false);
}

// ------------------------- CollectionConfig-------------------------
CollectionConfig::CollectionConfig()
{
}

CollectionConfig::~CollectionConfig()
{
}

bool CollectionConfig::parseConfigFile( const string& collectionName ,const string & fileName ) throw( XmlConfigParserException  )
{
    namespace bf=boost::filesystem;
    
    try
    {
        if( !boost::filesystem::exists(fileName) )
        {
            std::cerr << "[SF1Config] Config File doesn't exist." << std::endl;
            return false;
        }
        
        /** schema validate begin */
         bf::path config_file(fileName);
         bf::path config_dir = config_file.parent_path();
         bf::path schema_file = config_dir/"collection.xsd";
         std::string schema_file_string = schema_file.file_string();
         std::cout<<"XML Schema File: "<<schema_file_string<<std::endl;
         if( !boost::filesystem::exists(schema_file_string) )
         {
             std::cerr << "[SF1Config] Schema File doesn't exist." << std::endl;
             return false;
         }
         
         XmlSchema schema(schema_file_string);
         bool schema_valid = schema.validate(fileName);
         std::list<std::string> schema_warning = schema.getSchemaValidityWarnings();
         if(schema_warning.size()>0)
         {
           std::list<std::string>::iterator it = schema_warning.begin();
           while( it!= schema_warning.end() )
           {
             std::cout<<"[Schema-Waring] "<<*it<<std::endl;
             it++;
           }
         }
         if( !schema_valid )
         {
           //output schema errors
           std::list<std::string> schema_error = schema.getSchemaValidityErrors();
           if(schema_error.size()>0)
           {
             std::list<std::string>::iterator it = schema_error.begin();
             while( it!= schema_error.end() )
             {
               std::cerr<<"[Schema-Error] "<<*it<<std::endl;
               it++;
             }
           }
           return false;
         }
        /** schema validate end */

        ticpp::Document configDocument(fileName.c_str());
        configDocument.LoadFile();

        // make sure the top level element is "SF1Config"; if it isn't, an exception is thrown
        Element * collection = NULL;
        if( (collection =  configDocument.FirstChildElement( "Collection", false )) == NULL )
        {
            throw_NoElement( "Collection" );
        }

        CollectionMeta collectionMeta;
        collectionMeta.setName(collectionName);
        parseCollectionSettings( collection, collectionMeta );
	
        SF1Config::get()->collectionMetaList_.push_back( collectionMeta );
        SF1Config::get()->collectionMetaMap_.insert(std::make_pair( collectionMeta.getName(), collectionMeta));
        SF1Config::get()->cobraConfig_.collections.insert( collectionMeta.getName() );
	
        //---------- SETTING COLLECTION DATA ----------
	
        // get the property weight information by each collection
        for ( unsigned int i = 0; i< SF1Config::get()->collectionMetaList_.size(); i++ )
        {
            // map<property name, weight>
            map<string, float> propertyWeightMap;

            std::set<PropertyConfig, PropertyComp> schema = SF1Config::get()->collectionMetaList_[i].getDocumentSchema();
            std::set<PropertyConfig, PropertyComp>::iterator it;
	
            for ( it = schema.begin(); it != schema.end(); it++ )
            {
                if ( it->getRankWeight() >= 0.0f )
                {
                    propertyWeightMap.insert( pair<string, float>(it->getName(), it->getRankWeight()) );
                }
            }
	
            if ( propertyWeightMap.empty() == false )
            {
                SF1Config::get()->rankingManagerConfig_.propertyWeightMapByProperty_.insert(
                    pair<string, map<string, float> >( SF1Config::get()->collectionMetaList_[i].getName(), propertyWeightMap )
                    );
            }
        }

    }
    catch( ticpp::Exception err )
    {
        size_t substart = err.m_details.find( "\nDescription: " );
        substart = substart + strlen("\nDescription: ");


        string msg = err.m_details.substr( substart);
        size_t pos = 0;

        while( (pos = msg.find("\n", pos )) != string::npos )
        {
            msg.replace( pos, 1, " ");
            pos++;
        }

        msg.insert( 0, "Exception occured while parsing file: " );

        throw XmlConfigParserException( msg );
    }

    return true;
} // END - CollectionConfig::parseConfigFile()


void CollectionConfig::parseCollectionSettings( const ticpp::Element * collection, CollectionMeta & collectionMeta )
{
    std::string name, encoding, ranking, wildcardtype, classifierlanguage;
    izenelib::util::UString::EncodingType eType;        //converts the encoding string into EncodingType
    
    CollectionParameterConfig collection_param(SF1Config::get()->default_parameter_);
    collection_param.LoadXML(getUniqChildElement(collection, "Parameter"), true);
    
    collection_param.GetString("CollectionMeta/encoding", encoding);
    collection_param.GetString("CollectionMeta/rankingmodel", ranking);
    collection_param.GetString("CollectionMeta/wildcardtype", wildcardtype, "unigram");
    collection_param.GetString( "CollectionMeta/generalclassifierlanguage", classifierlanguage, "chinese");
    //for ranking
    std::map<std::string, RankingConfigUnit>::iterator it = SF1Config::get()->rankingConfigNameMap_.find( ranking );
    SF1Config::get()->rankingManagerConfig_.addRankingConfig( name, it->second );
    downCase(encoding);
    eType = parseEncodingType(encoding);
    
    collectionMeta.setName( name );
    collectionMeta.setEncoding( eType );
    collectionMeta.setRankingMethod( ranking );
    collectionMeta.setWildcardType( wildcardtype );
    collectionMeta.setGeneralClassifierLanguage( classifierlanguage );

    IndexManagerConfig indexmanager_config;
    
    //Index strategy
    int64_t memorypool = 0;
    collection_param.Get<int64_t>("IndexStrategy/memorypoolsize", memorypool);
    
    indexmanager_config.indexStrategy_.memory_ = memorypool;
    indexmanager_config.indexStrategy_.indexDocLength_ = true;
    indexmanager_config.indexStrategy_.skipInterval_ = 8;
    indexmanager_config.indexStrategy_.maxSkipLevel_ = 3;
    indexmanager_config.storeStrategy_.param_ = "file";

    std::string indexpolicy("default");
    collection_param.GetString("IndexStrategy/indexpolicy", indexpolicy, "default");
    
    indexmanager_config.indexStrategy_.indexMode_ = indexpolicy;
    indexmanager_config.mergeStrategy_.param_ = "dbt";

    std::set<std::string> directories;
    collection_param.Get("CollectionDataDirectory", directories);
    
    SMiaConfig smia_config;
    if(!directories.empty())
    {
//       std::cout<<"data dir size: "<<directories.size()<<std::endl;
      indexmanager_config.indexStrategy_.indexLocations_.assign(directories.begin(), directories.end());
      smia_config.collectionDataDirectories().assign(directories.begin(), directories.end());
    }

    collectionMeta.setIndexManagerConfig(indexmanager_config);
    
    bool triggerqa = false;
    std::size_t threadNum = 3;
    std::size_t docCacheNum = 1000;
    collection_param.Get("Sia/triggerqa", triggerqa);
    collection_param.Get<std::size_t>("Sia/threadnum", threadNum);
    collection_param.Get<std::size_t>("Sia/doccachenum", docCacheNum);
    smia_config.setTriggerQA( triggerqa );
    smia_config.setThreadNums( threadNum );
    smia_config.setDocumentCacheNum(docCacheNum);
    collectionMeta.setSMiaConfig(smia_config);
    
    // PARSING MINING CONFIG
    MiningConfig mining_config;
    //for TG
    collection_param.Get<uint32_t>("TaxonomyPara/topdocnum", mining_config.taxonomy_param.top_docnum);
    collection_param.Get<uint32_t>("TaxonomyPara/levels", mining_config.taxonomy_param.levels);
    collection_param.Get<uint32_t>("TaxonomyPara/perlevelnum", mining_config.taxonomy_param.per_levelnum);
    collection_param.Get<uint32_t>("TaxonomyPara/candlabelnum", mining_config.taxonomy_param.candidate_labelnum);
    collection_param.Get("TaxonomyPara/enablenec", mining_config.taxonomy_param.enable_nec);
    collection_param.Get<uint32_t>("TaxonomyPara/maxpeopnum", mining_config.taxonomy_param.max_peopnum);
    collection_param.Get<uint32_t>("TaxonomyPara/maxlocnum", mining_config.taxonomy_param.max_locnum);
    collection_param.Get<uint32_t>("TaxonomyPara/maxorgnum", mining_config.taxonomy_param.max_orgnum);
    //for recommend
    collection_param.Get<uint32_t>("RecommendPara/recommendnum", mining_config.recommend_param.recommend_num);
    collection_param.Get<uint32_t>("RecommendPara/popularnum", mining_config.recommend_param.popular_num);
    collection_param.Get<uint32_t>("RecommendPara/realtimenum", mining_config.recommend_param.realtime_num);
    //for similarity
    collection_param.Get<uint32_t>("SimilarityPara/docnumlimit", mining_config.similarity_param.docnum_limit);
    collection_param.Get<uint32_t>("SimilarityPara/termnumlimit", mining_config.similarity_param.termnum_limit);
    //for DC
    collection_param.Get("ClassificationPara/customizetraining", mining_config.dc_param.customize_training);
    std::string encoding_str;
    downCase(encoding_str);
    collection_param.GetString("ClassificationPara/trainingencoding", encoding_str); mining_config.dc_param.encoding_type = parseEncodingType(encoding_str);
    collectionMeta.setMiningConfig(mining_config);
    // ---------------- PARSING <Path> CONFIGURATION
    parseCollectionPath( getUniqChildElement( collection, "Path" ), collectionMeta );

    

    // ---------------- PARSING <DocumentSchema> CONFIGURATION

    //counts the number of document schema defined in one Collection
    unsigned int docSchemaCount = 0;
    bool hasDate = false;

    Iterator<Element> documentSchema( "DocumentSchema" );


    // only iterates once
    for ( documentSchema = documentSchema.begin( collection ); documentSchema != documentSchema.end(); documentSchema++ )
    {
        //used to check duplicate property names
        set<string> propertyNameList;

        if ( docSchemaCount > 1 )
        {
            throw XmlConfigParserException( "Too many document schema definitions in one Collection") ;
        }


        Iterator<Element> property( "Property" );

        for ( property = property.begin( documentSchema.Get() ); property != property.end(); property++ )
        {
            try
            {
                //PARSING PROPERTY CONFIGURATIOINS
                parseProperty( property.Get(), collectionMeta, propertyNameList );

            }
            catch ( XmlConfigParserException & e )
            {
                stringstream message;
                message << "<" << name << "> ";

                e.details_.insert( 0, message.str() );

                throw e;
            }

        } //property iteration

        for( CollectionMeta::property_config_iterator it = collectionMeta.propertyConfigBegin();
                it != collectionMeta.propertyConfigEnd();
                it++ )
        {
            string pName = it->getName();
            downCase(pName);
            if(pName == "date" )
            {
                hasDate = true;
                // always index and filterable
                PropertyConfig updatedDatePropertyConfig = *it;
                updatedDatePropertyConfig.setIsIndex(true);
                updatedDatePropertyConfig.setIsFilter(true);
                collectionMeta.erasePropertyConfig(it->getName());
                collectionMeta.insertPropertyConfig(updatedDatePropertyConfig);
                break;
            }
        }

        if( hasDate == false )
        {
            PropertyConfig propertyConfig;

            propertyConfig.setName( "DATE");
            propertyConfig.setOriginalName( "DATE" );
            propertyConfig.setType( STRING_PROPERTY_TYPE );
            propertyConfig.setIsIndex(true);
            propertyConfig.setIsFilter(true);
            
            collectionMeta.insertPropertyConfig( propertyConfig );
        }

        docSchemaCount++;

    } // document schema iteration

    if ( docSchemaCount < 1 )
    {
        stringstream  message;
        message << "Collection \"" << name << "\": Need one <DocumentSchema> definition in one Collection";
        throw XmlConfigParserException( message.str() );
    }
    
    //** PARSE MINING SCHEMA BEGIN
    MiningSchema mining_schema;
    ticpp::Element * mining_schema_node = getUniqChildElement( collection, "MiningSchema", false );
    PropertyDataType property_type;
    if( mining_schema_node!= NULL )
    {
      ticpp::Element * task_node = getUniqChildElement( mining_schema_node, "TaxonomyGeneration", false );
      mining_schema.tg_enable = false;
      std::string property_name;
      if( task_node!= NULL )
      {
        Iterator<Element> it( "Property" );
        for ( it = it.begin( task_node ); it != it.end(); it++ )
        {
            getAttribute( it.Get(), "name", property_name );
            bool gottype = collectionMeta.getPropertyType(property_name, property_type);
            if( !gottype || property_type != STRING_PROPERTY_TYPE )
            {
              throw XmlConfigParserException("Property ["+property_name+"] used in TaxonomyGeneration is not string type.");
            }
            mining_schema.tg_properties.push_back(property_name);
        }
        mining_schema.tg_enable = true;
      }
      task_node = getUniqChildElement( mining_schema_node, "DuplicateDetection", false );
      mining_schema.dupd_enable = false;
      if( task_node!= NULL )
      {
        Iterator<Element> it( "Property" );
        for ( it = it.begin( task_node ); it != it.end(); it++ )
        {
            getAttribute( it.Get(), "name", property_name );
            bool gottype = collectionMeta.getPropertyType(property_name, property_type);
            if( !gottype || property_type != STRING_PROPERTY_TYPE )
            {
              throw XmlConfigParserException("Property ["+property_name+"] used in DuplicateDetection is not string type.");
            }
            mining_schema.dupd_properties.push_back(property_name);
        }
        mining_schema.dupd_enable = true;
      }
      task_node = getUniqChildElement( mining_schema_node, "Similarity", false );
      mining_schema.sim_enable = false;
      if( task_node!= NULL )
      {
        Iterator<Element> it( "Property" );
        for ( it = it.begin( task_node ); it != it.end(); it++ )
        {
            getAttribute( it.Get(), "name", property_name );
            bool gottype = collectionMeta.getPropertyType(property_name, property_type);
            if( !gottype || property_type != STRING_PROPERTY_TYPE )
            {
              throw XmlConfigParserException("Property ["+property_name+"] used in Similarity is not string type.");
            }
            mining_schema.sim_properties.push_back(property_name);
        }
        mining_schema.sim_enable = true;
      }
      
      task_node = getUniqChildElement( mining_schema_node, "DocumentClassification", false );
      mining_schema.dc_enable = false;
      if( task_node!= NULL )
      {
        Iterator<Element> it( "Property" );
        for ( it = it.begin( task_node ); it != it.end(); it++ )
        {
            getAttribute( it.Get(), "name", property_name );
            bool gottype = collectionMeta.getPropertyType(property_name, property_type);
            if( !gottype || property_type != STRING_PROPERTY_TYPE )
            {
              throw XmlConfigParserException("Property ["+property_name+"] used in DocumentClassification is not string type.");
            }
            mining_schema.dc_properties.push_back(property_name);
        }
        mining_schema.dc_enable = true;
      }
      
      task_node = getUniqChildElement( mining_schema_node, "Faceted", false );
      mining_schema.faceted_enable = false;
      if( task_node!= NULL )
      {
        Iterator<Element> it( "Property" );
        for ( it = it.begin( task_node ); it != it.end(); it++ )
        {
            getAttribute( it.Get(), "name", property_name );
            bool gottype = collectionMeta.getPropertyType(property_name, property_type);
            if( !gottype || property_type != STRING_PROPERTY_TYPE )
            {
              throw XmlConfigParserException("Property ["+property_name+"] used in DocumentClassification is not string type.");
            }
            mining_schema.faceted_properties.push_back(property_name);
        }
        mining_schema.faceted_enable = true;
      }
      
      task_node = getUniqChildElement( mining_schema_node, "IISE", false );
      mining_schema.ise_enable = false;
      if( task_node!= NULL )
      {
        getAttribute( task_node, "property", property_name );
        bool gottype = collectionMeta.getPropertyType(property_name, property_type);
        if( !gottype || property_type != STRING_PROPERTY_TYPE )
        {
          throw XmlConfigParserException("Property ["+property_name+"] used in IISE is not string type.");
        }
        mining_schema.ise_property = property_name;
        mining_schema.ise_enable = true;
      }
      
      //for recommend schema
      task_node = getUniqChildElement( mining_schema_node, "QueryRecommend", false );
      mining_schema.recommend_tg = false;
      mining_schema.recommend_querylog = false;
      mining_schema.recommend_properties.resize(0);
      if( task_node!= NULL )
      {
        Iterator<Element> it( "Property" );
        for ( it = it.begin( task_node ); it != it.end(); it++ )
        {
            getAttribute( it.Get(), "name", property_name );
            bool gottype = collectionMeta.getPropertyType(property_name, property_type);
            if( !gottype || property_type != STRING_PROPERTY_TYPE )
            {
              throw XmlConfigParserException("Property ["+property_name+"] used in QueryRecommend is not string type.");
            }
            mining_schema.recommend_properties.push_back(property_name);
        }
        ticpp::Element * intern_node = getUniqChildElement(task_node, "TG", false);
        if( intern_node!= NULL)
        {
          mining_schema.recommend_tg = true;
        }
        intern_node = getUniqChildElement(task_node, "QueryLog", false);
        if( intern_node!= NULL)
        {
          mining_schema.recommend_querylog = true;
        }
      }
    }
    
    collectionMeta.setMiningSchema(mining_schema);

    // ACL
    Iterator<Element> aclIterator("ACL");
    for (aclIterator = aclIterator.begin(collection);
         aclIterator != aclIterator.end(); ++aclIterator)
    {
        std::string aclAllow;
        std::string aclDeny;
        getAttribute(aclIterator.Get(), "allow", aclAllow, false);
        getAttribute(aclIterator.Get(), "deny", aclDeny, false);

        collectionMeta.aclAllow(aclAllow);
        collectionMeta.aclDeny(aclDeny);
    }

} // END - CollectionConfig::parseCollectionSettings

// 3.1. Path -------------------------------------
void CollectionConfig::parseCollectionPath( const ticpp::Element * path, CollectionMeta & collectionMeta )
{
    CollectionPath collPath;
    string basepath;

    getAttribute( path, "basepath", basepath );

    Element * specificPath = NULL;
    string subPath;

    collPath.setBasePath( basepath );

    specificPath = getUniqChildElement( path, "SCD", false );
    if( specificPath != NULL )
    {
        getAttribute( specificPath, "path", subPath, false );
        collPath.setScdPath( subPath );
    }

    specificPath = getUniqChildElement( path, "CollectionData", false );
    if( specificPath != NULL )
    {
        getAttribute( specificPath, "path", subPath, false );
        collPath.setCollectionDataPath( subPath );
    }

    specificPath = getUniqChildElement( path, "Query", false );
    if( specificPath != NULL )
    {
        getAttribute( specificPath, "path", subPath, false );
        collPath.setQueryDataPath( subPath );
    }

//     specificPath = getUniqChildElement( path, "Log", false );
//     if( specificPath != NULL )
//     {
//         getAttribute( specificPath, "path", subPath, false );
//         collPath.setLogPath( subPath );
//     }

    collectionMeta.setCollectionPath( collPath );
} // END - CollectionConfig::parseCollectionPath()


// 3.2. Property -------------------------------------
void CollectionConfig::parseProperty(
        const ticpp::Element * property,
        CollectionMeta & collectionMeta,
        set<string> & propertyNameList )
{
    //holds all the configuration data of this property
    PropertyConfig propertyConfig;

    string propertyName, type;
    bool bIndex = false;
    PropertyDataType dataType = UNKNOWN_DATA_PROPERTY_TYPE;

    getAttribute( property, "name", propertyName );
    getAttribute( property, "type", type );

    downCase( type );

    if( !validateID(propertyName) ) throw_TypeMismatch( property, "name", propertyName, "Alphabets, Numbers, Dot(.), Dash(-) and Underscore(_)");

    // find right data type
    if ( type == "string" )
    {
        dataType = STRING_PROPERTY_TYPE;
    }
    else if ( type == "int" )
    {
        dataType = INT_PROPERTY_TYPE;
    }
    else if ( type == "float" )
    {
        dataType = FLOAT_PROPERTY_TYPE;
    }
    else
    {
        throw_TypeMismatch( property, "name", type, "\"string\", \"int\" or \"float\"" );
    }

    getAttribute( property, "index", bIndex, false );
    
    //set data --------------------------------------
    propertyConfig.setName( propertyName );
    propertyConfig.setOriginalName( propertyName );
    propertyConfig.setType( dataType );
    propertyConfig.setIsIndex( bIndex );
    



    //--- parse optional settings for "Property"

    //================== DISPLAY ==================

    Element * display = getUniqChildElement( property, "Display", false );

    // the setting is optional
    if ( display != NULL )
    {
        if( propertyConfig.getType() != STRING_PROPERTY_TYPE )
        {
            stringstream message;
            message << "\"" << propertyName << "\": Only a string type property can have <Display> option";
            throw XmlConfigParserException( message.str() );
        }

        try
        {
            parseProperty_Display( display, propertyConfig );
        }
        catch ( XmlConfigParserException & e )
        {
            stringstream ss;
            ss << " in \"" << propertyName << "\"";
            e.details_ += ss.str();
            throw e;
        }
    }


    
    //================== INDEXING ==================
    //TODO: if the property is set index="no", then no <Indexing> options???
    //there are several cases where it is allowed. Need to identify & narrow the cases


    Element * indexing = property->FirstChildElement( "Indexing", false );

    //if <Indexing> element  exists
    if ( indexing != NULL )
    {
        if ( !bIndex )
        {
            stringstream ss;
            ss << "<Indexing> configuration will be ignored "
                << "for Property \"" << propertyName << "\"";
            sflog->warn(SFL_INIT, 30601, ss.str().c_str() );
        }
        else
        {
            //the number of original names. One alias name has to be the same as the original name
            int nOriginalName = 0;

            Iterator<Element> indexing_it( "Indexing" );

            // for all the <indexing> config
            for ( indexing_it = indexing_it.begin( property ); indexing_it != indexing_it.end(); indexing_it++ )
            {
                try
                {
                    parseProperty_Indexing( indexing_it.Get(), propertyConfig );
                }
                catch ( ticpp::Exception & e )
                {
                    stringstream ss;
                    ss << " in \"" << propertyName << "\"";

                    //e.m_details = e.m_details.substr( 0, e.m_details.find_last_of( "ticpp" ) - 5 );
                    e.m_details += ss.str();
                    throw e;
                }

                //if there are duplicate property names(including alias)
                if ( (( propertyNameList.insert(propertyConfig.getName()) ).second) == false )
                {
                    throw XmlConfigParserException( "Duplicate property names (including alias)" );
                }

                if ( propertyConfig.getName() == propertyConfig.getOriginalName() )
                {
                    nOriginalName++;
                }

                //add this property setting(alias) to DocumentSchema
                collectionMeta.insertPropertyConfig( propertyConfig );
            }

            // if( collectionMeta.isUnigramWildcard() )
            {
                if ( (SF1Config::get()->laConfigIdNameMap_.find( "la_unigram" )) == SF1Config::get()->laConfigIdNameMap_.end() ) {
                    throw XmlConfigParserException("Undefined analyzer configuration id, \"la_unigram\"");
                }

                /// TODO, add a hidden alias here
                AnalysisInfo analysisInfo;
                analysisInfo.analyzerId_ = "la_unigram";
                SF1Config::get()->analysisPairList_.insert( analysisInfo );

                propertyConfig.setName(propertyName+"_unigram");
                propertyConfig.setAnalysisInfo(analysisInfo);
                collectionMeta.insertPropertyConfig(propertyConfig );
            }

            if ( nOriginalName == 0 )
            {
                throw XmlConfigParserException( "[Property : " +  propertyConfig.getName()
                        + "] No <Indexing> setting for the original property name. (only alias?)" );
            }
            else if ( nOriginalName != 1 )
            {
                throw XmlConfigParserException( "[Property : " +  propertyConfig.getName()
                        + "Only one alias name has to be the same as the property name" );
            }
        }
    }
    else
    {
        if( propertyConfig.isIndex() )
        {
            throw XmlConfigParserException( "[Property : " +  propertyConfig.getName()
                    + " filter=\"yes/no\" or analyzer=\"\" required." );
        }
    }



    //add this property setting(alias) to DocumentSchema
    collectionMeta.insertPropertyConfig( propertyConfig );
} // END - CollectionConfig::parseProperty()


// 3.2.1 Display -------------------------------------
void CollectionConfig::parseProperty_Display( const ticpp::Element * display, PropertyConfig & propertyConfig )
{
    int length = 0;
    bool bSnippet = false, bSummary = false;

    getAttribute( display, "length", length );
    getAttribute( display, "snippet", bSnippet, false );
    getAttribute( display, "summary", bSummary, false );

    if( length < 140 || length > 200 ) throw_TypeMismatch(display, "length", length, "140 to 200");


    propertyConfig.setDisplayLength( length );
    propertyConfig.setIsSnippet( bSnippet );
    propertyConfig.setIsSummary( bSummary );


    //==================== OPTIONAL SETTINGS ====================


    Element * settings   = getUniqChildElement( display, "settings", false );
    if ( settings != NULL )
    {
        unsigned int summaryNum = 0;

        if( getAttribute( settings, "summarynum", summaryNum, false ) )
        {
            if( summaryNum < 1 || summaryNum > 100 ) throw_TypeMismatch(settings, "summarynum", summaryNum, "1 to 100");
            propertyConfig.setSummaryNum( summaryNum );
        }
    }
} // END - CollectionConfig::parseProperty_Display()

// 3.2.2 Indexing -------------------------------------
void CollectionConfig::parseProperty_Indexing( const ticpp::Element * indexing, PropertyConfig & propertyConfig )
{
    string alias, analyzer, tokenizers, rankWeightTmp;
    bool bFilter = false;
    bool bMultiValue = false;
    bool bStoreDocLen = true;
    float rankWeight = 0.0f;


    // read XML
    //
    getAttribute( indexing, "alias", alias, false );
    getAttribute( indexing, "analyzer", analyzer, false );
    getAttribute( indexing, "filter", bFilter, false );
    getAttribute( indexing, "multivalue", bMultiValue, false );
    getAttribute( indexing, "doclen", bStoreDocLen, false );
    getAttribute( indexing, "tokenizer", tokenizers, false );
    getAttribute( indexing, "rankweight", rankWeightTmp, false );

    downCase( analyzer );
    downCase( tokenizers );


    // handle exceptions
    //
    if( bFilter == false && analyzer.empty() )
    {
        stringstream msg;
        msg << propertyConfig.getName() << ":  filter=\"yes/no\" or analyzer=\"\" required.";
        throw XmlConfigParserException( msg.str() );
    }

    if( propertyConfig.propertyType_ != STRING_PROPERTY_TYPE && !analyzer.empty() )
    {
        stringstream msg;
        msg << propertyConfig.getName() << "\": \"analyzer\" can only be given to a \"string\" type property.";
        throw XmlConfigParserException( msg.str() );
    }

    if( propertyConfig.propertyType_ != STRING_PROPERTY_TYPE && !tokenizers.empty() )
    {
        stringstream msg;
        msg << propertyConfig.getName() << "\": \"analyzer\" can only be given to a \"string\" type property.";
        throw XmlConfigParserException( msg.str() );
    }

    if( analyzer.empty() && !tokenizers.empty() )
    {
        stringstream msg;
        msg << propertyConfig.getName() << "\": an \"analyzer\" is required as a pair of the tokenizer";
        throw XmlConfigParserException( msg.str() );
    }



    // save la-relate data
    //
    AnalysisInfo analysisInfo;

    if( !analyzer.empty() )
    {
        analysisInfo.analyzerId_ = analyzer;

        if ( (SF1Config::get()->laConfigIdNameMap_.find( analyzer )) == SF1Config::get()->laConfigIdNameMap_.end() )
        {
            stringstream msg;
            msg << "Undefined analyzer configuration id, \"" << analyzer << "\"";
            throw XmlConfigParserException( msg.str() );
        }

        vector<string> tokenizerIdList;
        parseByComma( tokenizers, tokenizerIdList );

        unsigned int i = 0;
        for ( i = 0; i < tokenizerIdList.size(); i++ )
        {
            if ( (SF1Config::get()->tokenizerConfigNameMap_.find( tokenizerIdList[i] )) == SF1Config::get()->tokenizerConfigNameMap_.end() )
            {
                stringstream msg;
                msg << "Undefined tokenizer configuration id, \"" << tokenizerIdList[i] << "\"";
                throw XmlConfigParserException( msg.str() );
            }
            analysisInfo.tokenizerNameList_.insert( tokenizerIdList[i] );
        }
    }

    // save rankweight
    //
    if( !rankWeightTmp.empty() )
    {
        if( rankWeightTmp == "light" )
            rankWeight = RANK_LIGHT;
        else if( rankWeightTmp == "normal" )
            rankWeight = RANK_NORMAL;
        else if( rankWeightTmp == "heavy" )
            rankWeight = RANK_HEAVY;
        else if( rankWeightTmp == "max" )
            rankWeight = RANK_MAX;
        else
            throw_TypeMismatch( indexing, "rankweight", rankWeightTmp );
    }


    if( !validateID(alias) ) throw_TypeMismatch( indexing, "alias", alias, "Alphabets, Numbers, Dot(.), Dash(-) and Underscore(_)");
    // add alias name if any
    if ( !alias.empty() )
        propertyConfig.setName( alias );

    propertyConfig.setIsFilter( bFilter );
    propertyConfig.setIsMultiValue(bMultiValue);
    propertyConfig.setIsStoreDocLen(bStoreDocLen);
    propertyConfig.setAnalysisInfo( analysisInfo );
    propertyConfig.setRankWeight( rankWeight );


    // push to the list of all analysis Information in the configuration file.
    SF1Config::get()->analysisPairList_.insert( analysisInfo );

} // END  CollectionConfig::parseProperty_Indexing()



} // END - namespace sf1r


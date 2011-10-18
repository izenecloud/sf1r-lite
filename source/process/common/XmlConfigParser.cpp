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
#include <la-manager/LAPool.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/shared_ptr.hpp>

#include <glog/logging.h>

#include <iostream>
#include <sstream>

using namespace std;
using namespace izenelib::util::ticpp;

namespace sf1r
{

//------------------------- HELPER FUNCTIONS -------------------------
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
         bf::path schema_file = config_dir/"schema"/"sf1config.xsd";
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

        parseSystemSettings( getUniqChildElement( sf1config, "System" ) );
        parseDeploymentSettings( getUniqChildElement( sf1config, "Deployment" ));
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
  //get resource dir
  getAttribute( getUniqChildElement( system, "Resource" ), "path", resource_dir_);

  getAttribute( getUniqChildElement( system, "LogConnection" ), "str", log_conn_str_);

  parseBundlesDefault(getUniqChildElement( system, "BundlesDefault" ));

  parseFirewall( getUniqChildElement( system, "Firewall" ) );

  parseTokenizer( getUniqChildElement( system, "Tokenizing" ) );

  parseLanguageAnalyzer( getUniqChildElement( system, "LanguageAnalyzer" ) );

}

void SF1Config::parseBrokerAgent( const ticpp::Element * brokerAgent )
{
    getAttribute( brokerAgent, "usecache", brokerAgentConfig_.useCache_, false );
    getAttribute( brokerAgent, "enabletest", brokerAgentConfig_.enableTest_,false );
    getAttribute( brokerAgent, "threadnum", brokerAgentConfig_.threadNum_,false );
    getAttribute( brokerAgent, "port", brokerAgentConfig_.port_,false );
}

void SF1Config::parseDistributedTopology(const ticpp::Element * topology)
{
    if (!topology)
        return;

    getAttribute( topology, "enable", distributedTopologyConfig_.enabled_ );
    getAttribute( topology, "nodenum", distributedTopologyConfig_.nodeNum_ );
    getAttribute( topology, "mirrornum", distributedTopologyConfig_.mirrorNum_ );

    // Current SF1 node
    ticpp::Element * cursf1node = getUniqChildElement( topology, "CurrentNode" );
    getAttribute( cursf1node, "nodeid", distributedTopologyConfig_.curSF1Node_.nodeId_ );
    getAttribute( cursf1node, "mirrorid", distributedTopologyConfig_.curSF1Node_.mirrorId_ );
    getAttribute( cursf1node, "host", distributedTopologyConfig_.curSF1Node_.host_ );
    parseMasterAgent( getUniqChildElement( cursf1node, "MasterAgent", false ) );
    parseWorkerAgent( getUniqChildElement( cursf1node, "WorkerAgent", false ) );
}

void SF1Config::parseDistributedUtil(const ticpp::Element * distributedUtil)
{
    // ZooKeeper configuration
    ticpp::Element* zk = getUniqChildElement( distributedUtil, "ZooKeeper" );
    getAttribute(zk, "servers", distributedUtilConfig_.zkConfig_.zkHosts_);
    getAttribute(zk, "sessiontimeout", distributedUtilConfig_.zkConfig_.zkRecvTimeout_);

    // Distributed File System configuration
    ticpp::Element* dfs = getUniqChildElement( distributedUtil, "DFS" );
    getAttribute(dfs, "type", distributedUtilConfig_.dfsConfig_.type_, false);
    getAttribute(dfs, "supportfuse", distributedUtilConfig_.dfsConfig_.isSupportFuse_, false);
    getAttribute(dfs, "mountdir", distributedUtilConfig_.dfsConfig_.mountDir_, true);
    getAttribute(dfs, "server", distributedUtilConfig_.dfsConfig_.server_, false);
    getAttribute(dfs, "port", distributedUtilConfig_.dfsConfig_.port_, false);
}

void SF1Config::parseMasterAgent( const ticpp::Element * master )
{
    if (!master)
        return;

    MasterAgentConfig& masterAgent = distributedTopologyConfig_.curSF1Node_.masterAgent_;

    getAttribute(master, "enable", masterAgent.enabled_);
    getAttribute(master, "port", masterAgent.port_);
    //getAttribute(master, "enablelocalworker", masterAgent.aggregatorConfig_.enableLocalWorker_);

    Iterator<Element> aggregator_it( "Aggregator" );
    for (aggregator_it = aggregator_it.begin(master); aggregator_it != aggregator_it.end(); aggregator_it++)
    {
        AggregatorUnit aggregatorUnit;
        getAttribute(aggregator_it.Get(), "name", aggregatorUnit.name_);
        downCase(aggregatorUnit.name_);
        masterAgent.addAggregatorConfig(aggregatorUnit);
    }

    // todo, remove
    masterAgent.aggregatorConfig_.enableLocalWorker_ = true;
    Iterator<Element> worker_it( "Worker" );
    for (worker_it = worker_it.begin(master); worker_it != worker_it.end(); worker_it++)
    {
        std::string host;
        int port;
        getAttribute(worker_it.Get(), "host", host, true);
        getAttribute(worker_it.Get(), "port", port, true);
        masterAgent.aggregatorConfig_.addWorker(host, static_cast<uint16_t>(port));
    }
}

void SF1Config::parseWorkerAgent( const ticpp::Element * worker )
{
    if (!worker)
        return;

    WorkerAgentConfig& workerAgent = distributedTopologyConfig_.curSF1Node_.workerAgent_;

    getAttribute(worker, "enable", workerAgent.enabled_);
    getAttribute(worker, "port", workerAgent.port_);

    Iterator<Element> aggregator_it( "Service" );
    for (aggregator_it = aggregator_it.begin(worker); aggregator_it != aggregator_it.end(); aggregator_it++)
    {
        ServiceUnit serviceUnit;
        getAttribute(aggregator_it.Get(), "name", serviceUnit.name_);
        downCase(serviceUnit.name_);
        workerAgent.addServiceUnit(serviceUnit);
    }

    // todo, remove
    ticpp::Element* master = getUniqChildElement( worker, "Master", false );
    if (master)
    {
        getAttribute(master, "host", workerAgent.masterHost_);
        getAttribute(master, "port", workerAgent.masterPort_);
    }
}

void SF1Config::parseBundlesDefault(const ticpp::Element * bundles)
{
    Element * bundle = NULL;
    bundle = getUniqChildElement( bundles, "IndexBundle" );
    defaultIndexBundleParam_.LoadXML(getUniqChildElement( bundle, "Parameter" ), false);
    bundle = getUniqChildElement( bundles, "ProductBundle" );
    defaultProductBundleParam_.LoadXML(getUniqChildElement( bundle, "Parameter" ), false);
    bundle = getUniqChildElement( bundles, "MiningBundle" );
    defaultMiningBundleParam_.LoadXML(getUniqChildElement( bundle, "Parameter" ), false);
    bundle = getUniqChildElement( bundles, "RecommendBundle" );
    defaultRecommendBundleParam_.LoadXML(getUniqChildElement( bundle, "Parameter" ), false);
}

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


void SF1Config::parseLanguageAnalyzer( const ticpp::Element * languageAnalyzer )
{
    // 1. <LanguageAnalyzer>
    string dictionaryPath;

    getAttribute( languageAnalyzer, "dictionarypath", dictionaryPath );

    unsigned int  updateDictInterval;

    getAttribute( languageAnalyzer, "updatedictinterval", updateDictInterval );

    laManagerConfig_.updateDictInterval_ = updateDictInterval;
    std::cout<<"set update LA dictionary interval: "<<updateDictInterval<<std::endl;

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
            string advOption;
            getAttribute( analyzer_it.Get(), "advoption", advOption, false);
            laUnit.setAdvOption( advOption );
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
#ifdef USE_IZENEJMA
                || analysis == "japanese"
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
                    if(analysis == "chinese")
                    {
                        LAPool::getInstance()->set_cma_path(dictionaryPath_inner );
                        std::cout<<"set_cma_path : "<<dictionaryPath_inner<<std::endl;
                    }
                    else if(analysis == "japanese")
                    {
                        LAPool::getInstance()->set_jma_path(dictionaryPath_inner );
                        std::cout<<"set_jma_path : "<<dictionaryPath_inner<<std::endl;
                    }
                }
                else if ( !dictionaryPath.empty() && dictionaryPath_inner.empty() )
                {
                    laUnit.setDictionaryPath( dictionaryPath );
                    if(analysis == "chinese")
                    {
                        LAPool::getInstance()->set_cma_path(dictionaryPath );
                        std::cout<<"set_cma_path : "<<dictionaryPath<<std::endl;
                    }
                    else if(analysis == "japanese")
                    {
                        LAPool::getInstance()->set_jma_path(dictionaryPath_inner );
                        std::cout<<"set_jma_path : "<<dictionaryPath_inner<<std::endl;
                    }
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
#ifdef USE_IZENEJMA
                << "\"japanese\", "
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

void SF1Config::parseDeploymentSettings(const ticpp::Element * deploy)
{
    parseBrokerAgent( getUniqChildElement( deploy, "BrokerAgent" ) );

    parseDistributedTopology( getUniqChildElement( deploy, "DistributedTopology", false ) );

    parseDistributedUtil( getUniqChildElement( deploy, "DistributedUtil" ) );
}

// ------------------------- CollectionConfig-------------------------
CollectionConfig::CollectionConfig()
{
}

CollectionConfig::~CollectionConfig()
{
}

bool CollectionConfig::parseConfigFile( const string& collectionName ,const string & fileName, CollectionMeta& collectionMeta) throw( XmlConfigParserException  )
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
         bf::path schema_file = config_dir/"schema"/"collection.xsd";
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

        collectionMeta.setName(collectionName);
        parseCollectionSettings( collection, collectionMeta );

        SF1Config::get()->collectionMetaMap_.insert(std::make_pair( collectionMeta.getName(), collectionMeta));
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
}

void CollectionConfig::parseCollectionSettings( const ticpp::Element * collection, CollectionMeta & collectionMeta )
{
    parseCollectionPath( getUniqChildElement( collection, "Path" ), collectionMeta );

    parseCollectionSchema( getUniqChildElement( collection, "DocumentSchema" ), collectionMeta );

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

    // IndexBundle
    Element* indexBundle = getUniqChildElement( collection, "IndexBundle", false  );
    Element* indexParam = getUniqChildElement( indexBundle, "Parameter", false );
    parseIndexBundleParam(indexParam, collectionMeta);
    parseIndexEcSchema(getUniqChildElement( indexBundle, "EcSchema", false), collectionMeta);
    parseIndexBundleSchema(getUniqChildElement( indexBundle, "Schema" ), collectionMeta);

    // ProductBundle
    Element* productBundle = getUniqChildElement( collection, "ProductBundle", false  );
    if(productBundle)
    {
        Element* productParam = getUniqChildElement( productBundle, "Parameter", false );
        parseProductBundleParam(productParam, collectionMeta);
        
        Element* product_schema = getUniqChildElement( productBundle, "Schema", false );
        if(product_schema) parseProductBundleSchema(product_schema, collectionMeta);
    }
    // MiningBundle
    collectionMeta.miningBundleConfig_->isSupportByAggregator_ = collectionMeta.indexBundleConfig_->isSupportByAggregator_;
    Element* miningBundle = getUniqChildElement( collection, "MiningBundle" , false );
    if(miningBundle)
    {
        Element* miningSchema = getUniqChildElement( miningBundle, "Schema", false  );
        if(miningSchema) parseMiningBundleSchema(miningSchema, collectionMeta);
        Element* miningParam = getUniqChildElement( miningBundle, "Parameter", false  );
        parseMiningBundleParam(miningParam, collectionMeta);
    }

    // RecommendBundle
    Element* recommendBundle = getUniqChildElement( collection, "RecommendBundle" , false );
    Element* recommendParam = NULL;
    if(recommendBundle)
    {
        Element* recommendSchema = getUniqChildElement( recommendBundle, "Schema", false  );
        if(recommendSchema) parseRecommendBundleSchema(recommendSchema, collectionMeta);
        recommendParam = getUniqChildElement( recommendBundle, "Parameter", false  );
    }
    // the <Parameter> in <Collection> would overwrite that in <BundlesDefault>
    parseRecommendBundleParam(recommendParam, collectionMeta);
}


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

    collectionMeta.setCollectionPath( collPath );
    collectionMeta.indexBundleConfig_->collPath_ = collPath;
    collectionMeta.miningBundleConfig_->collPath_ = collPath;
    collectionMeta.recommendBundleConfig_->collPath_ = collPath;
}

void CollectionConfig::parseCollectionSchema( const ticpp::Element * documentSchema, CollectionMeta & collectionMeta )
{
    //used to check duplicate property names
    set<string> propertyNameList;

    Iterator<Element> property( "Property" );

    bool hasDate = false;

    for ( property = property.begin( documentSchema ); property != property.end(); property++ )
    {
        try
        {
            //holds all the configuration data of this property
            PropertyConfigBase propertyConfig;

            string propertyName, type;
            PropertyDataType dataType = UNKNOWN_DATA_PROPERTY_TYPE;

            getAttribute( property.Get(), "name", propertyName );
            getAttribute( property.Get(), "type", type );

            downCase( type );

            if( !validateID(propertyName) )
                throw XmlConfigParserException( "Alphabets, Numbers, Dot(.), Dash(-) and Underscore(_)");
            if ( (( propertyNameList.insert(propertyName) ).second) == false )
            {
                throw XmlConfigParserException( "Duplicate property names" );
            }

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
                throw_TypeMismatch( property.Get(), "name", type, "\"string\", \"int\" or \"float\"" );
            }
            propertyConfig.propertyName_ = propertyName;
            propertyConfig.propertyType_ = dataType;
            collectionMeta.insertProperty( propertyConfig );

            boost::to_lower(propertyName);
            if(propertyName == "date" )
                hasDate = true;
         }
         catch ( XmlConfigParserException & e )
         {
              throw e;
          }
     } //property iteration

    if( hasDate == false )
    {
	PropertyConfigBase propertyConfig;
	propertyConfig.propertyName_ = "DATE";
	propertyConfig.propertyType_ = STRING_PROPERTY_TYPE;
	collectionMeta.insertProperty( propertyConfig );
    }

    ///we set property Id here
    collectionMeta.numberProperty();
}

void CollectionConfig::parseIndexBundleParam(const ticpp::Element * index, CollectionMeta & collectionMeta)
{
    CollectionParameterConfig params(SF1Config::get()->defaultIndexBundleParam_);
    if(index) params.LoadXML(index, true);

    std::string encoding;

    params.GetString("Sia/encoding", encoding);
    downCase(encoding);
    IndexBundleConfiguration& indexBundleConfig = *collectionMeta.indexBundleConfig_;
    indexBundleConfig.encoding_ = parseEncodingType(encoding);
    params.GetString("Sia/wildcardtype", indexBundleConfig.wildcardType_, "unigram");
    params.Get("Sia/indexunigramproperty", indexBundleConfig.bIndexUnigramProperty_);
    params.Get("Sia/unigramsearchmode", indexBundleConfig.bUnigramSearchMode_);
    std::string multilangGranualarity;
    params.GetString("Sia/multilanggranularity", multilangGranualarity, "field");
    indexBundleConfig.setIndexMultiLangGranularity(multilangGranualarity);

    ///TODO for ranking
    ///ranking is not configed right now, although we keep such interface here
    RankingConfigUnit& rankingUnit = indexBundleConfig.rankingManagerConfig_.rankingConfigUnit_;
    TextRankingType e_qd_text_model = RankingType::BM25;
    rankingUnit.setTextRankingModel( e_qd_text_model );

    //Index strategy
    izenelib::ir::indexmanager::IndexManagerConfig& indexmanager_config = indexBundleConfig.indexConfig_;

    int64_t memorypool = 0;
    params.Get<int64_t>("IndexStrategy/memorypoolsize", memorypool);
    indexmanager_config.indexStrategy_.memory_ = memorypool;
    indexmanager_config.indexStrategy_.indexDocLength_ = true;
    indexmanager_config.indexStrategy_.skipInterval_ = 8;
    indexmanager_config.indexStrategy_.maxSkipLevel_ = 3;
    indexmanager_config.storeStrategy_.param_ = "file";
    params.GetString("IndexStrategy/indexlevel", indexmanager_config.indexStrategy_.indexLevel_, "wordlevel");
    params.GetString("IndexStrategy/indexpolicy", indexmanager_config.indexStrategy_.indexMode_, "default");
    std::string indexMergePolicy;
    params.GetString("IndexStrategy/mergepolicy",indexMergePolicy, "file");
    if(indexMergePolicy == "memory")
        indexmanager_config.mergeStrategy_.requireIntermediateFileForMerging_ = false;
    else
        indexmanager_config.mergeStrategy_.requireIntermediateFileForMerging_ = true;
    indexmanager_config.mergeStrategy_.param_ = "dbt";
    params.GetString("IndexStrategy/cron", indexBundleConfig.cronIndexer_, "");

    std::set<std::string> directories;
    params.Get("CollectionDataDirectory", directories);

    if(!directories.empty())
    {
        indexmanager_config.indexStrategy_.indexLocations_.assign(directories.begin(), directories.end());
        indexBundleConfig.collectionDataDirectories_.assign(directories.begin(), directories.end());
    }

    params.Get("Sia/triggerqa", indexBundleConfig.bTriggerQA_);
    params.Get<std::size_t>("Sia/doccachenum", indexBundleConfig.documentCacheNum_);
    params.Get<std::size_t>("Sia/searchcachenum", indexBundleConfig.searchCacheNum_);
    params.Get<std::size_t>("Sia/filtercachenum", indexBundleConfig.filterCacheNum_);
    params.Get<std::size_t>("Sia/mastersearchcachenum", indexBundleConfig.masterSearchCacheNum_);
    params.Get<std::size_t>("Sia/topknum", indexBundleConfig.topKNum_);
    params.GetString("LanguageIdentifier/dbpath", indexBundleConfig.languageIdentifierDbPath_, "");

    indexBundleConfig.isSupportByAggregator_ = SF1Config::get()->checkAggregatorSupport(collectionMeta.getName());
    indexBundleConfig.setAggregatorConfig(SF1Config::get()->getAggregatorConfig());
}

void CollectionConfig::parseIndexEcSchema(const ticpp::Element * indexEcSchema, CollectionMeta & collectionMeta)
{
    IndexBundleConfiguration& indexBundleConfig = *(collectionMeta.indexBundleConfig_);
    PropertyDataType property_type;
    if(indexEcSchema != NULL)
    {
        ticpp::Element * child_node = getUniqChildElement( indexEcSchema, "ProductSourceField", false );
        std::string property_name;
        if( child_node != NULL )
        {
          Iterator<Element> it( "Property" );
          for ( it = it.begin( child_node ); it != it.end(); it++ )
          {
              getAttribute( it.Get(), "name", property_name );
              bool gottype = collectionMeta.getPropertyType(property_name, property_type);
              if( !gottype)
              {
                throw XmlConfigParserException("Property ["+property_name+"] used in ProductSourceField is not in DocumentSchema.");
              }
              indexBundleConfig.productSourceField_ = property_name;
          }
        }
    }
}

void CollectionConfig::parseIndexBundleSchema(const ticpp::Element * indexSchema, CollectionMeta & collectionMeta)
{
    IndexBundleConfiguration& indexBundleConfig = *(collectionMeta.indexBundleConfig_);
    indexBundleConfig.setSchema(collectionMeta.schema_);

    Iterator<Element> property( "Property" );

    for ( property = property.begin( indexSchema ); property != property.end(); property++ )
    {
	try
	{
	    parseIndexSchemaProperty( property.Get(), collectionMeta);
	}
	catch ( XmlConfigParserException & e )
	{
	    throw e;
	}
    }

    ///we update property Id here
    ///It is because that IndexBundle might add properties "xx_unigram", in this case, we must keep the propertyId consistent
    ///between IndexBundle and other bundles
    ///collectionMeta.numberProperty();
    indexBundleConfig.numberProperty();

    // map<property name, weight>
    std::map<string, float>& propertyWeightMap = collectionMeta.indexBundleConfig_->rankingManagerConfig_.propertyWeightMapByProperty_;
    std::set<PropertyConfig, PropertyComp>& schema = collectionMeta.indexBundleConfig_->schema_;
    std::set<PropertyConfig, PropertyComp>::iterator it = schema.begin();

    for (; it != schema.end(); it++ )
    {
        if ( it->getRankWeight() >= 0.0f )
        {
            propertyWeightMap.insert( pair<string, float>(it->getName(), it->getRankWeight()) );
        }
    }
}

void CollectionConfig::parseProductBundleParam(const ticpp::Element * product_param, CollectionMeta & collectionMeta)
{
    CollectionParameterConfig params(SF1Config::get()->defaultProductBundleParam_);
    if(product_param)
    {
        params.LoadXML(product_param, true);
    }

    std::set<std::string> directories;
    params.Get("CollectionDataDirectory", directories);
    if(!directories.empty())
    {
        collectionMeta.productBundleConfig_->collectionDataDirectories_.assign(directories.begin(), directories.end());
    }
}

void CollectionConfig::parseProductBundleSchema(const ticpp::Element * product_schema, CollectionMeta & collectionMeta)
{
    ProductBundleConfiguration& productBundleConfig = *(collectionMeta.productBundleConfig_);
    productBundleConfig.enabled_ = true;
    productBundleConfig.collPath_ = collectionMeta.collPath_;
    productBundleConfig.setSchema(collectionMeta.schema_);
    std::string property_name;
    ticpp::Element* property_node = 0;
    property_node = getUniqChildElement( product_schema, "PriceProperty", false );
    getAttribute(property_node, "name", productBundleConfig.pm_config_.price_property_name );
    
    property_node = getUniqChildElement( product_schema, "DOCIDProperty", false );
    getAttribute(property_node, "name", productBundleConfig.pm_config_.docid_property_name );
    
    property_node = getUniqChildElement( product_schema, "UuidProperty", false );
    getAttribute(property_node, "name", productBundleConfig.pm_config_.uuid_property_name );
    
    property_node = getUniqChildElement( product_schema, "ItemCountProperty", false );
    getAttribute(property_node, "name", productBundleConfig.pm_config_.itemcount_property_name );
    
}

void CollectionConfig::parseMiningBundleParam(const ticpp::Element * mining, CollectionMeta & collectionMeta)
{
    CollectionParameterConfig params(SF1Config::get()->defaultMiningBundleParam_);
    if(mining) params.LoadXML(mining, true);
    // PARSING MINING CONFIG
    MiningConfig& mining_config = collectionMeta.miningBundleConfig_->mining_config_;
    //for TG
    params.Get<uint32_t>("TaxonomyPara/topdocnum", mining_config.taxonomy_param.top_docnum);
    params.Get<uint32_t>("TaxonomyPara/levels", mining_config.taxonomy_param.levels);
    params.Get<uint32_t>("TaxonomyPara/perlevelnum", mining_config.taxonomy_param.per_levelnum);
    params.Get<uint32_t>("TaxonomyPara/candlabelnum", mining_config.taxonomy_param.candidate_labelnum);
    params.Get("TaxonomyPara/enablenec", mining_config.taxonomy_param.enable_nec);
    params.Get<uint32_t>("TaxonomyPara/maxpeopnum", mining_config.taxonomy_param.max_peopnum);
    params.Get<uint32_t>("TaxonomyPara/maxlocnum", mining_config.taxonomy_param.max_locnum);
    params.Get<uint32_t>("TaxonomyPara/maxorgnum", mining_config.taxonomy_param.max_orgnum);
    //for recommend
    params.Get<uint32_t>("RecommendPara/recommendnum", mining_config.recommend_param.recommend_num);
    params.Get<uint32_t>("RecommendPara/popularnum", mining_config.recommend_param.popular_num);
    params.Get<uint32_t>("RecommendPara/realtimenum", mining_config.recommend_param.realtime_num);
    //for similarity
    params.Get<uint32_t>("SimilarityPara/docnumlimit", mining_config.similarity_param.docnum_limit);
    params.Get<uint32_t>("SimilarityPara/termnumlimit", mining_config.similarity_param.termnum_limit);
    mining_config.similarity_param.enable_esa = false; // default value
    params.Get("SimilarityPara/enableesa", mining_config.similarity_param.enable_esa);
    //for DC
    params.Get("ClassificationPara/customizetraining", mining_config.dc_param.customize_training);
    std::string encoding_str;
    downCase(encoding_str);
    params.GetString("ClassificationPara/trainingencoding", encoding_str);
    mining_config.dc_param.encoding_type = parseEncodingType(encoding_str);

    static bool QueryCorrectionInitiated = false;

    if (!QueryCorrectionInitiated)
    {
        //for query correction
        params.GetString("QueryCorrectionPara/basepath", MiningConfig::query_correction_param.base_path);
        params.Get("QueryCorrectionPara/enableEK", MiningConfig::query_correction_param.enableEK);
        params.Get("QueryCorrectionPara/enableCN", MiningConfig::query_correction_param.enableCN);
        //for query log
        params.Get<uint32_t>("QueryLogPara/updatetime", MiningConfig::query_log_param.update_time);
        params.Get<uint32_t>("QueryLogPara/logdays", MiningConfig::query_log_param.log_days);
        params.GetString("QueryLogPara/cron", MiningConfig::query_log_param.cron);

        QueryCorrectionInitiated = true;
    }

    std::set<std::string> directories;
    params.Get("CollectionDataDirectory", directories);

    if(!directories.empty())
    {
        collectionMeta.miningBundleConfig_->collectionDataDirectories_.assign(directories.begin(), directories.end());
    }
    collectionMeta.miningBundleConfig_->system_resource_path_ = SF1Config::get()->getResourceDir();

}

void CollectionConfig::parseMiningBundleSchema(const ticpp::Element * mining_schema_node, CollectionMeta & collectionMeta)
{
    //** PARSE MINING SCHEMA BEGIN
    MiningBundleConfiguration& miningBundleConfig = *(collectionMeta.miningBundleConfig_);
    miningBundleConfig.schema_ = collectionMeta.schema_;

    MiningSchema& mining_schema = miningBundleConfig.mining_schema_;
    PropertyDataType property_type;
    if( mining_schema_node!= NULL )
    {
      ticpp::Element * task_node = getUniqChildElement( mining_schema_node, "TaxonomyGeneration", false );
      mining_schema.tg_enable = false;
      mining_schema.tg_kpe_only = false;
      std::string property_name;
      if( task_node!= NULL )
      {
        bool kpe_only = false;
        if( getAttribute( task_node, "kpe_only", kpe_only, false ) )
        {
            mining_schema.tg_kpe_only = kpe_only;
        }
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
                  throw XmlConfigParserException("Property ["+property_name+"] used in Faceted is not string type.");
              }
              mining_schema.faceted_properties.push_back(property_name);
          }
          mining_schema.faceted_enable = true;
      }

        task_node = getUniqChildElement(mining_schema_node, "Group", false);
        mining_schema.group_enable = false;
        if (task_node != NULL)
        {
            Iterator<Element> it("Property");
            for (it = it.begin(task_node); it != it.end(); ++it)
            {
                getAttribute(it.Get(), "name", property_name);
                bool gottype = collectionMeta.getPropertyType(property_name, property_type);
                if( !gottype )
                {
                    throw XmlConfigParserException("The type of property ["+property_name+"] in <Group> is unknown.");
                }

                GroupConfig groupConfig(property_name, property_type);

                if ( groupConfig.isNumericType() )
                {
                    const std::set<PropertyConfig, PropertyComp>& indexSchema = collectionMeta.indexBundleConfig_->schema_;
                    PropertyConfig p;
                    p.setName(property_name);
                    std::set<PropertyConfig, PropertyComp>::const_iterator propIt = indexSchema.find(p);
                    if (propIt == indexSchema.end()
                        || !propIt->isIndex() || !propIt->getIsFilter())
                    {
                        throw XmlConfigParserException("As property ["+property_name+"] in <Group> is int or float type, "
                                                       "it needs to be configured as a filter property like below:\n"
                                                       "<IndexBundle> <Schema> <Property name=\"Price\"> <Indexing filter=\"yes\" ...");
                    }
                }
                else if( !groupConfig.isStringType() )
                {
                    throw XmlConfigParserException("Property ["+property_name+"] in <Group> is not string, int or float type.");
                }

                mining_schema.group_properties.push_back(groupConfig);

                LOG(INFO) << "group property: " << property_name
                          << ", type: " << property_type;
            }
            mining_schema.group_enable = true;
        }

        task_node = getUniqChildElement(mining_schema_node, "Attr", false);
        mining_schema.attr_enable = false;
        if (task_node!= NULL)
        {
            int propNum = 0;
            Iterator<Element> it("Property");
            for (it = it.begin(task_node); it != it.end(); ++it)
            {
                if (++propNum > 1)
                {
                    throw XmlConfigParserException("in <Attr> Config, at most one <Property> is allowed.");
                }

                getAttribute(it.Get(), "name", property_name);
                bool gottype = collectionMeta.getPropertyType(property_name, property_type);
                if( !gottype || property_type != STRING_PROPERTY_TYPE)
                {
                    throw XmlConfigParserException("<Property> ["+property_name+"] in <Attr> is not string type.");
                }
                mining_schema.attr_property.propName = property_name;
                mining_schema.attr_enable = true;
            }
        }

      task_node = getUniqChildElement( mining_schema_node, "Rerank", false );
      mining_schema.property_rerank_enable = false;
      if( task_node!= NULL )
      {
          int propNum = 0;
          Iterator<Element> it( "Property" );
          for ( it = it.begin( task_node ); it != it.end(); it++ )
          {
              getAttribute( it.Get(), "name", property_name );
              std::vector<GroupConfig>& group_properties = mining_schema.group_properties;
              bool gottype = false;
              for(std::vector<GroupConfig>::iterator git = group_properties.begin(); git != group_properties.end(); ++git)
              	{
              	    if(git->propName == property_name)
              	    {
              	        gottype = true;
                       break;
              	    }
              	}
              if( !gottype )
              {
                  throw XmlConfigParserException("<Property> ["+property_name+"] in <Rerank> is not configured in <Group>.");
              }
              mining_schema.prop_rerank_property.propName = property_name;
              mining_schema.property_rerank_enable = true;
          }
          propNum = 0;
          Iterator<Element> bit( "Boosting" );
          for ( bit = bit.begin( task_node ); bit != bit.end(); bit++ )
          {
              getAttribute( bit.Get(), "name", property_name );
              std::vector<GroupConfig>& group_properties = mining_schema.group_properties;
              bool gottype = false;
              for(std::vector<GroupConfig>::iterator git = group_properties.begin(); git != group_properties.end(); ++git)
              	{
              	    if(git->propName == property_name)
              	    {
              	        gottype = true;
                       break;
              	    }
              	}
              if( !gottype )
              {
                  throw XmlConfigParserException("<Boosting> ["+property_name+"] in <Rerank> is not string type.");
              }
              mining_schema.prop_rerank_property.boostingPropName = property_name;
          }
      }

      task_node = getUniqChildElement( mining_schema_node, "TDT", false );
      mining_schema.tdt_enable = false;
      if( task_node!= NULL )
      {
          mining_schema.tdt_enable = true;
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

}

void CollectionConfig::parseRecommendBundleParam(const ticpp::Element * recParamNode, CollectionMeta & collectionMeta)
{
    CollectionParameterConfig params(SF1Config::get()->defaultRecommendBundleParam_);
    if(recParamNode)
    {
        params.LoadXML(recParamNode, true);
    }

    boost::shared_ptr<RecommendBundleConfiguration> recBundleConfig = collectionMeta.recommendBundleConfig_;
    std::set<std::string> directories;
    params.Get("CollectionDataDirectory", directories);
    if(directories.empty())
    {
        throw XmlConfigParserException("Require configure <RecommendBundle> <Parameter> <CollectionDataDirectory>");
    }
    else
    {
        recBundleConfig->collectionDataDirectories_.assign(directories.begin(), directories.end());
    }

    params.Get("CronPara/value", recBundleConfig->cronStr_);
    params.Get("CacheSize/purchase", recBundleConfig->purchaseCacheSize_);
    params.Get("CacheSize/visit", recBundleConfig->visitCacheSize_);
    params.Get("CacheSize/index", recBundleConfig->indexCacheSize_);
    params.Get("FreqItemSet/enable", recBundleConfig->freqItemSetEnable_);
    params.Get("FreqItemSet/minfreq", recBundleConfig->itemSetMinFreq_);

    LOG(INFO) << "RecommendBundle [CronPara] value: " << recBundleConfig->cronStr_;
    LOG(INFO) << "RecommendBundle [CacheSize] purchase: " << recBundleConfig->purchaseCacheSize_;
    LOG(INFO) << "RecommendBundle [CacheSize] visit: " << recBundleConfig->visitCacheSize_;
    LOG(INFO) << "RecommendBundle [CacheSize] index: " << recBundleConfig->indexCacheSize_;
    LOG(INFO) << "RecommendBundle [FreqItemSet] enable: " << recBundleConfig->freqItemSetEnable_;
    LOG(INFO) << "RecommendBundle [FreqItemSet] min freq: " << recBundleConfig->itemSetMinFreq_;
}

void CollectionConfig::parseRecommendBundleSchema(const ticpp::Element * recSchemaNode, CollectionMeta & collectionMeta)
{
    if (recSchemaNode == NULL)
        return;

    //** PARSE RECOMMEND SCHEMA BEGIN
    RecommendBundleConfiguration& recommendBundleConfig = *(collectionMeta.recommendBundleConfig_);
    RecommendSchema& recommendSchema = recommendBundleConfig.recommendSchema_;

    // get user schema
    Element* userSchemaNode = getUniqChildElement( recSchemaNode, "User", false  );
    if (userSchemaNode)
    {
        Iterator<Element> property("Property");

        for (property = property.begin(userSchemaNode); property != property.end(); ++property)
        {
            try
            {
                string propName;
                getAttribute(property.Get(), "name", propName);

                // ignore default property
                if (propName != "USERID")
                {
                    RecommendProperty recommendProperty;
                    recommendProperty.propertyName_ = propName;
                    recommendSchema.userSchema_.push_back(recommendProperty);
                }
            }
            catch ( XmlConfigParserException & e )
            {
                throw e;
            }
        } //property iteration
    }

    // get item schema
    Element* itemSchemaNode = getUniqChildElement( recSchemaNode, "Item", false  );
    if (itemSchemaNode)
    {
        Iterator<Element> property("Property");

        for (property = property.begin(itemSchemaNode); property != property.end(); ++property)
        {
            try
            {
                string propName;
                getAttribute(property.Get(), "name", propName);

                // ignore default property
                if (propName != "ITEMID")
                {
                    RecommendProperty recommendProperty;
                    recommendProperty.propertyName_ = propName;
                    recommendSchema.itemSchema_.push_back(recommendProperty);
                }
            }
            catch ( XmlConfigParserException & e )
            {
                throw e;
            }
        } //property iteration
    }

    // get track schema
    Element* trackSchemaNode = getUniqChildElement( recSchemaNode, "Track", false  );
    if (trackSchemaNode)
    {
        Iterator<Element> event("Event");

        for (event = event.begin(trackSchemaNode); event != event.end(); ++event)
        {
            try
            {
                string eventName;
                getAttribute(event.Get(), "name", eventName);
                recommendSchema.eventSet_.insert(eventName);
            }
            catch ( XmlConfigParserException & e )
            {
                throw e;
            }
        } //event iteration
    }
}

void CollectionConfig::parseIndexSchemaProperty(
        const ticpp::Element * property,
        CollectionMeta & collectionMeta)
{
    std::set<PropertyConfig, PropertyComp>& indexSchema = collectionMeta.indexBundleConfig_->schema_;

    //holds all the configuration data of this property

    string propertyName;

    getAttribute( property, "name", propertyName );

    string pName = propertyName;
    boost::to_lower(pName);
    if((pName == "date" )||(pName == "docid"))
    {
        stringstream message;
        message << "DATE/DOCID are inherent properties and should not exist within IndexBundleSchema";
        throw XmlConfigParserException( message.str() );
    }

    PropertyConfig p;
    p.setName(propertyName);

    std::set<PropertyConfig, PropertyComp>::iterator sp = indexSchema.find(p);
    PropertyConfig propertyConfig(*sp);
    indexSchema.erase(*sp);

    //set data --------------------------------------
    propertyConfig.setOriginalName( propertyName );
    propertyConfig.setIsIndex( true );

    //--- parse optional settings for "Property"

    //================== DISPLAY ==================

    Element * display = getUniqChildElement( property, "Display", false );

    // the setting is optional
    if ( display != NULL )
    {
        PropertyDataType dataType;
        collectionMeta.getPropertyType(propertyName, dataType);
        if( dataType != STRING_PROPERTY_TYPE )
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

            if ( propertyConfig.getName() == propertyConfig.getOriginalName() )
            {
                nOriginalName++;
            }

            //add this property setting(alias) to DocumentSchema
            indexSchema.insert( propertyConfig );
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

            if ( collectionMeta.indexBundleConfig_->bIndexUnigramProperty_ )
            {
                propertyConfig.setName(propertyName+"_unigram");
                propertyConfig.setAnalysisInfo(analysisInfo);
                propertyConfig.setIsFilter(false);
                indexSchema.insert(propertyConfig );

                /// Attention
                /// update CollectionMeta
                /// xxx, Alais-ed property no need to be insert into collectionMeta
            }
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
    else
    {
        if( propertyConfig.isIndex() )
        {
            throw XmlConfigParserException( "[Property : " +  propertyConfig.getName()
                    + " filter=\"yes/no\" or analyzer=\"\" required." );
        }
    }



    //add this property setting(alias) to DocumentSchema
    indexSchema.insert( propertyConfig );
}

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
    string alias, analyzer, tokenizers;
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
    getAttribute_FloatType( indexing, "rankweight", rankWeight, false );

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

    // xxx, "la_sia_with_unigram" used for indexing, add "la_sia" for searching
    if (analyzer == "la_sia_with_unigram")
    {
        analysisInfo.analyzerId_ = "la_sia";
    }
    SF1Config::get()->analysisPairList_.insert( analysisInfo );
}


} // END - namespace sf1r


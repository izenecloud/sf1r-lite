#include "TestResources.h"
#include "test_def.h"

using namespace std;
using namespace sf1r;
using namespace la;

#ifdef USE_IZENECMA
std::string getCmaKnowledgePath()
{
    return IZENECMA_KNOWLEDGE;
}
#endif


#ifdef USE_IZENECMA
    string CMA_KNOWLEDGE = getCmaKnowledgePath();
#endif

void initConfig( std::vector<AnalysisInfo>& analysisList, sf1r::LAManagerConfig & config )
{
    //======= 1. Declaring the analyzer & tokenizer instances
    
    // --------- Update interval
    config.updateDictInterval_ = 600;

    // --------- Analyers
    
    LAConfigUnit laConfig;
    TokenizerConfigUnit toknConfig;

    AnalysisInfo analysisInfo;          //dummy
    // --------- Tokenzer


    {
            // 1. set Tokenizer config
            toknConfig.id_          = "tok_unite";
            toknConfig.method_      = "unite";
            toknConfig.value_       = "-";
            //toknConfig.code_     = "0x2d";   //'-'
            //toknConfig.code_     = "";
            config.addTokenizerConfig( toknConfig );        // need to change name to Tokenizer

            // 2. add to pair list
            analysisInfo.tokenizerNameList_.insert( "tok_unite" );

            // 1. set Tokenizer config
            toknConfig.id_       = "tok_divide";
            toknConfig.method_   = "divide";
            toknConfig.value_    = "/";
            config.addTokenizerConfig( toknConfig );        // need to change name to Tokenizer

            // 2. add to pair list
            analysisInfo.tokenizerNameList_.insert( "tok_divide" );

            // 1. set Tokenizer config
            toknConfig.id_       = "tok_allow";
            toknConfig.method_   = "allow";
            toknConfig.value_    = "$";
            config.addTokenizerConfig( toknConfig );        // need to change name to Tokenizer

            // 2. add to pair list
            analysisInfo.tokenizerNameList_.insert( "tok_allow" );

            // 1. set Tokenizer config
            config.addTokenizerConfig( toknConfig );        // need to change name to Tokenizer
            
            // 2. add to pair list
            analysisInfo.tokenizerNameList_.insert( "" );

            // 1. set Tokenizer config
            toknConfig.id_       = "tok_allow_ex";
            toknConfig.method_   = "allow";
            toknConfig.value_    = "|";
            config.addTokenizerConfig( toknConfig );        // need to change name to Tokenizer

            // 2. add to pair list
            analysisInfo.tokenizerNameList_.insert( "tok_allow_ex" );
    }

    config.setAnalysisPairList( analysisList );
}


void downcase( std::string & str )
{
    for(size_t i = 0; i < str.length(); i++ )
    {
        str[i] = tolower( str[i] );
    }
}

void initLAs( sf1r::LAManagerConfig & config )
{
    if( sf1r::LAPool::getInstance()->init( config ) == false )
    {
        throw std::runtime_error( "error initializing LAPool" );
    }
}

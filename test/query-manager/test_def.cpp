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


string KMA_KNOWLEDGE = getKmaKnowledgePath();
#ifdef USE_IZENECMA
    string CMA_KNOWLEDGE = getCmaKnowledgePath();
#endif

void initConfig( std::vector<AnalysisInfo>& analysisList, sf1r::LAManagerConfig & config )
{
    //======= 1. Declaring the analyzer & tokenizer instances
    
    // --------- Analyers
    
    LAConfigUnit laConfig;
    TokenizerConfigUnit toknConfig;

    AnalysisInfo analysisInfo;          //dummy

    {
        // 1. set la config
        laConfig.setId( "la_korall" );
        laConfig.setAnalysis( "korean" );
        laConfig.setMode( "all" );
        //laConfig.setOption( "R+S+H+" );
        //laConfig.setSpecialChar( "-" );
        laConfig.setDictionaryPath( KMA_KNOWLEDGE );
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_korall";
        analysisList.push_back(analysisInfo);

        // 1. set la config
        laConfig.clear();
        laConfig.setId( "la_kornoun" );
        laConfig.setAnalysis( "korean" );
        laConfig.setMode( "noun" );
        laConfig.setDictionaryPath( KMA_KNOWLEDGE );
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_kornoun";
        analysisList.push_back(analysisInfo);

        // 1. set la config
        laConfig.clear();
        laConfig.setId( "la_korlabel" );
        laConfig.setAnalysis( "korean" );
        laConfig.setMode( "label" );
        laConfig.setDictionaryPath( KMA_KNOWLEDGE );
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_korlabel";
        analysisList.push_back(analysisInfo);
#ifdef USE_IZENECMA
        // 1. set la config
        laConfig.setId( "la_cnall" );
        laConfig.setAnalysis( "chinese" );
        laConfig.setMode( "all" );
        //laConfig.option_            = "R+S+H+";
        //laConfig.specialChar_       = "-";
        laConfig.setDictionaryPath( CMA_KNOWLEDGE );
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_cnall";
        analysisList.push_back(analysisInfo);

        // 1. set la config
        laConfig.clear();
        laConfig.setId( "la_cnnoun" );
        laConfig.setAnalysis( "chinese" );
        laConfig.setMode( "noun" );
        laConfig.setDictionaryPath( CMA_KNOWLEDGE );
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_cnnoun";
        analysisList.push_back(analysisInfo);

        // 1. set la config
        laConfig.clear();
        laConfig.setId( "la_cnlabel" );
        laConfig.setAnalysis( "chinese" );
        laConfig.setMode( "label" );
        laConfig.setDictionaryPath( CMA_KNOWLEDGE );
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_cnlabel";
        analysisList.push_back(analysisInfo);
#endif

        // 1. set la config
        laConfig.clear();
        laConfig.setId( "la_matrix" );
        laConfig.setAnalysis( "matrix" );
        laConfig.setPrefix( true );
        laConfig.setSuffix( true );
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_matrix";
        analysisList.push_back(analysisInfo);


        // 1. set la config
        laConfig.clear();
        laConfig.setId            ( "la_ngram" );
        laConfig.setAnalysis      ( "ngram" );
        laConfig.setApart         ( false );
        laConfig.setMinLevel      ( 2 );
        laConfig.setMaxLevel      ( 2 );
        laConfig.setMaxNo         ( 16 );
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_ngram";
        analysisList.push_back(analysisInfo);

        // 1. set la config
        laConfig.clear();
        laConfig.setId            ( "la_unigram" );
        laConfig.setAnalysis      ( "char" );
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_unigram";
        analysisList.push_back(analysisInfo);

        /*
        // 1. set la config
        laConfig.clear();
        laConfig.setId( "inner_la_korall_sia" );
        laConfig.setAnalysis( "korean" );
        laConfig.setCaseSensitive(true);
        laConfig.setMode("label");
        laConfig.setOption("R1H-S-");
        laConfig.setSpecialChar("#");
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "inner_la_korall_sia";
        analysisList.push_back(analysisInfo);

        // 1. set la config
        laConfig.clear();
        laConfig.setId( "inner_la_cnall_sia" );
        laConfig.setAnalysis( "chinese" );
        laConfig.setCaseSensitive(true);
        laConfig.setMode("label");
        laConfig.setOption("R+H+S+T2");
        laConfig.setSpecialChar("#");
        std::cerr << getCmaKnowledgePath() << std::endl;
        laConfig.setDictionaryPath(getCmaKnowledgePath());
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "inner_la_korall_sia";
        analysisList.push_back(analysisInfo);
        
        // 1. set la config
        laConfig.clear();
        laConfig.setId( "la_sia" );
        laConfig.setAnalysis( "multilang" );
        laConfig.setAdvOption("default,inner_la_korall_sia;cn,ma,inner_la_cnall_sia");
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_sia";
        analysisList.push_back(analysisInfo);
        */

        // 1. set la config
        laConfig.clear();
        laConfig.setId( "la_token" );
        laConfig.setAnalysis( "token" );
        config.addLAConfig( laConfig );

        // 2. add to pair list
        analysisInfo.analyzerId_    = "la_token";
        analysisList.push_back(analysisInfo);

    }


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


std::string getKmaKnowledgePath()
{
    return WISEKMA_KNOWLEDGE;
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

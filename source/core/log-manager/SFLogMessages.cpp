///
/// @file   SFLogMessages.cpp
/// @brief  Keeps all log messages in map
/// @date   2010.01.02
/// @author Deepesh Shrestha
///

#include "SFLogMessages.h"

namespace SFLogMessage
{
//static std::map<int, std::string> errMsgIdPair;
static bool bErrMsgInit = false;
izenelib::util::UString::EncodingType logEncodingType_;
void initErrMsgsInEng();
void initErrMsgsInKor();
void initErrMsgsInChi();

bool initLogMsg(const std::string& language)
{
    std::string lang("LANG");
    std::string nameOfEnv = get_env_(lang.c_str());
    parse_Lang(nameOfEnv);

    if (!bErrMsgInit)
    {
        if (language == "Korean")
        {
            initErrMsgsInKor();
        }
        else if (language == "Chinese")
        {
            initErrMsgsInChi();
        }
        else
        {
            initErrMsgsInEng();
        }

        bErrMsgInit = true;
    }

    if (bErrMsgInit == false)
        return false;
    return true;

}

char* get_env_(const char* envVarName)
{
    char* defaultName = "utf8";

    char *value = getenv(envVarName);
    if (value == NULL || *value == '\0')
        return defaultName;
    return value;
}

void parse_Lang(const std::string& envStr)
{
    std::string encodingStr;
    if ( std::string::npos != envStr.find("euckr", 0) )
        encodingStr = "EUC-KR";
    else if ( std::string::npos != envStr.find("eucjp", 0) )
        encodingStr = "EUC-JP";
    else if ( std::string::npos != envStr.find("ujis", 0) )
        encodingStr = "SJIS";
    else if ( std::string::npos != envStr.find("big5", 0) )
        encodingStr = "BIG5";
    else if ( std::string::npos != envStr.find("gb2312", 0) )
        encodingStr = "GB2312";
    else if ( std::string::npos != envStr.find("iso8859", 0) )
        encodingStr = "ISO8859-15";
    else
        encodingStr = "UTF-8";
    logEncodingType_ = izenelib::util::UString::convertEncodingTypeFromStringToEnum(encodingStr.c_str());
}

std::string getLogMsg(int id)
{
    std::string logMsg;
    izenelib::util::UString temp;

    std::map<int, std::string>::iterator it = errMsgIdPair.find(id);
    if (it == errMsgIdPair.end() )
    {
        temp= izenelib::util::UString(errMsgIdPair[UNKNOWN], logEncodingType_);
        temp.convertString(logMsg, logEncodingType_);
        return logMsg;
    }
    else
    {
        temp=izenelib::util::UString((*it).second, logEncodingType_);
        temp.convertString(logMsg, logEncodingType_);
        return logMsg;
    }
}

void initErrMsgsInEng()
{
    errMsgIdPair[NO_ERR]="No Error";
    // 1 SIA Prcoess
    // 1.1 IndexBuilder.cpp
    errMsgIdPair[10101]="Starting Indexer";
    errMsgIdPair[10102]="SCD Path does not exist. Path %s";
    errMsgIdPair[10103]="SCD File not valid. File %s";
    errMsgIdPair[10104]="SCD Files do not exist. Path %s";
    errMsgIdPair[10105]="Processing SCD file. File %s";
    errMsgIdPair[10106]="Indexed documents do not exist. File %s";
    errMsgIdPair[10107]="Cannot rename indexed scd file. File %s";
    errMsgIdPair[10109]="Indexing Finished! Documents Indexed: %d  Deleted: %d  Updated: %d";
    errMsgIdPair[10110]="Could not Load Scd File. File %s";
    errMsgIdPair[10111]="Loading SCD File succesuful. File: %s";
    errMsgIdPair[10112]="Document Insert Failed in SDB. File %s";
    errMsgIdPair[10116]="Cannot delete removed Document. docid %d";
    errMsgIdPair[10119]="Index builder finds bad updated document docid %d";
    errMsgIdPair[10122]="There are no documents to replace. file: %s";
    errMsgIdPair[10123]="Document skipped while replacing %d";
    errMsgIdPair[10124]="DocId skipped while indexing. DocId %d";
    errMsgIdPair[10125]="Duplicated Doc ID %d. Duplicated SCD DOCID %s";
    errMsgIdPair[10127]="Forward Indexing Failed %s";
    errMsgIdPair[10128]="Forward Index of Alias Failed %s";
    errMsgIdPair[10129]="Indexing Finished";
    errMsgIdPair[10130]="Update Finished";
    errMsgIdPair[10131]="Delete Finished";
    errMsgIdPair[10132]="Replace Finished";
    errMsgIdPair[10133]="Cannot create file in the path. Path %s";
    errMsgIdPair[10134]="Renaming SCD file from. From %s To %s";
    errMsgIdPair[10135]="SCD Files in Path processed in given  order. Path %s";
    errMsgIdPair[10136]="SCD File %s";
    errMsgIdPair[10137]="Error while opening directory: %s";
    errMsgIdPair[10139]="<DATE> is generated automatically while indexing. DocId %d";
    errMsgIdPair[10140]="Wrong format of number value. DocId %d";
    errMsgIdPair[10141]="Updated document %s does not exist, skip it.";
    errMsgIdPair[10142]="Deleted document %s does not exist, skip it.";

    // 1.2 SIAProcess.cpp
    errMsgIdPair[10202]="Config for Document Manager not found";
    errMsgIdPair[10203]="Cannot find Collection Information. Collection %s";
    errMsgIdPair[10204]="Config for Index Manager not found. Collection %s";
    errMsgIdPair[10205]="Some index directory cannot be opened";
    errMsgIdPair[10206]="Current Index Directory %s";
    errMsgIdPair[10207]="KMA path %s";
    errMsgIdPair[10208]="Create Extractor Failed";
    errMsgIdPair[10209]="Config of Ranking Manager of collection not found. Collection %s";
    errMsgIdPair[10210]="Failed to get property-weight map for collection. Collection %s";
    errMsgIdPair[10212]="Config of Query Manager not found";
    errMsgIdPair[10213]="Config of LA manager not found";
    errMsgIdPair[10214]="Cannot get Index Dir for collection not found";
    errMsgIdPair[10215]="onExit called for signal shut down";
    errMsgIdPair[10216]="Caught unknown exception during onExit";
    errMsgIdPair[10217]="onExit already called, please wait!!";
    errMsgIdPair[10218]="Managers not ready for Search manager";
    errMsgIdPair[10219]="Managers not ready for MF.";
    errMsgIdPair[10220]="Waiting for command.";
    errMsgIdPair[10221]="Set multiple index directories";
    errMsgIdPair[10222]="Config for Ranking Manager not found";
    errMsgIdPair[10223]="Exiting SIA due to exception %s";

    // 1.3 SIAServiceHandler.cpp
    errMsgIdPair[10301]="Initializing for collection. Collection %s";
    errMsgIdPair[10302]="No dup docs removing %d";
    errMsgIdPair[10303]="Index directory is corrupted";
    errMsgIdPair[10305]="Failed to copy index directory";
    errMsgIdPair[10306]="Wildcard Information is mismatched";
    errMsgIdPair[10307]="Invalid document Id";
    errMsgIdPair[10308]="Invalid SCD document Id. %s";

    // 2 BA Process
    // 2.1 BAMain.cpp
    errMsgIdPair[30001]="BA Initializing";
    errMsgIdPair[30002]="start MIA replication in BA";
    errMsgIdPair[30003]="Base Path for Collection do not exist %s";
    errMsgIdPair[30004]="Speller Dictionary Loading Failed. Where: \"%s\"";
    errMsgIdPair[30005]="Initializing MF";
    errMsgIdPair[30006]="Initializing Threads";
    errMsgIdPair[30007]="Exist signal occurs. Please look at error or info log of ba to trace the error. Signal : %d";
    errMsgIdPair[30008]="Cannot open spell-support path : %s";

    // 2.2 BAProcess.cpp
    errMsgIdPair[30100]="Range of query thread number is between 0 and %d";
    errMsgIdPair[30101]="Range of collection thread number is between 0 and %d";
    errMsgIdPair[30102]="Query Thread Full";

    //2.3 BaProcessWorker.cpp
    errMsgIdPair[30201]="Query type do to match to defined types. Type %d";
    errMsgIdPair[30202]="Optimize Index command Parsing Error";
    errMsgIdPair[30203]="Optimize Index For collection. Collection %s";
    errMsgIdPair[30204]="Restricted IP arrives. IP %s";

    // 2.4 CollectionWorker.cpp
    errMsgIdPair[30301]="Error while getting result from MF. Collection %s";
    errMsgIdPair[30302]="SIAProcess reports error in result %s";
    errMsgIdPair[30303]="Error while getting mining result from MF. Collection %s";
    errMsgIdPair[30304]="MIAProcess reports error. %s";

    // 2.4
    errMsgIdPair[30401]="MIA Process not started";

    // 2.4 IPRestrictor.cpp
    errMsgIdPair[30501]="Appears in IP Address. IP %s";
    errMsgIdPair[30502]="Incorrect IP address format. IP]=%s";
    errMsgIdPair[30503]="Unknown IP type. IP]=%s";
    errMsgIdPair[30504]="Bad IP Address Unknown IP type. IP %s";
    errMsgIdPair[30505]="Incorrect IP address type. Type]=%d";

    // 2.6 ConfigServiceHandler.cpp
    errMsgIdPair[30601]="Configuration file Parsing Error : %s";
    errMsgIdPair[30602]="BACache is refreshed.";

    // 3 MIAProcess
    // 3.1 MIAProcess.cpp
    errMsgIdPair[40101]="Failed to initialize MIA process";
    errMsgIdPair[40102]="Exiting MIA due to exception %s";
    errMsgIdPair[40103]="MIAProccess closed with signal shutdown";
    errMsgIdPair[40104]="Config not found";
    errMsgIdPair[40105]="Config of Document Manager not found. Collection %s";
    errMsgIdPair[40106]="Mining path. Path %s";
    errMsgIdPair[40107]="KMA Path. Path %s";
    errMsgIdPair[40108]="Create Extractor Failed";
    errMsgIdPair[40109]="Depending of Mining Manager are not ready";
    errMsgIdPair[40110]="Failed to get Mining Manager Config";
    // 3.2 MIAServiceHandler.cpp
    errMsgIdPair[40201]="Mining Collection Started";
    errMsgIdPair[40202]="Mining Collection Finished";

    // 4 DocumentManager
    // 4.1 DocumentManager.cpp
    errMsgIdPair[50101]="Failed to insert document in SDB. File %s";
    errMsgIdPair[50102]="Serialization Error while saving property length. %s";
    errMsgIdPair[50103]="Serialization Error restoring property length. %s";
    errMsgIdPair[50104]="No Document to Search";
    errMsgIdPair[50104]="Pre-stored summary does not exist";
    errMsgIdPair[50105]="No RawText For This Property. Property Name %s";
    errMsgIdPair[50106]="No Pre-Stored Summary for this Property. Please check config.  Property Name %s";
    errMsgIdPair[50107]="Snippet-Summary do not exist for this Property. Please check config or query settings. Property Name %s";
    errMsgIdPair[50108]="The Property do not exist. Please check config or query settings. Property Name %s";

    // 4.2 SentenceParser.cpp
    errMsgIdPair[50201]="Exception error in Text Summary module";

    // 5 ConfigurationManager
    // 5.1 TokenizerConfigUnit.cpp
    errMsgIdPair[60101]="TokenizerConfigunit wrong code setting";

    // 6 IndexManager
    // 6.1 IndexManager.cpp
    errMsgIdPair[70101]="Type not supported in PropertyType";
    //errMsgIdPair[70101]="The size of config unit map is zero";
    errMsgIdPair[70102]="Cannot Find LA analyzer. AnalyzerID %d";
    errMsgIdPair[70103]="Analysis type is not available. Type %s";
    errMsgIdPair[70104]="Cannot Find LA analyzer. AnalyzerID %d";
    errMsgIdPair[70105]="Invalid KoreanLA Option %d";

    // 7 LogManager
    // 7.1 LogManager.cpp
    errMsgIdPair[80101]="Collection not found in LogManager. Collection %s";
    errMsgIdPair[80102]="Collection name and path list mismatch";
    errMsgIdPair[80103]="Create Log Manager Directory %s";
    errMsgIdPair[80104]="Error opening a Log File. Path %s";
    errMsgIdPair[80105]="Error creating Log Dir. Path %s";

    // 8 MiningManager
    // 8.1 MiningManager.cpp
    errMsgIdPair[90101]="Mining Property Info. Id %d, name %s ";
    errMsgIdPair[90102]="Fatal Error, No mining property";
    errMsgIdPair[90103]="Fatal, a stop word file does not exist. File %s";
    errMsgIdPair[90104]="Start Running Mining Collection for docs. Number of docse %d";
    errMsgIdPair[90105]="Mining in progress... Number of docs %d";
    errMsgIdPair[90106]="Start Taxonomy Generation";
    errMsgIdPair[90107]="Failed Taxonomy Analysis ";
    errMsgIdPair[90108]="Finished Taxonomy Generation";
    errMsgIdPair[90109]="Start Label Replication.";
    errMsgIdPair[90110]="Finish Label Replication.";
    errMsgIdPair[90111]="Start Duplicate Detection Analysis.";
    errMsgIdPair[90112]="Duplicate Detection Analysis Failed";
    errMsgIdPair[90113]="Finished Duplicate Detection Analysis.";
    errMsgIdPair[90114]="Start Similarity Detection Analysis.";
    errMsgIdPair[90115]="Null Operation in Similarity Detection.";
    errMsgIdPair[90116]="End Similarity Detection Analysis.";
    errMsgIdPair[90117]="No Document in Term Index.";
    errMsgIdPair[90118]="Not enough documents for similarity computation.";
    errMsgIdPair[90119]="Similarity index is not executed.";
    errMsgIdPair[90120]="Skip insert term index for docID. %d";
    errMsgIdPair[90121]="Insert term index for docID. %d";
    errMsgIdPair[90122]="Delete term index for docID. DocId %d";
    errMsgIdPair[90123]="Operation on files or directories error: %s";
    errMsgIdPair[90124]="Resource directory %s does not existed. Please set the ResourcePara correctly in MIA config";
    errMsgIdPair[90125]="Mining Process is in broken state, please review the log and fix those issues and restart";
    errMsgIdPair[90126]="Error while getMiningResult: %s";
    errMsgIdPair[90127]="Error while Updating query db: %s";
    errMsgIdPair[90128]="Error while creating directory: %s";
    errMsgIdPair[90129]="Error while opening directory: %s";

    // 9 PresentationManager
    // 9.1 presentationManager.cpp
    errMsgIdPair[100101]="RawText Empty. Size  %d";
    errMsgIdPair[100102]="Error in display property size. Size %d";
    errMsgIdPair[100103]="Mismatch in Document size and doc IDlist size. Size %d, %d";
    errMsgIdPair[100104]="Mismatch in Document size and full text. Size %d";

    // 10 QueryManager
    // 10.1 QueryManager.cpp
    errMsgIdPair[110101]="Collection Tag Not found. Collection %s";
    errMsgIdPair[110102]="Missing tag <DocumentNum> ";
    errMsgIdPair[110103]="Missing value for tag <DocumentNum> ";
    errMsgIdPair[110104]="Missing collection field";
    errMsgIdPair[110105]="Missing value in Collection name field";
    errMsgIdPair[110106]="Missing Collection field.";
    errMsgIdPair[110107]="Missing value in Collection field.";
    errMsgIdPair[110108]="Missing Collection element.";
    errMsgIdPair[110109]="Missing Common element.";
    errMsgIdPair[110110]="Missing encoding attribute";
    errMsgIdPair[110111]="Error parsing XML grammar. Query]=%s";
    errMsgIdPair[110112]="Error parsing XML grammar, Null root node";
    errMsgIdPair[110113]="Error parsing XML grammar, Null root element";
    errMsgIdPair[110114]="Missing Result Tag in Collection Tag.";
    errMsgIdPair[110115]="Missing Property Name description in Field Tag";
    errMsgIdPair[110116]="Missing Page Info Tag. Default value 0,20 is used";
    errMsgIdPair[110117]="Missing DocId Tag in Collection Tag";
    errMsgIdPair[110118]="Empty DocId";
    errMsgIdPair[110119]="Missing Ranking Model. It will use default Ranking model : BM25";
    errMsgIdPair[110120]="Missing Language Analyzer";
    //errMsgIdPair[110121]="Missing Language Analyzer Type in Language Analyzer Tag."; // Not used anymore
    //errMsgIdPair[110122]="Missing Search Tag."; // Not used anymore
    //errMsgIdPair[110123]="Missing Search Field."; // Not used anymore
    errMsgIdPair[110124]="Missing Sorting Priority Tag. Default value is used. : Sort by RANK DESC";
    errMsgIdPair[110125]="Undefined order attributes.";
    errMsgIdPair[110126]="Incorrect values for order attribute.";
    errMsgIdPair[110126]="Empty sort field.";
    errMsgIdPair[110127]="Missing Type Attribute.";
    errMsgIdPair[110128]="Unknown attribute in Filter Option. Type %s";
    errMsgIdPair[110129]="Missing Target Field attribute";
    errMsgIdPair[110130]="Filtering value is empty";
    errMsgIdPair[110131]="Missing field in configuration. Field %s ";
    errMsgIdPair[110132]="Missing Attribute, skip grouping option";
    errMsgIdPair[110133]="Number of recent keyword absent";
    errMsgIdPair[110134]="Missing Page Info Tag";
    errMsgIdPair[110135]="Summary Sentence Num of Property(%s) is too big. It will use default num(%d)";
    errMsgIdPair[110136]="Collection is not exist : %s";

    // 10.2 QueryParser.cpp
    errMsgIdPair[110201]="Error pushing keyword in operand stack";
    errMsgIdPair[110202]="Grammar Error in Query String";

    // 10.4 QMCommonFunc.cpp
    errMsgIdPair[110401]="Restrict Word Dictionary File. Path %s";
    errMsgIdPair[110402]="Restricted Term. Term %s";
    errMsgIdPair[110403]="Restricted Keyword. Keyword %s";
    errMsgIdPair[110404]="Restricted TermId. TermId %d";
    errMsgIdPair[110405]="Failed Parsing Query";
    errMsgIdPair[110406]="Reloading of Restrict Dictionary must be done after building of that.";

    // 10.5 PropertyExpr.cpp
    errMsgIdPair[110501]="Expanded Query Tree Error";
    errMsgIdPair[110502]="Error in Configuration, Missing Analysis Info. Property %s";
    errMsgIdPair[110503]="Empty Expanded Query String";
    errMsgIdPair[110504]="The size between Wild Strings and ids are not matched. %u:%u";

    // 10.6 AutoFillSubmanager.cpp
    errMsgIdPair[110601]="File not good. File %s";
    errMsgIdPair[110602]="Path does not exist. Path %s";
    errMsgIdPair[110603]="Fail to load autofill data file. SF1 removes %s";

    // 11 RankingManager
    // 11.1 RankingManager.cpp
    errMsgIdPair[120101]="Ranker has not be initialized";

    // 12 SearchManager
    // 12.1 Searchmanager.cpp
    errMsgIdPair[130101]="Nominal group is not supported";
    errMsgIdPair[130102]="Unknown property Type for GroupBy and Index";
    errMsgIdPair[130103]="Sort By Date";

    // 13 ControllerProcess
    // 13.1 ControllerProcess.cpp
    errMsgIdPair[140101]="Process Service Registration Request";
    errMsgIdPair[140102]="Process Service Permission";
    errMsgIdPair[140103]="Process Service Request From Client";
    errMsgIdPair[140104]="Process Service Result For Client";

    // 15 LicenseManager
    // 15.1 LicenseManager.cpp
    errMsgIdPair[150001]="Total Index Size reached to the MAXIMUM size of license : %ju - %ju";

    errMsgIdPair[UNKNOWN]= "Unknown Error";

}
void initErrMsgsInKor()
{
    initErrMsgsInEng();
    errMsgIdPair[NO_ERR] = "No Error";
    errMsgIdPair[UNKNOWN]= "Unknown Error";

}
void initErrMsgsInChi()
{
    initErrMsgsInEng();
    errMsgIdPair[NO_ERR] = "No Error";
    errMsgIdPair[UNKNOWN]= "Unknown Error";
}

}

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string>
#include <b5m-manager/image_server_client.h>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>


#define MAX_STORAGE_COUNT 2
#define STAT_FILENAME_BY_STORAGE_IP  "stat_upload_by_storage"
#define STAT_FILENAME_BY_OVERALL "stat_upload_by_overall"
#define FILENAME_FILE_ID  "success_upload"
#define FILENAME_FAIL  "fail_upload"
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)
#define INT64_PRINTF_FORMAT   "%lld"

using namespace std;
namespace po = boost::program_options;
using namespace sf1r;


typedef struct {
    int total_count;
    int success_count;
    int64_t time_used;
} StorageStat;

const char* nsip = "172.16.0.168:8108";

static StorageStat storages[MAX_STORAGE_COUNT];
static int storage_count = 0;
static time_t start_time;
static int total_count = 0;
static int success_count = 0;
static FILE *fpSuccess = NULL;
static FILE *fpFail = NULL;

static int test_init();
static int save_stats_by_overall();
static int save_stats_by_storage_ip();
static int add_to_storage_stat(int storage_i, const int result, const int time_used);

bool write_dir(const std::string& dir)
{
    std::string file_id;
    int result = 0;
	struct timeval tv_start;
	struct timeval tv_end;
	int time_used;


    char dirpath[1024];
    strcpy(dirpath, dir.c_str());
    if(dirpath[strlen(dirpath)-1]=='/'){
    }else{
        strcat(dirpath,"/");
    }
    DIR *d;
    struct dirent *file;
    struct stat sb;
    if(!(d=opendir(dirpath))){
        return false;
    }
    std::string filepath;
    while((file=readdir(d))!=NULL){
        if(strncmp(file->d_name,".",1)==0){
            continue;
        }
        filepath = dirpath;
        filepath += file->d_name;
        if(stat(filepath.c_str(), &sb)>=0){
            if(!S_ISDIR(sb.st_mode)){

                if(filepath.find(".pic") == std::string::npos &&
                    filepath.find(".jpeg") == std::string::npos &&
                    filepath.find(".jpg") == std::string::npos &&
                    filepath.find(".png") == std::string::npos&&
                    filepath.find(".bmp") == std::string::npos )
                    continue;

                total_count++;

                gettimeofday(&tv_start, NULL);

                result = ImageServerClient::UploadImageFile(filepath, file_id) ? 0:-1;
                gettimeofday(&tv_end, NULL);
                time_used = TIME_SUB_MS(tv_end, tv_start);
                add_to_storage_stat(0, result, time_used);

                if (result == 0) //success
                {
                    success_count++;
                    fprintf(fpSuccess, "%d %s %zu %s %d\n", 
                        (int)tv_end.tv_sec, filepath.c_str(), sb.st_size,  
                        file_id.c_str(), time_used);
                }
                else //fail
                {
                    fprintf(fpFail, "%d %s %d\n", (int)tv_end.tv_sec, 
                        filepath.c_str(), result);
                    fflush(fpFail);
                    fflush(fpSuccess);
                    if(total_count - success_count > 100)
                        exit(-1);
                }

                if (total_count % 100 == 0)
                {
                    if ((result=save_stats_by_overall()) != 0)
                    {
                        break;
                    }

                    if ((result=save_stats_by_storage_ip()) != 0)
                    {
                        break;
                    }
                    fflush(fpSuccess);
                }
            }
            else
            {
                write_dir(filepath);
            }
        }
    }
    closedir(d);
    return true;
}

void write_file(const std::string& image_path)
{
    int result = 0;

	if ((result=test_init()) != 0)
	{
		return;
	}

    if(image_path.empty())
    {
        std::cout << "empty local image path, no images will be uploaded." << std::endl;
        return;
    }
	memset(&storages, 0, sizeof(storages));
	start_time = time(NULL);
	result = 0;
	total_count = 0;
    success_count = 0;

    write_dir(image_path);

    save_stats_by_overall();
    save_stats_by_storage_ip();

    fclose(fpSuccess);
    fclose(fpFail);

    printf("time used: %ds\n", (int)(time(NULL) - start_time));
    return;
	
}

int main(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "produce help message")
        ("imgserver-config", po::value<std::string>(), "image server config string")
        ("localimagepath", po::value<std::string>(), "specify the root path of local image that need to be uploaded.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm); 
    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }
 
    boost::shared_ptr<RpcServerConnectionConfig> imgserver_config;
    if(vm.count("imgserver-config"))
    {
        std::string config_string = vm["imgserver-config"].as<std::string>();
        std::vector<std::string> vec;
        boost::algorithm::split( vec, config_string, boost::algorithm::is_any_of("|") );
        if(vec.size()==2)
        {
            std::string host = vec[0];
            uint32_t rpc_port = boost::lexical_cast<uint32_t>(vec[1]);
            imgserver_config.reset(new RpcServerConnectionConfig(host, rpc_port));
        }
        else
        {
            std::cout << "image server configure string invalid" << std::endl;
            return EXIT_FAILURE;
        }
    }

    if(!imgserver_config)
    {
        std::cout << "image server must be configured" << std::endl;
        return -1;
    }

    if(!ImageServerClient::Init(*(imgserver_config.get())))
    {
        LOG(ERROR) << "Image Server Client Init failed." << std::endl;
        return -1;
    }
    if(!ImageServerClient::Test())
    {
        LOG(ERROR) << "Image Server Client Test failed." << std::endl;
        return -1;
    }

    std::string local_image_path;
    if(vm.count("localimagepath")) 
    {
        local_image_path = vm["localimagepath"].as<std::string>();
    }
    write_file(local_image_path);
    return 0;
}

static int save_stats_by_storage_ip()
{
	int k;
	char filename[256];
	FILE *fp;

	sprintf(filename, "%s", STAT_FILENAME_BY_STORAGE_IP);
	if ((fp=fopen(filename, "wb")) == NULL)
	{
		printf("open file %s fail \n", 
			filename);
        perror("error:");
		return errno != 0 ? errno : EPERM;
	}

	fprintf(fp, "#ip_addr total_count success_count time_used(ms)\n");
	for (k=0; k<storage_count; k++)
	{
		fprintf(fp, "%d %d "INT64_PRINTF_FORMAT"\n", \
			storages[k].total_count, \
			storages[k].success_count, storages[k].time_used);
	}

	fclose(fp);
	return 0;
}

static int save_stats_by_overall()
{
	char filename[256];
	FILE *fp;

	sprintf(filename, "%s", STAT_FILENAME_BY_OVERALL);
	if ((fp=fopen(filename, "wb")) == NULL)
	{
		printf("open file %s fail\n", 
			filename);
        perror("error:");
		return errno != 0 ? errno : EPERM;
	}

	fprintf(fp, "#total_count success_count  time_used(s)\n");
	fprintf(fp, "%d %d %d\n", total_count, success_count, (int)(time(NULL) - start_time));

	fclose(fp);
	return 0;
}

static int add_to_storage_stat(int storage_i, const int result, const int time_used)
{
	StorageStat *pStorage;
    pStorage = storages + storage_i;
	
    storage_count = storage_i + 1;

	pStorage->time_used += time_used;
	pStorage->total_count++;
	if (result == 0)
	{
		pStorage->success_count++;
	}

	return 0;
}

static int test_init()
{
	char filename[256];

	if (access("tfs_upload", 0) != 0 && mkdir("tfs_upload", 0755) != 0)
	{
	}

	if (chdir("tfs_upload") != 0)
	{
		printf("chdir fail\n");
        perror("err:");
		return errno != 0 ? errno : EPERM;
	}

	sprintf(filename, "%s", FILENAME_FILE_ID);
	if ((fpSuccess=fopen(filename, "wb")) == NULL)
	{
		printf("open file %s fail\n", 
			filename);
        perror("err:");
		return errno != 0 ? errno : EPERM;
	}

	sprintf(filename, "%s", FILENAME_FAIL);
	if ((fpFail=fopen(filename, "wb")) == NULL)
	{
		printf("open file %s fail\n", 
			filename);
        perror("err:");
		return errno != 0 ? errno : EPERM;
	}

	return 0;
}


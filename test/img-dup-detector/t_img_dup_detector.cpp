#include <b5m-manager/img_dup_detector.h>

#include <iostream>
#include <cstdlib>

using namespace sf1r;
/* Glibals */
void usage();
void version();
void parse_parameters(int argc, char **argv);

std::string scd_path;
std::string output_path;
std::string source_name;

int main(int argc, char **argv)
{
  version();

  parse_parameters(argc, argv);

  ImgDupDetector imgdupdetector;
  imgdupdetector.DupDetectByImgUrlNotIn(scd_path, output_path, source_name, 3);
//  imgdupdetector.DupDetectByImgSift(scd_path, output_path, true);
  return 0;
}

void version(){
  std::cerr << "ImgDupDetector version 0.0.1" << std::endl
            << "Written by Kuilong Liu" << std::endl << std::endl;
}

void usage(){
  std::cerr << std::endl
       << "Usage: ./t_img_dup_detector scd-path output-path source-name" << std::endl << std::endl
       << "             scd-path       is the position of input scd files" << std::endl
       << "             output-path    is the position of output scd files" << std::endl
       << "             source-name    is the ShareSourceName of a document to be deleted firstly" << std::endl << std::endl
       << std::endl;
  exit(0);
}

void parse_parameters (int argc, char **argv){
  if (argc != 3) usage();
  scd_path = argv[1];
  output_path = argv[2];
  source_name = argv[3];
}

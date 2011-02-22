#ifndef COBRACONFIG_H_
#define COBRACONFIG_H_

#include <set>
#include <map>
#include <vector>
#include <string>

#include <boost/assign/list_of.hpp>

class CobraConfig {
public:
	CobraConfig() {
	}
	std::vector<std::string> getControllerParams() {
		return boost::assign::list_of<std::string>
			("-P")(ctrPort);
	}
	std::vector<std::string> getBAParams() {
		return boost::assign::list_of<std::string>
			("-H")(baPort)
			("-I")("localhost")
			("-P")(ctrPort)
			("-M")(baConfigPort)
			("-F")(baConfigPath);
	}

	std::vector<std::string> getSMIAParams(const std::string& collectionName) {
		return boost::assign::list_of<std::string>
            ("-I")("localhost")
            ("-P")(ctrPort)
            ("-M")(smiaPortMap[collectionName])
            ("-C")(collectionName);
	}
	void setConfigPath(const std::string& path ){
		baConfigPath = path;
	}
	
	std::set<std::string> getCollections(){
		return collections;
	}
	
public:
	std::string ctrPort;
	std::string baPort;
	std::string baConfigPort;
	std::string baConfigPath;
	
	std::set<std::string> collections;
	std::map<std::string, std::string> smiaPortMap;
// 	std::map<std::string, std::string> miaPortMap;
	
// 	bool isSMIA;
// 	std::map<std::string, std::pair<std::string,std::string> > smiaPortMap;

};

#endif /*COBRACONFIG_H_*/

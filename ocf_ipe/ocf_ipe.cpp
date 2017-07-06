/**
 * Copyright (c) 2015, OCEAN
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Created by ChenNan in KETI on 2016-08-20.
 */
//============================================================================
// Name        : ocf_ipe.cpp
// Author      : Chen Nan
// Version     : 1.1.0
// Copyright   : KETI FLAB 
// Description : OCF_IPE in C++, Ansi-style
//============================================================================

#include "ocf_ipe.h"

const std::string filename = "conf.json";

typedef vector<std::string> OICInterfaces;
typedef std::map<std::string, std::shared_ptr<OCResource>> MasterTable;
typedef std::map<std::string, std::string> ServiceIDMap;

static MasterTable master;
static ServiceIDMap serviceTable;
static ServiceIDMap resourceTable;

static ObserveType OBSERVE_TYPE_TO_USE = ObserveType::Observe;
std::mutex curResourceLock;

static ConfigInfo conf;
static int iSock;

//send message to Thyme
void socketClient()
{
	struct sockaddr_in staddr;
	int iRet;

	iSock = socket(AF_INET, SOCK_STREAM, 0);
	if (iSock == -1)
	{
	    perror("socket create failed\n");
	}

	staddr.sin_family = AF_INET;
	staddr.sin_port = htons(conf.host_port);
	staddr.sin_addr.s_addr = inet_addr(conf.host_name.c_str());

	iRet = connect(iSock, (struct sockaddr*) &staddr, sizeof(staddr));

	if (iRet < 0)
	{
	    perror("connection failed\n");
	    close(iSock);
	}
}

//register control container to Thyme
void sendHelloMessageForControlContainer()
{
    ConfigList::iterator i;

    for(i = conf.downloads.begin(); i != conf.downloads.end(); i++){
		std::string strDevID(i->devid);
		std::string strCntName(i->ctnm);
		//std::string strAEName(i->aenm);

		Json::Value root;
		Json::FastWriter writer;

		//root["aename"] = strAEName;
		root["ctname"] = strCntName;
		root["con"] = "hello";

		std::string msg = writer.write(root);

		std::cout << "\tupload: " << msg << std::endl;

		char *dataChar = new char[msg.length() + 1];
		strcpy(dataChar, msg.c_str());
		socketWrite(iSock, dataChar);

		sleep(0.2);
    }
}

//receive command from Thyme(impletement command adaptor for you new OIC device here)
void *socketDataReceive(void* arg)
{
    while(1)
    {
		char buffer[1024];
		memset(&buffer, 0x00, sizeof(buffer));

		int msg_size = read(iSock, buffer, sizeof(buffer));

		if(msg_size > 0){
			printf("recv data : %s", buffer);
		}

		std::string rcv_str(buffer);

		Json::Reader reader;
		Json::Value root;

		if(reader.parse(rcv_str, root))
		{
			//std::string ae = root["aename"].asString();
			std::string cnt = root["ctname"].asString();
			std::string con = root["con"].asString();
			if(con == "hello")
			{
				ConfigList::iterator i;

				for(i = conf.downloads.begin(); i != conf.downloads.end(); i++)
				{
					//std::string strAEName(i->aenm);
					std::string strDevID(i->devid);
					std::string strCntName(i->ctnm);

					if(strCntName == cnt)
					{
						i->status = true;
						break;
					}
				}
			}
			else
			{
				ConfigList::iterator i;

				for(i = conf.downloads.begin(); i != conf.downloads.end(); i++)
				{
					//std::string strAEName(i->aenm);
					std::string strDevID(i->devid);
					std::string strCntName(i->ctnm);

					if(strCntName == cnt)
					{
						std::string strHostID = getHostIDByResoureceID(strDevID);

						MasterTable::iterator it = master.find(strHostID);
						if(it != master.end())
						{
							Json::Reader conReader;
							Json::Value conRoot;

						if(hasEnding(strDevID, "/a/light"))
						{
							if(conReader.parse(con, conRoot))
							{
								std::string str_status = conRoot["state"].asString();
								std::shared_ptr<OCResource> resource = it->second;

								if(resource)
								{
									OCRepresentation rep;
									if(str_status == "true"){
										rep.setValue("state", true);

									}
									else if(str_status == "false")
									{
										rep.setValue("state", false);
									}
									resource->put(rep, QueryParamsMap(), &onPut);
								}
							}
						}
						 else if(hasEnding(strDevID, "/atmel/light")
							|| hasEnding(strDevID, "/oic/r/television/switch/binary")
							|| hasEnding(strDevID, "/sec/fridge/power")
							|| hasEnding(strDevID, "/sec/aircon/power")
							|| hasEnding(strDevID, "/sec/oven/upper/power")
							|| hasEnding(strDevID, "/sec/oven/lower/power"))
						{
							if(conReader.parse(con, conRoot))
							{
								std::string str_value = conRoot["value"].asString();
								std::shared_ptr<OCResource> resource = it->second;

								if(resource)
								{
									OCRepresentation rep;
									if(str_value == "true"){
										rep.setValue("value", true);

									}
									else if(str_value == "false")
									{
										rep.setValue("value", false);
									}
									resource->put(rep, QueryParamsMap(), &onPut);
								}
							}
						}
						else
						{
							std::cout << "unknown resource type!" << std::endl;
						}
					}
					else 
					{
						 std::cout << "can not find resource[" + strDevID + "]!" << std::endl;
					}
					break;
					}
				}
			}
		}
    }
    pthread_exit((void *) 0);
}

//check string end
bool hasEnding (std::string const &fullString, std::string const &ending) 
{
    if (fullString.length() >= ending.length()) 
    {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    }
    else
    {
        return false;
    }
}

//replace a string part by new string
void string_replace(string&s1,const string&s2,const string&s3)
{
    string::size_type pos=0;
    string::size_type a=s2.size();
    string::size_type b=s3.size();
    while((pos=s1.find(s2,pos))!=string::npos)
    {
		s1.replace(pos,a,s3);
		pos+=b;
    }
}

//control command callback function
void onPut(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode)
{
    try
    {
        if(eCode == OC_STACK_OK || eCode == OC_STACK_RESOURCE_CHANGED)
        {
            std::cout << "PUT request was successful" << std::endl;

			std::string hostUri = rep.getHost();
			std::string resourcePath = rep.getUri();
			std::string fullPath = hostUri + resourcePath;

            std::cout << "PUT RESULT:"<<std::endl;
			std::cout << "\tUri: " << hostUri << std::endl;
			std::cout << "\tResourcePath: " << resourcePath << std::endl;
			std::cout << "\tFullPath: " << fullPath << std::endl;
        }
        else
        {
            std::cout << "onPut Response error: " << eCode << std::endl;
            std::exit(-1);
        }
    }
    catch(std::exception& e)
    {
        std::cout << "Exception: " << e.what() << " in onPut" << std::endl;
    }
}

//receive sensing data from OIC device(impletement new resource type for you OIC device here)
void onObserve(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep,
                    const int& eCode, const int& sequenceNumber)
{
    try
    {
        if(eCode == OC_STACK_OK && sequenceNumber <= MAX_SEQUENCE_NUMBER)
        {
            if(sequenceNumber == OC_OBSERVE_REGISTER)
            {
                std::cout << "Observe registration action is successful" << std::endl;
            }

			std::string hostUri = rep.getHost();
			std::string resourcePath = rep.getUri();
			std::string fullPath = hostUri + resourcePath;
			ServiceIDMap::iterator it = serviceTable.find(fullPath);
			std::string resourceID;
			if(it != serviceTable.end()){
			    resourceID = it->second;
			} else {
			    return;
			}

		    std::cout << "OBSERVE RESULT:"<<std::endl;
		    std::cout << "\tSequenceNumber: "<< sequenceNumber << std::endl;
			std::cout << "\tUri: " << hostUri << std::endl;
			std::cout << "\tResourcePath: " << resourcePath << std::endl;
			std::cout << "\tFullPath: " << fullPath << std::endl;
			std::cout << "\tResourceID: " << resourceID << std::endl;

			OICInterfaces interfaces = rep.getResourceInterfaces();
			OICInterfaces::iterator i;

			std::cout << "\tInterface: " << interfaces.size() << std::endl;
			for(i = interfaces.begin(); i != interfaces.end(); i++){
				std::cout << *i << std::endl;
			}

			std::string content = "";

			if(resourcePath == "/a/light")
			{
				Light dev;

				rep.getValue("state", dev.m_state);
				rep.getValue("power", dev.m_power);
				rep.getValue("name", dev.m_name);

				std::cout << "\tstate: " << dev.m_state << std::endl;
				std::cout << "\tpower: " << dev.m_power << std::endl;
				std::cout << "\tname: " << dev.m_name << std::endl;

				Json::Value root;
				Json::FastWriter writer;

				root["state"] = dev.m_state;
				root["power"] = dev.m_power;

				content = writer.write(root);
				string_replace(content, "\n", "");
			}
			else if(resourcePath == "/oic/r/television/switch/binary"
				|| resourcePath == "/sec/fridge/power"
				|| resourcePath == "/sec/aircon/power"
				|| resourcePath == "/sec/oven/upper/power"
				|| resourcePath == "/sec/oven/lower/power"
				|| resourcePath == "/atmel/motion"
				|| resourcePath == "/atmel/light")
			{
				BinarySwitch dev;

				rep.getValue("value", dev.value);

				std::cout << "\tvalue: " << dev.value << std::endl;

				Json::Value root;
				Json::FastWriter writer;

				root["value"] = dev.value;

				content = writer.write(root);
				string_replace(content, "\n", "");
			}
			else if(resourcePath == "/oic/r/television/audio")
			{
			//need impl
			}
			else if(resourcePath == "/oic/r/television/mediainput")
			{
			//need impl
			}
			else if(resourcePath == "/sec/fridge/refrigeration")
			{
			//need impl
			}
			else if(resourcePath == "/sec/fridge/cooler/temp")
			{
			//need impl
			}
			else if(resourcePath == "/sec/fridge/freezer/temp")
			{
			//need impl
			}
			else if(resourcePath == "/sec/fridge/cooler/door"
				|| resourcePath == "/sec/fridge/freezer/door"
				|| resourcePath == "/sec/fridge/cvroom/door")
			{
				Door dev;

				rep.getValue("openState", dev.openState);
            	rep.getValue("openAlarm", dev.openAlarm);

                std::cout << "\topenState: " << dev.openState << std::endl;
                std::cout << "\topenAlarm: " << dev.openAlarm << std::endl;

				Json::Value root;
				Json::FastWriter writer;

				root["openState"] = dev.openState;
				root["openAlarm"] = dev.openAlarm;

				content = writer.write(root);
				string_replace(content, "\n", "");
			}
			else if(resourcePath == "/sec/fridge/icemaker")
			{
			//need impl
			}
			else if(resourcePath == "/sec/aircon/temperature")
			{
			//need impl
			}
			else if(resourcePath == "/sec/aircon/airFlow")
			{
			//need impl
			}
			else if(resourcePath == "/sec/aircon/mode")
			{
			//need impl
			}
			else if(resourcePath == "/sec/oven/upper/temp")
			{
			//need impl
			}
			else if(resourcePath == "/sec/oven/lower/temp")
			{
			//need impl
			}
			else if(resourcePath == "/atmel/temperature")
			{
				Temperature dev;

				rep.getValue("temperature", dev.temperature);
				rep.getValue("range", dev.range);
				rep.getValue("units", dev.units);

				std::cout << "\ttemperature: " << dev.temperature << std::endl;
				std::cout << "\trange: " << dev.range << std::endl;
				std::cout << "\tunits: " << dev.units << std::endl;

				Json::Value root;
				Json::FastWriter writer;

				root["temperature"] = dev.temperature;
				root["range"] = dev.range;
				root["units"] = dev.units;

				content = writer.write(root);
				string_replace(content, "\n", "");
			}
			else if(resourcePath == "/atmel/illuminance")
			{
				Illuminance dev;

				rep.getValue("illuminance", dev.illuminance);

				std::cout << "\tilluminance: " << dev.illuminance << std::endl;

				Json::Value root;
					Json::FastWriter writer;

				root["illuminance"] = dev.illuminance;

				content = writer.write(root);
				string_replace(content, "\n", "");
			}
			std::cout << "\tcontent: " << content << std::endl;
			std::string devID = getResourceIDByHostID(hostUri + resourcePath);
			DeviceItem dev = getUploadItem(devID);
			std::string msg = getUploadMessage(dev.ctnm, content);
			std::cout << "\tupload: " << msg << std::endl;

			char *dataChar = new char[msg.length() + 1];
			strcpy(dataChar, msg.c_str());
			socketWrite(iSock, dataChar);
		}
		else
		{
			if(eCode == OC_STACK_OK)
            {
                std::cout << "No observe option header is returned in the response." << std::endl;
                std::cout << "For a registration request, it means the registration failed" << std::endl;
            }
            else
            {
                std::cout << "onObserve Response error: " << eCode << std::endl;
                std::exit(-1);
            }
        }
    }
    catch(std::exception& e)
    {
        std::cout << "Exception: " << e.what() << " in onObserve" << std::endl;
    }

}

// Callback to found resources
void foundResource(std::shared_ptr<OCResource> resource)
{
    std::cout << "In foundResource\n";
    std::string resourceURI;
    std::string hostAddress;
    
    try
    {
	std::lock_guard<std::mutex> lock(curResourceLock);
        // Do some operations with resource object.
        if(resource)
        {
            std::cout<<"DISCOVERED Resource:"<<std::endl;
            // Get the resource URI
            resourceURI = resource->uri();
            std::cout << "\tURI of the resource: " << resourceURI << std::endl;

            // Get the resource host address
            hostAddress = resource->host();
            std::cout << "\tHost address of the resource: " << hostAddress << std::endl;

	    std::string fullPath= hostAddress + resourceURI;
	    std::cout << "\tFull path of the resource: " << fullPath << std::endl;

            // Get the resource types
            std::cout << "\tList of resource types: " << std::endl;
            for(auto &resourceTypes : resource->getResourceTypes())
            {
                std::cout << "\t\t" << resourceTypes << std::endl;
            }

            // Get the resource interfaces
            std::cout << "\tList of resource interfaces: " << std::endl;
            for(auto &resourceInterfaces : resource->getResourceInterfaces())
            {
                std::cout << "\t\t" << resourceInterfaces << std::endl;
            }

	    std::string host = resource->host();
	    std::string serviceID = resource->sid();

	    if(serviceTable.find(host + resourceURI) == serviceTable.end()){
		
		serviceTable[host + resourceURI] = serviceID + resourceURI;
	    }

	    if(resourceTable.find(serviceID + resourceURI) == resourceTable.end()){

		resourceTable[serviceID + resourceURI] = host + resourceURI;
	    }

//	    ServiceIDMap::iterator i;

//	    std::cout << std::endl;
//	    for(i = serviceTable.begin(); i != serviceTable.end(); i++){
//		std::cout << "Key:\t" << i->first << " Value:\t" << i->second << std::endl;
//	    }
//	    std::cout << std::endl;

	    if(master.find(fullPath) == master.end())
	    {
		if(isSettingResource(serviceID + resourceURI))
		{
			std::cout << "\t["<< serviceID + resourceURI << "] match!" << std::endl;

			QueryParamsMap queryParam;
			resource->observe(OBSERVE_TYPE_TO_USE, queryParam, &onObserve);

			std::cout << "\tObserve ["<< serviceID + resourceURI << "]!" << std::endl;

		}
		master[fullPath] = resource;
	    }
        }
        else
        {
            // Resource is invalid
            std::cout << "Resource is invalid" << std::endl;
        }

    }
    catch(std::exception& e)
    {
        std::cerr << "Exception in foundResource: "<< e.what() << std::endl;
    }
}

void socketWrite(int sock, char* data)
{
    printf("socket write\n");
    printf("%s\n", data);
    write(sock, data, strlen(data));
}

bool isSettingResource(std::string fullPath)
{
    bool bResult = false;

    ConfigList::iterator i;

    for(i = conf.uploads.begin(); i != conf.uploads.end(); i++)
	{
		std::string strDevID(i->devid);
		if(strDevID == fullPath)
		{
			 bResult = true;
			 break;
		}
    }

    return bResult;
}

//make upload json message
std::string getUploadMessage(std::string ctnm, std::string con)
{
    std::string str_msg;
    Json::Value root;
    Json::FastWriter writer;

    string_replace(con, "\'", "\\'");
    root["ctname"] = ctnm;
    root["con"] = con;

    str_msg = writer.write(root);

    return str_msg;
}

//seek resource id in master table
std::string getResourceIDByHostID(std::string host){
    std::string devID = "";

    ServiceIDMap::iterator i = serviceTable.find(host);

    if(i != serviceTable.end()){
	devID = i->second;
    }

    return devID;
}

//seek resource COAP path in master table
std::string getHostIDByResoureceID(std::string resc){
    std::string rescID = "";

    ServiceIDMap::iterator i = resourceTable.find(resc);

    if(i != resourceTable.end()){
	rescID = i->second;
    }
    return rescID;
}

DeviceItem getUploadItem(std::string fullPath)
{
    DeviceItem dev;

    ConfigList::iterator i;

    for(i = conf.uploads.begin(); i != conf.uploads.end(); i++){
		std::string strDevID(i->devid);
		if(strDevID == fullPath)
		{
			 //std::string container(i->ctnm);
			 //container_name = container;
			 dev = *i;
			 break;
		}
    }
 
    return dev;
}

static FILE* client_open(const char* /*path*/, const char *mode)
{
    return fopen("./oic_svr_db_client.json", mode);
}

//load tas config file
void loadconfig()
{
	ifstream ifs;
    ifs.open(filename);
    assert(ifs.is_open());
	
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(ifs, root, false))
    {
        exit(0);
    }
	Json::Value jsonConf = root["m2m:conf"];
	Json::Value jsonTas = jsonConf["tas"];
	Json::Value jsonUploads = jsonConf["upload"];
	Json::Value jsonDownloads = jsonConf["download"];
	
	conf.host_name =  jsonTas["parenthostname"].asString();
	conf.host_port = atoi(jsonTas["parentport"].asString().c_str());

	int size = 0;
	
	size = jsonUploads.size();
	
	for(int i = 0; i < size; i++){
		DeviceItem dev;
		
		//dev.aenm = jsonUploads[i]["ae"].asString();
		dev.ctnm = jsonUploads[i]["ctname"].asString();
		dev.devid = jsonUploads[i]["id"].asString();
		dev.status = false;
		
		conf.uploads.push_back(dev);
	}
	
	size = jsonDownloads.size();
	
	for(int i = 0; i < size; i++){
		DeviceItem dev;
		
		//dev.aenm = jsonDownloads[i]["ae"].asString();
		dev.ctnm = jsonDownloads[i]["ctname"].asString();
		dev.devid = jsonDownloads[i]["id"].asString();
		dev.status = false;
		
		conf.downloads.push_back(dev);
	}
	
	return;
}


int main() {
	
	loadconfig();
	std::cout << "TAS Configuration Info" << std::endl;
	std::cout << std::endl;
	std::cout << "\tHost Name: [" << conf.host_name << "]" << std::endl;
	std::cout << "\tHost Port: [" << conf.host_port << "]" << std::endl;
	std::cout << std::endl;
	std::cout << "Upload List" << std::endl;

	ConfigList::iterator i;

	for(i = conf.uploads.begin(); i != conf.uploads.end(); i++){
		std::cout << std::endl;
		//std::cout << "\tAE Name: [" << i->aenm << "]" << std::endl;
		std::cout << "\tContainer Name: [" << i->ctnm << "]" << std::endl;
		std::cout << "\tDevice ID: [" << i->devid << "]" << std::endl;
	}

	std::cout << std::endl;
	std::cout << "Download List" << std::endl;

	for(i = conf.downloads.begin(); i != conf.downloads.end(); i++){
		std::cout << std::endl;
		//std::cout << "\tAE Name: [" << i->aenm << "]" << std::endl;
		std::cout << "\tContainer Name: [" << i->ctnm << "]" << std::endl;
		std::cout << "\tDevice ID: [" << i->devid << "]" << std::endl;
	}

	std::cout << std::endl;

	socketClient();

	int thread_num = 100;
	pthread_t sock_thread;

	pthread_create(&sock_thread, NULL, &socketDataReceive, (void *)&thread_num);

	sleep(1);

	sendHelloMessageForControlContainer();

	std::ostringstream requestURI;
	OCPersistentStorage ps {client_open, fread, fwrite, fclose, unlink };

	// Create PlatformConfig object
	PlatformConfig cfg {
		OC::ServiceType::InProc,
		OC::ModeType::Both,
		"0.0.0.0",
		0,
		OC::QualityOfService::LowQos,
		&ps
	};

	OCPlatform::Configure(cfg);
	try
	{
	    // makes it so that all boolean values are printed as 'true/false' in this stream
	    std::cout.setf(std::ios::boolalpha);
	    // Find all resources
	    requestURI << OC_RSRVD_WELL_KNOWN_URI;// << "?rt=core.light";

	    OCPlatform::findResource("", requestURI.str(), CT_DEFAULT, &foundResource);
	    std::cout<< "Finding Resource... " <<std::endl;

	    std::mutex blocker;
	    std::condition_variable cv;
	    std::unique_lock<std::mutex> lock(blocker);
	    cv.wait(lock);
	}catch(OCException& e)
	{
	    oclog() << "Exception in main: " << e.what();
	}
}



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
// Name        : ocf_ipe.h
// Author      : Chen Nan
// Version     : 1.1.0
// Copyright   : KETI FLAB 
// Description : OCF_IPE in C++, Ansi-style
//============================================================================

#include <iostream>
#include <fstream>
#include <string.h>
#include <list>
#include <memory>
#include <iostream>
#include <map>
#include <cstdlib>
#include <vector>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <cassert>
//#include "tinyxml2.h"
#include "json/json.h"

#include "OCPlatform.h"
#include "OCApi.h"

using namespace std;
//using namespace tinyxml2;
using namespace OC;

struct DeviceItem {
    std::string ctnm;
    std::string devid;
    std::string aenm;
    bool status;
};

typedef list<DeviceItem> ConfigList;

struct ConfigInfo {
    std::string host_name;
    int host_port = -1;

    ConfigList downloads;
    ConfigList uploads;
};

//Resource type
class BinarySwitch
{
public:

	bool value;

	BinarySwitch() : value(false)
	{}
};

// Class for Audio resource type
class Audio
{
public:

	int volume;
	std::string range;
	bool mute;
	int value;

	Audio() : volume(0), range(""), mute(false), value(0)
	{}
};

// Class for MediaInput resource type
class MediaInput
{
public:

	//std::String[] sources;
	//std::String sourceName;
	//bool status;
	std::string range;
	int value;

	MediaInput() : range(""), value(0)
	{}
};

// Class for Refridgeration resource type
class Refridgeration
{
public:

	int filter;
	bool rapidCool;
	bool defrost;
	std::string range;
	bool rapidFreeze;
	int value;

	Refridgeration() : filter(0), rapidCool(false), defrost(false), range(""), rapidFreeze(false), value(0)
	{}
};

// Class for Temperature resource type
class Temperature
{
public:

	double temperature;
	std::string range;
	std::string units;

	Temperature() : temperature(0.00), range(""), units("")
	{}
};

// Class for Door resource type
class Door
{
public:

	std::string openState;
	bool openAlarm;
	//std::String range;
	//openDuration
	//value

	Door() : openState(""), openAlarm(false)
	{}
};

// Class for AirFlow resource type
class AirFlow
{
public:

	std::string range;
	int speed;
	std::string direction;

	AirFlow() : range(""), speed(0), direction("")
	{}
};

// Class for Mode resource type
class Mode
{
public:

	std::string modes;
	std::string range;
	std::string supportedModes;
	std::string value;

	Mode() : modes(""), range(""), supportedModes(""), value("")
	{}
};

// Class for Illuminance resource type
class Illuminance
{
public:

	int illuminance;

	Illuminance() : illuminance(0)
	{}
};

// Class for Light resource type
class Light
{
public:

    bool m_state;
    int m_power;
    std::string m_name;

    Light() : m_state(false), m_power(0), m_name("")
    {}
};

void loadconfig();
void string_replace(string&s1,const string&s2,const string&s3);
bool hasEnding (std::string const &fullString, std::string const &ending);
bool isSettingResource(std::string fullPath);
DeviceItem getUploadItem(std::string fullPath);
std::string getUploadMessage(std::string ctnm, std::string con);
void socketWrite(int sock, char* data);
void socketClient();
void *socketDataReceive(void* arg);
void onPut(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int eCode);
void onObserve(const HeaderOptions& /*headerOptions*/, const OCRepresentation& rep, const int& eCode, const int& sequenceNumber);
void foundResource(std::shared_ptr<OCResource> resource);
std::string getResourceIDByHostID(std::string host);
std::string getHostIDByResoureceID(std::string resc);

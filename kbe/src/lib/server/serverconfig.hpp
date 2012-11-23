/*
This source file is part of KBEngine
For the latest info, see http://www.kbengine.org/

Copyright (c) 2008-2012 KBEngine.

KBEngine is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

KBEngine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public License
along with KBEngine.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
		ServerConfig::getSingleton().loadConfig("../../res/server/KBEngine.xml");
		ENGINE_COMPONENT_INFO& ecinfo = ServerConfig::getSingleton().getCellApp();													
*/
#ifndef __SERVER_CONFIG_H__
#define __SERVER_CONFIG_H__
#define __LIB_DLLAPI__	
// common include
#include "cstdkbe/cstdkbe.hpp"
#if KBE_PLATFORM == PLATFORM_WIN32
#pragma warning (disable : 4996)
#endif
//#define NDEBUG
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>	
#include <stdarg.h> 
#include "cstdkbe/singleton.hpp"
#include "thread/threadmutex.hpp"
#include "thread/threadguard.hpp"
#include "xmlplus/xmlplus.hpp"	
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#include <errno.h>
#endif
	
namespace KBEngine{
namespace Mercury
{
class Address;
}

// ���������Ϣ�ṹ��
typedef struct EngineComponentInfo
{
	uint32 port;											// ��������к�����Ķ˿�
	char ip[MAX_BUF];										// �����������ip��ַ

	char entryScriptFile[MAX_NAME];							// �������ڽű��ļ�
	char dbAccountEntityScriptType[MAX_NAME];				// ���ݿ��ʺŽű����
	float defaultAoIRadius;									// ������cellapp�ڵ��е�player��aoi�뾶��С
	float defaultAoIHysteresisArea;							// ������cellapp�ڵ��е�player��aoi���ͺ�Χ
	const Mercury::Address* externalAddr;					// �ⲿ��ַ
	const Mercury::Address* internalAddr;					// �ڲ���ַ
	COMPONENT_ID componentID;

	char internalInterface[MAX_NAME];						// �ڲ������ӿ�����
	char externalInterface[MAX_NAME];						// �ⲿ�����ӿ�����
	int32 externalPorts_min;								// ����socket�˿�ʹ��ָ����Χ
	int32 externalPorts_max;

	char db_type[MAX_BUF];									// ���ݿ�����
	uint32 db_port;											// ���ݿ�Ķ˿�
	char db_ip[MAX_BUF];									// ���ݿ��ip��ַ
	char db_username[MAX_BUF];								// ���ݿ���û���
	char db_password[MAX_BUF];								// ���ݿ������
	char db_name[MAX_BUF];									// ���ݿ���
	uint16 db_numConnections;								// ���ݿ��������

	float archivePeriod;									// entity�洢���ݿ�����
	float backupPeriod;										// entity��������
	bool backUpUndefinedProperties;							// entity�Ƿ񱸷�δ��������

	float loadSmoothingBias;								// baseapp������ƽ�����ֵ�� 
	uint32 login_port;										// ��������¼�˿� Ŀǰbots����
	char login_ip[MAX_BUF];									// ��������¼ip��ַ

}ENGINE_COMPONENT_INFO;

class ServerConfig : public Singleton<ServerConfig>
{
public:
	ServerConfig();
	~ServerConfig();
	
	bool loadConfig(std::string fileName);
	
	ENGINE_COMPONENT_INFO& getCellApp(void);
	ENGINE_COMPONENT_INFO& getBaseApp(void);
	ENGINE_COMPONENT_INFO& getDBMgr(void);
	ENGINE_COMPONENT_INFO& getLoginApp(void);
	ENGINE_COMPONENT_INFO& getCellAppMgr(void);
	ENGINE_COMPONENT_INFO& getBaseAppMgr(void);
	ENGINE_COMPONENT_INFO& getKBMachine(void);
	ENGINE_COMPONENT_INFO& getKBCenter(void);
	ENGINE_COMPONENT_INFO& getBots(void);
	ENGINE_COMPONENT_INFO& getResourcemgr(void);
	ENGINE_COMPONENT_INFO& getMessagelog(void);

	inline ENGINE_COMPONENT_INFO& getComponent(COMPONENT_TYPE ComponentType);
 	
 	void updateInfos(bool isPrint, COMPONENT_TYPE componentType, COMPONENT_ID componentID, 
 				const Mercury::Address& internalAddr, const Mercury::Address& externalAddr);
 	
	inline int16 gameUpdateHertz(void)const { return gameUpdateHertz_;}
private:
	ENGINE_COMPONENT_INFO _cellAppInfo;
	ENGINE_COMPONENT_INFO _baseAppInfo;
	ENGINE_COMPONENT_INFO _dbmgrInfo;
	ENGINE_COMPONENT_INFO _loginAppInfo;
	ENGINE_COMPONENT_INFO _cellAppMgrInfo;
	ENGINE_COMPONENT_INFO _baseAppMgrInfo;
	ENGINE_COMPONENT_INFO _kbMachineInfo;
	ENGINE_COMPONENT_INFO _kbCenterInfo;
	ENGINE_COMPONENT_INFO _botsInfo;
	ENGINE_COMPONENT_INFO _resourcemgrInfo;
	ENGINE_COMPONENT_INFO _messagelogInfo;
public:
	int16 gameUpdateHertz_;
};

#define g_kbeSrvConfig ServerConfig::getSingleton()
}
#endif

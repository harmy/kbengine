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


#ifndef __BASEAPPMGR_H__
#define __BASEAPPMGR_H__
	
// common include	
#include "baseapp.hpp"
#include "server/kbemain.hpp"
#include "server/serverapp.hpp"
#include "server/idallocate.hpp"
#include "server/serverconfig.hpp"
#include "server/forward_messagebuffer.hpp"
#include "cstdkbe/timer.hpp"
#include "network/endpoint.hpp"

//#define NDEBUG
#include <map>	
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{

class Baseappmgr :	public ServerApp, 
					public Singleton<Baseappmgr>
{
public:
	enum TimeOutType
	{
		TIMEOUT_GAME_TICK = TIMEOUT_SERVERAPP_MAX + 1
	};
	
	Baseappmgr(Mercury::EventDispatcher& dispatcher, 
		Mercury::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~Baseappmgr();
	
	bool run();
	
	void handleTimeout(TimerHandle handle, void * arg);
	void handleGameTick();

	/* ��ʼ����ؽӿ� */
	bool initializeBegin();
	bool inInitialize();
	bool initializeEnd();
	void finalise();

	COMPONENT_ID findFreeBaseapp();
	void updateBestBaseapp();

	/** ����ӿ�
		�յ�baseapp::createBaseAnywhere������ĳ�����е�baseapp�ϴ���һ��baseEntity
		@param sp: ������ݰ��д洢���� entityType	: entity����� entities.xml�еĶ���ġ�
										strInitData	: ���entity��������Ӧ�ø�����ʼ����һЩ���ݣ� 
													  ��Ҫʹ��pickle.loads���.
										componentID	: ���󴴽�entity��baseapp�����ID
	*/
	void reqCreateBaseAnywhere(Mercury::Channel* pChannel, MemoryStream& s);

	/** ����ӿ�
		��Ϣת���� ��ĳ��app��ͨ����app����Ϣת����ĳ��app��
	*/
	void forwardMessage(Mercury::Channel* pChannel, MemoryStream& s);

	/** ����ӿ�
		һ���µ�¼���˺Ż�úϷ�����baseapp��Ȩ���� ������Ҫ���˺�ע���baseapp
		ʹ�������ڴ�baseapp�ϵ�¼��
	*/
	void registerPendingAccountToBaseapp(Mercury::Channel* pChannel, 
								std::string& accountName, std::string& password, DBID entityDBID);

	/** ����ӿ�
		һ���µ�¼���˺Ż�úϷ�����baseapp��Ȩ���� ������Ҫ���˺�ע���ָ����baseapp
		ʹ�������ڴ�baseapp�ϵ�¼��
	*/
	void registerPendingAccountToBaseappAddr(Mercury::Channel* pChannel, COMPONENT_ID componentID,
								std::string& accountName, std::string& password, ENTITY_ID entityID, DBID entityDBID);

	/** ����ӿ�
		baseapp���Լ��ĵ�ַ���͸�loginapp��ת�����ͻ��ˡ�
	*/
	void onPendingAccountGetBaseappAddr(Mercury::Channel* pChannel, 
								  std::string& accountName, uint32 addr, uint16 port);

	/** ����ӿ�
		����baseapp�����
	*/
	void updateBaseapp(Mercury::Channel* pChannel, 
								ENTITY_ID numBases, ENTITY_ID numProxices, float load);
protected:
	TimerHandle							gameTimer_;
	ForwardAnywhere_MessageBuffer		forward_baseapp_messagebuffer_;
	COMPONENT_ID						bestBaseappID_;
};

}
#endif

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


#ifndef __BASEAPP_H__
#define __BASEAPP_H__
	
// common include	
#include "base.hpp"
#include "proxy.hpp"
#include "server/entity_app.hpp"
#include "server/pendingLoginmgr.hpp"
#include "server/forward_messagebuffer.hpp"
#include "network/endpoint.hpp"

//#define NDEBUG
// windows include	
#if KBE_PLATFORM == PLATFORM_WIN32
#else
// linux include
#endif
	
namespace KBEngine{

namespace Mercury{
	class Channel;
}

class Proxy;
class BackupSender;
class Archiver;

class Baseapp :	public EntityApp<Base>, 
				public Singleton<Baseapp>
{
public:
	enum TimeOutType
	{
		TIMEOUT_CHECK_STATUS = TIMEOUT_ENTITYAPP_MAX + 1,
		TIMEOUT_MAX
	};
	
	Baseapp(Mercury::EventDispatcher& dispatcher, 
		Mercury::NetworkInterface& ninterface, 
		COMPONENT_TYPE componentType,
		COMPONENT_ID componentID);

	~Baseapp();
	
	virtual bool installPyModules();
	virtual void onInstallPyModules();
	virtual bool uninstallPyModules();

	bool run();
	
	/** 
		��ش���ӿ� 
	*/
	virtual void handleTimeout(TimerHandle handle, void * arg);
	virtual void handleGameTick();
	void handleCheckStatusTick();
	void handleBackup();
	void handleArchive();

	/** 
		��ʼ����ؽӿ� 
	*/
	bool initializeBegin();
	bool initializeEnd();
	void finalise();
	
	float getLoad()const { return load_; }
	
	void updateLoad();

	static uint64 checkTickPeriod();

	static int quantumPassedPercent(uint64 curr = timestamp());
	static PyObject* __py_quantumPassedPercent(PyObject* self, PyObject* args);

	virtual void onChannelDeregister(Mercury::Channel * pChannel);

	/** ����ӿ�
		dbmgr��֪�Ѿ�����������baseapp����cellapp�ĵ�ַ
		��ǰapp��Ҫ������ȥ�����ǽ�������
	*/
	virtual void onGetEntityAppFromDbmgr(Mercury::Channel* pChannel, 
							int32 uid, 
							std::string& username, 
							int8 componentType, uint64 componentID, 
							uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport);
	
	/** ����ӿ�
		ĳ��client��app��֪���ڻ״̬��
	*/
	void onClientActiveTick(Mercury::Channel* pChannel);

	/** 
		������һ��entity�ص�
	*/
	virtual Base* onCreateEntityCommon(PyObject* pyEntity, ScriptDefModule* sm, ENTITY_ID eid);

	/** 
		����һ��entity 
	*/
	static PyObject* __py_createBase(PyObject* self, PyObject* args);
	static PyObject* __py_createBaseAnywhere(PyObject* self, PyObject* args);
	static PyObject* __py_createBaseFromDBID(PyObject* self, PyObject* args);
	
	/**
		����һ���µ�space 
	*/
	void createInNewSpace(Base* base, PyObject* cell);

	/** 
		��һ�����ؽϵ͵�baseapp�ϴ���һ��baseEntity 
	*/
	void createBaseAnywhere(const char* entityType, PyObject* params, PyObject* pyCallback);

	/** �յ�baseappmgr������ĳ��baseappҪ��createBaseAnywhere�������ڱ�baseapp��ִ�� 
		@param entityType	: entity����� entities.xml�еĶ���ġ�
		@param strInitData	: ���entity��������Ӧ�ø�����ʼ����һЩ���ݣ� ��Ҫʹ��pickle.loads���.
		@param componentID	: ���󴴽�entity��baseapp�����ID
	*/
	void onCreateBaseAnywhere(Mercury::Channel* pChannel, MemoryStream& s);

	/** 
		��db��ȡ��Ϣ����һ��entity
	*/
	void createBaseFromDBID(const char* entityType, DBID dbid, PyObject* pyCallback);

	/** ����ӿ�
		createBaseFromDBID�Ļص���
	*/
	void onCreateBaseFromDBIDCallback(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);

	/** 
		baseapp ��createBaseAnywhere�Ļص� 
	*/
	void onCreateBaseAnywhereCallback(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);
	void _onCreateBaseAnywhereCallback(Mercury::Channel* pChannel, CALLBACK_ID callbackID, 
		std::string& entityType, ENTITY_ID eid, COMPONENT_ID componentID);

	/** 
		Ϊһ��baseEntity���ƶ���cell�ϴ���һ��cellEntity 
	*/
	void createCellEntity(EntityMailboxAbstract* createToCellMailbox, Base* base);
	
	/** ����ӿ�
		createCellEntityʧ�ܵĻص���
	*/
	void onCreateCellFailure(Mercury::Channel* pChannel, ENTITY_ID entityID);

	/** ����ӿ�
		createCellEntity��cellʵ�崴���ɹ��ص���
	*/
	void onEntityGetCell(Mercury::Channel* pChannel, ENTITY_ID id, COMPONENT_ID componentID, SPACE_ID spaceID);

	/** 
		֪ͨ�ͻ��˴���һ��proxy��Ӧ��ʵ�� 
	*/
	bool createClientProxies(Proxy* base, bool reload = false);

	/** 
		��dbmgr����ִ��һ�����ݿ�����
	*/
	static PyObject* __py_executeRawDatabaseCommand(PyObject* self, PyObject* args);
	void executeRawDatabaseCommand(const char* datas, uint32 size, PyObject* pycallback);
	void onExecuteRawDatabaseCommandCB(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);

	/** ����ӿ�
		dbmgr���ͳ�ʼ��Ϣ
		startID: ��ʼ����ENTITY_ID ����ʼλ��
		endID: ��ʼ����ENTITY_ID �ν���λ��
		startGlobalOrder: ȫ������˳�� �������ֲ�ͬ���
		startGroupOrder: ��������˳�� ����������baseapp�еڼ���������
	*/
	void onDbmgrInitCompleted(Mercury::Channel* pChannel, 
		ENTITY_ID startID, ENTITY_ID endID, int32 startGlobalOrder, int32 startGroupOrder);

	/** ����ӿ�
		dbmgr�㲥global���ݵĸı�
	*/
	void onBroadcastGlobalBasesChange(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);

	/** ����ӿ�
		ע�ὫҪ��¼���˺�, ע����������¼��������
	*/
	void registerPendingLogin(Mercury::Channel* pChannel, std::string& accountName, 
		std::string& password, ENTITY_ID entityID, DBID entityDBID);

	/** ����ӿ�
		���û������¼��������
	*/
	void loginGateway(Mercury::Channel* pChannel, std::string& accountName, std::string& password);

	/** ����ӿ�
		���µ�¼ ���������ؽ���������ϵ(ǰ����֮ǰ�Ѿ���¼�ˣ� 
		֮��Ͽ��ڷ������ж���ǰ�˵�Entityδ��ʱ���ٵ�ǰ���¿��Կ�����������������Ӳ��ﵽ�ٿظ�entity��Ŀ��)
	*/
	void reLoginGateway(Mercury::Channel* pChannel, std::string& accountName, 
		std::string& password, uint64 key, ENTITY_ID entityID);

	/**
	   ��¼ʧ��
	   @failedcode: ʧ�ܷ����� MERCURY_ERR_SRV_NO_READY:������û��׼����, 
									MERCURY_ERR_ILLEGAL_LOGIN:�Ƿ���¼, 
									MERCURY_ERR_NAME_PASSWORD:�û����������벻��ȷ
	*/
	void loginGatewayFailed(Mercury::Channel* pChannel, std::string& accountName, SERVER_ERROR_CODE failedcode);

	/** ����ӿ�
		��dbmgr��ȡ���˺�Entity��Ϣ
	*/
	void onQueryAccountCBFromDbmgr(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);

	/** ����ӿ�
		֪ͨ�ͻ��˽�����cell���������AOI)
	*/
	void onEntityEnterWorldFromCellapp(Mercury::Channel* pChannel, ENTITY_ID entityID);
	
	/**
		�ͻ����������������
	*/
	void onClientEntityEnterWorld(Proxy* base);

	/** ����ӿ�
		֪ͨ�ͻ����뿪��cell���������AOI)
	*/
	void onEntityLeaveWorldFromCellapp(Mercury::Channel* pChannel, ENTITY_ID entityID);

	/** ����ӿ�
		֪ͨ�ͻ��˽�����ĳ��space
	*/
	void onEntityEnterSpaceFromCellapp(Mercury::Channel* pChannel, ENTITY_ID entityID, SPACE_ID spaceID);

	/** ����ӿ�
		֪ͨ�ͻ����뿪��ĳ��space
	*/
	void onEntityLeaveSpaceFromCellapp(Mercury::Channel* pChannel, ENTITY_ID entityID, SPACE_ID spaceID);

	/** ����ӿ�
		entity�յ�һ��mail, ��ĳ��app�ϵ�mailbox����(ֻ����������ڲ�ʹ�ã� �ͻ��˵�mailbox���÷�����
		onRemoteCellMethodCallFromClient)
	*/
	void onEntityMail(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);
	
	/** ����ӿ�
		client����entity��cell����
	*/
	void onRemoteCallCellMethodFromClient(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);

	/** ����ӿ�
		cellapp����entity��cell����
	*/
	void onBackupEntityCellData(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);

	/** ����ӿ�
		cellapp writeToDB���
	*/
	void onCellWriteToDBCompleted(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);

	/** ����ӿ�
		cellappת��entity��Ϣ��client
	*/
	void forwardMessageToClientFromCellapp(Mercury::Channel* pChannel, KBEngine::MemoryStream& s);

	/**
		��ȡ��Ϸʱ��
	*/
	static PyObject* __py_gametime(PyObject* self, PyObject* args);

	/** ����ӿ�
		дentity��db�ص�
	*/
	void onWriteToDBCallback(Mercury::Channel* pChannel, ENTITY_ID eid, DBID entityDBID, CALLBACK_ID callbackID, bool success);
protected:
	TimerHandle								loopCheckTimerHandle_;

	GlobalDataClient*						pGlobalBases_;								// globalBases

	// ��¼��¼������������δ������ϵ��˺�
	PendingLoginMgr							pendingLoginMgr_;

	ForwardComponent_MessageBuffer			forward_messagebuffer_;

	// ���ݴ浵���
	std::tr1::shared_ptr< BackupSender >	pBackupSender_;	
	std::tr1::shared_ptr< Archiver >		pArchiver_;	

	float									load_;

	static uint64							_g_lastTimestamp;
	
};

}
#endif

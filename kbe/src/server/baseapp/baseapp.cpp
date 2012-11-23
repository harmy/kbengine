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


#include "baseapp.hpp"
#include "proxy.hpp"
#include "base.hpp"
#include "baseapp_interface.hpp"
#include "archiver.hpp"
#include "backup_sender.hpp"
#include "forward_message_over_handler.hpp"
#include "sync_entitystreamtemplate_handler.hpp"
#include "cstdkbe/timestamp.hpp"
#include "network/common.hpp"
#include "network/tcp_packet.hpp"
#include "network/udp_packet.hpp"
#include "network/fixed_messages.hpp"
#include "server/componentbridge.hpp"
#include "server/components.hpp"
#include "math/math.hpp"
#include "client_lib/client_interface.hpp"

#include "../../server/baseappmgr/baseappmgr_interface.hpp"
#include "../../server/cellappmgr/cellappmgr_interface.hpp"
#include "../../server/baseapp/baseapp_interface.hpp"
#include "../../server/cellapp/cellapp_interface.hpp"
#include "../../server/dbmgr/dbmgr_interface.hpp"
#include "../../server/loginapp/loginapp_interface.hpp"

namespace KBEngine{
	
ServerConfig g_serverConfig;
KBE_SINGLETON_INIT(Baseapp);

uint64 Baseapp::_g_lastTimestamp = timestamp();

PyObject* create_celldatadict_from_stream(MemoryStream& s, const char* entityType)
{
	PyObject* pyDict = PyDict_New();
	ScriptDefModule* scriptModule = EntityDef::findScriptModule(entityType);

	// �Ƚ�celldata�еĴ洢����ȡ��
	ScriptDefModule::PROPERTYDESCRIPTION_MAP& propertyDescrs = scriptModule->getPersistentPropertyDescriptions();
	ScriptDefModule::PROPERTYDESCRIPTION_MAP::const_iterator iter = propertyDescrs.begin();

	for(; iter != propertyDescrs.end(); iter++)
	{
		PropertyDescription* propertyDescription = iter->second;

		const char* attrname = propertyDescription->getName();
		PyObject* pyVal = propertyDescription->createFromStream(&s);
		PyDict_SetItemString(pyDict, attrname, pyVal);
	}
	
	if(scriptModule->hasCell())
	{
		ArraySize size = 0;
#ifdef CLIENT_NO_FLOAT
		int32 v1, v2, v3;
		int32 vv1, vv2, vv3;
#else
		float v1, v2, v3;
		float vv1, vv2, vv3;
#endif
		
		s >> size >> v1 >> v2 >> v3;
		s >> size >> vv1 >> vv2 >> vv3;

		PyObject* position = PyTuple_New(3);
		PyTuple_SET_ITEM(position, 0, PyFloat_FromDouble((float)v1));
		PyTuple_SET_ITEM(position, 1, PyFloat_FromDouble((float)v2));
		PyTuple_SET_ITEM(position, 2, PyFloat_FromDouble((float)v3));
		
		PyObject* direction = PyTuple_New(3);
		PyTuple_SET_ITEM(direction, 0, PyFloat_FromDouble((float)vv1));
		PyTuple_SET_ITEM(direction, 1, PyFloat_FromDouble((float)vv2));
		PyTuple_SET_ITEM(direction, 2, PyFloat_FromDouble((float)vv3));
		
		PyDict_SetItemString(pyDict, "position", position);
		PyDict_SetItemString(pyDict, "direction", direction);

		Py_DECREF(position);
		Py_DECREF(direction);
	}

	return pyDict;
}

//-------------------------------------------------------------------------------------
Baseapp::Baseapp(Mercury::EventDispatcher& dispatcher, 
			 Mercury::NetworkInterface& ninterface, 
			 COMPONENT_TYPE componentType,
			 COMPONENT_ID componentID):
	EntityApp<Base>(dispatcher, ninterface, componentType, componentID),
	loopCheckTimerHandle_(),
	pGlobalBases_(NULL),
	pendingLoginMgr_(ninterface),
	forward_messagebuffer_(ninterface),
	pBackupSender_()
{
	KBEngine::Mercury::MessageHandlers::pMainMessageHandlers = &BaseappInterface::messageHandlers;
}

//-------------------------------------------------------------------------------------
Baseapp::~Baseapp()
{
}

//-------------------------------------------------------------------------------------
bool Baseapp::installPyModules()
{
	Base::installScript(getScript().getModule());
	Proxy::installScript(getScript().getModule());
	GlobalDataClient::installScript(getScript().getModule());

	registerScript(Base::getScriptType());
	registerScript(Proxy::getScriptType());

	// ע�ᴴ��entity�ķ�����py
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		time,							__py_gametime,						METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		createBase,						__py_createBase,					METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		createBaseLocally,				__py_createBase,					METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(),		createEntity,					__py_createBase,					METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		createBaseAnywhere,				__py_createBaseAnywhere,			METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		createBaseFromDBID,				__py_createBaseFromDBID,			METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		executeRawDatabaseCommand,		__py_executeRawDatabaseCommand,		METH_VARARGS,			0);
	APPEND_SCRIPT_MODULE_METHOD(getScript().getModule(), 		quantumPassedPercent,			__py_quantumPassedPercent,			METH_VARARGS,			0);

	return EntityApp<Base>::installPyModules();
}

//-------------------------------------------------------------------------------------
void Baseapp::onInstallPyModules()
{
	// ���globalData, globalBases֧��
	pGlobalBases_ = new GlobalDataClient(DBMGR_TYPE, GlobalDataServer::GLOBAL_BASES);
	registerPyObjectToScript("globalBases", pGlobalBases_);

	if(PyModule_AddIntMacro(this->getScript().getModule(), LOG_ON_REJECT))
	{
		ERROR_MSG( "Baseapp::onInstallPyModules: Unable to set KBEngine.LOG_ON_REJECT.\n");
	}

	if(PyModule_AddIntMacro(this->getScript().getModule(), LOG_ON_ACCEPT))
	{
		ERROR_MSG( "Baseapp::onInstallPyModules: Unable to set KBEngine.LOG_ON_ACCEPT.\n");
	}

	if(PyModule_AddIntMacro(this->getScript().getModule(), LOG_ON_WAIT_FOR_DESTROY))
	{
		ERROR_MSG( "Baseapp::onInstallPyModules: Unable to set KBEngine.LOG_ON_WAIT_FOR_DESTROY.\n");
	}
}

//-------------------------------------------------------------------------------------
bool Baseapp::uninstallPyModules()
{	
	unregisterPyObjectToScript("globalBases");
	S_RELEASE(pGlobalBases_); 

	Base::uninstallScript();
	Proxy::uninstallScript();
	return EntityApp<Base>::uninstallPyModules();
}

//-------------------------------------------------------------------------------------
PyObject* Baseapp::__py_gametime(PyObject* self, PyObject* args)
{
	return PyLong_FromUnsignedLong(Baseapp::getSingleton().time());
}

//-------------------------------------------------------------------------------------
PyObject* Baseapp::__py_quantumPassedPercent(PyObject* self, PyObject* args)
{
	return PyLong_FromLong(quantumPassedPercent());
}

//-------------------------------------------------------------------------------------
int Baseapp::quantumPassedPercent(uint64 curr)
{
	// ȡ�ü����תΪ΢��� / ÿ��cpu-stamps = һ��������ȥ��cpu-stamps
	uint64 pass_stamps = (curr - _g_lastTimestamp) * uint64(1000) / stampsPerSecond();

	static int ms_expected = (1000 / g_kbeSrvConfig.gameUpdateHertz());

	// �õ�һ����ǰtick-pass_stampsռһ��ʱ�����ڵĵİٷֱ�
	return int(pass_stamps) * 100 / ms_expected;
}

//-------------------------------------------------------------------------------------
uint64 Baseapp::checkTickPeriod()
{
	uint64 curr = timestamp();
	int percent = quantumPassedPercent(curr);

	if (percent > 200)
	{
		WARNING_MSG( "Baseapp::handleGameTick: tick took %d%% (%.2f seconds)!\n",
			percent, float(percent)/1000.f );
	}

	uint64 elapsed = curr - _g_lastTimestamp;
	_g_lastTimestamp = curr;
	return elapsed;
}

//-------------------------------------------------------------------------------------
bool Baseapp::run()
{
	return EntityApp<Base>::run();
}

//-------------------------------------------------------------------------------------
void Baseapp::handleTimeout(TimerHandle handle, void * arg)
{
	switch (reinterpret_cast<uintptr>(arg))
	{
		case TIMEOUT_CHECK_STATUS:
			this->handleCheckStatusTick();
			return;
		default:
			break;
	}

	EntityApp<Base>::handleTimeout(handle, arg);
}

//-------------------------------------------------------------------------------------
void Baseapp::handleCheckStatusTick()
{
	pendingLoginMgr_.process();
}

//-------------------------------------------------------------------------------------
void Baseapp::updateLoad()
{
	uint64 lastTickInStamps = checkTickPeriod();

	// ��ÿ���ʱ�����
	double spareTime = 1.0;
	if (lastTickInStamps != 0)
	{
		spareTime = double(mainDispatcher_.getSpareTime())/double(lastTickInStamps);
	}

	mainDispatcher_.clearSpareTime();

	// �������ʱ�����С��0 ���ߴ���1�������ʱ��׼ȷ
	if ((spareTime < 0.f) || (1.f < spareTime))
	{
		if (g_timingMethod == RDTSC_TIMING_METHOD)
		{
			CRITICAL_MSG( "Baseapp::handleGameTick: "
				"Invalid timing result %.3f.\n"
				"please  to change for timingMethod(curr = RDTSC_TIMING_METHOD)!",
				spareTime );
		}
		else
		{
			CRITICAL_MSG( "Baseapp::handleGameTick: Invalid timing result %.3f.\n", 
				spareTime);
		}
	}

	// ���ص�ֵΪ1.0 - ����ʱ�����, ������0-1.f֮��
	float load = G3D::clamp(0.f, 1.f - float(spareTime), 1.f);

	// �˴��㷨��server_operations_guide.pdf����loadSmoothingBias��
	// loadSmoothingBias �������θ���ȡ���һ�θ��ص�loadSmoothingBiasʣ����� + ��ǰ���ص�loadSmoothingBias����
	static float loadSmoothingBias = g_kbeSrvConfig.getBaseApp().loadSmoothingBias;
	load_ = (1 - loadSmoothingBias) * load_ + loadSmoothingBias * load;
}

//-------------------------------------------------------------------------------------
void Baseapp::handleGameTick()
{
	// һ��Ҫ����ǰ��
	updateLoad();

	EntityApp<Base>::handleGameTick();

	handleBackup();
	handleArchive();
}

//-------------------------------------------------------------------------------------
void Baseapp::handleBackup()
{
	pBackupSender_->tick();
}

//-------------------------------------------------------------------------------------
void Baseapp::handleArchive()
{
	pArchiver_->tick();
}

//-------------------------------------------------------------------------------------
bool Baseapp::initializeBegin()
{
	return true;
}

//-------------------------------------------------------------------------------------
bool Baseapp::initializeEnd()
{
	// ���һ��timer�� ÿ����һЩ״̬
	loopCheckTimerHandle_ = this->getMainDispatcher().addTimer(1000000, this,
							reinterpret_cast<void *>(TIMEOUT_CHECK_STATUS));

	pBackupSender_.reset(new BackupSender());
	pArchiver_.reset(new Archiver());

	new SyncEntityStreamTemplateHandler(this->getNetworkInterface());

	_g_lastTimestamp = timestamp();

	return true;
}

//-------------------------------------------------------------------------------------
void Baseapp::finalise()
{
	loopCheckTimerHandle_.cancel();
	EntityApp<Base>::finalise();
}

//-------------------------------------------------------------------------------------
void Baseapp::onChannelDeregister(Mercury::Channel * pChannel)
{
	ENTITY_ID pid = pChannel->proxyID();
	EntityApp<Base>::onChannelDeregister(pChannel);
	
	// �й���entity�Ŀͻ����˳�����Ҫ����entity��client
	if(pid > 0)
	{
		Proxy* proxy = static_cast<Proxy*>(this->findEntity(pid));
		if(proxy)
		{
			proxy->onClientDeath();
		}
	}
}

//-------------------------------------------------------------------------------------
void Baseapp::onGetEntityAppFromDbmgr(Mercury::Channel* pChannel, int32 uid, std::string& username, 
						int8 componentType, uint64 componentID, 
						uint32 intaddr, uint16 intport, uint32 extaddr, uint16 extport)
{
	if(pChannel->isExternal())
		return;

	EntityApp<Base>::onRegisterNewApp(pChannel, uid, username, componentType, componentID, 
									intaddr, intport, extaddr, extport);

	KBEngine::COMPONENT_TYPE tcomponentType = (KBEngine::COMPONENT_TYPE)componentType;

	Components::COMPONENTS cts = Componentbridge::getComponents().getComponents(DBMGR_TYPE);
	KBE_ASSERT(cts.size() >= 1);
	
	Components::ComponentInfos* cinfos = 
		Componentbridge::getComponents().findComponent(tcomponentType, uid, componentID);

	cinfos->pChannel = NULL;

	int ret = Components::getSingleton().connectComponent(tcomponentType, uid, componentID);
	KBE_ASSERT(ret != -1);

	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();

	switch(tcomponentType)
	{
	case BASEAPP_TYPE:
		(*pBundle).newMessage(BaseappInterface::onRegisterNewApp);
		BaseappInterface::onRegisterNewAppArgs8::staticAddToBundle((*pBundle), 
			getUserUID(), getUsername(), BASEAPP_TYPE, componentID_, 

			this->getNetworkInterface().intaddr().ip, this->getNetworkInterface().intaddr().port, 
			this->getNetworkInterface().extaddr().ip, this->getNetworkInterface().extaddr().port);
		break;
	case CELLAPP_TYPE:
		(*pBundle).newMessage(CellappInterface::onRegisterNewApp);
		CellappInterface::onRegisterNewAppArgs8::staticAddToBundle((*pBundle), 
			getUserUID(), getUsername(), BASEAPP_TYPE, componentID_, 

			this->getNetworkInterface().intaddr().ip, this->getNetworkInterface().intaddr().port, 
			this->getNetworkInterface().extaddr().ip, this->getNetworkInterface().extaddr().port);
		break;
	default:
		KBE_ASSERT(false && "no support!\n");
		break;
	};
	
	(*pBundle).send(this->getNetworkInterface(), cinfos->pChannel);
	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
}

//-------------------------------------------------------------------------------------
Base* Baseapp::onCreateEntityCommon(PyObject* pyEntity, ScriptDefModule* sm, ENTITY_ID eid)
{
	if(PyType_IsSubtype(sm->getScriptType(), Proxy::getScriptType()))
	{
		return new(pyEntity) Proxy(eid, sm);
	}

	return EntityApp<Base>::onCreateEntityCommon(pyEntity, sm, eid);
}

//-------------------------------------------------------------------------------------
PyObject* Baseapp::__py_createBase(PyObject* self, PyObject* args)
{
	int argCount = PyTuple_Size(args);
	PyObject* params = NULL;
	char* entityType = NULL;
	int ret = -1;

	if(argCount == 2)
		ret = PyArg_ParseTuple(args, "s|O", &entityType, &params);
	else
		ret = PyArg_ParseTuple(args, "s", &entityType);

	if(entityType == NULL || ret == -1)
	{
		ERROR_MSG("Baseapp::createBase: args is error!");
		S_Return;
	}
	
	PyObject* e = Baseapp::getSingleton().createEntityCommon(entityType, params);
	if(e != NULL)
		Py_INCREF(e);

	return e;
}

//-------------------------------------------------------------------------------------
PyObject* Baseapp::__py_createBaseAnywhere(PyObject* self, PyObject* args)
{
	int argCount = PyTuple_Size(args);
	PyObject* params = NULL, *pyCallback = NULL;
	char* entityType = NULL;
	int ret = -1;

	switch(argCount)
	{
	case 3:
		ret = PyArg_ParseTuple(args, "s|O|O", &entityType, &params, &pyCallback);
		break;
	case 2:
		ret = PyArg_ParseTuple(args, "s|O", &entityType, &params);
		break;
	default:
		ret = PyArg_ParseTuple(args, "s", &entityType);
	};


	if(entityType == NULL || ret == -1)
	{
		ERROR_MSG("Baseapp::createBaseAnywhere: args is error!");
		S_Return;
	}

	if(!PyCallable_Check(pyCallback))
		pyCallback = NULL;

	Baseapp::getSingleton().createBaseAnywhere(entityType, params, pyCallback);
	S_Return;
}

//-------------------------------------------------------------------------------------
PyObject* Baseapp::__py_createBaseFromDBID(PyObject* self, PyObject* args)
{
	int argCount = PyTuple_Size(args);
	PyObject* pyCallback = NULL;
	wchar_t* wEntityType = NULL;
	char* entityType = NULL;
	int ret = -1;
	DBID dbid;
	PyObject* pyEntityType = NULL;

	switch(argCount)
	{
	case 3:
		ret = PyArg_ParseTuple(args, "O|K|O", &pyEntityType, &dbid, &pyCallback);
		break;
	case 2:
		ret = PyArg_ParseTuple(args, "O|K", &pyEntityType, &dbid);
		break;
	default:
		{
			PyErr_Format(PyExc_AssertionError, "%s: args require 2 or 3 args, gived %d!\n",
				__FUNCTION__, argCount);	
			PyErr_PrintEx(0);
			S_Return;
		}
	};

	if(pyEntityType)
	{
		wEntityType = PyUnicode_AsWideCharString(pyEntityType, NULL);					
		entityType = wchar2char(wEntityType);									
		PyMem_Free(wEntityType);		
	}

	if(entityType == NULL || strlen(entityType) <= 0 || ret == -1)
	{
		PyErr_Format(PyExc_AssertionError, "Baseapp::createBaseFromDBID: args is error, entityType=%s!", entityType);
		PyErr_PrintEx(0);

		if(entityType)
			free(entityType);

		S_Return;
	}

	if(EntityDef::findScriptModule(entityType) == NULL)
	{
		PyErr_Format(PyExc_AssertionError, "Baseapp::createBaseFromDBID: entityType is error!");
		PyErr_PrintEx(0);
		free(entityType);
		S_Return;
	}

	if(dbid <= 0)
	{
		PyErr_Format(PyExc_AssertionError, "Baseapp::createBaseFromDBID: dbid is error!");
		PyErr_PrintEx(0);
		free(entityType);
		S_Return;
	}

	if(pyCallback && !PyCallable_Check(pyCallback))
	{
		pyCallback = NULL;

		PyErr_Format(PyExc_AssertionError, "Baseapp::createBaseFromDBID: callback is error!");
		PyErr_PrintEx(0);
		free(entityType);
		S_Return;
	}

	Baseapp::getSingleton().createBaseFromDBID(entityType, dbid, pyCallback);

	free(entityType);
	S_Return;
}

//-------------------------------------------------------------------------------------
void Baseapp::createBaseFromDBID(const char* entityType, DBID dbid, PyObject* pyCallback)
{
	Components::COMPONENTS cts = Components::getSingleton().getComponents(DBMGR_TYPE);
	Components::ComponentInfos* dbmgrinfos = NULL;

	if(cts.size() > 0)
		dbmgrinfos = &(*cts.begin());

	if(dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
	{
		ERROR_MSG("Baseapp::createBaseFromDBID: not found dbmgr!\n");
		return;
	}

	CALLBACK_ID callbackID = 0;
	if(pyCallback != NULL)
	{
		callbackID = callbackMgr().save(pyCallback);
	}

	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
	pBundle->newMessage(DbmgrInterface::queryEntity);

	ENTITY_ID entityID = idClient_.alloc();
	KBE_ASSERT(entityID > 0);

	DbmgrInterface::queryEntityArgs5::staticAddToBundle((*pBundle), 
		g_componentID, dbid, entityType, callbackID, entityID);
	
	pBundle->send(this->getNetworkInterface(), dbmgrinfos->pChannel);
	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
}

//-------------------------------------------------------------------------------------
void Baseapp::onCreateBaseFromDBIDCallback(Mercury::Channel* pChannel, KBEngine::MemoryStream& s)
{
	std::string entityType;
	DBID dbid;
	CALLBACK_ID callbackID;
	bool success = false;
	bool wasActive = false;
	ENTITY_ID entityID;

	s >> entityType;
	s >> dbid;
	s >> callbackID;
	s >> success;
	s >> entityID;
	s >> wasActive;

	if(!success)
	{
		ERROR_MSG("Baseapp::onCreateBaseFromDBID: create %s(%"PRDBID") is failed.\n", entityType.c_str(), dbid);
		if(callbackID > 0)
		{
			Py_INCREF(Py_None);
			// baseRef, dbid, wasActive
			PyObjectPtr pyfunc = pyCallbackMgr_.take(callbackID);
			PyObject* pyResult = PyObject_CallFunction(pyfunc.get(), 
												const_cast<char*>("OKi"), 
												Py_None, dbid, wasActive);

			if(pyResult != NULL)
				Py_DECREF(pyResult);
			else
				SCRIPT_ERROR_CHECK();
		}

		return;
	}

	PyObject* pyDict = create_celldatadict_from_stream(s, entityType.c_str());
	PyObject* e = Baseapp::getSingleton().createEntityCommon(entityType.c_str(), pyDict, false, entityID);
	if(e)
	{
		static_cast<Base*>(e)->setDBID(dbid);
		static_cast<Base*>(e)->initializeEntity(pyDict);
		Py_DECREF(pyDict);
	}

	if(callbackID > 0)
	{
		//if(e != NULL)
		//	Py_INCREF(e);

		// baseRef, dbid, wasActive
		PyObjectPtr pyfunc = pyCallbackMgr_.take(callbackID);
		PyObject* pyResult = PyObject_CallFunction(pyfunc.get(), 
											const_cast<char*>("OKi"), 
											e, dbid, wasActive);

		if(pyResult != NULL)
			Py_DECREF(pyResult);
		else
			SCRIPT_ERROR_CHECK();
	}
}

//-------------------------------------------------------------------------------------
void Baseapp::createInNewSpace(Base* base, PyObject* cell)
{
	ENTITY_ID id = base->getID();
	std::string entityType = base->ob_type->tp_name;
	std::string strCellData = script::Pickler::pickle(base->getCellData());
	ArraySize cellDataLength = strCellData.length();

	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();

	(*pBundle).newMessage(CellappmgrInterface::reqCreateInNewSpace);

	(*pBundle) << entityType;
	(*pBundle) << id;
	(*pBundle) << componentID_;
	(*pBundle) << cellDataLength;

	if(cellDataLength > 0)
		(*pBundle).append(strCellData.data(), cellDataLength);
	
	Components::COMPONENTS& components = Components::getSingleton().getComponents(CELLAPPMGR_TYPE);
	Components::COMPONENTS::iterator iter = components.begin();
	if(iter != components.end())
	{
		if((*iter).pChannel != NULL)
		{
			(*pBundle).send(this->getNetworkInterface(), (*iter).pChannel);
		}
		else
		{
			ERROR_MSG("Baseapp::createInNewSpace: cellappmgr channel is NULL.\n");
		}
		
		Mercury::Bundle::ObjPool().reclaimObject(pBundle);
		return;
	}
	
	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
	ERROR_MSG("Baseapp::createInNewSpace: not found cellappmgr.\n");
}

//-------------------------------------------------------------------------------------
void Baseapp::createBaseAnywhere(const char* entityType, PyObject* params, PyObject* pyCallback)
{
	std::string strInitData = "";
	uint32 initDataLength = 0;
	if(params != NULL && PyDict_Check(params))
	{
		strInitData = script::Pickler::pickle(params);
		initDataLength = strInitData.length();
	}

	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(BaseappmgrInterface::reqCreateBaseAnywhere);

	(*pBundle) << entityType;
	(*pBundle) << initDataLength;
	if(initDataLength > 0)
		(*pBundle).append(strInitData.data(), initDataLength);

	(*pBundle) << componentID_;

	CALLBACK_ID callbackID = 0;
	if(pyCallback != NULL)
	{
		callbackID = callbackMgr().save(pyCallback);
	}

	(*pBundle) << callbackID;

	Components::COMPONENTS& components = Components::getSingleton().getComponents(BASEAPPMGR_TYPE);
	Components::COMPONENTS::iterator iter = components.begin();
	if(iter != components.end())
	{
		if((*iter).pChannel != NULL)
		{
			(*pBundle).send(this->getNetworkInterface(), (*iter).pChannel);
		}
		else
		{
			ERROR_MSG("Baseapp::createInNewSpace: baseappmgr channel is NULL.\n");
		}
		
		Mercury::Bundle::ObjPool().reclaimObject(pBundle);
		return;
	}

	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
	ERROR_MSG("Baseapp::createBaseAnywhere: not found baseappmgr.\n");
}

//-------------------------------------------------------------------------------------
void Baseapp::onCreateBaseAnywhere(Mercury::Channel* pChannel, MemoryStream& s)
{
	if(pChannel->isExternal())
		return;

	std::string strInitData = "";
	PyObject* params = NULL;
	std::string entityType;
	COMPONENT_ID componentID;
	CALLBACK_ID callbackID;

	s >> entityType;
	s.readBlob(strInitData);

	s >> componentID;
	s >> callbackID;

	if(strInitData.size() > 0)
		params = script::Pickler::unpickle(strInitData);

	Base* base = createEntityCommon(entityType.c_str(), params);
	Py_XDECREF(params);

	if(base == NULL)
		return;

	// ��������ڷ��𴴽�entity��baseapp�ϴ�������Ҫת���ص�������
	if(componentID != componentID_)
	{
		Components::ComponentInfos* cinfos = Components::getSingleton().findComponent(componentID);
		if(cinfos == NULL || cinfos->pChannel == NULL)
		{
			Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
			ForwardItem* pFI = new ForwardItem();
			pFI->pHandler = NULL;
			pFI->pBundle = pBundle;

			(*pBundle).newMessage(BaseappInterface::onCreateBaseAnywhereCallback);
			(*pBundle) << callbackID;
			(*pBundle) << entityType;
			(*pBundle) << base->getID();
			(*pBundle) << componentID_;
			forward_messagebuffer_.push(componentID, pFI);
			WARNING_MSG("Baseapp::onCreateBaseAnywhere: not found baseapp, message is buffered.\n");
			return;
		}

		Mercury::Channel* lpChannel = cinfos->pChannel;

		// ��Ҫbaseappmgrת����Ŀ��baseapp
		Mercury::Bundle* pForwardbundle = Mercury::Bundle::ObjPool().createObject();
		(*pForwardbundle).newMessage(BaseappInterface::onCreateBaseAnywhereCallback);
		(*pForwardbundle) << callbackID;
		(*pForwardbundle) << entityType;
		(*pForwardbundle) << base->getID();
		(*pForwardbundle) << componentID_;
		(*pForwardbundle).send(this->getNetworkInterface(), lpChannel);
		Mercury::Bundle::ObjPool().reclaimObject(pForwardbundle);
	}
	else
	{
		ENTITY_ID eid = base->getID();
		_onCreateBaseAnywhereCallback(NULL, callbackID, entityType, eid, componentID_);
	}
}

//-------------------------------------------------------------------------------------
void Baseapp::onCreateBaseAnywhereCallback(Mercury::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;

	CALLBACK_ID callbackID = 0;
	std::string entityType;
	ENTITY_ID eid = 0;
	COMPONENT_ID componentID = 0;
	
	s >> callbackID;
	s >> entityType;
	s >> eid;
	s >> componentID;
	_onCreateBaseAnywhereCallback(pChannel, callbackID, entityType, eid, componentID);
}

//-------------------------------------------------------------------------------------
void Baseapp::_onCreateBaseAnywhereCallback(Mercury::Channel* pChannel, CALLBACK_ID callbackID, 
	std::string& entityType, ENTITY_ID eid, COMPONENT_ID componentID)
{
	if(callbackID == 0)
		return;

	PyObjectPtr pyCallback = callbackMgr().take(callbackID);
	PyObject* pyargs = PyTuple_New(1);

	if(pChannel != NULL)
	{
		ScriptDefModule* sm = EntityDef::findScriptModule(entityType.c_str());
		if(sm == NULL)
		{
			ERROR_MSG("Baseapp::onCreateBaseAnywhereCallback: can't found entityType:%s.\n", entityType.c_str());
			Py_DECREF(pyargs);
			return;
		}
		
		// ���entity������һ��baseapp��������������mailbox
		Mercury::Channel* pOtherBaseappChannel = Components::getSingleton().findComponent(componentID)->pChannel;
		KBE_ASSERT(pOtherBaseappChannel != NULL);
		PyObject* mb = static_cast<EntityMailbox*>(new EntityMailbox(sm, NULL, componentID, eid, MAILBOX_TYPE_BASE));
		PyTuple_SET_ITEM(pyargs, 0, mb);

		PyObject* pyRet = PyObject_CallObject(pyCallback.get(), pyargs);
		if(pyRet == NULL)
		{
			SCRIPT_ERROR_CHECK();
		}
		else
		{
			Py_DECREF(pyRet);
		}

		//Py_DECREF(mb);
		int i=0;
		i++;
	}
	else
	{
		Base* base = pEntities_->find(eid);
		if(base == NULL)
		{
			ERROR_MSG("Baseapp::onCreateBaseAnywhereCallback: can't found entity:%d.\n", eid);
			Py_DECREF(pyargs);
			return;
		}

		Py_INCREF(base);
		PyTuple_SET_ITEM(pyargs, 0, base);

		PyObject* pyRet = PyObject_CallObject(pyCallback.get(), pyargs);
		if(pyRet == NULL)
		{
			SCRIPT_ERROR_CHECK();
		}
		else
		{
			Py_DECREF(pyRet);
		}
	}

	SCRIPT_ERROR_CHECK();
	Py_DECREF(pyargs);
}

//-------------------------------------------------------------------------------------
void Baseapp::createCellEntity(EntityMailboxAbstract* createToCellMailbox, Base* base)
{
	if(base->getCellMailbox())
	{
		ERROR_MSG("Baseapp::createCellEntity: %s %d has a cell!\n", 
			base->getScriptName(), base->getID());

		return;
	}

	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(CellappInterface::onCreateCellEntityFromBaseapp);

	ENTITY_ID id = base->getID();
	std::string entityType = base->ob_type->tp_name;
	std::string strCellData = script::Pickler::pickle(base->getCellData());
	ArraySize cellDataLength = strCellData.length();
	EntityMailbox* clientMailbox = base->getClientMailbox();
	bool hasClient = (clientMailbox != NULL);
	
	(*pBundle) << createToCellMailbox->getID();				// �����mailbox���ڵ�cellspace�ϴ���
	(*pBundle) << entityType;
	(*pBundle) << id;
	(*pBundle) << componentID_;
	(*pBundle) << hasClient;
	(*pBundle) << cellDataLength;

	if(cellDataLength > 0)
		(*pBundle).append(strCellData.data(), cellDataLength);

	if(createToCellMailbox->getChannel() == NULL)
	{
		ERROR_MSG("Baseapp::createCellEntity: not found cellapp(createToCellMailbox:"
			"componentID=%"PRAppID", entityID=%d), create is error!\n", 
			createToCellMailbox->getComponentID(), createToCellMailbox->getID());

		base->onCreateCellFailure();
		Mercury::Bundle::ObjPool().reclaimObject(pBundle);
		return;
	}

	(*pBundle).send(this->getNetworkInterface(), createToCellMailbox->getChannel());
	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
}

//-------------------------------------------------------------------------------------
void Baseapp::onCreateCellFailure(Mercury::Channel* pChannel, ENTITY_ID entityID)
{
	if(pChannel->isExternal())
		return;

	Base* base = pEntities_->find(entityID);
	KBE_ASSERT(base != NULL);

	base->onCreateCellFailure();
}

//-------------------------------------------------------------------------------------
void Baseapp::onEntityGetCell(Mercury::Channel* pChannel, ENTITY_ID id, 
							  COMPONENT_ID componentID, SPACE_ID spaceID)
{
	if(pChannel->isExternal())
		return;

	Base* base = pEntities_->find(id);

	// DEBUG_MSG("Baseapp::onEntityGetCell: entityID %d.\n", id);
	KBE_ASSERT(base != NULL);

	base->setSpaceID(spaceID);
	// ������пͻ��˵�entity����Ҫ��֪�ͻ��ˣ� ����entity�Ѿ����������ˡ�
	if(base->getClientMailbox() != NULL)
	{
		onClientEntityEnterWorld(static_cast<Proxy*>(base));
	}

	base->onGetCell(pChannel, componentID);
}

//-------------------------------------------------------------------------------------
void Baseapp::onClientEntityEnterWorld(Proxy* base)
{
	base->initClientCellPropertys();

	/*
	Mercury::Bundle bundle;
	bundle.newMessage(ClientInterface::onEntityEnterWorld);
	bundle << base->getID();
	bundle << base->getSpaceID();
	base->getClientMailbox()->postMail(bundle);
	*/
}

//-------------------------------------------------------------------------------------
bool Baseapp::createClientProxies(Proxy* base, bool reload)
{
	// ��ͨ������Ĺ�ϵ���entity�󶨣� �ں���ͨ���п��ṩ��ݺϷ���ʶ��
	base->getClientMailbox()->getChannel()->proxyID(base->getID());
	
	// ��������һ��ID
	if(reload)
		base->rndUUID(genUUID64());

	// �ÿͻ���֪���Ѿ�������proxices, ����ʼ��һ��������
	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(ClientInterface::onCreatedProxies);
	(*pBundle) << base->rndUUID();
	(*pBundle) << base->getID();
	(*pBundle) << base->ob_type->tp_name;
	base->getClientMailbox()->postMail((*pBundle));
	Mercury::Bundle::ObjPool().reclaimObject(pBundle);

	base->initClientBasePropertys();

	// ��Ӧ���ɿͻ��˸�֪�Ѿ�������entity���������ӿڡ�
	if(!reload)
		base->onEntitiesEnabled();
	return true;
}

//-------------------------------------------------------------------------------------
PyObject* Baseapp::__py_executeRawDatabaseCommand(PyObject* self, PyObject* args)
{
	int argCount = PyTuple_Size(args);
	PyObject* pycallback = NULL;
	int ret = -1;

	char* data = NULL;
	Py_ssize_t size;
	
	if(argCount == 2)
		ret = PyArg_ParseTuple(args, "s#|O", &data, &size, &pycallback);
	else if(argCount == 1)
		ret = PyArg_ParseTuple(args, "s#", &data, &size);

	if(ret == -1)
	{
		PyErr_Format(PyExc_TypeError, "KBEngine::executeRawDatabaseCommand: args is error!");
		PyErr_PrintEx(0);
	}
	
	Baseapp::getSingleton().executeRawDatabaseCommand(data, size, pycallback);
	S_Return;
}

//-------------------------------------------------------------------------------------
void Baseapp::executeRawDatabaseCommand(const char* datas, uint32 size, PyObject* pycallback)
{
	if(datas == NULL)
	{
		ERROR_MSG("Baseapp::executeRawDatabaseCommand: execute is error!\n");
		return;
	}

	Components::COMPONENTS cts = Components::getSingleton().getComponents(DBMGR_TYPE);
	Components::ComponentInfos* dbmgrinfos = NULL;

	if(cts.size() > 0)
		dbmgrinfos = &(*cts.begin());

	if(dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
	{
		ERROR_MSG("Baseapp::executeRawDatabaseCommand: not found dbmgr!\n");
		return;
	}

	DEBUG_MSG("KBEngine::executeRawDatabaseCommand:%s.\n", datas);

	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(DbmgrInterface::executeRawDatabaseCommand);
	(*pBundle) << componentID_ << componentType_;

	CALLBACK_ID callbackID = 0;

	if(pycallback && PyCallable_Check(pycallback))
		callbackID = callbackMgr().save(pycallback);

	(*pBundle) << callbackID;
	(*pBundle) << size;
	(*pBundle).append(datas, size);
	(*pBundle).send(this->getNetworkInterface(), dbmgrinfos->pChannel);
	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
}

//-------------------------------------------------------------------------------------
void Baseapp::onExecuteRawDatabaseCommandCB(Mercury::Channel* pChannel, KBEngine::MemoryStream& s)
{
	std::string err;
	CALLBACK_ID callbackID = 0;
	uint32 nrows = 0;
	uint32 nfields = 0;
	uint64 affectedRows = 0;

	PyObject* pResultSet = NULL;
	PyObject* pAffectedRows = NULL;
	PyObject* pErrorMsg = NULL;

	s >> callbackID;
	s >> err;

	if(err.size() <= 0)
	{
		s >> nfields;

		pErrorMsg = Py_None;
		Py_INCREF(pErrorMsg);

		if(nfields > 0)
		{
			pAffectedRows = Py_None;
			Py_INCREF(pAffectedRows);

			s >> nrows;

			pResultSet = PyList_New(nrows);
			for (uint32 i = 0; i < nrows; ++i)
			{
				PyObject* pRow = PyList_New(nfields);
				for(uint32 j = 0; j < nfields; ++j)
				{
					std::string cell;
					s.readBlob(cell);

					PyObject* pCell = NULL;
						
					if(cell == "NULL")
					{
						Py_INCREF(Py_None);
						pCell = Py_None;
					}
					else
					{
						pCell = PyBytes_FromStringAndSize(cell.data(), cell.length());
					}

					PyList_SET_ITEM(pRow, j, pCell);
				}

				PyList_SET_ITEM(pResultSet, i, pRow);
			}
		}
		else
		{
			pResultSet = Py_None;
			Py_INCREF(pResultSet);

			pErrorMsg = Py_None;
			Py_INCREF(pErrorMsg);

			s >> affectedRows;

			pAffectedRows = PyLong_FromUnsignedLongLong(affectedRows);
		}
	}
	else
	{
			pResultSet = Py_None;
			Py_INCREF(pResultSet);

			pErrorMsg = PyUnicode_FromString(err.c_str());

			pAffectedRows = Py_None;
			Py_INCREF(pAffectedRows);
	}

	DEBUG_MSG("Baseapp::onExecuteRawDatabaseCommandCB: nrows=%u, nfields=%u, err=%s.\n", 
		nrows, nfields, err.c_str());

	if(callbackID > 0)
	{
		PyObjectPtr pyfunc = pyCallbackMgr_.take(callbackID);
		PyObject* pyResult = PyObject_CallFunction(pyfunc.get(), 
											const_cast<char*>("OOO"), 
											pResultSet, pAffectedRows, pErrorMsg);

		if(pyResult != NULL)
			Py_DECREF(pyResult);
		else
			SCRIPT_ERROR_CHECK();
	}

	Py_XDECREF(pResultSet);
	Py_XDECREF(pAffectedRows);
	Py_XDECREF(pErrorMsg);
}

//-------------------------------------------------------------------------------------
void Baseapp::onDbmgrInitCompleted(Mercury::Channel* pChannel, 
		ENTITY_ID startID, ENTITY_ID endID, int32 startGlobalOrder, int32 startGroupOrder)
{
	if(pChannel->isExternal())
		return;

	EntityApp<Base>::onDbmgrInitCompleted(pChannel, startID, endID, 
		startGlobalOrder, startGroupOrder);

	PyObject* pyResult = PyObject_CallMethod(getEntryScript().get(), 
										const_cast<char*>("onInit"), 
										const_cast<char*>("i"), 
										0);

	if(pyResult != NULL)
		Py_DECREF(pyResult);
	else
		SCRIPT_ERROR_CHECK();

	// ���нű����������
	pyResult = PyObject_CallMethod(getEntryScript().get(), 
										const_cast<char*>("onBaseAppReady"), 
										const_cast<char*>("i"), 
										startGroupOrder);

	if(pyResult != NULL)
		Py_DECREF(pyResult);
	else
		SCRIPT_ERROR_CHECK();
}

//-------------------------------------------------------------------------------------
void Baseapp::onBroadcastGlobalBasesChange(Mercury::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;

	std::string key, value;
	bool isDelete;
	
	s >> isDelete;

	s.readBlob(key);

	if(!isDelete)
	{
		s.readBlob(value);
	}

	PyObject * pyKey = script::Pickler::unpickle(key);
	if(pyKey == NULL)
	{
		ERROR_MSG("Baseapp::onBroadcastCellAppDataChange: no has key!\n");
		return;
	}

	Py_INCREF(pyKey);

	if(isDelete)
	{
		if(pGlobalBases_->del(pyKey))
		{
			// ֪ͨ�ű�
			SCRIPT_OBJECT_CALL_ARGS1(getEntryScript().get(), const_cast<char*>("onGlobalBasesDel"), 
				const_cast<char*>("O"), pyKey);
		}
	}
	else
	{
		PyObject * pyValue = script::Pickler::unpickle(value);
		if(pyValue == NULL)
		{
			ERROR_MSG("Baseapp::onBroadcastCellAppDataChange: no has value!\n");
			Py_DECREF(pyKey);
			return;
		}

		Py_INCREF(pyValue);

		if(pGlobalBases_->write(pyKey, pyValue))
		{
			// ֪ͨ�ű�
			SCRIPT_OBJECT_CALL_ARGS2(getEntryScript().get(), const_cast<char*>("onGlobalBases"), 
				const_cast<char*>("OO"), pyKey, pyValue);
		}

		Py_DECREF(pyValue);
	}

	Py_DECREF(pyKey);
}

//-------------------------------------------------------------------------------------
void Baseapp::registerPendingLogin(Mercury::Channel* pChannel, std::string& accountName, 
								   std::string& password, ENTITY_ID entityID, DBID entityDBID)
{
	if(pChannel->isExternal())
		return;

	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(BaseappmgrInterface::onPendingAccountGetBaseappAddr);

	(*pBundle) << accountName;
	(*pBundle) << this->getNetworkInterface().extaddr().ip;
	(*pBundle) << this->getNetworkInterface().extaddr().port;
	(*pBundle).send(this->getNetworkInterface(), pChannel);

	PendingLoginMgr::PLInfos* ptinfos = new PendingLoginMgr::PLInfos;
	ptinfos->accountName = accountName;
	ptinfos->password = password;
	ptinfos->entityID = entityID;
	ptinfos->entityDBID = entityDBID;
	pendingLoginMgr_.add(ptinfos);

	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
}

//-------------------------------------------------------------------------------------
void Baseapp::loginGatewayFailed(Mercury::Channel* pChannel, std::string& accountName, 
								 SERVER_ERROR_CODE failedcode)
{
	if(failedcode == SERVER_ERR_NAME)
	{
		DEBUG_MSG("Baseapp::login: not found user[%s], login is failed!\n", 
			accountName.c_str());

		failedcode = SERVER_ERR_NAME_PASSWORD;
	}
	else if(failedcode == SERVER_ERR_PASSWORD)
	{
		DEBUG_MSG("Baseapp::login: user[%s] password is error, login is failed!\n", 
			accountName.c_str());

		failedcode = SERVER_ERR_NAME_PASSWORD;
	}

	if(pChannel == NULL)
		return;

	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(ClientInterface::onLoginGatewayFailed);
	ClientInterface::onLoginGatewayFailedArgs1::staticAddToBundle((*pBundle), failedcode);
	(*pBundle).send(this->getNetworkInterface(), pChannel);
	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
}

//-------------------------------------------------------------------------------------
void Baseapp::loginGateway(Mercury::Channel* pChannel, 
						   std::string& accountName, 
						   std::string& password)
{
	DEBUG_MSG("Baseapp::loginGateway: new user[%s].\n", accountName.c_str());

	Components::COMPONENTS cts = Components::getSingleton().getComponents(DBMGR_TYPE);
	Components::ComponentInfos* dbmgrinfos = NULL;

	if(cts.size() > 0)
		dbmgrinfos = &(*cts.begin());

	if(dbmgrinfos == NULL || dbmgrinfos->pChannel == NULL || dbmgrinfos->cid == 0)
	{
		loginGatewayFailed(pChannel, accountName, SERVER_ERR_SRV_NO_READY);
		return;
	}

	PendingLoginMgr::PLInfos* ptinfos = pendingLoginMgr_.find(accountName);
	if(ptinfos == NULL)
	{
		loginGatewayFailed(pChannel, accountName, SERVER_ERR_NAME);
		return;
	}

	if(ptinfos->password != password)
	{
		loginGatewayFailed(pChannel, accountName, SERVER_ERR_PASSWORD);
		return;
	}

	// ���entityID����0��˵����entity�Ǵ��״̬��¼
	if(ptinfos->entityID > 0)
	{
		DEBUG_MSG("Baseapp::loginGateway: user[%s] has entity(%d).\n", 
			accountName.c_str(), ptinfos->entityID);

		Proxy* base = static_cast<Proxy*>(findEntity(ptinfos->entityID));
		if(base == NULL)
		{
			loginGatewayFailed(pChannel, accountName, SERVER_ERR_SRV_NO_READY);
			return;
		}
		
		if(base->getClientMailbox() == NULL)
		{
			// ����entity�Ŀͻ���mailbox
			EntityMailbox* entityClientMailbox = new EntityMailbox(base->getScriptModule(), 
				&pChannel->addr(), 0, base->getID(), MAILBOX_TYPE_CLIENT);

			base->setClientMailbox(entityClientMailbox);
			base->addr(pChannel->addr());

			// ��ͨ������Ĺ�ϵ���entity�󶨣� �ں���ͨ���п��ṩ��ݺϷ���ʶ��
			entityClientMailbox->getChannel()->proxyID(base->getID());
			createClientProxies(base, true);
		}
		else
		{
			// ֪ͨ�ű��쳣��¼�����нű������Ƿ��������ͨ��ǿ�Ƶ�¼
			int32 ret = base->onLogOnAttempt(inet_ntoa((struct in_addr&)pChannel->addr().ip), 
				ntohs(pChannel->addr().port), password.c_str());

			switch(ret)
			{
			case LOG_ON_ACCEPT:
				DEBUG_MSG("Baseapp::loginGateway: script LOG_ON_ACCEPT.\n");
				if(base->getClientMailbox() != NULL)
				{
					// ͨ���ڱ𴦵�¼
					Mercury::Channel* pOldClientChannel = base->getClientMailbox()->getChannel();
					if(pOldClientChannel != NULL)
					{
						loginGatewayFailed(pOldClientChannel, accountName, SERVER_ERR_ANOTHER_LOGON);
						pOldClientChannel->proxyID(0);
					}

					base->getClientMailbox()->addr(pChannel->addr());
					base->addr(pChannel->addr());
					createClientProxies(base, true);
				}
			case LOG_ON_WAIT_FOR_DESTROY:
			default:
				DEBUG_MSG("Baseapp::loginGateway: script LOG_ON_REJECT.\n");
				loginGatewayFailed(pChannel, accountName, SERVER_ERR_ACCOUNT_ONLINE);
				return;
			};
		}
	}
	else
	{
		Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
		(*pBundle).newMessage(DbmgrInterface::queryAccount);

		ENTITY_ID entityID = idClient_.alloc();
		KBE_ASSERT(entityID > 0);

		DbmgrInterface::queryAccountArgs5::staticAddToBundle((*pBundle), accountName, password, g_componentID, 
			entityID, ptinfos->entityDBID);

		(*pBundle).send(this->getNetworkInterface(), dbmgrinfos->pChannel);
		Mercury::Bundle::ObjPool().reclaimObject(pBundle);
	}

	// ��¼�ͻ��˵�ַ
	ptinfos->addr = pChannel->addr();
}

//-------------------------------------------------------------------------------------
void Baseapp::reLoginGateway(Mercury::Channel* pChannel, std::string& accountName, 
							 std::string& password, uint64 key, ENTITY_ID entityID)
{
	DEBUG_MSG("Baseapp::reLoginGateway: accountName=%s, key="PRIu64", entityID=%d.\n", 
		accountName.c_str(), key, entityID);

	Base* base = findEntity(entityID);
	if(base == NULL || !PyObject_TypeCheck(base, Proxy::getScriptType()))
	{
		loginGatewayFailed(pChannel, accountName, SERVER_ERR_ILLEGAL_LOGIN);
		return;
	}
	
	Proxy* proxy = static_cast<Proxy*>(base);
	
	if(proxy->rndUUID() != key)
	{
		loginGatewayFailed(pChannel, accountName, SERVER_ERR_ILLEGAL_LOGIN);
		return;
	}

	if(proxy->getClientMailbox() == NULL)
	{
		// ����entity�Ŀͻ���mailbox
		EntityMailbox* entityClientMailbox = new EntityMailbox(proxy->getScriptModule(), 
			&pChannel->addr(), 0, proxy->getID(), MAILBOX_TYPE_CLIENT);

		proxy->setClientMailbox(entityClientMailbox);
		proxy->addr(pChannel->addr());

		// ��ͨ������Ĺ�ϵ���entity�󶨣� �ں���ͨ���п��ṩ��ݺϷ���ʶ��
		entityClientMailbox->getChannel()->proxyID(proxy->getID());
		createClientProxies(proxy, true);
	}
	else
	{
		WARNING_MSG("Baseapp::reLoginGateway: accountName=%s, key="PRIu64", "
			"entityID=%d, ClientMailbox is exist.\n", 
			accountName.c_str(), key, entityID);
	}
}

//-------------------------------------------------------------------------------------
void Baseapp::onQueryAccountCBFromDbmgr(Mercury::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;

	std::string accountName;
	std::string password;
	bool success = false;
	DBID dbid;
	ENTITY_ID entityID;

	s >> accountName >> password >> dbid >> success >> entityID;

	PendingLoginMgr::PLInfos* ptinfos = pendingLoginMgr_.remove(accountName);
	if(ptinfos == NULL)
		return;

	if(!success)
	{
		ERROR_MSG("Baseapp::onQueryAccountCBFromDbmgr: query %s is failed!\n", accountName.c_str());
		return;
	}

	Proxy* base = static_cast<Proxy*>(createEntityCommon(g_serverConfig.getDBMgr().dbAccountEntityScriptType, 
		NULL, false, entityID));

	Mercury::Channel* pClientChannel = this->getNetworkInterface().findChannel(ptinfos->addr);

	KBE_ASSERT(base != NULL);
	base->hasDB(true);
	base->setDBID(dbid);

	PyObject* pyDict = create_celldatadict_from_stream(s, g_serverConfig.getDBMgr().dbAccountEntityScriptType);
	base->initializeEntity(pyDict);
	Py_DECREF(pyDict);

	if(pClientChannel != NULL)
	{
		// ����entity�Ŀͻ���mailbox
		EntityMailbox* entityClientMailbox = new EntityMailbox(base->getScriptModule(), 
			&pClientChannel->addr(), 0, base->getID(), MAILBOX_TYPE_CLIENT);

		base->setClientMailbox(entityClientMailbox);
		base->addr(pClientChannel->addr());

		createClientProxies(base);

		Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
		(*pBundle).newMessage(DbmgrInterface::onAccountOnline);

		DbmgrInterface::onAccountOnlineArgs3::staticAddToBundle((*pBundle), accountName, 
			componentID_, base->getID());

		(*pBundle).send(this->getNetworkInterface(), pChannel);
		Mercury::Bundle::ObjPool().reclaimObject(pBundle);
	}

	DEBUG_MSG("Baseapp::onQueryAccountCBFromDbmgr: user[%s], uuid[%"PRIu64"], entityID=%d.\n", 
		accountName.c_str(), base->rndUUID(), base->getID());

	SAFE_RELEASE(ptinfos);
}

//-------------------------------------------------------------------------------------
void Baseapp::onEntityEnterWorldFromCellapp(Mercury::Channel* pChannel, ENTITY_ID entityID)
{
	if(pChannel->isExternal())
		return;

	Proxy* base = static_cast<Proxy*>(pEntities_->find(entityID));
	// DEBUG_MSG("Baseapp::onEntityEnterWorldFromCellapp: entityID %d.\n", entityID);
	KBE_ASSERT(base != NULL);

	Mercury::Channel* pClientChannel = this->getNetworkInterface().findChannel(base->addr());
	if(pClientChannel)
	{
		Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
		(*pBundle).newMessage(ClientInterface::onEntityEnterWorld);

		ClientInterface::onEntityEnterWorldArgs2::staticAddToBundle((*pBundle), entityID, 
			base->getSpaceID());

		(*pBundle).send(this->getNetworkInterface(), pClientChannel);
		Mercury::Bundle::ObjPool().reclaimObject(pBundle);
	}
}

//-------------------------------------------------------------------------------------
void Baseapp::onEntityLeaveWorldFromCellapp(Mercury::Channel* pChannel, 
											ENTITY_ID entityID)
{
	if(pChannel->isExternal())
		return;

	Proxy* base = static_cast<Proxy*>(pEntities_->find(entityID));
	// DEBUG_MSG("Baseapp::onEntityEnterWorldFromCellapp: entityID %d.\n", entityID);
	KBE_ASSERT(base != NULL);

	Mercury::Channel* pClientChannel = this->getNetworkInterface().findChannel(base->addr());
	if(pClientChannel)
	{
		Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
		(*pBundle).newMessage(ClientInterface::onEntityLeaveWorld);

		ClientInterface::onEntityLeaveWorldArgs2::staticAddToBundle((*pBundle), entityID, 
			base->getSpaceID());

		(*pBundle).send(this->getNetworkInterface(), pClientChannel);
		Mercury::Bundle::ObjPool().reclaimObject(pBundle);
	}
}

//-------------------------------------------------------------------------------------
void Baseapp::onEntityEnterSpaceFromCellapp(Mercury::Channel* pChannel, 
											ENTITY_ID entityID, SPACE_ID spaceID)
{
	if(pChannel->isExternal())
		return;
}

//-------------------------------------------------------------------------------------
void Baseapp::onEntityLeaveSpaceFromCellapp(Mercury::Channel* pChannel, 
											ENTITY_ID entityID, SPACE_ID spaceID)
{
	if(pChannel->isExternal())
		return;
}

//-------------------------------------------------------------------------------------
void Baseapp::forwardMessageToClientFromCellapp(Mercury::Channel* pChannel, 
												KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;
	
	ENTITY_ID eid;
	s >> eid;

	Base* base = pEntities_->find(eid);
	if(base == NULL)
	{
		ERROR_MSG("Baseapp::forwardMessageToClientFromCellapp: entityID %d not found.\n", eid);
		return;
	}

	EntityMailboxAbstract* mailbox = static_cast<EntityMailboxAbstract*>(base->getClientMailbox());
	if(mailbox == NULL)
	{
		ERROR_MSG("Baseapp::forwardMessageToClientFromCellapp: "
			"occur a error(can't found clientMailbox)! entityID=%d.\n", 
			eid);

		return;
	}
	
	if(s.opsize() <= 0)
		return;

	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
	(*pBundle).append(s);
	s.read_skip(s.opsize());
	mailbox->postMail((*pBundle));
	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
}

//-------------------------------------------------------------------------------------
void Baseapp::onEntityMail(Mercury::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;

	ENTITY_ID eid;
	s >> eid;

	ENTITY_MAILBOX_TYPE	mailtype;
	s >> mailtype;

	// �ڱ��������Բ��Ҹ��ռ�����Ϣ�� ���ռ����Ƿ����ڱ�����
	Base* base = pEntities_->find(eid);
	if(base == NULL)
	{
		ERROR_MSG("Baseapp::onEntityMail: entityID %d not found.\n", eid);
		return;
	}
	
	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
	Mercury::Bundle& bundle = *pBundle;

	switch(mailtype)
	{
		case MAILBOX_TYPE_BASE:		// �������baseapp����ôȷ���ʼ���Ŀ�ĵ������ ��ôִ�����ղ���
			base->onRemoteMethodCall(pChannel, s);
			break;
		case MAILBOX_TYPE_CELL_VIA_BASE: // entity.cell.base.xxx
			{
				EntityMailboxAbstract* mailbox = static_cast<EntityMailboxAbstract*>(base->getCellMailbox());
				if(mailbox == NULL)
				{
					ERROR_MSG("Baseapp::onEntityMail: occur a error(can't found cellMailbox)! "
						"mailboxType=%d, entityID=%d.\n", mailtype, eid);
					break;
				}
				
				mailbox->newMail(bundle);
				bundle.append(s);
				mailbox->postMail(bundle);
			}
			break;
		case MAILBOX_TYPE_CLIENT_VIA_BASE: // entity.base.client
			{
				EntityMailboxAbstract* mailbox = static_cast<EntityMailboxAbstract*>(base->getClientMailbox());
				if(mailbox == NULL)
				{
					ERROR_MSG("Baseapp::onEntityMail: occur a error(can't found clientMailbox)! "
						"mailboxType=%d, entityID=%d.\n", 
						mailtype, eid);

					break;
				}
				
				mailbox->newMail(bundle);
				bundle.append(s);
				s.read_skip(s.opsize());
				mailbox->postMail(bundle);
			}
			break;
		default:
			{
				ERROR_MSG("Baseapp::onEntityMail: mailboxType %d is error! must a baseType. entityID=%d.\n", 
					mailtype, eid);
			}
	};

	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
}

//-------------------------------------------------------------------------------------
void Baseapp::onRemoteCallCellMethodFromClient(Mercury::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isInternal())
		return;

	ENTITY_ID srcEntityID = pChannel->proxyID();
	if(srcEntityID <= 0)
		return;
	
	if(s.opsize() <= 0)
		return;

	KBEngine::Proxy* e = static_cast<KBEngine::Proxy*>
			(KBEngine::Baseapp::getSingleton().findEntity(srcEntityID));		

	if(e == NULL || e->getCellMailbox() == NULL)
		return;

	Mercury::Bundle* pBundle = Mercury::Bundle::ObjPool().createObject();
	(*pBundle).newMessage(CellappInterface::onRemoteCallMethodFromClient);
	(*pBundle) << srcEntityID;
	(*pBundle).append(s);
	
	e->getCellMailbox()->postMail((*pBundle));
	s.read_skip(s.opsize());
	Mercury::Bundle::ObjPool().reclaimObject(pBundle);
}

//-------------------------------------------------------------------------------------
void Baseapp::onBackupEntityCellData(Mercury::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;

	ENTITY_ID baseID = 0;
	s >> baseID;
	
	INFO_MSG("Baseapp::onBackupEntityCellData: entityID=%d, size=%u.\n", baseID, s.opsize());

	Base* base = this->findEntity(baseID);

	if(base)
	{
		base->onBackupCellData(pChannel, s);
	}
	else
	{
		ERROR_MSG("Baseapp::onBackupEntityCellData: not found entityID=%d\n", baseID);
	}
}

//-------------------------------------------------------------------------------------
void Baseapp::onCellWriteToDBCompleted(Mercury::Channel* pChannel, KBEngine::MemoryStream& s)
{
	if(pChannel->isExternal())
		return;

	ENTITY_ID baseID = 0;
	CALLBACK_ID callbackID = 0;

	s >> baseID;
	s >> callbackID;

	INFO_MSG("Baseapp::onCellWriteToDBCompleted: entityID=%d.\n", 
		baseID);

	Base* base = this->findEntity(baseID);

	if(base)
	{
		base->onCellWriteToDBCompleted(callbackID);
	}
	else
	{
		ERROR_MSG("Baseapp::onCellWriteToDBCompleted: not found entityID=%d\n", baseID);
	}
}

//-------------------------------------------------------------------------------------
void Baseapp::onClientActiveTick(Mercury::Channel* pChannel)
{
	if(!pChannel->isExternal())
		return;

	onAppActiveTick(pChannel, CLIENT_TYPE, 0);
}

//-------------------------------------------------------------------------------------
void Baseapp::onWriteToDBCallback(Mercury::Channel* pChannel, ENTITY_ID eid, 
								  DBID entityDBID, CALLBACK_ID callbackID, bool success)
{
	if(pChannel->isExternal())
		return;

	Base* base = pEntities_->find(eid);
	if(base == NULL)
	{
		// ERROR_MSG("Baseapp::onWriteToDBCallback: can't found entity:%d.\n", eid);
		return;
	}

	base->onWriteToDBCallback(eid, entityDBID, callbackID, success);
}

//-------------------------------------------------------------------------------------

}

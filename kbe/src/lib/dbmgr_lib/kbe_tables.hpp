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

#ifndef __KBE_KBE_TABLES__
#define __KBE_KBE_TABLES__

#include "entity_table.hpp"
#include "cstdkbe/cstdkbe.hpp"
#include "cstdkbe/memorystream.hpp"
#include "helper/debug_helper.hpp"

namespace KBEngine { 

class KBETable : public EntityTable
{
public:
	KBETable():
	EntityTable()
	{
	}
	
	virtual ~KBETable()
	{
	}
	
	/**
		同步entity表到数据库中
	*/
	virtual bool syncToDB(DBInterface* dbi) = 0;
	
	/**
		初始化
	*/
	virtual bool initialize(ScriptDefModule* sm, std::string name){ return true; };
	
	virtual EntityTableItem* createItem(std::string type) {return NULL;}
	
protected:

};

/*
	kbe系统表
*/
class KBEEntityLogTable : public KBETable
{
public:
	struct EntityLog
	{
		DBID dbid;
		ENTITY_ID entityID;
		uint32 ip;
		uint16 port;
		COMPONENT_ID componentID;
	};

	KBEEntityLogTable():
	KBETable()
	{
		tableName("kbe_entitylog");
	}
	
	virtual ~KBEEntityLogTable()
	{
	}
	
	virtual bool logEntity(DBInterface * dbi, const char* ip, uint32 port, DBID dbid,
						COMPONENT_ID componentID, ENTITY_ID entityID, ENTITY_SCRIPT_UID entityType) = 0;

	virtual bool queryEntity(DBInterface * dbi, DBID dbid, EntityLog& entitylog, ENTITY_SCRIPT_UID entityType) = 0;

	virtual bool eraseEntityLog(DBInterface * dbi, DBID dbid) = 0;
protected:
	
};

class KBEAccountTable : public KBETable
{
public:
	KBEAccountTable():
	KBETable(),
	accountDefMemoryStream_()
	{
		tableName("kbe_accountinfos");
	}
	
	virtual ~KBEAccountTable()
	{
	}

	virtual bool queryAccount(DBInterface * dbi, std::string& name, ACCOUNT_INFOS& info) = 0;
	virtual bool logAccount(DBInterface * dbi, ACCOUNT_INFOS& info) = 0;

	MemoryStream& accountDefMemoryStream(){ return accountDefMemoryStream_; }
	void accountDefMemoryStream(MemoryStream& s){accountDefMemoryStream_.append(s.data() + s.rpos(), s.opsize()); }
protected:
	MemoryStream accountDefMemoryStream_;
};

class KBEEntityType : public KBETable
{
public:
	KBEEntityType():
	KBETable()
	{
		tableName("kbe_entitytypes");
	}
	
	virtual ~KBEEntityType()
	{
	}


protected:
};


}

#endif // __KBE_KBE_TABLES__

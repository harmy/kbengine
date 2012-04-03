#include "dataTypes.hpp"
namespace KBEngine{
DataTypes::DATATYPE_MAP DataTypes::dataTypes_;

//-------------------------------------------------------------------------------------
DataTypes::DataTypes()
{
}

//-------------------------------------------------------------------------------------
DataTypes::~DataTypes()
{
}

//-------------------------------------------------------------------------------------
void DataTypes::finish(void)
{
	DATATYPE_MAP::iterator iter = dataTypes_.begin();
	for (; iter != dataTypes_.end(); iter++) 
		iter->second->decRef();

	dataTypes_.clear();
}

//-------------------------------------------------------------------------------------
bool DataTypes::initialize(std::string file)
{
	// ��ʼ��һЩ�������
	addDateType("UINT8",	new IntType<uint8>);
	addDateType("UINT16",	new IntType<uint16>);
	addDateType("UINT64",	new UInt64Type);
	addDateType("UINT32",	new UInt32Type);

	addDateType("INT8",		new IntType<int8>);
	addDateType("INT16",	new IntType<int16>);
	addDateType("INT32",	new IntType<int32>);
	addDateType("INT64",	new Int64Type);

	addDateType("STRING",	new StringType);
	addDateType("FLOAT",	new FloatType);
	addDateType("PYTHON",	new PythonType);
	addDateType("MAILBOX",	new MailboxType);
	
	addDateType("VECTOR2",	new VectorType(2));
	addDateType("VECTOR3",	new VectorType(3));
	addDateType("VECTOR4",	new VectorType(4));
	return loadAlias(file);
}

//-------------------------------------------------------------------------------------
bool DataTypes::loadAlias(std::string& file)
{
	TiXmlNode* node = NULL;
	XmlPlus* xml = new XmlPlus(file.c_str());
	node = xml->getRootNode();

	XML_FOR_BEGIN(node)
	{
		std::string type = "";
		std::string aliasName = xml->getKey(node);
		TiXmlNode* childNode = node->FirstChild();

		if(childNode != NULL)
		{
			type = xml->getValStr(childNode);
			if(type == "FIXED_DICT")
			{
				FixedDictType* fixedDict = new FixedDictType;
				if(fixedDict->initialize(xml, childNode))
					addDateType(aliasName, fixedDict);
				else
				{
					ERROR_MSG("DataTypes::loadAlias:parse FIXED_DICT [%s] is error!\n", aliasName.c_str());
					return false;
				}
			}
			else
			{
				DataType* dataType = getDataType(type);
				if(dataType == NULL)
				{
					ERROR_MSG("DataTypes::loadAlias:can't fount type %s by alias[%s].\n", type.c_str(), aliasName.c_str());
					return false;
				}

				addDateType(aliasName, dataType);
			}
		}
	}
	XML_FOR_END(node);
	
	delete xml;
	return true;
}

//-------------------------------------------------------------------------------------
bool DataTypes::addDateType(std::string name, DataType* dataType)
{
	DATATYPE_MAP::iterator iter = dataTypes_.find(name);
	if (iter != dataTypes_.end()){ 
		ERROR_MSG("DataTypes::addDateType:exist a type %s.\n", name.c_str());
		return false;
	}

	dataTypes_[name] = dataType;
	dataType->addRef();
	return true;
}

//-------------------------------------------------------------------------------------
void DataTypes::delDataType(std::string name)
{
	DATATYPE_MAP::iterator iter = dataTypes_.find(name);
	if (iter == dataTypes_.end())
		ERROR_MSG("DataTypes::delDataType:not found type %s.\n", name.c_str());
	else
	{
		iter->second->decRef();
		dataTypes_.erase(iter);
	}
}

//-------------------------------------------------------------------------------------
DataType* DataTypes::getDataType(std::string name)
{
	DATATYPE_MAP::iterator iter = dataTypes_.find(name);
	if (iter != dataTypes_.end()) 
		return iter->second;

	ERROR_MSG("DataTypes::getDataType:not found type %s.\n", name.c_str());
	return NULL;
}

//-------------------------------------------------------------------------------------
DataType* DataTypes::getDataType(const char* name)
{
	DATATYPE_MAP::iterator iter = dataTypes_.find(name);
	if (iter != dataTypes_.end()) 
		return iter->second;

	ERROR_MSG("DataTypes::getDataType:not found type %s.\n", name);
	return NULL;
}


//-------------------------------------------------------------------------------------

}
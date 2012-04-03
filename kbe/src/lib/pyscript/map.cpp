#include "map.hpp"
namespace KBEngine{ namespace script{

/** python map��������Ҫ�ķ����� */
PyMappingMethods Map::mappingMethods =
{
	(lenfunc)Map::mp_length,					// mp_length
	(binaryfunc)Map::mp_subscript,				// mp_subscript
	(objobjargproc)Map::mp_ass_subscript		// mp_ass_subscript
};


SCRIPT_METHOD_DECLARE_BEGIN(Map)
SCRIPT_METHOD_DECLARE("has_key",			_has_key,			METH_VARARGS,		0)
SCRIPT_METHOD_DECLARE("keys",				_keys,				METH_VARARGS,		0)
SCRIPT_METHOD_DECLARE("values",				_values,			METH_VARARGS,		0)
SCRIPT_METHOD_DECLARE("items",				_items,				METH_VARARGS,		0)
SCRIPT_METHOD_DECLARE("update",				_update,			METH_VARARGS,		0)	
SCRIPT_METHOD_DECLARE_END()

SCRIPT_MEMBER_DECLARE_BEGIN(Map)
SCRIPT_MEMBER_DECLARE_END()

SCRIPT_GETSET_DECLARE_BEGIN(Map)
SCRIPT_GETSET_DECLARE_END()
SCRIPT_INIT(Map, 0, 0, &Map::mappingMethods, 0, 0)	
	
//-------------------------------------------------------------------------------------
Map::Map(PyTypeObject* pyType, bool isInitialised):
ScriptObject(pyType, isInitialised)
{
	m_pyDict_ = PyDict_New();
}

//-------------------------------------------------------------------------------------
Map::~Map()
{
	Py_DECREF(m_pyDict_);
}

//-------------------------------------------------------------------------------------
int Map::mp_length(PyObject* self)
{
	return PyDict_Size(static_cast<Map*>(self)->m_pyDict_);
}

//-------------------------------------------------------------------------------------
int Map::mp_ass_subscript(PyObject* self, PyObject* key, PyObject* value)
{
	Map* lpScriptData = static_cast<Map*>(self);
	std::string skey = script::Pickler::pickle(key, 0);
	std::string sval = script::Pickler::pickle(value, 0);
	
	if (value == NULL)
	{
		sval = "";
		lpScriptData->onDataChanged(skey, sval, true);
		return PyDict_DelItem(lpScriptData->m_pyDict_, key);
	}
	
	lpScriptData->onDataChanged(sval, sval);
	return PyDict_SetItem(lpScriptData->m_pyDict_, key, value);
}

//-------------------------------------------------------------------------------------
void Map::onDataChanged(std::string& key, std::string& value, bool isDelete)
{
}
	
//-------------------------------------------------------------------------------------
PyObject* Map::mp_subscript(PyObject* self, PyObject* key)
{
	Map* lpScriptData = static_cast<Map*>(self);

	PyObject* pyObj = PyDict_GetItem(lpScriptData->m_pyDict_, key);
	if (!pyObj)
		PyErr_SetObject(PyExc_KeyError, key);
	else
		Py_INCREF(pyObj);
	return pyObj;
}

//-------------------------------------------------------------------------------------
PyObject* Map::_has_key(PyObject* self, PyObject* args)
{
	char key[] = {"has_key\0"}; // ��ֹgcc -Werror����
	char fmt[] = {"0\0"};
	return PyObject_CallMethod(static_cast<Map*>(self)->m_pyDict_, key, fmt, args);
}

//-------------------------------------------------------------------------------------
PyObject* Map::_keys(PyObject* self, PyObject* args)
{
	return PyDict_Keys(static_cast<Map*>(self)->m_pyDict_);
}

//-------------------------------------------------------------------------------------
PyObject* Map::_values(PyObject* self, PyObject* args)
{
	return PyDict_Values(static_cast<Map*>(self)->m_pyDict_);
}

//-------------------------------------------------------------------------------------
PyObject* Map::_items(PyObject* self, PyObject* args)
{
	return PyDict_Items(static_cast<Map*>(self)->m_pyDict_);
}

//-------------------------------------------------------------------------------------
PyObject* Map::_update(PyObject* self, PyObject* args)
{
	PyDict_Update(static_cast<Map*>(self)->m_pyDict_, args);
	S_Return; 
}

//-------------------------------------------------------------------------------------

}
}
// +build ignore

/*
  Python wrapper around C++ class.
*/
#include <Python.h>
#include "adclient.h"

static PyObject *ADBindError;
static PyObject *ADSearchError;
static PyObject *ADOperationalError;
static int error_num;

string unicode2string(PyObject *pobj) {
    return PyBytes_AS_STRING( PyUnicode_AsEncodedString(pobj, "utf-8", "Error ~") );
}

PyObject *vector2list(vector <string> vec) {
    string st;
    PyObject *list = PyList_New(vec.size());;

    for (unsigned int j=0; (j < vec.size()); j++) {
        if (PyList_SET_ITEM(list, j, PyUnicode_FromString(vec[j].c_str())) < 0)
            return NULL;
    }
    return list;
}

adclient *convert_ad(PyObject *obj) {
    void * temp = PyCapsule_GetPointer(obj, "adclient");
    return static_cast<adclient*>(temp);
}

static PyObject *wrapper_get_error_num(PyObject *self, PyObject *args) {
    return Py_BuildValue("i", error_num);
}

static PyObject *wrapper_int2ip(PyObject *self, PyObject *args) {
    char *ipstr;
    if (!PyArg_ParseTuple(args, "s", &ipstr)) return NULL;
    string result = int2ip(ipstr);
    return Py_BuildValue("s", result.c_str());
}

void delete_adclient(PyObject *capsule) {
    adclient *ad = convert_ad(capsule);
    delete ad;
}

static PyObject *wrapper_new_adclient(PyObject *self, PyObject *args) {
    error_num = 0;
    adclient *obj = new adclient();
    return PyCapsule_New(obj, "adclient", delete_adclient);
}

static PyObject *wrapper_login_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *binddn, *bindpw, *search_base;
    int secured;
    PyObject * listObj;
    unsigned int numLines;

    if (!PyArg_ParseTuple(args, "OO!sssi", &obj, &PyList_Type, &listObj, &binddn, &bindpw, &search_base, &secured)) return NULL;

    if ((numLines = PyList_Size(listObj)) < 0) return NULL; /* Not a list */

    vector <string> uries;
    for (unsigned int i=0; i<numLines; i++) {
       PyObject *pListItem = PyList_GetItem(listObj, i);
       string item = unicode2string(pListItem);
       uries.push_back(item);
    }

    adclient *ad = convert_ad(obj);
    try {
       ad->login(uries, binddn, bindpw, search_base, secured == 1);
    }
    catch(ADBindException& ex) {
         error_num = ex.code;
         PyErr_SetString(ADBindError, ex.msg.c_str());
         return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_binded_uri_adclient(PyObject *self, PyObject *args) {
       PyObject *obj;

       if (!PyArg_ParseTuple(args, "O", &obj)) return NULL;

       adclient *ad = convert_ad(obj);
       return Py_BuildValue("s", ad->binded_uri().c_str());
}

static PyObject *wrapper_search_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *ou, *filter;
    PyObject * listObj;
    unsigned int numLines;
    int scope;

    map < string, map < string, vector<string> > > res;

    if (!PyArg_ParseTuple(args, "OsisO!", &obj, &ou, &scope, &filter, &PyList_Type, &listObj)) return NULL;

    if ((numLines = PyList_Size(listObj)) < 0) return NULL; /* Not a list */

    vector <string> attrs;

    for (unsigned int i=0; i<numLines; i++) {
        PyObject *strObj = PyList_GetItem(listObj, i);
        string item = unicode2string(strObj);
        attrs.push_back(item);
    }

    adclient *ad = convert_ad(obj);
    try {
        res = ad->search(ou, scope, filter, attrs);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }

    PyObject *res_dict = PyDict_New();
    map < string, map < string, vector<string> > >::iterator res_it;
    for ( res_it=res.begin() ; res_it != res.end(); ++res_it ) {
        PyObject *attrs_dict = PyDict_New();
        string dn = (*res_it).first;
        map < string, vector<string> > attrs = (*res_it).second;
        map < string, vector<string> >::iterator attrs_it;
        for ( attrs_it=attrs.begin() ; attrs_it != attrs.end(); ++attrs_it ) {
            string attribute = (*attrs_it).first;
            vector<string> values_v = (*attrs_it).second;
            PyObject *values_list = vector2list(values_v);
            if (PyDict_SetItemString(attrs_dict, attribute.c_str(), values_list) < 0) return NULL;
        }
        if (PyDict_SetItemString(res_dict, dn.c_str(), attrs_dict) < 0) return NULL;
    }
    return res_dict;
}

static PyObject *wrapper_searchDN_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *filter;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &filter)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->searchDN(filter);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getUserGroups_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getUserGroups(user);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getUsersInGroup_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *group;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &group)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getUsersInGroup(group);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getUserControls_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user;
    map <string, bool> result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getUserControls(user);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    PyObject *res_dict = PyDict_New();
    map <string, bool>::iterator it;
    for (it = result.begin(); it != result.end(); ++it) {
        if (PyDict_SetItemString(res_dict, (*it).first.c_str(), PyBool_FromLong((*it).second)) < 0)
            return NULL;
    }

    return res_dict;
}

static PyObject *wrapper_groupAddUser_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *group, *user;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &group, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->groupAddUser(group, user);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_groupRemoveUser_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *group, *user;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &group, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->groupRemoveUser(group, user);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_ifDialinUser_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        return Py_BuildValue("i", ad->ifDialinUser(user));
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
}

static PyObject *wrapper_getDialinUsers_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "O", &obj)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getDialinUsers();
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getDisabledUsers_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "O", &obj)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getDisabledUsers();
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getObjectDN_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user;
    string result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getObjectDN(user);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return Py_BuildValue("s", result.c_str());
}

static PyObject *wrapper_ifUserDisabled_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        return Py_BuildValue("N", PyBool_FromLong(ad->ifUserDisabled(user)));
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
}

static PyObject *wrapper_ifDNExists_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *dn;
    char *objectclass;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &dn, &objectclass)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        return Py_BuildValue("N", PyBool_FromLong(ad->ifDNExists(dn, objectclass)));
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str()); 
        return NULL; 
    }
}

static PyObject *wrapper_getAllOUs_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "O", &obj)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getAllOUs();
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getOUsInOU_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *OU;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &OU)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getOUsInOU(OU);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getUsersInOU_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *OU;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &OU)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getUsersInOU(OU);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getUsersInOU_SubTree_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *OU;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &OU)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getUsersInOU_SubTree(OU);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getGroups_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "O", &obj)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getGroups();
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getUsers_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "O", &obj)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getUsers();
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getUserDisplayName_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user_short;
    string result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user_short)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getUserDisplayName(user_short);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return Py_BuildValue("s", result.c_str());
}

static PyObject *wrapper_getUserIpAddress_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user_short;
    string result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user_short)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getUserIpAddress(user_short);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return Py_BuildValue("s", result.c_str());
}

static PyObject *wrapper_getObjectAttribute_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user_short, *attribute;
    vector <string> result;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user_short, &attribute)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getObjectAttribute(user_short, attribute);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    return vector2list(result);
}

static PyObject *wrapper_getObjectAttributes_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *object_short;
    map <string, vector <string> > result;
    if (!PyArg_ParseTuple(args, "Os", &obj, &object_short)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        result = ad->getObjectAttributes(object_short);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }

    string attr;
    vector <string> values;

    PyObject *fin_list = PyList_New(result.size());

    map<string, vector<string> >::iterator it;
    for (it = result.begin(); it != result.end(); ++it) {
        attr = it->first;
        values = it->second;
        // TODO: some attrs can't be directly converted to unicode (use bytes?)
        PyObject *temp_list = vector2list(values);
        PyObject *tuple = Py_BuildValue("(s,N)", attr.c_str(), temp_list);
        PyList_SET_ITEM(fin_list, std::distance(result.begin(), it), tuple);
    }
    return fin_list;
}

static PyObject *wrapper_CreateComputer_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *name, *container;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &name, &container)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->CreateComputer(name, container);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_CreateUser_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *cn, *short_name, *container;
    if (!PyArg_ParseTuple(args, "Osss", &obj, &cn, &container, &short_name)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->CreateUser(cn, container, short_name);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_CreateGroup_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *cn, *short_name, *container;
    if (!PyArg_ParseTuple(args, "Osss", &obj, &cn, &container, &short_name)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->CreateGroup(cn, container, short_name);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_DeleteDN_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *dn;
    if (!PyArg_ParseTuple(args, "Os", &obj, &dn)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->DeleteDN(dn);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_CreateOU_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *ou;
    if (!PyArg_ParseTuple(args, "Os", &obj, &ou)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->CreateOU(ou);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str()); 
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_EnableUser_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->EnableUser(user);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_DisableUser_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->DisableUser(user);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserDescription_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *dn, *descr;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &dn, &descr)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserDescription(dn, descr);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserPassword_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *password;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &password)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserPassword(user, password);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_checkUserPassword_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *password;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &password)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        return Py_BuildValue("N", PyBool_FromLong(ad->checkUserPassword(user, password)));
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
}

static PyObject *wrapper_setUserDialinAllowed_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserDialinAllowed(user);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserDialinDisabled_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserDialinDisabled(user);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserSN_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *sn;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &sn)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserSN(user, sn);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserInitials_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *initials;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &initials)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserInitials(user, initials);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str()); 
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserGivenName_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *givenName;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &givenName)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserGivenName(user, givenName);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());  
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserDisplayName_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *displayName;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &displayName)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserDisplayName(user, displayName);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());  
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserRoomNumber_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *roomNum;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &roomNum)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserRoomNumber(user, roomNum);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());  
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    } 
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserAddress_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *streetAddress;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &streetAddress)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserAddress(user, streetAddress);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());  
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserInfo_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *info;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &info)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserInfo(user, info);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    } 
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserTitle_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *title;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &title)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserTitle(user, title);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL; 
    } 
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserDepartment_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *department;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &department)) return NULL;
    adclient *ad = convert_ad(obj);
    try { 
        ad->setUserDepartment(user, department);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL; 
    } 
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserCompany_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *company;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &company)) return NULL;
    adclient *ad = convert_ad(obj);
    try { 
        ad->setUserCompany(user, company);
    }
    catch(ADSearchException& ex) { 
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL; 
    } 
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserPhone_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *phone;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &phone)) return NULL;
    adclient *ad = convert_ad(obj);
    try { 
        ad->setUserPhone(user, phone);
    }
    catch(ADSearchException& ex) { 
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL; 
    } 
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    } 
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setUserIpAddress_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user, *ip;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &user, &ip)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setUserIpAddress(user, ip);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_clearObjectAttribute_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *object, *attr;
    if (!PyArg_ParseTuple(args, "Oss", &obj, &object, &attr)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->clearObjectAttribute(object, attr);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_setObjectAttribute_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *object, *attr, *value;
    if (!PyArg_ParseTuple(args, "Osss", &obj, &object, &attr, &value)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->setObjectAttribute(object, attr, value);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *wrapper_UnLockUser_adclient(PyObject *self, PyObject *args) {
    PyObject *obj;
    char *user;
    if (!PyArg_ParseTuple(args, "Os", &obj, &user)) return NULL;
    adclient *ad = convert_ad(obj);
    try {
        ad->UnLockUser(user);
    }
    catch(ADSearchException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADSearchError, ex.msg.c_str());
        return NULL;
    }
    catch(ADOperationalException& ex) {
        error_num = ex.code;
        PyErr_SetString(ADOperationalError, ex.msg.c_str());
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef adclient_methods[] = {
    { "new_adclient",                    (PyCFunction)wrapper_new_adclient,                      METH_VARARGS,   NULL },
    { "login_adclient",                  (PyCFunction)wrapper_login_adclient,                    METH_VARARGS,   NULL },
    { "searchDN_adclient",               (PyCFunction)wrapper_searchDN_adclient,                 METH_VARARGS,   NULL },
    { "search_adclient",                 (PyCFunction)wrapper_search_adclient,                   METH_VARARGS,   NULL },
    { "getUserGroups_adclient",          (PyCFunction)wrapper_getUserGroups_adclient,            METH_VARARGS,   NULL },
    { "getUsersInGroup_adclient",        (PyCFunction)wrapper_getUsersInGroup_adclient,          METH_VARARGS,   NULL },
    { "getUserControls_adclient",        (PyCFunction)wrapper_getUserControls_adclient,          METH_VARARGS,   NULL },
    { "groupAddUser_adclient",           (PyCFunction)wrapper_groupAddUser_adclient,             METH_VARARGS,   NULL },
    { "groupRemoveUser_adclient",        (PyCFunction)wrapper_groupRemoveUser_adclient,          METH_VARARGS,   NULL },
    { "ifDialinUser_adclient",           (PyCFunction)wrapper_ifDialinUser_adclient,             METH_VARARGS,   NULL },
    { "getDialinUsers_adclient",         (PyCFunction)wrapper_getDialinUsers_adclient,           METH_VARARGS,   NULL },
    { "getDisabledUsers_adclient",       (PyCFunction)wrapper_getDisabledUsers_adclient,         METH_VARARGS,   NULL },
    { "getObjectDN_adclient",            (PyCFunction)wrapper_getObjectDN_adclient,              METH_VARARGS,   NULL },
    { "ifUserDisabled_adclient",         (PyCFunction)wrapper_ifUserDisabled_adclient,           METH_VARARGS,   NULL },
    { "getAllOUs_adclient",              (PyCFunction)wrapper_getAllOUs_adclient,                METH_VARARGS,   NULL },
    { "getOUsInOU_adclient",             (PyCFunction)wrapper_getOUsInOU_adclient,               METH_VARARGS,   NULL },
    { "getUsersInOU_adclient",           (PyCFunction)wrapper_getUsersInOU_adclient,             METH_VARARGS,   NULL },
    { "getUsersInOU_SubTree_adclient",   (PyCFunction)wrapper_getUsersInOU_SubTree_adclient,     METH_VARARGS,   NULL },
    { "getGroups_adclient",              (PyCFunction)wrapper_getGroups_adclient,                METH_VARARGS,   NULL },
    { "getUsers_adclient",               (PyCFunction)wrapper_getUsers_adclient,                 METH_VARARGS,   NULL },
    { "getUserDisplayName_adclient",     (PyCFunction)wrapper_getUserDisplayName_adclient,       METH_VARARGS,   NULL },
    { "getUserIpAddress_adclient",       (PyCFunction)wrapper_getUserIpAddress_adclient,         METH_VARARGS,   NULL },
    { "getObjectAttribute_adclient",     (PyCFunction)wrapper_getObjectAttribute_adclient,       METH_VARARGS,   NULL },
    { "getObjectAttributes_adclient",    (PyCFunction)wrapper_getObjectAttributes_adclient,      METH_VARARGS,   NULL },
    { "CreateUser_adclient",             (PyCFunction)wrapper_CreateUser_adclient,               METH_VARARGS,   NULL },
    { "CreateComputer_adclient",         (PyCFunction)wrapper_CreateComputer_adclient,           METH_VARARGS,   NULL },
    { "CreateGroup_adclient",            (PyCFunction)wrapper_CreateGroup_adclient,              METH_VARARGS,   NULL },
    { "DeleteDN_adclient",               (PyCFunction)wrapper_DeleteDN_adclient,                 METH_VARARGS,   NULL },
    { "CreateOU_adclient",               (PyCFunction)wrapper_CreateOU_adclient,                 METH_VARARGS,   NULL },
    { "EnableUser_adclient",             (PyCFunction)wrapper_EnableUser_adclient,               METH_VARARGS,   NULL },
    { "DisableUser_adclient",            (PyCFunction)wrapper_DisableUser_adclient,              METH_VARARGS,   NULL },
    { "setUserDescription_adclient",     (PyCFunction)wrapper_setUserDescription_adclient,       METH_VARARGS,   NULL },
    { "setUserPassword_adclient",        (PyCFunction)wrapper_setUserPassword_adclient,          METH_VARARGS,   NULL },
    { "checkUserPassword_adclient",      (PyCFunction)wrapper_checkUserPassword_adclient,        METH_VARARGS,   NULL },
    { "setUserDialinAllowed_adclient",   (PyCFunction)wrapper_setUserDialinAllowed_adclient,     METH_VARARGS,   NULL },
    { "setUserDialinDisabled_adclient",  (PyCFunction)wrapper_setUserDialinDisabled_adclient,    METH_VARARGS,   NULL },
    { "setUserSN_adclient",              (PyCFunction)wrapper_setUserSN_adclient,                METH_VARARGS,   NULL },
    { "setUserInitials_adclient",        (PyCFunction)wrapper_setUserInitials_adclient,          METH_VARARGS,   NULL },
    { "setUserGivenName_adclient",       (PyCFunction)wrapper_setUserGivenName_adclient,         METH_VARARGS,   NULL },
    { "setUserDisplayName_adclient",     (PyCFunction)wrapper_setUserDisplayName_adclient,       METH_VARARGS,   NULL },
    { "setUserRoomNumber_adclient",      (PyCFunction)wrapper_setUserRoomNumber_adclient,        METH_VARARGS,   NULL },
    { "setUserAddress_adclient",         (PyCFunction)wrapper_setUserAddress_adclient,           METH_VARARGS,   NULL },
    { "setUserInfo_adclient",            (PyCFunction)wrapper_setUserInfo_adclient,              METH_VARARGS,   NULL },
    { "setUserTitle_adclient",           (PyCFunction)wrapper_setUserTitle_adclient,             METH_VARARGS,   NULL },
    { "setUserDepartment_adclient",      (PyCFunction)wrapper_setUserDepartment_adclient,        METH_VARARGS,   NULL },
    { "setUserCompany_adclient",         (PyCFunction)wrapper_setUserCompany_adclient,           METH_VARARGS,   NULL },
    { "setUserPhone_adclient",           (PyCFunction)wrapper_setUserPhone_adclient,             METH_VARARGS,   NULL },
    { "setUserIpAddress_adclient",       (PyCFunction)wrapper_setUserIpAddress_adclient,         METH_VARARGS,   NULL },
    { "clearObjectAttribute_adclient",   (PyCFunction)wrapper_clearObjectAttribute_adclient,     METH_VARARGS,   NULL },
    { "setObjectAttribute_adclient",     (PyCFunction)wrapper_setObjectAttribute_adclient,       METH_VARARGS,   NULL },
    { "UnLockUser_adclient",             (PyCFunction)wrapper_UnLockUser_adclient,               METH_VARARGS,   NULL },
    { "ifDNExists_adclient",             (PyCFunction)wrapper_ifDNExists_adclient,               METH_VARARGS,   NULL },
    { "binded_uri_adclient",             (PyCFunction)wrapper_binded_uri_adclient,               METH_VARARGS,   NULL },
    { "get_error_num",                   (PyCFunction)wrapper_get_error_num,                     METH_VARARGS,   NULL },
    { "int2ip",                          (PyCFunction)wrapper_int2ip,                            METH_VARARGS,   NULL },
    { NULL, NULL }
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_adclient",
    NULL,
    -1,
    adclient_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__adclient() {
    PyObject *module = PyModule_Create(&moduledef);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wwrite-strings"
    ADBindError = PyErr_NewException("ADBindError.error", NULL, NULL);
    ADSearchError = PyErr_NewException("ADSearchError.error", NULL, NULL);
    ADOperationalError = PyErr_NewException("ADOperationalError.error", NULL, NULL);
#pragma GCC diagnostic pop
    Py_INCREF(ADBindError);
    Py_INCREF(ADSearchError);
    Py_INCREF(ADOperationalError);
    PyModule_AddObject(module, "ADBindError", ADBindError);
    PyModule_AddObject(module, "ADSearchError", ADSearchError);
    PyModule_AddObject(module, "ADOperationalError", ADOperationalError);

    PyModule_AddIntMacro(module, AD_SUCCESS);
    PyModule_AddIntMacro(module, AD_LDAP_CONNECTION_ERROR);
    PyModule_AddIntMacro(module, AD_PARAMS_ERROR);
    PyModule_AddIntMacro(module, AD_SERVER_CONNECT_FAILURE);
    PyModule_AddIntMacro(module, AD_OBJECT_NOT_FOUND);
    PyModule_AddIntMacro(module, AD_ATTRIBUTE_ENTRY_NOT_FOUND);
    PyModule_AddIntMacro(module, AD_OU_SYNTAX_ERROR);

    PyModule_AddIntMacro(module, AD_SCOPE_BASE);
    PyModule_AddIntMacro(module, AD_SCOPE_ONELEVEL);
    PyModule_AddIntMacro(module, AD_SCOPE_SUBTREE);

    return module;
}

// vim: ai ts=4 sts=4 et sw=4 expandtab

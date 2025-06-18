#pragma once

#include <Python.h>
#include <string>
#include <vector>

namespace Utils {

	class PythonUtils {
	public:
		static void AddPythonPath(const std::string& path) {
			PyGILState_STATE gstate = PyGILState_Ensure();

			PyObject* sysPath = PySys_GetObject("path");
			PyObject* pyPath = PyUnicode_FromString(path.c_str());
			PyList_Append(sysPath, pyPath);
			Py_DECREF(pyPath);

			PyGILState_Release(gstate);
		}

		static std::vector<std::string> GetPythonPaths() {
			PyGILState_STATE gstate = PyGILState_Ensure();
			std::vector<std::string> paths;

			PyObject* sysPath = PySys_GetObject("path");
			if (sysPath && PyList_Check(sysPath)) {
				Py_ssize_t size = PyList_Size(sysPath);
				for (Py_ssize_t i = 0; i < size; i++) {
					PyObject* item = PyList_GetItem(sysPath, i);
					if (item && PyUnicode_Check(item)) {
						const char* path = PyUnicode_AsUTF8(item);
						if (path) {
							paths.emplace_back(path);
						}
					}
				}
			}

			PyGILState_Release(gstate);
			return paths;
		}

		static bool CheckPythonFunction(const std::string& moduleName, const std::string& functionName) {
			PyGILState_STATE gstate = PyGILState_Ensure();
			bool exists = false;

			PyObject* pModule = PyImport_ImportModule(moduleName.c_str());
			if (pModule) {
				PyObject* pFunc = PyObject_GetAttrString(pModule, functionName.c_str());
				exists = (pFunc && PyCallable_Check(pFunc));
				Py_XDECREF(pFunc);
				Py_DECREF(pModule);
			}

			PyGILState_Release(gstate);
			return exists;
		}
	};

} // namespace Utils
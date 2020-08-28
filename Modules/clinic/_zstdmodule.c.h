/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_zstd_ZstdDict___init____doc__,
"ZstdDict(dict_content)\n"
"--\n"
"\n"
"Initialize ZstdDict object.\n"
"\n"
"  dict_content\n"
"    Dictionary\'s content, a bytes object.");

static int
_zstd_ZstdDict___init___impl(ZstdDict *self, PyObject *dict_content);

static int
_zstd_ZstdDict___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"dict_content", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "ZstdDict", 0};
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *dict_content;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    dict_content = fastargs[0];
    return_value = _zstd_ZstdDict___init___impl((ZstdDict *)self, dict_content);

exit:
    return return_value;
}

PyDoc_STRVAR(_zstd__train_dict__doc__,
"_train_dict($module, /, dst_data, dst_data_sizes, dict_size)\n"
"--\n"
"\n"
"Train a Zstd dictionary.");

#define _ZSTD__TRAIN_DICT_METHODDEF    \
    {"_train_dict", (PyCFunction)(void(*)(void))_zstd__train_dict, METH_FASTCALL|METH_KEYWORDS, _zstd__train_dict__doc__},

static PyObject *
_zstd__train_dict_impl(PyObject *module, PyBytesObject *dst_data,
                       PyObject *dst_data_sizes, Py_ssize_t dict_size);

static PyObject *
_zstd__train_dict(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"dst_data", "dst_data_sizes", "dict_size", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_train_dict", 0};
    PyObject *argsbuf[3];
    PyBytesObject *dst_data;
    PyObject *dst_data_sizes;
    Py_ssize_t dict_size;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 3, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyBytes_Check(args[0])) {
        _PyArg_BadArgument("_train_dict", "argument 'dst_data'", "bytes", args[0]);
        goto exit;
    }
    dst_data = (PyBytesObject *)args[0];
    dst_data_sizes = args[1];
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dict_size = ival;
    }
    return_value = _zstd__train_dict_impl(module, dst_data, dst_data_sizes, dict_size);

exit:
    return return_value;
}

PyDoc_STRVAR(_zstd_compress__doc__,
"compress($module, /, data, level_or_option=None, dict=None)\n"
"--\n"
"\n"
"Returns a bytes object containing compressed data.\n"
"\n"
"  data\n"
"    Binary data to be compressed.\n"
"  level_or_option\n"
"    It can be an int object, in this case represents the compression\n"
"    level. It can also be a dictionary for setting various advanced\n"
"    parameters. The default value None means to use zstd\'s default\n"
"    compression parameters.\n"
"  dict\n"
"    Pre-trained dictionary for compression, a ZstdDict object.");

#define _ZSTD_COMPRESS_METHODDEF    \
    {"compress", (PyCFunction)(void(*)(void))_zstd_compress, METH_FASTCALL|METH_KEYWORDS, _zstd_compress__doc__},

static PyObject *
_zstd_compress_impl(PyObject *module, Py_buffer *data,
                    PyObject *level_or_option, PyObject *dict);

static PyObject *
_zstd_compress(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", "level_or_option", "dict", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "compress", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    PyObject *level_or_option = Py_None;
    PyObject *dict = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("compress", "argument 'data'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        level_or_option = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    dict = args[2];
skip_optional_pos:
    return_value = _zstd_compress_impl(module, &data, level_or_option, dict);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_zstd_decompress__doc__,
"decompress($module, /, data, dict=None, option=None)\n"
"--\n"
"\n"
"Returns a bytes object containing the uncompressed data.\n"
"\n"
"  data\n"
"    Compressed data.\n"
"  dict\n"
"    Pre-trained dictionary for compression, a ZstdDict object.\n"
"  option\n"
"    A dictionary for setting various advanced parameters. The default value\n"
"    None means to use zstd\'s default decompression parameters.");

#define _ZSTD_DECOMPRESS_METHODDEF    \
    {"decompress", (PyCFunction)(void(*)(void))_zstd_decompress, METH_FASTCALL|METH_KEYWORDS, _zstd_decompress__doc__},

static PyObject *
_zstd_decompress_impl(PyObject *module, Py_buffer *data, PyObject *dict,
                      PyObject *option);

static PyObject *
_zstd_decompress(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", "dict", "option", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "decompress", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    PyObject *dict = Py_None;
    PyObject *option = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("decompress", "argument 'data'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        dict = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    option = args[2];
skip_optional_pos:
    return_value = _zstd_decompress_impl(module, &data, dict, option);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_zstd__get_cparam_bounds__doc__,
"_get_cparam_bounds($module, /, cParam)\n"
"--\n"
"\n"
"Get cParameter bounds.");

#define _ZSTD__GET_CPARAM_BOUNDS_METHODDEF    \
    {"_get_cparam_bounds", (PyCFunction)(void(*)(void))_zstd__get_cparam_bounds, METH_FASTCALL|METH_KEYWORDS, _zstd__get_cparam_bounds__doc__},

static PyObject *
_zstd__get_cparam_bounds_impl(PyObject *module, int cParam);

static PyObject *
_zstd__get_cparam_bounds(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"cParam", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_get_cparam_bounds", 0};
    PyObject *argsbuf[1];
    int cParam;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    cParam = _PyLong_AsInt(args[0]);
    if (cParam == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _zstd__get_cparam_bounds_impl(module, cParam);

exit:
    return return_value;
}

PyDoc_STRVAR(_zstd__get_dparam_bounds__doc__,
"_get_dparam_bounds($module, /, dParam)\n"
"--\n"
"\n"
"Get dParameter bounds.");

#define _ZSTD__GET_DPARAM_BOUNDS_METHODDEF    \
    {"_get_dparam_bounds", (PyCFunction)(void(*)(void))_zstd__get_dparam_bounds, METH_FASTCALL|METH_KEYWORDS, _zstd__get_dparam_bounds__doc__},

static PyObject *
_zstd__get_dparam_bounds_impl(PyObject *module, int dParam);

static PyObject *
_zstd__get_dparam_bounds(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"dParam", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "_get_dparam_bounds", 0};
    PyObject *argsbuf[1];
    int dParam;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    dParam = _PyLong_AsInt(args[0]);
    if (dParam == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _zstd__get_dparam_bounds_impl(module, dParam);

exit:
    return return_value;
}
/*[clinic end generated code: output=a60834fe949a87bc input=a9049054013a1b77]*/

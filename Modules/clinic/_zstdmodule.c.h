/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_zstd_compress__doc__,
"compress($module, /, data, level_or_option=None, dict=None)\n"
"--\n"
"\n"
"Returns a bytes object containing compressed data.\n"
"\n"
"  data\n"
"    Binary data to be compressed.\n"
"  level_or_option\n"
"    It can be an int object, which in this case represents the compression\n"
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
"decompress($module, /, data)\n"
"--\n"
"\n"
"Returns a bytes object containing the uncompressed data.\n"
"\n"
"  data\n"
"    Compressed data.");

#define _ZSTD_DECOMPRESS_METHODDEF    \
    {"decompress", (PyCFunction)(void(*)(void))_zstd_decompress, METH_FASTCALL|METH_KEYWORDS, _zstd_decompress__doc__},

static PyObject *
_zstd_decompress_impl(PyObject *module, Py_buffer *data);

static PyObject *
_zstd_decompress(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "decompress", 0};
    PyObject *argsbuf[1];
    Py_buffer data = {NULL, NULL};

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
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
    return_value = _zstd_decompress_impl(module, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_zstd_ZstdDict___init____doc__,
"ZstdDict(dict_data)\n"
"--\n"
"\n"
"xxxxxxxxxxxxxxxxxxxxxxxxxxxx");

static int
_zstd_ZstdDict___init___impl(ZstdDict *self, PyObject *dict_data);

static int
_zstd_ZstdDict___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"dict_data", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "ZstdDict", 0};
    PyObject *argsbuf[1];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *dict_data;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 1, 1, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    dict_data = fastargs[0];
    return_value = _zstd_ZstdDict___init___impl((ZstdDict *)self, dict_data);

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
/*[clinic end generated code: output=343a8b90392b5455 input=a9049054013a1b77]*/

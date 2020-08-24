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
"    Compress level or option.\n"
"  dict\n"
"    Dictionary");

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

PyDoc_STRVAR(_zstd_train_dict__doc__,
"train_dict($module, /, iterable_of_bytes, dict_size=102400)\n"
"--\n"
"\n"
"xxxxxxxxxxxxxxxxxx");

#define _ZSTD_TRAIN_DICT_METHODDEF    \
    {"train_dict", (PyCFunction)(void(*)(void))_zstd_train_dict, METH_FASTCALL|METH_KEYWORDS, _zstd_train_dict__doc__},

static PyObject *
_zstd_train_dict_impl(PyObject *module, PyObject *iterable_of_bytes,
                      Py_ssize_t dict_size);

static PyObject *
_zstd_train_dict(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"iterable_of_bytes", "dict_size", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "train_dict", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *iterable_of_bytes;
    Py_ssize_t dict_size = 102400;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    iterable_of_bytes = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        dict_size = ival;
    }
skip_optional_pos:
    return_value = _zstd_train_dict_impl(module, iterable_of_bytes, dict_size);

exit:
    return return_value;
}
/*[clinic end generated code: output=53c449a1ebe9e14a input=a9049054013a1b77]*/

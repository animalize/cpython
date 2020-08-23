
#include "Python.h"
#include "structmember.h"         // PyMemberDef

#include "zstd.h"
#include "dictBuilder\zdict.h"


#define ACQUIRE_LOCK(lock) do { \
    if (!PyThread_acquire_lock(lock, 0)) { \
        Py_BEGIN_ALLOW_THREADS \
        PyThread_acquire_lock(lock, 1); \
        Py_END_ALLOW_THREADS \
    } } while (0)
#define RELEASE_LOCK(lock) PyThread_release_lock(lock)

typedef struct {
    PyObject_HEAD

    // Content of the dictionary
    PyObject *dict_buffer;
    // Dictionary id
    UINT32 dict_id;

    // Reuseable compress/decompress dictionary.
    // They can be created once and shared by multiple threads concurrently,
    // since its usage is read-only.
    ZSTD_CDict *c_dict;
    ZSTD_DDict *d_dict;

    // Thread lock for generating c_dict/d_dict
    PyThread_type_lock c_dict_lock;
    PyThread_type_lock d_dict_lock;
} ZstdDict;

/*[clinic input]
module _zstd
class _zstd.ZstdDict "ZstdDict *" "&ZstdDict_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e5f7d2e8dbc268ec]*/

#include "clinic\_zstd.c.h"

#ifdef UCHAR_MAX
/* _BlocksOutputBuffer code */
typedef struct {
    /* List of blocks */
    PyObject *list;
    /* Number of whole allocated size. */
    Py_ssize_t allocated;

    /* Max length of the buffer, negative number for unlimited length. */
    Py_ssize_t max_length;
} _BlocksOutputBuffer;


/* Block size sequence. Some compressor/decompressor can't process large
   buffer (>4GB), so the type is int. Below functions assume the type is int.
*/
#define KB (1024)
#define MB (1024*1024)
static const int BUFFER_BLOCK_SIZE[] =
    { 32*KB, 64*KB, 256*KB, 1*MB, 4*MB, 8*MB, 16*MB, 16*MB,
      32*MB, 32*MB, 32*MB, 32*MB, 64*MB, 64*MB, 128*MB, 128*MB,
      256*MB };
#undef KB
#undef MB

/* According to the block sizes defined by BUFFER_BLOCK_SIZE, the whole
   allocated size growth step is:
    1   32 KB       +32 KB
    2   96 KB       +64 KB
    3   352 KB      +256 KB
    4   1.34 MB     +1 MB
    5   5.34 MB     +4 MB
    6   13.34 MB    +8 MB
    7   29.34 MB    +16 MB
    8   45.34 MB    +16 MB
    9   77.34 MB    +32 MB
    10  109.34 MB   +32 MB
    11  141.34 MB   +32 MB
    12  173.34 MB   +32 MB
    13  237.34 MB   +64 MB
    14  301.34 MB   +64 MB
    15  429.34 MB   +128 MB
    16  557.34 MB   +128 MB
    17  813.34 MB   +256 MB
    18  1069.34 MB  +256 MB
    19  1325.34 MB  +256 MB
    20  1581.34 MB  +256 MB
    21  1837.34 MB  +256 MB
    22  2093.34 MB  +256 MB
    ...
*/


/* Initialize the buffer, and grow the buffer.
   max_length: Max length of the buffer, -1 for unlimited length.
   Return 0 on success
   Return -1 on failure
*/
static int
_BlocksOutputBuffer_InitAndGrow(_BlocksOutputBuffer *buffer, Py_ssize_t max_length,
                                ZSTD_outBuffer *ob)
{
    PyObject *b;
    int block_size;

    // Set & check max_length
    buffer->max_length = max_length;
    if (max_length >= 0 && BUFFER_BLOCK_SIZE[0] > max_length) {
        block_size = (int) max_length;
    } else {
        block_size = BUFFER_BLOCK_SIZE[0];
    }

    // The first block
    b = PyBytes_FromStringAndSize(NULL, block_size);
    if (b == NULL) {
        buffer->list = NULL; // For _BlocksOutputBuffer_OnError()
        return -1;
    }

    // Create list
    buffer->list = PyList_New(1);
    if (buffer->list == NULL) {
        Py_DECREF(b);
        return -1;
    }
    PyList_SET_ITEM(buffer->list, 0, b);

    // Set variables
    buffer->allocated = block_size;

    ob->dst = PyBytes_AS_STRING(b);
    ob->size = block_size;
    ob->pos = 0;
    return 0;
}


/* Grow the buffer. The avail_out must be 0, please check it before calling.
   Return 0 on success
   Return -1 on failure
*/
static int
_BlocksOutputBuffer_Grow(_BlocksOutputBuffer *buffer, ZSTD_outBuffer *ob)
{
    PyObject *b;
    const Py_ssize_t list_len = Py_SIZE(buffer->list);
    int block_size;

    // Ensure no gaps in the data
    assert(ob->pos == ob->size);

    // Get block size
    if (list_len < Py_ARRAY_LENGTH(BUFFER_BLOCK_SIZE)) {
        block_size = BUFFER_BLOCK_SIZE[list_len];
    } else {
        block_size = BUFFER_BLOCK_SIZE[Py_ARRAY_LENGTH(BUFFER_BLOCK_SIZE) - 1];
    }

    // Check max_length
    if (buffer->max_length >= 0) {
        // Prevent adding unlimited number of empty bytes to the list.
        if (buffer->max_length == 0) {
            assert(ob->pos == ob->size);
            return 0;
        }
        // block_size of the last block
        if (block_size > buffer->max_length - buffer->allocated) {
            block_size = (int) (buffer->max_length - buffer->allocated);
        }
    }

    // Create the block
    b = PyBytes_FromStringAndSize(NULL, block_size);
    if (b == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate output buffer.");
        return -1;
    }
    if (PyList_Append(buffer->list, b) < 0) {
        Py_DECREF(b);
        return -1;
    }
    Py_DECREF(b);

    // Set variables
    buffer->allocated += block_size;

    ob->dst = PyBytes_AS_STRING(b);
    ob->size = block_size;
    ob->pos = 0;
    return 0;
}


/* Return the current outputted data size. */
static inline Py_ssize_t
_BlocksOutputBuffer_GetDataSize(_BlocksOutputBuffer *buffer, ZSTD_outBuffer *ob)
{
    return buffer->allocated - (ob->size - ob->pos);
}


/* Finish the buffer.
   Return a bytes object on success
   Return NULL on failure
*/
static PyObject *
_BlocksOutputBuffer_Finish(_BlocksOutputBuffer *buffer, ZSTD_outBuffer *ob)
{
    PyObject *result, *block;
    int8_t *offset;

    // Final bytes object
    result = PyBytes_FromStringAndSize(NULL, buffer->allocated - (ob->size - ob->pos));
    if (result == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate output buffer.");
        return NULL;
    }

    // Memory copy
    if (Py_SIZE(buffer->list) > 0) {
        offset = PyBytes_AS_STRING(result);

        // blocks except the last one
        Py_ssize_t i = 0;
        for (; i < Py_SIZE(buffer->list)-1; i++) {
            block = PyList_GET_ITEM(buffer->list, i);
            memcpy(offset,  PyBytes_AS_STRING(block), Py_SIZE(block));
            offset += Py_SIZE(block);
        }
        // the last block
        block = PyList_GET_ITEM(buffer->list, i);
        memcpy(offset, PyBytes_AS_STRING(block), Py_SIZE(block) - (ob->size - ob->pos));
    } else {
        assert(Py_SIZE(result) == 0);
    }

    Py_DECREF(buffer->list);
    return result;
}


/* clean up the buffer. */
static inline void
_BlocksOutputBuffer_OnError(_BlocksOutputBuffer *buffer)
{
    Py_XDECREF(buffer->list);
}

#define OutputBuffer(F) _BlocksOutputBuffer_##F
#endif /* _BlocksOutputBuffer code end */

typedef struct {
    PyTypeObject *ZstdDict_type;
    PyObject *ZstdError;
} _zstd_state;

static inline _zstd_state*
get_zstd_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_zstd_state *)state;
}


/*[clinic input]
_zstd.compress

    data: Py_buffer
        Binary data to be compressed.

Returns a bytes object containing compressed data.
[clinic start generated code]*/

static PyObject *
_zstd_compress_impl(PyObject *module, Py_buffer *data)
/*[clinic end generated code: output=01007fa703be1682 input=26f155ed6047c8ba]*/
{
    ZSTD_CCtx *cctx = NULL;
    ZSTD_inBuffer in;
    ZSTD_outBuffer out;
    _BlocksOutputBuffer buffer;
    size_t zstd_ret;
    PyObject *ret;

    // prepare input & output buffers
    in.src = data->buf;
    in.size = data->len;
    in.pos = 0;

    if (OutputBuffer(InitAndGrow)(&buffer, -1, &out) < 0) {
        goto error;
    }

    // creat zstd context
    cctx = ZSTD_createCCtx();
    if (cctx == NULL) {
        goto error;
    }

    while(1) {
        /* Zstd optimizes the case where the first flush mode is ZSTD_e_end,
           since it knows it is compressing the entire source in one pass. */
        Py_BEGIN_ALLOW_THREADS
        zstd_ret = ZSTD_compressStream2(cctx, &out, &in, ZSTD_e_end);
        Py_END_ALLOW_THREADS

        // check error
        if (ZSTD_isError(zstd_ret)) {
            _zstd_state *state = get_zstd_state(module);
            PyErr_SetString(state->ZstdError, ZSTD_getErrorName(zstd_ret));
            goto error;
        }

        // finished?
        if (zstd_ret == 0) {
            ret = OutputBuffer(Finish)(&buffer, &out);
            if (ret != NULL) {
                goto success;
            } else {
                goto error;
            }
        }

        // output buffer exhausted, grow the buffer
        if (out.pos == out.size) {
            if (OutputBuffer(Grow)(&buffer, &out) < 0) {
                goto error;
            }
        }

        assert(in.pos < in.size);
    }

error:
    OutputBuffer(OnError)(&buffer);
    ret = NULL;
success:
    if (cctx != NULL) {
        ZSTD_freeCCtx(cctx);
    }
    return ret;
}


/*[clinic input]
_zstd.decompress

    data: Py_buffer
        Compressed data.

Returns a bytes object containing the uncompressed data.
[clinic start generated code]*/

static PyObject *
_zstd_decompress_impl(PyObject *module, Py_buffer *data)
/*[clinic end generated code: output=69aee3f2cf35b025 input=a603d6aa31e2ef0c]*/
{
    ZSTD_DCtx *dctx = NULL;
    ZSTD_inBuffer in;
    ZSTD_outBuffer out;
    _BlocksOutputBuffer buffer;
    size_t zstd_ret;
    PyObject *ret;

    // prepare input & output buffers
    in.src = data->buf;
    in.size = data->len;
    in.pos = 0;

    if (OutputBuffer(InitAndGrow)(&buffer, -1, &out) < 0) {
        goto error;
    }

    // creat zstd context
    dctx = ZSTD_createDCtx();
    if (dctx == NULL) {
        goto error;
    }

    while(1) {
        Py_BEGIN_ALLOW_THREADS
        zstd_ret = ZSTD_decompressStream(dctx, &out , &in);
        Py_END_ALLOW_THREADS

        // check error
        if (ZSTD_isError(zstd_ret)) {
            _zstd_state *state = get_zstd_state(module);
            PyErr_SetString(state->ZstdError, ZSTD_getErrorName(zstd_ret));
            goto error;
        }

        // finished?
        if (in.pos == in.size) {
            ret = OutputBuffer(Finish)(&buffer, &out);
            if (ret != NULL) {
                goto success;
            } else {
                goto error;
            }
        }

        // output buffer exhausted, grow the buffer
        if (out.pos == out.size) {
            if (OutputBuffer(Grow)(&buffer, &out) < 0) {
                goto error;
            }
        }
    }

error:
    OutputBuffer(OnError)(&buffer);
    ret = NULL;
success:
    if (dctx != NULL) {
        ZSTD_freeDCtx(dctx);
    }
    return ret;
}

static PyObject *
_ZstdDict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ZstdDict *self;
    self = (ZstdDict *) type->tp_alloc(type, 0);

    self->dict_buffer = NULL;
    self->dict_id = 0;

    self->c_dict = NULL;
    self->d_dict = NULL;
    
    self->c_dict_lock = PyThread_allocate_lock();
    if (self->c_dict_lock == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return NULL;
    }

    self->d_dict_lock = PyThread_allocate_lock();
    if (self->d_dict_lock == NULL) {
        PyThread_free_lock(self->c_dict_lock);
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return NULL;
    }

    return (PyObject *) self;
}

static void
_ZstdDict_dealloc(ZstdDict *self)
{
    if (self->c_dict) {
        ZSTD_freeCDict(self->c_dict);
    }
    if (self->d_dict) {
        ZSTD_freeDDict(self->d_dict);
    }

    if (self->c_dict_lock != NULL) {
        PyThread_free_lock(self->c_dict_lock);
    }
    if (self->d_dict_lock != NULL) {
        PyThread_free_lock(self->d_dict_lock);
    }

    // Release dict_buffer at the end, self->c_dict and self->d_dict
    // may refer to self->dict_buffer.
    Py_XDECREF(self->dict_buffer);

    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

/*[clinic input]
_zstd.ZstdDict.__init__

    dict_data: object

xxxxxxxxxxxxxxxxxxxxxxxxxxxx
[clinic start generated code]*/

static int
_zstd_ZstdDict___init___impl(ZstdDict *self, PyObject *dict_data)
/*[clinic end generated code: output=fc8c5b06f93621d8 input=0af5a98ba7fa7882]*/
{
    if (!PyBytes_Check(dict_data)) {
        PyErr_SetString(PyExc_TypeError, "dict_data should be bytes object.");
        goto error;
    }

    Py_INCREF(dict_data);
    self->dict_buffer = dict_data;
    
    // get dict_id
    self->dict_id = ZDICT_getDictID(PyBytes_AS_STRING(dict_data), Py_SIZE(dict_data));
    if (self->dict_id == 0) {
        PyErr_SetString(PyExc_ValueError, "Not a valid Zstd dictionary content.");
        goto error;
    }

    return 0;

error:
    Py_XDECREF(self->dict_buffer);
    return -1;
}

static ZSTD_CDict*
_get_CDict(ZstdDict *self, const void *dictBuffer, size_t dictSize, int compressionLevel){

    ACQUIRE_LOCK(self->c_dict_lock);
    if (self->c_dict == NULL) {
        Py_BEGIN_ALLOW_THREADS
        self->c_dict = ZSTD_createCDict(dictBuffer, dictSize, compressionLevel);
        Py_END_ALLOW_THREADS
    }
    RELEASE_LOCK(self->c_dict_lock);

    return self->c_dict;
}

static ZSTD_DDict*
_get_DDict(ZstdDict *self, const void *dictBuffer, size_t dictSize){

    ACQUIRE_LOCK(self->d_dict_lock);
    if (self->d_dict == NULL) {
        Py_BEGIN_ALLOW_THREADS
        self->d_dict = ZSTD_createDDict(dictBuffer, dictSize);
        Py_END_ALLOW_THREADS
    }
    RELEASE_LOCK(self->d_dict_lock);

    return self->d_dict;
}

static PyMethodDef _ZstdDict_methods[] = {
    {NULL}
};

PyDoc_STRVAR(ZstdDict_dictid_doc,
"ID of the Zstd dictionary.");

static int
_ZstdDict_traverse(ZstdDict *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static PyMemberDef _ZstdDict_members[] = {
    {"dict_id", T_UINT, offsetof(ZstdDict, dict_id), READONLY, ZstdDict_dictid_doc},
    {NULL}
};

static PyType_Slot zstddict_slots[] = {
    {Py_tp_methods, _ZstdDict_methods},
    {Py_tp_new, _ZstdDict_new},
    {Py_tp_dealloc, _ZstdDict_dealloc},
    {Py_tp_init, _zstd_ZstdDict___init__},
    // {Py_tp_doc, (char *)Compressor_doc},
    {Py_tp_traverse, _ZstdDict_traverse},
    {Py_tp_members, _ZstdDict_members},
    {0, 0}
};

static PyType_Spec zstddict_type_spec = {
    .name = "_zstd.ZstdDict",
    .basicsize = sizeof(ZstdDict),
    // Calling PyType_GetModuleState() on a subclass is not safe.
    // lzma_compressor_type_spec does not have Py_TPFLAGS_BASETYPE flag
    // which prevents to create a subclass.
    // So calling PyType_GetModuleState() in this file is always safe.
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = zstddict_slots,
};

/*[clinic input]
_zstd.train_dict

    iterable_of_bytes: object
    dict_size: Py_ssize_t=102400

xxxxxxxxxxxxxxxxxx
[clinic start generated code]*/

static PyObject *
_zstd_train_dict_impl(PyObject *module, PyObject *iterable_of_bytes,
                      Py_ssize_t dict_size)
/*[clinic end generated code: output=85e3aca2c9b1d960 input=eb203800208cd895]*/
{
    PyObject *list = NULL;
    PyObject *full_data = NULL;
    Py_ssize_t full_size;
    size_t *chunk_sizes = NULL;
    PyObject *dict_buffer = NULL;
    size_t zstd_ret;

    // list
    if (PyList_Check(iterable_of_bytes)) {
        list = iterable_of_bytes;
        Py_INCREF(list);
    } else {
        list = PySequence_List(iterable_of_bytes);
        if (list == NULL) {
            goto error;
        }
    }

    // chunk_sizes
    const Py_ssize_t list_len = Py_SIZE(list);
    if (list_len > UINT32_MAX) {
        PyErr_SetString(PyExc_ValueError, "Number of data chunks is too big.");
        goto error;
    }

    chunk_sizes = PyMem_Malloc(list_len * sizeof(size_t));
    if (chunk_sizes == NULL) {
        goto error;
    }

    full_size = 0;
    for(Py_ssize_t i = 0; i < list_len; i++) {
        PyObject *chunk = PyList_GET_ITEM(list, i);
        chunk_sizes[i] = Py_SIZE(chunk);
        full_size += Py_SIZE(chunk);
    }

    // join the list
    full_data = PyBytes_FromStringAndSize(NULL, full_size);
    if (full_data == NULL) {
        goto error;
    }

    full_size = 0;
    for(Py_ssize_t i = 0; i < list_len; i++) {
        PyObject *chunk = PyList_GET_ITEM(list, i);
        memcpy(PyBytes_AS_STRING(full_data)+full_size, PyBytes_AS_STRING(chunk), Py_SIZE(chunk));
        full_size += Py_SIZE(chunk);
    }

    // dict buffer
    dict_buffer = PyBytes_FromStringAndSize(NULL, dict_size);
    if (dict_buffer == NULL) {
        goto error;
    }

    Py_BEGIN_ALLOW_THREADS
    zstd_ret = ZDICT_trainFromBuffer(PyBytes_AS_STRING(dict_buffer), dict_size,
                                     PyBytes_AS_STRING(full_data), chunk_sizes, (UINT32)list_len);
    Py_END_ALLOW_THREADS

    if (ZDICT_isError(zstd_ret)) {
        _zstd_state *state = get_zstd_state(module);
        PyErr_SetString(state->ZstdError, ZDICT_getErrorName(zstd_ret));
        goto error;
    }

    Py_DECREF(list);
    Py_DECREF(full_data);
    PyMem_Free(chunk_sizes);
    return dict_buffer;

error:
    Py_XDECREF(list);
    Py_XDECREF(full_data);
    Py_XDECREF(dict_buffer);
    if (chunk_sizes != NULL) {
        PyMem_Free(chunk_sizes);
    }
    return NULL;
}

static int
zstd_exec(PyObject *module)
{
    _zstd_state *state = get_zstd_state(module);

    // ZstdError
    state->ZstdError = PyErr_NewExceptionWithDoc("_zstd.ZstdError", "Call to zstd failed.", NULL, NULL);
    if (state->ZstdError == NULL) {
        return -1;
    }
    Py_INCREF(state->ZstdError);
    if (PyModule_AddType(module, (PyTypeObject *)state->ZstdError) < 0) {
        Py_DECREF(state->ZstdError);
        return -1;
    }

    // ZstdDict
    state->ZstdDict_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
                                                                    &zstddict_type_spec,
                                                                    NULL);
    if (state->ZstdDict_type == NULL) {
        return -1;
    }
    Py_INCREF(state->ZstdDict_type);
    if (PyModule_AddType(module, (PyTypeObject *)state->ZstdDict_type) < 0) {
        Py_DECREF(state->ZstdDict_type);
        return -1;
    }

    // state->lzma_decompressor_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
    //                                                      &lzma_decompressor_type_spec, NULL);
    // if (state->lzma_decompressor_type == NULL) {
    //     return -1;
    // }

    // if (PyModule_AddType(module, state->lzma_decompressor_type) < 0) {
    //     return -1;
    // }

    return 0;
}

static PyMethodDef _zstd_methods[] = {
    _ZSTD_COMPRESS_METHODDEF
    _ZSTD_DECOMPRESS_METHODDEF
    _ZSTD_TRAIN_DICT_METHODDEF
    {NULL}
};

static PyModuleDef_Slot _zstd_slots[] = {
    {Py_mod_exec, zstd_exec},
    {0, NULL}
};

static int
_zstd_traverse(PyObject *module, visitproc visit, void *arg)
{
    _zstd_state *state = get_zstd_state(module);
    Py_VISIT(state->ZstdDict_type);
    Py_VISIT(state->ZstdError);
    return 0;
}

static int
_zstd_clear(PyObject *module)
{
    _zstd_state *state = get_zstd_state(module);
    Py_CLEAR(state->ZstdDict_type);
    Py_CLEAR(state->ZstdError);
    return 0;
}

static void
_zstd_free(void *module)
{
    _zstd_state *state = get_zstd_state(module);
    Py_CLEAR(state->ZstdDict_type);
    Py_CLEAR(state->ZstdError);
}

static PyModuleDef _zstdmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_zstd",
    .m_size = sizeof(_zstd_state),
    .m_methods = _zstd_methods,
    .m_slots = _zstd_slots,
    .m_traverse = _zstd_traverse,
    .m_clear = _zstd_clear,
    .m_free = _zstd_free,
};

PyMODINIT_FUNC
PyInit__zstd(void)
{
    return PyModuleDef_Init(&_zstdmodule);
}
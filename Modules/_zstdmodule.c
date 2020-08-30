
#include "Python.h"
#include "structmember.h"         // PyMemberDef

#include "lib\zstd.h"
#include "lib\dictBuilder\zdict.h"


typedef struct {
    PyObject_HEAD

    /* Content of the dictionary, bytes object. */
    PyObject *dict_content;
    /* Dictionary id */
    UINT32 dict_id;

    /* Reuseable compress/decompress dictionary, they are created once and
       can be shared by multiple threads concurrently, since its usage is
       read-only.

       c_dicts is a dict, int(compressionLevel):PyCapsule(ZSTD_CDict*) */
    PyObject *c_dicts;
    ZSTD_DDict *d_dict;

    /* Thread lock for generating ZSTD_CDict/ZSTD_DDict */
    PyThread_type_lock lock;

    /* __init__ has been called */
    int inited;
} ZstdDict;

typedef struct {
    PyObject_HEAD

    ZSTD_CCtx *cctx;
    PyObject *dict;

    PyThread_type_lock lock;
} ZstdCompressor;

typedef struct {
    PyObject_HEAD

    ZSTD_DCtx *dctx;
    PyObject *dict;

    char needs_input;
    uint8_t *input_buffer;
    size_t input_buffer_size;

    PyThread_type_lock lock;
} ZstdDecompressor;

/*[clinic input]
module _zstd
class _zstd.ZstdDict "ZstdDict *" "&ZstdDict_Type"
class _zstd.ZstdCompressor "ZstdCompressor *" "&ZstdCompressor_type"
class _zstd.ZstdDecompressor "ZstdDecompressor *" "&ZstdDecompressor_type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7208e8cc544a5228]*/

#include "clinic\_zstdmodule.c.h"

/* -----------------------------------
     BlocksOutputBuffer code 
   ----------------------------------- */
#define BLOCK_OUTPUT_BUFFER_CODE_BLOCK
#ifdef BLOCK_OUTPUT_BUFFER_CODE_BLOCK
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
#endif
#undef BLOCK_OUTPUT_BUFFER_CODE_BLOCK /* _BlocksOutputBuffer code end */

typedef struct {
    PyTypeObject *ZstdDict_type;
    PyTypeObject *ZstdCompressor_type;
    PyTypeObject *ZstdDecompressor_type;
    PyObject *ZstdError;
} _zstd_state;

static inline _zstd_state *
get_zstd_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_zstd_state *)state;
}

#define ACQUIRE_LOCK(obj) do { \
    if (!PyThread_acquire_lock((obj)->lock, 0)) { \
        Py_BEGIN_ALLOW_THREADS \
        PyThread_acquire_lock((obj)->lock, 1); \
        Py_END_ALLOW_THREADS \
    } } while (0)
#define RELEASE_LOCK(obj) PyThread_release_lock((obj)->lock)


/* -----------------------------------
     Parameters from zstd 
   ----------------------------------- */

typedef struct {
    const int parameter;
    const char parameter_name[32];
} ParameterInfo;

static const ParameterInfo cp_list[] =
{
    {ZSTD_c_compressionLevel, "compressionLevel"},
    {ZSTD_c_windowLog,        "windowLog"},
    {ZSTD_c_hashLog,          "hashLog"},
    {ZSTD_c_chainLog,         "chainLog"},
    {ZSTD_c_searchLog,        "searchLog"},
    {ZSTD_c_minMatch,         "minMatch"},
    {ZSTD_c_targetLength,     "targetLength"},
    {ZSTD_c_strategy,         "strategy"},
    {ZSTD_c_enableLongDistanceMatching, "enableLongDistanceMatching"},
    {ZSTD_c_ldmHashLog,       "ldmHashLog"},
    {ZSTD_c_ldmMinMatch,      "ldmMinMatch"},
    {ZSTD_c_ldmBucketSizeLog, "ldmBucketSizeLog"},
    {ZSTD_c_ldmHashRateLog,   "ldmHashRateLog"},
    {ZSTD_c_contentSizeFlag,  "contentSizeFlag"},
    {ZSTD_c_checksumFlag,     "checksumFlag"},
    {ZSTD_c_dictIDFlag,       "dictIDFlag"}
};

static const ParameterInfo dp_list[] =
{
    {ZSTD_d_windowLogMax, "windowLogMax"}
};


/* Format an user friendly error mssage. */
static inline void
get_parameter_error_msg(char *buf, int buf_size, Py_ssize_t pos,
                        int key_v, int value_v, char is_compress)
{
    ParameterInfo const *list;
    int list_size;
    char const *name;
    char *type;
    ZSTD_bounds bounds;

    assert(buf_size >= 160);

    if (is_compress) {
        list = cp_list;
        list_size = Py_ARRAY_LENGTH(cp_list);
        type = "compress";
    } else {
        list = dp_list;
        list_size = Py_ARRAY_LENGTH(dp_list);
        type = "decompress";
    }

    /* Find parameter's name */
    name = NULL;
    for (int i = 0; i < list_size; i++) {
        if (key_v == (list+i)->parameter) {
            name = (list+i)->parameter_name;
        }
    }
    
    /* Not a valid parameter */
    if (name == NULL) {
        PyOS_snprintf(buf, buf_size,
                      "The %zdth zstd %s parameter is invalid.",
                      pos, type);
        return;
    }

    /* Get parameter bounds */
    if (is_compress) {
        bounds = ZSTD_cParam_getBounds(key_v);
    } else {
        bounds = ZSTD_dParam_getBounds(key_v);
    }
    if (ZSTD_isError(bounds.error)) {
        PyOS_snprintf(buf, buf_size,
                      "Error when getting bounds of zstd %s parameter \"%s\".",
                      type, name);
        return;
    }

    /* Error message */
    PyOS_snprintf(buf, buf_size,
                  "Error when setting zstd %s parameter \"%s\", it "
                  "should %d <= value <= %d, provided value is %d.",
                  type, name, bounds.lowerBound, bounds.upperBound, value_v);
}

static int
module_add_int_constant(PyObject *m, const char *name, long long value)
{
    PyObject *o = PyLong_FromLongLong(value);
    if (o == NULL) {
        return -1;
    }
    if (PyModule_AddObject(m, name, o) == 0) {
        return 0;
    }
    Py_DECREF(o);
    return -1;
}

#define ADD_INT_PREFIX_MACRO(module, macro)                              \
    do {                                                                 \
        if (module_add_int_constant(module, ("_" #macro), macro) < 0) {  \
            return -1;                                                   \
        }                                                                \
    } while(0)

static int
add_parameters(PyObject *module)
{
    /* Compress parameters */
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_compressionLevel);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_windowLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_hashLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_chainLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_searchLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_minMatch);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_targetLength);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_strategy);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_enableLongDistanceMatching);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_ldmHashLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_ldmMinMatch);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_ldmBucketSizeLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_ldmHashRateLog);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_contentSizeFlag);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_checksumFlag);
    ADD_INT_PREFIX_MACRO(module, ZSTD_c_dictIDFlag);

    /* Decompress parameters */
    ADD_INT_PREFIX_MACRO(module, ZSTD_d_windowLogMax);

    return 0;
}


/* -----------------------------------
     ZstdDict code 
   ----------------------------------- */
static void
capsule_free_cdict(PyObject *capsule)
{
    ZSTD_CDict *cdict = PyCapsule_GetPointer(capsule, NULL);
    ZSTD_freeCDict(cdict);
}

static ZSTD_CDict *
_get_CDict(ZstdDict *self, int compressionLevel)
{
    PyObject *level = NULL;
    PyObject *capsule;
    ZSTD_CDict *cdict;

    ACQUIRE_LOCK(self);

    /* int level object */
    level = PyLong_FromLong(compressionLevel);
    if (level == NULL) {
        goto error;
    }

    capsule = PyDict_GetItem(self->c_dicts, level);

    if (capsule != NULL) {
        cdict = PyCapsule_GetPointer(capsule, NULL);
        goto success;
    } else {
        Py_BEGIN_ALLOW_THREADS
        cdict = ZSTD_createCDict(PyBytes_AS_STRING(self->dict_content),
                                 Py_SIZE(self->dict_content), compressionLevel);
        Py_END_ALLOW_THREADS

        if (cdict == NULL) {
            goto error;
        }

        capsule = PyCapsule_New(cdict, NULL, capsule_free_cdict);
        if (capsule == NULL) {
            ZSTD_freeCDict(cdict);
            goto error;
        }

        if (PyDict_SetItem(self->c_dicts, level, capsule) < 0) {
            Py_DECREF(capsule);
            goto error;
        }
        Py_DECREF(capsule);
        goto success;
    }

error:
    cdict = NULL;
success:
    Py_XDECREF(level);
    RELEASE_LOCK(self);
    return cdict;
}

static inline ZSTD_DDict *
_get_DDict(ZstdDict *self)
{
    ACQUIRE_LOCK(self);
    if (self->d_dict == NULL) {
        Py_BEGIN_ALLOW_THREADS
        self->d_dict = ZSTD_createDDict(PyBytes_AS_STRING(self->dict_content),
                                        Py_SIZE(self->dict_content));
        Py_END_ALLOW_THREADS
    }
    RELEASE_LOCK(self);

    return self->d_dict;
}

static PyObject *
_ZstdDict_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ZstdDict *self;
    self = (ZstdDict*)type->tp_alloc(type, 0);

    self->dict_content = NULL;
    self->dict_id = 0;

    self->c_dicts = PyDict_New();
    if (self->c_dicts == NULL) {
        return NULL;
    }

    self->d_dict = NULL;

    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        Py_DECREF(self->c_dicts);
        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return NULL;
    }

    self->inited = 0;

    return (PyObject*)self;
}

static void
_ZstdDict_dealloc(ZstdDict *self)
{
    Py_DECREF(self->c_dicts);

    if (self->d_dict) {
        ZSTD_freeDDict(self->d_dict);
    }

    PyThread_free_lock(self->lock);

    /* Release dict_buffer at the end, self->c_dicts and self->d_dict
       may refer to self->dict_buffer. */
    Py_DECREF(self->dict_content);

    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free((PyObject*)self);
    Py_DECREF(tp);
}

/*[clinic input]
_zstd.ZstdDict.__init__

    dict_content: object
        Dictionary's content, a bytes object.

Initialize ZstdDict object.
[clinic start generated code]*/

static int
_zstd_ZstdDict___init___impl(ZstdDict *self, PyObject *dict_content)
/*[clinic end generated code: output=49ae79dcbb8ad2df input=85b3c5d16d12a001]*/
{
    /* Only called once */
    if (self->inited) {
        PyErr_SetString(PyExc_RuntimeError, "ZstdDict.__init__ function was called twice.");
        return -1;
    }
    self->inited = 1;

    /* Check dict_content's type */
    self->dict_content = PyBytes_FromObject(dict_content);
    if (self->dict_content == NULL) {
        PyErr_Clear();
        PyErr_SetString(PyExc_TypeError,
                        "dict_content argument should be bytes-like object.");
        return -1;
    }

    /* Get dict_id */
    self->dict_id = ZDICT_getDictID(PyBytes_AS_STRING(dict_content),
                                    Py_SIZE(dict_content));
    if (self->dict_id == 0) {
        Py_CLEAR(self->dict_content);
        PyErr_SetString(PyExc_ValueError, "Not a valid Zstd dictionary content.");
        return -1;
    }

    return 0;
}


/*[clinic input]
_zstd.ZstdDict.__reduce__

Return state information for pickling.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDict___reduce___impl(ZstdDict *self)
/*[clinic end generated code: output=5c9b8a3550429417 input=1a45441f8f3f7085]*/
{
    return Py_BuildValue("O(O)", Py_TYPE(self), self->dict_content);
}


static PyMethodDef _ZstdDict_methods[] = {
    _ZSTD_ZSTDDICT___REDUCE___METHODDEF
    {NULL}
};

PyDoc_STRVAR(_ZstdDict_dict_doc,
    "Zstd dictionary.");

PyDoc_STRVAR(ZstdDict_dictid_doc,
    "ID of Zstd dictionary, a 32-bit unsigned int value.");

PyDoc_STRVAR(ZstdDict_dictbuffer_doc,
    "The content of the Zstd dictionary, a bytes object.");

static int
_ZstdDict_traverse(ZstdDict *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static PyObject *
_ZstdDict_repr(ZstdDict *dict)
{
    char buf[128];
    PyOS_snprintf(buf, sizeof(buf),
        "<ZstdDict dict_id=%u dict_size=%zd>",
        dict->dict_id, Py_SIZE(dict->dict_content));

    return PyUnicode_FromString(buf);
}

static PyMemberDef _ZstdDict_members[] = {
    {"dict_id", T_UINT, offsetof(ZstdDict, dict_id), READONLY, ZstdDict_dictid_doc},
    {"dict_content", T_OBJECT_EX, offsetof(ZstdDict, dict_content), READONLY, ZstdDict_dictbuffer_doc},
    {NULL}
};

static PyType_Slot zstddict_slots[] = {
    {Py_tp_methods, _ZstdDict_methods},
    {Py_tp_members, _ZstdDict_members},
    {Py_tp_new, _ZstdDict_new},
    {Py_tp_dealloc, _ZstdDict_dealloc},
    {Py_tp_init, _zstd_ZstdDict___init__},
    {Py_tp_repr, _ZstdDict_repr},
    {Py_tp_doc, (char*)_ZstdDict_dict_doc},
    {Py_tp_traverse, _ZstdDict_traverse},
    {0, 0}
};

static PyType_Spec zstddict_type_spec = {
    .name = "_zstd.ZstdDict",
    .basicsize = sizeof(ZstdDict),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .slots = zstddict_slots,
};
/* ZstdDict code end */

/*[clinic input]
_zstd._train_dict

    dst_data: PyBytesObject
    dst_data_sizes: object
    dict_size: Py_ssize_t

Train a Zstd dictionary.
[clinic start generated code]*/

static PyObject *
_zstd__train_dict_impl(PyObject *module, PyBytesObject *dst_data,
                       PyObject *dst_data_sizes, Py_ssize_t dict_size)
/*[clinic end generated code: output=d39b262ebfcac776 input=385ab3b6c6dcdc3d]*/
{
    size_t *chunk_sizes = NULL;
    PyObject *dict_buffer = NULL;
    size_t zstd_ret;

    /* Prepare chunk_sizes */
    const Py_ssize_t chunks_number = Py_SIZE(dst_data_sizes);
    if (chunks_number > UINT32_MAX) {
        PyErr_SetString(PyExc_ValueError, "Number of data chunks is too big, should <= 4294967295.");
        goto error;
    }

    chunk_sizes = PyMem_Malloc(chunks_number * sizeof(size_t));
    if (chunk_sizes == NULL) {
        goto error;
    }

    for (Py_ssize_t i = 0; i < chunks_number; i++) {
        PyObject *size = PyList_GET_ITEM(dst_data_sizes, i);
        chunk_sizes[i] = PyLong_AsSize_t(size);
        if (chunk_sizes[i] == -1 && PyErr_Occurred()) {
            goto error;
        }
    }

    /* Allocate dict buffer */
    dict_buffer = PyBytes_FromStringAndSize(NULL, dict_size);
    if (dict_buffer == NULL) {
        goto error;
    }

    /* Train the dictionary. */
    Py_BEGIN_ALLOW_THREADS
    zstd_ret = ZDICT_trainFromBuffer(PyBytes_AS_STRING(dict_buffer), dict_size,
                                     PyBytes_AS_STRING(dst_data),
                                     chunk_sizes, (UINT32)chunks_number);
    Py_END_ALLOW_THREADS

    /* Check zstd dict error. */
    if (ZDICT_isError(zstd_ret)) {
        _zstd_state *state = get_zstd_state(module);
        PyErr_SetString(state->ZstdError, ZDICT_getErrorName(zstd_ret));
        goto error;
    }

    PyMem_Free(chunk_sizes);
    return dict_buffer;

error:
    if (chunk_sizes != NULL) {
        PyMem_Free(chunk_sizes);
    }
    Py_XDECREF(dict_buffer);
    return NULL;
}

/* Set compressLevel or compress parameters to compress context. */
static int
set_c_parameters(_zstd_state *state, ZSTD_CCtx *cctx,
                 PyObject *level_or_option, int *compress_level)
{
    size_t zstd_ret;
    char buf[160];

    /* Integer compression level */
    if (PyLong_Check(level_or_option)) {
        *compress_level = _PyLong_AsInt(level_or_option);
        if (*compress_level == -1 && PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                            "Compress level should be 32-bit signed int value.");
            return -1;
        }

        /* Set ZSTD_c_compressionLevel to compress context */
        zstd_ret = ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, *compress_level);

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            PyErr_Format(state->ZstdError,
                         "Error when setting compression level: %s",
                         ZSTD_getErrorName(zstd_ret));
            return -1;
        }
        return 0;
    }

    /* Options dict */
    if (PyDict_Check(level_or_option)) {
        PyObject *key, * value;
        Py_ssize_t pos = 0;

        while (PyDict_Next(level_or_option, &pos, &key, &value)) {
            /* Both key & value should be 32-bit signed int */
            int key_v = _PyLong_AsInt(key);
            if (key_v == -1 && PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError,
                                "Key of option dict should be 32-bit signed int value.");
                return -1;
            }

            int value_v = _PyLong_AsInt(value);
            if (value_v == -1 && PyErr_Occurred()) {
                PyErr_SetString(PyExc_ValueError,
                                "Value of option dict should be 32-bit signed int value.");
                return -1;
            }

            /* Get ZSTD_c_compressionLevel for generating ZSTD_CDICT */
            if (key_v == ZSTD_c_compressionLevel) {
                *compress_level = value_v;
            }

            /* Set parameter to compress context */
            zstd_ret = ZSTD_CCtx_setParameter(cctx, key_v, value_v);
            if (ZSTD_isError(zstd_ret)) {
                get_parameter_error_msg(buf, sizeof(buf), pos, key_v, value_v, 1),
                PyErr_Format(state->ZstdError, buf);
                return -1;
            }
        }
        return 0;
    }

    /* Wrong type */
    PyErr_SetString(PyExc_TypeError, "level_or_option argument wrong type.");
    return -1;
}

/* Load dictionary (ZSTD_CDict instance) to compress context (ZSTD_CCtx instance). */
static int
load_c_dict(_zstd_state *state, ZSTD_CCtx *cctx,
            PyObject *dict, int compress_level)
{
    size_t zstd_ret;
    ZSTD_CDict *c_dict;
    int ret;

    /* Check dict type */
    ret = PyObject_IsInstance(dict, (PyObject*)state->ZstdDict_type);
    if (ret < 0) {
        return -1;
    } else if (ret == 0) {
        PyErr_SetString(PyExc_TypeError, "dict argument should be ZstdDict object.");
        return -1;
    }

    /* Get ZSTD_CDict */
    c_dict = _get_CDict((ZstdDict*)dict, compress_level);
    if (c_dict == NULL) {
        PyErr_SetString(PyExc_SystemError, "Failed to get ZSTD_CDict.");
        return -1;
    }

    /* Reference a prepared dictionary */
    zstd_ret = ZSTD_CCtx_refCDict(cctx, c_dict);

    /* Check error */
    if (ZSTD_isError(zstd_ret)) {
        PyErr_SetString(state->ZstdError, ZSTD_getErrorName(zstd_ret));
        return -1;
    }
    return 0;
}


/* Set decompress parameters to decompress context. */
static int
set_d_parameters(_zstd_state *state, ZSTD_DCtx *dctx, PyObject *option)
{
    size_t zstd_ret;
    PyObject *key, *value;
    Py_ssize_t pos;
    char buf[160];

    if (!PyDict_Check(option)) {
        PyErr_SetString(PyExc_TypeError, "option argument wrong type.");
        return -1;
    }

    pos = 0;
    while (PyDict_Next(option, &pos, &key, &value)) {
        /* Both key & value should be 32-bit signed int */
        int key_v = _PyLong_AsInt(key);
        if (key_v == -1 && PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                            "Key of option dict should be 32-bit signed integer value.");
            return -1;
        }

        int value_v = _PyLong_AsInt(value);
        if (value_v == -1 && PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                            "Value of option dict should be 32-bit signed integer value.");
            return -1;
        }

        /* Set parameter to compress context */
        zstd_ret = ZSTD_DCtx_setParameter(dctx, key_v, value_v);

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            get_parameter_error_msg(buf, sizeof(buf), pos, key_v, value_v, 0),
            PyErr_Format(state->ZstdError, buf);
            return -1;
        }
    }
    return 0;
}

/* Load dictionary (ZSTD_DDict instance) to decompress context (ZSTD_DCtx instance). */
static int
load_d_dict(_zstd_state *state, ZSTD_DCtx *dctx, PyObject *dict)
{
    size_t zstd_ret;
    ZSTD_DDict *d_dict;
    int ret;

    /* Check dict type */
    ret = PyObject_IsInstance(dict, (PyObject*)state->ZstdDict_type);
    if (ret < 0) {
        return -1;
    } else if (ret == 0) {
        PyErr_SetString(PyExc_TypeError, "dict argument should be ZstdDict object.");
        return -1;
    }

    /* Get ZSTD_DDict */
    d_dict = _get_DDict((ZstdDict*)dict);
    if (d_dict == NULL) {
        PyErr_SetString(PyExc_SystemError, "Failed to get ZSTD_DDict.");
        return -1;
    }

    /* Reference a decompress dictionary */
    zstd_ret = ZSTD_DCtx_refDDict(dctx, d_dict);

    /* Check error */
    if (ZSTD_isError(zstd_ret)) {
        PyErr_SetString(state->ZstdError, ZSTD_getErrorName(zstd_ret));
        return -1;
    }
    return 0;
}


static PyObject *
_ZstdCompressor_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ZstdCompressor *self;
    self = (ZstdCompressor*)type->tp_alloc(type, 0);

    self->cctx = ZSTD_createCCtx();
    if (self->cctx == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Unable to create ZSTD_CCtx instance.");
        return NULL;
    }

    self->dict = NULL;

    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        ZSTD_freeCCtx(self->cctx);

        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return NULL;
    }

    return (PyObject*)self;
}

static void
_ZstdCompressor_dealloc(ZstdCompressor *self)
{
    ZSTD_freeCCtx(self->cctx);
    /* Py_XDECREF the dict after free the compress context */
    Py_XDECREF(self->dict);
    PyThread_free_lock(self->lock);

    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free((PyObject*)self);
    Py_DECREF(tp);
}

/*[clinic input]
_zstd.ZstdCompressor.__init__

    level_or_option: object = None
        It can be an int object, in this case represents the compression
        level. It can also be a dictionary for setting various advanced
        parameters. The default value None means to use zstd's default
        compression parameters.
    dict: object = None
        Pre-trained dictionary for compression, a ZstdDict object.

Initialize ZstdCompressor object.
[clinic start generated code]*/

static int
_zstd_ZstdCompressor___init___impl(ZstdCompressor *self,
                                   PyObject *level_or_option, PyObject *dict)
/*[clinic end generated code: output=3688e3ca73e5d48f input=1a51f5a845ded76f]*/
{
    size_t zstd_ret;
    int compress_level = 0; /* 0 means use zstd's default compression level */
    int ret = 0;
    _zstd_state *state = PyType_GetModuleState(Py_TYPE(self));
    assert(state != NULL);

    ACQUIRE_LOCK(self);

    /* __init__() may be called multiple times, reset options and clear dict. */
    zstd_ret = ZSTD_CCtx_reset(self->cctx, ZSTD_reset_session_and_parameters);
    if (ZSTD_isError(zstd_ret)) {
        PyErr_SetString(state->ZstdError, ZSTD_getErrorName(zstd_ret));
        goto error;
    }
    Py_CLEAR(self->dict);

    /* Set compressLevel/options to compress context */
    if (level_or_option != Py_None) {
        if (set_c_parameters(state, self->cctx, level_or_option, &compress_level) < 0) {
            goto error;
        }
    }

    /* Load dictionary to compress context */
    if (dict != Py_None) {
        if (load_c_dict(state, self->cctx, dict, compress_level) < 0) {
            goto error;
        }

        /* Py_INCREF the dict */
        Py_INCREF(dict);
        self->dict = dict;
    }

    goto success;

error:
    ret = -1;
success:
    RELEASE_LOCK(self);
    return ret;
}

static inline PyObject *
compress_impl(ZstdCompressor *self, Py_buffer *data,
              ZSTD_EndDirective end_directive)
{
    ZSTD_inBuffer in;
    ZSTD_outBuffer out;
    _BlocksOutputBuffer buffer;
    size_t zstd_ret;
    PyObject *ret;
    _zstd_state *state = PyType_GetModuleState(Py_TYPE(self));
    assert(state != NULL);

    /* Prepare input & output buffers */
    if (data != NULL) {
        in.src = data->buf;
        in.size = data->len;
        in.pos = 0;
    } else {
        in.src = &in;
        in.size = 0;
        in.pos = 0;
    }

    /* OutputBuffer(OnError)(&buffer) is after `error` label,
       so initialize the buffer before any `goto error` statement. */
    if (OutputBuffer(InitAndGrow)(&buffer, -1, &out) < 0) {
        goto error;
    }

    /* zstd stream compress */
    while (1) {
        Py_BEGIN_ALLOW_THREADS
        zstd_ret = ZSTD_compressStream2(self->cctx, &out, &in, end_directive);
        Py_END_ALLOW_THREADS

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            PyErr_SetString(state->ZstdError, ZSTD_getErrorName(zstd_ret));
            goto error;
        }

        /* Finished */
        if (zstd_ret == 0) {
            ret = OutputBuffer(Finish)(&buffer, &out);
            if (ret != NULL) {
                goto success;
            } else {
                goto error;
            }
        }

        /* Output buffer exhausted, grow the buffer */
        if (out.pos == out.size) {
            if (OutputBuffer(Grow)(&buffer, &out) < 0) {
                goto error;
            }
        }

        assert(in.pos <= in.size);
    }

error:
    OutputBuffer(OnError)(&buffer);
    ret = NULL;
success:
    return ret;
}

/*[clinic input]
_zstd.ZstdCompressor.compress

    data: Py_buffer
    end_directive: int(c_default="ZSTD_e_continue") = EndDirective.CONTINUE

Provide data to the compressor object.

Returns a chunk of compressed data if possible, or b'' otherwise.

When you have finished providing data to the compressor, call the
flush() method to finish the compression process.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdCompressor_compress_impl(ZstdCompressor *self, Py_buffer *data,
                                   int end_directive)
/*[clinic end generated code: output=09f541ea51afd468 input=901a2d3535fa9161]*/
{
    PyObject *ret;

    ACQUIRE_LOCK(self);
    ret = compress_impl(self, data, end_directive);
    RELEASE_LOCK(self);

    return ret;
}

/*[clinic input]
_zstd.ZstdCompressor.flush

    end_frame: bool=True
        True flush data and end the frame. False flush data, don't end the
        frame.

Finish the compression process.

Returns the compressed data left in internal buffers.

The compressor object may be used after this method is called.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdCompressor_flush_impl(ZstdCompressor *self, int end_frame)
/*[clinic end generated code: output=0206a53c394f4620 input=9f5cfc3560d831ac]*/
{
    PyObject *ret;

    ACQUIRE_LOCK(self);
    ret = compress_impl(self, NULL, end_frame ? ZSTD_e_end : ZSTD_e_flush);
    RELEASE_LOCK(self);

    return ret;
}


static int
_ZstdCompressor_traverse(ZstdCompressor *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static PyMethodDef _ZstdCompressor_methods[] = {
    _ZSTD_ZSTDCOMPRESSOR_COMPRESS_METHODDEF
    _ZSTD_ZSTDCOMPRESSOR_FLUSH_METHODDEF
    {NULL, NULL}
};

static PyType_Slot zstdcompressor_slots[] = {
    {Py_tp_new, _ZstdCompressor_new},
    {Py_tp_dealloc, _ZstdCompressor_dealloc},
    {Py_tp_init, _zstd_ZstdCompressor___init__},
    {Py_tp_methods, _ZstdCompressor_methods},
    //{Py_tp_doc, (char*)Compressor_doc},
    {Py_tp_traverse, _ZstdCompressor_traverse},
    {0, 0}
};

static PyType_Spec zstdcompressor_type_spec = {
    .name = "_zstd.ZstdCompressor",
    .basicsize = sizeof(ZstdCompressor),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = zstdcompressor_slots,
};

/* ZstdDecompressor */

static PyObject *
_ZstdDecompressor_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    ZstdDecompressor *self;
    self = (ZstdDecompressor*)type->tp_alloc(type, 0);

    self->dctx = ZSTD_createDCtx();
    if (self->dctx == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Unable to create ZSTD_DCtx instance.");
        return NULL;
    }

    self->dict = NULL;

    self->needs_input = 1;
    self->input_buffer = NULL;
    self->input_buffer_size = 0;

    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        ZSTD_freeDCtx(self->dctx);

        PyErr_SetString(PyExc_MemoryError, "Unable to allocate lock");
        return NULL;
    }

    return (PyObject*)self;
}

static void
_ZstdDecompressor_dealloc(ZstdDecompressor *self)
{
    ZSTD_freeDCtx(self->dctx);
  
    if (self->input_buffer != NULL) {
        PyMem_Free(self->input_buffer);
    }

    /* Py_XDECREF the dict after free the compress context */
    Py_XDECREF(self->dict);

    PyThread_free_lock(self->lock);

    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free((PyObject*)self);
    Py_DECREF(tp);
}

/*[clinic input]
_zstd.ZstdDecompressor.__init__

    dict: object = None
        Pre-trained dictionary for decompression, a ZstdDict object.
    option: object = None
        A dictionary for setting advanced parameters. The default
        value None means to use zstd's default decompression parameters.

Initialize ZstdDecompressor object.
[clinic start generated code]*/

static int
_zstd_ZstdDecompressor___init___impl(ZstdDecompressor *self, PyObject *dict,
                                     PyObject *option)
/*[clinic end generated code: output=667351c5096f94cb input=4dc564486c5dc983]*/
{
    size_t zstd_ret;
    int ret = 0;
    _zstd_state *state = PyType_GetModuleState(Py_TYPE(self));
    assert(state != NULL);

    ACQUIRE_LOCK(self);

    /* __init__() may be called multiple times, reset options and clear dict. */
    zstd_ret = ZSTD_DCtx_reset(self->dctx, ZSTD_reset_session_and_parameters);
    if (ZSTD_isError(zstd_ret)) {
        PyErr_SetString(state->ZstdError, ZSTD_getErrorName(zstd_ret));
        goto error;
    }
    Py_CLEAR(self->dict);

    /* Set decompressLevel/options to decompress context */
    if (option != Py_None) {
        if (set_d_parameters(state, self->dctx, option) < 0) {
            goto error;
        }
    }

    /* Load dictionary to decompress context */
    if (dict != Py_None) {
        if (load_d_dict(state, self->dctx, dict) < 0) {
            goto error;
        }

        /* Py_INCREF the dict */
        Py_INCREF(dict);
        self->dict = dict;
    }

    /* Clear unconsumed data */
    self->needs_input = 1;
    if (self->input_buffer) {
        PyMem_Free(self->input_buffer);
        self->input_buffer = NULL;
    }
    self->input_buffer_size = 0;

    goto success;

error:
    ret = -1;
success:
    RELEASE_LOCK(self);
    return ret;
}

static inline PyObject *
decompress_impl(ZstdDecompressor *self, ZSTD_inBuffer *in,
                Py_buffer *data, Py_ssize_t max_length)
{
    size_t zstd_ret;
    ZSTD_outBuffer out;
    _BlocksOutputBuffer buffer;
    PyObject *ret;
    _zstd_state *state = PyType_GetModuleState(Py_TYPE(self));
    assert(state != NULL);

    /* OutputBuffer(OnError)(&buffer) is after `error` label,
       so initialize the buffer before any `goto error` statement. */
    if (OutputBuffer(InitAndGrow)(&buffer, max_length, &out) < 0) {
        goto error;
    }

    while (1) {
        Py_BEGIN_ALLOW_THREADS
        zstd_ret = ZSTD_decompressStream(self->dctx, &out, in);
        Py_END_ALLOW_THREADS

        /* Check error */
        if (ZSTD_isError(zstd_ret)) {
            PyErr_SetString(state->ZstdError, ZSTD_getErrorName(zstd_ret));
            goto error;
        }

        /* Output buffer exhausted.
           Need to check `out` before `in`. Maybe zstd's internal buffer still
           have a few bytes can be output, grow the output buffer and continue
           if max_lengh < 0. */
        if (out.pos == out.size) {
            /* Output buffer reached max_length */
            if (OutputBuffer(GetDataSize)(&buffer, &out) == max_length) {
                ret = OutputBuffer(Finish)(&buffer, &out);
                if (ret != NULL) {
                    goto success;
                } else {
                    goto error;
                }
            }

            /* Grow output buffer */
            if (OutputBuffer(Grow)(&buffer, &out) < 0) {
                goto error;
            }
        } else if (in->pos == in->size) {
            /* Finished */
            ret = OutputBuffer(Finish)(&buffer, &out);
            if (ret != NULL) {
                goto success;
            } else {
                goto error;
            }
        }
    }

error:
    OutputBuffer(OnError)(&buffer);
    ret = NULL;
success:
    return ret;
}

/*[clinic input]
_zstd.ZstdDecompressor.decompress

    data: Py_buffer
    max_length: Py_ssize_t = -1

Decompress *data*, returning uncompressed data as bytes.

If *max_length *is nonnegative, returns at most *max_length *bytes of
decompressed data. If this limit is reached and further output can be
produced, *self.needs_input *will be set to ``False``. In this case, the next
call to *decompress()* may provide *data *as b'' to obtain more of the output.

If all of the input data was decompressed and returned (either because this
was less than *max_length *bytes, or because *max_length *was negative),
*self.needs_input *will be set to True.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDecompressor_decompress_impl(ZstdDecompressor *self,
                                       Py_buffer *data,
                                       Py_ssize_t max_length)
/*[clinic end generated code: output=a4302b3c940dbec6 input=52d061d9477a95ff]*/
{
    ZSTD_inBuffer in;
    PyObject *ret;
    uint8_t *temp;

    ACQUIRE_LOCK(self);

    /* Input position */
    in.pos = 0;

    /* Unconsumed data */
    if (self->input_buffer == NULL) {         
        in.src = data->buf;
        in.size = data->len;
    } else {
        temp = PyMem_Realloc(self->input_buffer,
                             self->input_buffer_size + data->len);
        if (temp == NULL) {
            PyErr_NoMemory();
            goto error;
        }
        memcpy(temp + self->input_buffer_size, data->buf, data->len);

        self->input_buffer = temp;
        self->input_buffer_size += data->len;

        in.src = self->input_buffer;
        in.size = self->input_buffer_size;
    }

    /* Decompress */
    ret = decompress_impl(self, &in, data, max_length);
    if (ret == NULL) {
        goto error;
    }

    if (in.pos < in.size) {
        /* Has unconsumed data */
        self->needs_input = 0;

        size_t const buffer_size = in.size - in.pos;
        temp = PyMem_Malloc(buffer_size);
        if (temp == NULL) {
            PyErr_NoMemory();
            goto error;
        }
        memcpy(temp, (uint8_t*)in.src + in.pos, buffer_size);
        
        if (self->input_buffer) {
            PyMem_Free(self->input_buffer);
        }

        self->input_buffer = temp;
        self->input_buffer_size = buffer_size;
    } else if (in.pos == in.size) {
        /* Both input and output buffer exhausted, try to output
           internal buffer's data next time. */
        if (Py_SIZE(ret) == max_length) {
            self->needs_input = 0;
        } else {
            self->needs_input = 1;
        }

        /* Clear input_buffer */
        if (self->input_buffer) {
            PyMem_Free(self->input_buffer);
            self->input_buffer = NULL;
        }
        self->input_buffer_size = 0;
    }

    goto success;

error:
    /* Clear unconsumed data */
    if (self->input_buffer) {
        PyMem_Free(self->input_buffer);
        self->input_buffer = NULL;
    }
    self->input_buffer_size = 0;

    ret = NULL;
success:
    RELEASE_LOCK(self);
    return ret;
}

static int
_ZstdDecompressor_traverse(ZstdDecompressor *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

static PyMethodDef _ZstdDecompressor_methods[] = {
    _ZSTD_ZSTDDECOMPRESSOR_DECOMPRESS_METHODDEF
    {NULL, NULL}
};

PyDoc_STRVAR(ZstdDecompressor_needs_input_doc,
"True if more input is needed before more decompressed data can be produced.");


static PyMemberDef _ZstdDecompressor_members[] = {
    {"needs_input", T_BOOL, offsetof(ZstdDecompressor, needs_input),
      READONLY, ZstdDecompressor_needs_input_doc},
    {NULL}
};

static PyType_Slot zstddecompressor_slots[] = {
    {Py_tp_new, _ZstdDecompressor_new},
    {Py_tp_dealloc, _ZstdDecompressor_dealloc},
    {Py_tp_init, _zstd_ZstdDecompressor___init__},
    {Py_tp_methods, _ZstdDecompressor_methods},
    {Py_tp_members, _ZstdDecompressor_members},
    //{Py_tp_doc, (char*)Decompressor_doc},
    {Py_tp_traverse, _ZstdDecompressor_traverse},
    {0, 0}
};

static PyType_Spec zstddecompressor_type_spec = {
    .name = "_zstd.ZstdDecompressor",
    .basicsize = sizeof(ZstdDecompressor),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = zstddecompressor_slots,
};


/*[clinic input]
_zstd._get_cparam_bounds

    cParam: int

Get cParameter bounds.
[clinic start generated code]*/

static PyObject *
_zstd__get_cparam_bounds_impl(PyObject *module, int cParam)
/*[clinic end generated code: output=5b0f68046a6f0721 input=d98575eb2b39d124]*/
{
    PyObject *ret;

    ZSTD_bounds const bound = ZSTD_cParam_getBounds(cParam);
    if (ZSTD_isError(bound.error)) {
        _zstd_state *state = get_zstd_state(module);
        PyErr_SetString(state->ZstdError, ZSTD_getErrorName(bound.error));
        return NULL;
    }

    ret = PyTuple_New(2);
    if (ret == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    PyTuple_SET_ITEM(ret, 0, PyLong_FromLong(bound.lowerBound));
    PyTuple_SET_ITEM(ret, 1, PyLong_FromLong(bound.upperBound));

    return ret;
}

/*[clinic input]
_zstd._get_dparam_bounds

    dParam: int

Get dParameter bounds.
[clinic start generated code]*/

static PyObject *
_zstd__get_dparam_bounds_impl(PyObject *module, int dParam)
/*[clinic end generated code: output=6382b8e9779430c2 input=4a29b9fd6fafe7c6]*/
{
    PyObject *ret;

    ZSTD_bounds const bound = ZSTD_dParam_getBounds(dParam);
    if (ZSTD_isError(bound.error)) {
        _zstd_state *state = get_zstd_state(module);
        PyErr_SetString(state->ZstdError, ZSTD_getErrorName(bound.error));
        return NULL;
    }

    ret = PyTuple_New(2);
    if (ret == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    PyTuple_SET_ITEM(ret, 0, PyLong_FromLong(bound.lowerBound));
    PyTuple_SET_ITEM(ret, 1, PyLong_FromLong(bound.upperBound));

    return ret;
}

/*[clinic input]
_zstd.get_frame_info

    frame_buffer: Py_buffer
        The start bytes of a frame, a frame header is 6-18 bytes, should not
        be smaller than this size.

Get zstd frame infomation.

Return a three-items tuple: (decompressed_size, dictinary_id). If decompressed
size is unknown, it will be None. If no dictionary, dictinary_id will be 0.
[clinic start generated code]*/

static PyObject *
_zstd_get_frame_info_impl(PyObject *module, Py_buffer *frame_buffer)
/*[clinic end generated code: output=56e033cf48001929 input=23c7c2557e996f06]*/
{
    unsigned long long content_size;
    char unknown_content_size;
    UINT32 dict_id;
    PyObject *temp;
    PyObject *ret = NULL;

    /* ZSTD_getFrameContentSize */
    content_size = ZSTD_getFrameContentSize(frame_buffer->buf,
                                            frame_buffer->len);
    if (content_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        unknown_content_size = 1;
    } else if (content_size == ZSTD_CONTENTSIZE_ERROR) {
        _zstd_state *state = get_zstd_state(module);
        PyErr_SetString(state->ZstdError,
                        "Error when getting frame content size, "
                        "please make sure that frame_buffer points "
                        "to the beginning of a frame and provide "
                        "a size larger than the frame header.");
        goto error;
    } else {
        unknown_content_size = 0;
    }

    /* ZSTD_getDictID_fromFrame */
    dict_id = ZSTD_getDictID_fromFrame(frame_buffer->buf, frame_buffer->len);

    /* Build tuple */
    ret = PyTuple_New(2);
    if (ret == NULL) {
        PyErr_NoMemory();
        goto error;
    }

    /* 0, content_size */
    if (unknown_content_size) {
        temp = Py_None;
        Py_INCREF(temp);
    } else {
        temp = PyLong_FromUnsignedLongLong(content_size);
        if (temp == NULL) {
            goto error;
        }
    }
    PyTuple_SET_ITEM(ret, 0, temp);

    /* 1, dict_id */
    temp = PyLong_FromUnsignedLong(dict_id);
    if (temp == NULL) {
        goto error;
    }
    PyTuple_SET_ITEM(ret, 1, temp);

    return ret;
error:
    Py_XDECREF(ret);
    return NULL;
}


/*[clinic input]
_zstd.get_frame_size

    frame_buffer: Py_buffer
        At least one complete frame, start at a frame's first byte.

Get the size of a zstd frame.

It will iterate all the blocks in a frame.
[clinic start generated code]*/

static PyObject *
_zstd_get_frame_size_impl(PyObject *module, Py_buffer *frame_buffer)
/*[clinic end generated code: output=a7384c2f8780f442 input=67213417bcbc2db6]*/
{
    size_t frame_size;
    PyObject *ret;

    frame_size = ZSTD_findFrameCompressedSize(frame_buffer->buf, frame_buffer->len);
    if (ZSTD_isError(frame_size)) {
        _zstd_state *state = get_zstd_state(module);
        PyErr_SetString(state->ZstdError, ZSTD_getErrorName(frame_size));
        return NULL;
    }

    ret = PyLong_FromSize_t(frame_size);
    if (ret == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    return ret;
}


static int
zstd_exec(PyObject *module)
{
    PyObject *temp;
    _zstd_state *state = get_zstd_state(module);

    /* Add zstd parameters */
    if (add_parameters(module) < 0) {
        goto error;
    }

    /* ZSTD_strategy enum */
    ADD_INT_PREFIX_MACRO(module, ZSTD_fast);
    ADD_INT_PREFIX_MACRO(module, ZSTD_dfast);
    ADD_INT_PREFIX_MACRO(module, ZSTD_greedy);
    ADD_INT_PREFIX_MACRO(module, ZSTD_lazy);
    ADD_INT_PREFIX_MACRO(module, ZSTD_lazy2);
    ADD_INT_PREFIX_MACRO(module, ZSTD_btlazy2);
    ADD_INT_PREFIX_MACRO(module, ZSTD_btopt);
    ADD_INT_PREFIX_MACRO(module, ZSTD_btultra);
    ADD_INT_PREFIX_MACRO(module, ZSTD_btultra2);

    /* EndDirective enum */
    ADD_INT_PREFIX_MACRO(module, ZSTD_e_continue);
    ADD_INT_PREFIX_MACRO(module, ZSTD_e_flush);
    ADD_INT_PREFIX_MACRO(module, ZSTD_e_end);

    state->ZstdError = NULL;
    state->ZstdDict_type = NULL;
    state->ZstdCompressor_type = NULL;
    state->ZstdDecompressor_type = NULL;

    /* ZstdError */
    state->ZstdError = PyErr_NewExceptionWithDoc("_zstd.ZstdError", "Call to zstd failed.", NULL, NULL);
    if (state->ZstdError == NULL) {
        goto error;
    }
    if (PyModule_AddType(module, (PyTypeObject *)state->ZstdError) < 0) {
        goto error;
    }

    /* ZstdDict */
    state->ZstdDict_type = (PyTypeObject *)PyType_FromModuleAndSpec(module,
                                                                    &zstddict_type_spec,
                                                                    NULL);
    if (state->ZstdDict_type == NULL) {
        goto error;
    }
    if (PyModule_AddType(module, (PyTypeObject *)state->ZstdDict_type) < 0) {
        goto error;
    }

    /* ZstdCompressor */
    state->ZstdCompressor_type = (PyTypeObject*)PyType_FromModuleAndSpec(module,
                                                &zstdcompressor_type_spec, NULL);
    if (state->ZstdCompressor_type == NULL) {
        goto error;
    }
    if (PyModule_AddType(module, (PyTypeObject*)state->ZstdCompressor_type) < 0) {
        goto error;
    }

    /* ZstdDecompressor */
    state->ZstdDecompressor_type = (PyTypeObject*)PyType_FromModuleAndSpec(module,
                                                  &zstddecompressor_type_spec, NULL);
    if (state->ZstdDecompressor_type == NULL) {
        goto error;
    }
    if (PyModule_AddType(module, (PyTypeObject*)state->ZstdDecompressor_type) < 0) {
        goto error;
    }

    /* zstd_version, ZSTD_versionString() requires zstd v1.3.0+ */
    if (!(temp = PyUnicode_FromString(ZSTD_versionString()))) {
        goto error;
    }
    if (PyModule_AddObject(module, "zstd_version", temp) < 0) {
        Py_DECREF(temp);
        goto error;
    }

    /* zstd_version_info */
    if (!(temp = PyTuple_New(3))) {
        goto error;
    }
    PyTuple_SET_ITEM(temp, 0, PyLong_FromLong(ZSTD_VERSION_MAJOR));
    PyTuple_SET_ITEM(temp, 1, PyLong_FromLong(ZSTD_VERSION_MINOR));
    PyTuple_SET_ITEM(temp, 2, PyLong_FromLong(ZSTD_VERSION_RELEASE));
    if (PyModule_AddObject(module, "zstd_version_info", temp) < 0) {
        Py_DECREF(temp);
        goto error;
    }

    /* level_bounds */
    if (!(temp = PyTuple_New(2))) {
        goto error;
    }
    PyTuple_SET_ITEM(temp, 0, PyLong_FromLong(ZSTD_minCLevel()));
    PyTuple_SET_ITEM(temp, 1, PyLong_FromLong(ZSTD_maxCLevel()));
    if (PyModule_AddObject(module, "level_bounds", temp) < 0) {
        Py_DECREF(temp);
        goto error;
    }

    return 0;

error:
    Py_XDECREF(state->ZstdError);
    Py_XDECREF(state->ZstdDict_type);
    Py_XDECREF(state->ZstdCompressor_type);
    Py_XDECREF(state->ZstdDecompressor_type);
    return -1;
}

static PyMethodDef _zstd_methods[] = {
    _ZSTD__TRAIN_DICT_METHODDEF
    _ZSTD__GET_CPARAM_BOUNDS_METHODDEF
    _ZSTD__GET_DPARAM_BOUNDS_METHODDEF
    _ZSTD_GET_FRAME_INFO_METHODDEF
    _ZSTD_GET_FRAME_SIZE_METHODDEF
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
    Py_VISIT(state->ZstdError);
    Py_VISIT(state->ZstdDict_type);
    Py_VISIT(state->ZstdCompressor_type);
    Py_VISIT(state->ZstdDecompressor_type);
    return 0;
}

static int
_zstd_clear(PyObject *module)
{
    _zstd_state *state = get_zstd_state(module);
    Py_CLEAR(state->ZstdError);
    Py_CLEAR(state->ZstdDict_type);
    Py_CLEAR(state->ZstdCompressor_type);
    Py_CLEAR(state->ZstdDecompressor_type);
    return 0;
}

static void
_zstd_free(void *module)
{
    _zstd_clear((PyObject *)module);
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
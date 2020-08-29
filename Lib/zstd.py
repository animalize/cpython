__all__ = ('ZstdDict', 'ZstdError',
           'compress', 'decompress', 'train_dict',
           'ZstdCompressor', 'ZstdDecompressor',
           'cParameter', 'dParameter')

import builtins
import io
import os
import _compression
from enum import IntEnum

from _zstd import *
import _zstd

class CompressParameter(IntEnum):
    compressionLevel = _zstd._ZSTD_c_compressionLevel
    windowLog = _zstd._ZSTD_c_windowLog
    hashLog = _zstd._ZSTD_c_hashLog
    chainLog = _zstd._ZSTD_c_chainLog
    searchLog = _zstd._ZSTD_c_searchLog
    minMatch = _zstd._ZSTD_c_minMatch
    targetLength = _zstd._ZSTD_c_targetLength
    strategy = _zstd._ZSTD_c_strategy
    enableLongDistanceMatching = _zstd._ZSTD_c_enableLongDistanceMatching
    ldmHashLog = _zstd._ZSTD_c_ldmHashLog
    ldmMinMatch = _zstd._ZSTD_c_ldmMinMatch
    ldmBucketSizeLog = _zstd._ZSTD_c_ldmBucketSizeLog
    ldmHashRateLog = _zstd._ZSTD_c_ldmHashRateLog
    contentSizeFlag = _zstd._ZSTD_c_contentSizeFlag
    checksumFlag = _zstd._ZSTD_c_checksumFlag
    dictIDFlag = _zstd._ZSTD_c_dictIDFlag

    def bounds(self):
        """Return lower and upper bounds of a parameter, both inclusive."""
        return _zstd._get_cparam_bounds(self.value)


class DecompressParameter(IntEnum):
    windowLogMax = _zstd._ZSTD_d_windowLogMax

    def bounds(self):
        """Return lower and upper bounds of a parameter, both inclusive."""
        return _zstd._get_dparam_bounds(self.value)

class EndDirective(IntEnum):
    CONTINUE = _zstd._ZSTD_e_continue
    FLUSH = _zstd._ZSTD_e_flush
    END = _zstd._ZSTD_e_end

def compress(data, level_or_option=None, dict=None):
    """Compress a block of data.

    Refer to ZstdCompressor's docstring for a description of the
    optional arguments *level_or_option* and *dict*.

    For incremental compression, use an ZstdCompressor instead.
    """
    comp = ZstdCompressor(level_or_option, dict)
    return comp.compress(data, _zstd._ZSTD_e_end)

def decompress(data, dict=None, option=None):
    """Decompress a block of data.

    Refer to ZstdDecompressor's docstring for a description of the
    optional arguments *option* and *dict*.

    For incremental decompression, use an ZstdDecompressor instead.
    """
    results = []
    while True:
        decomp = ZstdDecompressor(dict, option)
        try:
            res = decomp.decompress(data)
        except ZstdError:
            if results:
                break  # Leftover data is not a valid LZMA/XZ stream; ignore it.
            else:
                raise  # Error on the first iteration; bail out.
        results.append(res)

        #data = decomp.unused_data
        if not data:
            break
    return b"".join(results)

def train_dict(iterable_of_chunks, dict_size=100*1024):
    chunks = []
    chunk_sizes = []
    for chunk in iterable_of_chunks:
        chunks.append(chunk)
        chunk_sizes.append(len(chunk))

    chunks = b''.join(chunks)
    # chunks: samples be stored concatenated in a single flat buffer.
    # chunk_sizes: a list of each sample's size.
    # dict_size: size of the dictionary.
    dict_content = _zstd._train_dict(chunks, chunk_sizes, dict_size)

    return ZstdDict(dict_content)
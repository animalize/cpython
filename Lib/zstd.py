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
from _zstd import _train_dict, _get_cparam_bounds, _get_dparam_bounds


class CompressParameter(IntEnum):
    compressionLevel = ZSTD_c_compressionLevel
    windowLog = ZSTD_c_windowLog
    hashLog = ZSTD_c_hashLog
    chainLog = ZSTD_c_chainLog
    searchLog = ZSTD_c_searchLog
    minMatch = ZSTD_c_minMatch
    targetLength = ZSTD_c_targetLength
    strategy = ZSTD_c_strategy
    enableLongDistanceMatching = ZSTD_c_enableLongDistanceMatching
    ldmHashLog = ZSTD_c_ldmHashLog
    ldmMinMatch = ZSTD_c_ldmMinMatch
    ldmBucketSizeLog = ZSTD_c_ldmBucketSizeLog
    ldmHashRateLog = ZSTD_c_ldmHashRateLog
    contentSizeFlag = ZSTD_c_contentSizeFlag
    checksumFlag = ZSTD_c_checksumFlag
    dictIDFlag = ZSTD_c_dictIDFlag

    def bounds(self):
        """Return lower and upper bounds of a parameter, both inclusive."""
        return _get_cparam_bounds(self.value)


class DecompressParameter(IntEnum):
    windowLogMax = ZSTD_d_windowLogMax

    def bounds(self):
        """Return lower and upper bounds of a parameter, both inclusive."""
        return _get_dparam_bounds(self.value)


def train_dict(iterable_of_chunks, dict_size=100*1024):
    chunks = []
    chunk_sizes = []
    for chunk in iterable_of_chunks:
        chunks.append(chunk)
        chunk_sizes.append(len(chunk))

    chunks = b''.join(chunks)
    dict_content = _train_dict(chunks, chunk_sizes, dict_size)

    return ZstdDict(dict_content)
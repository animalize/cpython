"""Interface to the liblzma compression library.

This module provides a class for reading and writing compressed files,
classes for incremental (de)compression, and convenience functions for
one-shot (de)compression.

These classes and functions support both the XZ and legacy LZMA
container formats, as well as raw compressed data streams.
"""

__all__ = ('ZstdDict', 'ZstdError',
           'compress', 'decompress', 'train_dict',
           'cParameter', 'dParameter')

import builtins
import io
import os

from _zstd import *
from _zstd import _train_dict, _get_cparam_bounds, _get_dparam_bounds
import _compression

class CompressParameter:
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

    @staticmethod
    def get_bounds(cParameter):
        return _get_cparam_bounds(cParameter)


class DecompressParameter:
    windowLogMax = ZSTD_d_windowLogMax

    @staticmethod
    def get_bounds(dParameter):
        return _get_dparam_bounds(dParameter)


def train_dict(iterable_of_chunks, dict_size=100*1024):
    chunks = []
    chunk_sizes = []
    for chunk in iterable_of_chunks:
        chunks.append(chunk)
        chunk_sizes.append(len(chunk))

    chunks = b''.join(chunks)
    dict_content = _train_dict(chunks, chunk_sizes, dict_size)

    return ZstdDict(dict_content)
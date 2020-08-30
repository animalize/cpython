__all__ = ('ZstdDict', 'ZstdError',
           'compress', 'decompress',
           'train_dict', 'get_frame_info',
           'ZstdCompressor', 'ZstdDecompressor',
           'EndDirective',
           'CompressParameter', 'DecompressParameter')

import builtins
import io
import os
import _compression
from enum import IntEnum

from _zstd import *
import _zstd


class CompressParameter(IntEnum):
    compressionLevel           = _zstd._ZSTD_c_compressionLevel
    windowLog                  = _zstd._ZSTD_c_windowLog
    hashLog                    = _zstd._ZSTD_c_hashLog
    chainLog                   = _zstd._ZSTD_c_chainLog
    searchLog                  = _zstd._ZSTD_c_searchLog
    minMatch                   = _zstd._ZSTD_c_minMatch
    targetLength               = _zstd._ZSTD_c_targetLength
    strategy                   = _zstd._ZSTD_c_strategy
    enableLongDistanceMatching = _zstd._ZSTD_c_enableLongDistanceMatching
    ldmHashLog                 = _zstd._ZSTD_c_ldmHashLog
    ldmMinMatch                = _zstd._ZSTD_c_ldmMinMatch
    ldmBucketSizeLog           = _zstd._ZSTD_c_ldmBucketSizeLog
    ldmHashRateLog             = _zstd._ZSTD_c_ldmHashRateLog
    contentSizeFlag            = _zstd._ZSTD_c_contentSizeFlag
    checksumFlag               = _zstd._ZSTD_c_checksumFlag
    dictIDFlag                 = _zstd._ZSTD_c_dictIDFlag

    def bounds(self):
        """Return lower and upper bounds of a parameter, both inclusive."""
        return _zstd._get_cparam_bounds(self.value)
    

class DecompressParameter(IntEnum):
    windowLogMax = _zstd._ZSTD_d_windowLogMax

    def bounds(self):
        """Return lower and upper bounds of a parameter, both inclusive."""
        return _zstd._get_dparam_bounds(self.value)


class Strategy(IntEnum):
    """Compression strategies, listed from fastest to strongest.

       Note : new strategies _might_ be added in the future, only the order
       (from fast to strong) is guaranteed.
    """
    fast     = _zstd._ZSTD_fast
    dfast    = _zstd._ZSTD_dfast
    greedy   = _zstd._ZSTD_greedy
    lazy     = _zstd._ZSTD_lazy
    lazy2    = _zstd._ZSTD_lazy2
    btlazy2  = _zstd._ZSTD_btlazy2
    btopt    = _zstd._ZSTD_btopt
    btultra  = _zstd._ZSTD_btultra
    btultra2 = _zstd._ZSTD_btultra2


class EndDirective(IntEnum):
    """Stream compressor's end directive.
    
    CONTINUE: Collect more data, encoder decides when to output compressed
              result, for optimal compression ratio. Usually used for ordinary
              streaming compression.
    FLUSH:    Flush any remaining data, but don't end current frame. Usually
              used for communication, the receiver can decode immediately.
    END:      Flush any remaining data _and_ close current frame.
    """
    CONTINUE = _zstd._ZSTD_e_continue
    FLUSH    = _zstd._ZSTD_e_flush
    END      = _zstd._ZSTD_e_end


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
    optional arguments *dict* and *option*.

    For incremental decompression, use an ZstdDecompressor instead.
    """
    decomp = ZstdDecompressor(dict, option)
    return decomp.decompress(data, -1)


def train_dict(iterable_of_chunks, dict_size=100*1024):
    """Train a zstd dictionary, return a ZstdDict object.

    In general:
    1) A reasonable dictionary has a size of ~100 KB. It's possible to select
       smaller or larger size, just by specifying dict_size argument.
    2) It's recommended to provide a few thousands samples, though this can
       vary a lot.
    3) It's recommended that total size of all samples be about ~x100 times the
       target size of dictionary.
    """
    chunks = []
    chunk_sizes = []

    for chunk in iterable_of_chunks:
        chunks.append(chunk)
        chunk_sizes.append(len(chunk))
    chunks = b''.join(chunks)

    # chunks: samples be stored concatenated in a single flat buffer.
    # chunk_sizes: a list of each sample's size.
    # dict_size: size of the dictionary, in bytes.
    dict_content = _zstd._train_dict(chunks, chunk_sizes, dict_size)

    return ZstdDict(dict_content)
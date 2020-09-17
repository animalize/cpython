import _compression
from io import BytesIO, UnsupportedOperation, DEFAULT_BUFFER_SIZE
import os
import pathlib
import pickle
import random
import sys
from test import support
import unittest

from test.support import (
    _4G, bigmemtest, run_unittest
)
from test.support.import_helper import import_module
from test.support.os_helper import (
    TESTFN, unlink
)

zstd = import_module("zstd")
from zstd import ZstdCompressor, ZstdDecompressor, ZstdError, \
                 CParameter, DParameter, Strategy, compress, decompress

COMPRESSED_DAT1 = compress(b'abcdefg123456' * 1000)

class CompressorDecompressorTestCase(unittest.TestCase):

    def test_simple_bad_args(self):
        # ZstdCompressor
        self.assertRaises(TypeError, ZstdCompressor, [])
        self.assertRaises(TypeError, ZstdCompressor, level_or_option=3.14)
        self.assertRaises(TypeError, ZstdCompressor, level_or_option='abc')
        self.assertRaises(TypeError, ZstdCompressor, level_or_option=b'abc')

        self.assertRaises(TypeError, ZstdCompressor, zstd_dict=123)
        self.assertRaises(TypeError, ZstdCompressor, zstd_dict=b'abc')
        self.assertRaises(TypeError, ZstdCompressor, zstd_dict={1:2, 3:4})

        with self.assertRaises(ValueError):
            ZstdCompressor(2**31)
        with self.assertRaises(ValueError):
            ZstdCompressor({2**31 : 100})

        with self.assertRaises(ZstdError):
            ZstdCompressor({CParameter.windowLog:100})
        with self.assertRaises(ZstdError):
            ZstdCompressor({3333 : 100})

        # ZstdDecompressor
        self.assertRaises(TypeError, ZstdDecompressor, ())
        self.assertRaises(TypeError, ZstdDecompressor, zstd_dict=123)
        self.assertRaises(TypeError, ZstdDecompressor, zstd_dict=b'abc')
        self.assertRaises(TypeError, ZstdDecompressor, zstd_dict={1:2, 3:4})

        self.assertRaises(TypeError, ZstdDecompressor, option=123)
        self.assertRaises(TypeError, ZstdDecompressor, option='abc')
        self.assertRaises(TypeError, ZstdDecompressor, option=b'abc')

        with self.assertRaises(ValueError):
            ZstdDecompressor(option={2**31 : 100})

        with self.assertRaises(ZstdError):
            ZstdDecompressor(option={DParameter.windowLogMax:100})
        with self.assertRaises(ZstdError):
            ZstdDecompressor(option={3333 : 100})

        # Method bad arguments
        zc = ZstdCompressor()
        self.assertRaises(TypeError, zc.compress)
        self.assertRaises(TypeError, zc.compress, b"foo", b"bar")
        self.assertRaises(TypeError, zc.compress, "str")
        self.assertRaises(TypeError, zc.flush, b"blah", 1)
        self.assertRaises(ValueError, zc.compress, b"foo", 3)
        empty = zc.flush()

        lzd = ZstdDecompressor()
        self.assertRaises(TypeError, lzd.decompress)
        self.assertRaises(TypeError, lzd.decompress, b"foo", b"bar")
        self.assertRaises(TypeError, lzd.decompress, "str")
        lzd.decompress(empty)

    def test_compress_parameters(self):
        d = {CParameter.compressionLevel : 10,
             CParameter.windowLog : 12,
             CParameter.hashLog : 10,
             CParameter.chainLog : 12,
             CParameter.searchLog : 12,
             CParameter.minMatch : 4,
             CParameter.targetLength : 12,
             CParameter.strategy : Strategy.lazy,
             CParameter.enableLongDistanceMatching : 1,
             CParameter.ldmHashLog : 12,
             CParameter.ldmMinMatch : 11,
             CParameter.ldmBucketSizeLog : 5,
             CParameter.ldmHashRateLog : 12,
             CParameter.contentSizeFlag : 1,
             CParameter.checksumFlag : 1,
             CParameter.dictIDFlag : 0,
             }
        ZstdCompressor(level_or_option=d)

        # larger than signed int, ValueError
        d1 = d.copy()
        d1[CParameter.ldmBucketSizeLog] = 2**31
        self.assertRaises(ValueError, ZstdCompressor, d1)

        # value out of bounds, ZstdError
        d2 = d.copy()
        d2[CParameter.ldmBucketSizeLog] = 10
        self.assertRaises(ZstdError, ZstdCompressor, d2)

    def test_decompress_parameters(self):
        d = {DParameter.windowLogMax : 15}
        ZstdDecompressor(option=d)

        # larger than signed int, ValueError
        d1 = d.copy()
        d1[DParameter.windowLogMax] = 2**31
        self.assertRaises(ValueError, ZstdDecompressor, None, d1)

        # value out of bounds, ZstdError
        d2 = d.copy()
        d2[DParameter.windowLogMax] = 32
        self.assertRaises(ZstdError, ZstdDecompressor, None, d2)


class DecompressorFlagsTestCase(unittest.TestCase):

    def test_empty_input(self):
        d = ZstdDecompressor()
        self.assertTrue(d.at_frame_edge)

        for _ in range(3):
            d.decompress(b'')
            self.assertTrue(d.at_frame_edge)
            self.assertTrue(d.needs_input)

    def test_empty_input_after_frame(self):
        d = ZstdDecompressor()

        # decompress a frame
        d.decompress(COMPRESSED_DAT1)
        self.assertTrue(d.at_frame_edge)
        self.assertTrue(d.needs_input)

        # empty input
        d.decompress(b'')
        self.assertTrue(d.at_frame_edge)
        self.assertTrue(d.needs_input)

    def test_empty_input_after_32K_dat(self):
        # 100 + 32KB
        dat = compress(b'a' * (100 + 32*1024))

        d = ZstdDecompressor()

        # decompress first 100 bytes
        d.decompress(dat, 100)
        self.assertFalse(d.at_frame_edge)
        self.assertFalse(d.needs_input)

        # decompress the rest
        d.decompress(b'')
        self.assertTrue(d.at_frame_edge)
        self.assertTrue(d.needs_input)

        # empty input
        d.decompress(b'')
        self.assertTrue(d.at_frame_edge)
        self.assertTrue(d.needs_input)


# # Test data:


def test_main():
    run_unittest(
        CompressorDecompressorTestCase,
        DecompressorFlagsTestCase,
    )

if __name__ == "__main__":
    test_main()

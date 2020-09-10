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
                 CParameter, DParameter, Strategy


class CompressorDecompressorTestCase(unittest.TestCase):

    # Test error cases.

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

    # def test_decompressor_after_eof(self):
        # lzd = LZMADecompressor()
        # lzd.decompress(COMPRESSED_XZ)
        # self.assertRaises(EOFError, lzd.decompress, b"nyan")

    # def test_decompressor_memlimit(self):
        # lzd = LZMADecompressor(memlimit=1024)
        # self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_XZ)

        # lzd = LZMADecompressor(lzma.FORMAT_XZ, memlimit=1024)
        # self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_XZ)

        # lzd = LZMADecompressor(lzma.FORMAT_ALONE, memlimit=1024)
        # self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_ALONE)

    # # Test LZMADecompressor on known-good input data.

    # def _test_decompressor(self, lzd, data, check, unused_data=b""):
        # self.assertFalse(lzd.eof)
        # out = lzd.decompress(data)
        # self.assertEqual(out, INPUT)
        # self.assertEqual(lzd.check, check)
        # self.assertTrue(lzd.eof)
        # self.assertEqual(lzd.unused_data, unused_data)

    # def test_decompressor_auto(self):
        # lzd = LZMADecompressor()
        # self._test_decompressor(lzd, COMPRESSED_XZ, lzma.CHECK_CRC64)

        # lzd = LZMADecompressor()
        # self._test_decompressor(lzd, COMPRESSED_ALONE, lzma.CHECK_NONE)

    # def test_decompressor_xz(self):
        # lzd = LZMADecompressor(lzma.FORMAT_XZ)
        # self._test_decompressor(lzd, COMPRESSED_XZ, lzma.CHECK_CRC64)

    # def test_decompressor_alone(self):
        # lzd = LZMADecompressor(lzma.FORMAT_ALONE)
        # self._test_decompressor(lzd, COMPRESSED_ALONE, lzma.CHECK_NONE)

    # def test_decompressor_raw_1(self):
        # lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_1)
        # self._test_decompressor(lzd, COMPRESSED_RAW_1, lzma.CHECK_NONE)

    # def test_decompressor_raw_2(self):
        # lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_2)
        # self._test_decompressor(lzd, COMPRESSED_RAW_2, lzma.CHECK_NONE)

    # def test_decompressor_raw_3(self):
        # lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_3)
        # self._test_decompressor(lzd, COMPRESSED_RAW_3, lzma.CHECK_NONE)

    # def test_decompressor_raw_4(self):
        # lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        # self._test_decompressor(lzd, COMPRESSED_RAW_4, lzma.CHECK_NONE)

    # def test_decompressor_chunks(self):
        # lzd = LZMADecompressor()
        # out = []
        # for i in range(0, len(COMPRESSED_XZ), 10):
            # self.assertFalse(lzd.eof)
            # out.append(lzd.decompress(COMPRESSED_XZ[i:i+10]))
        # out = b"".join(out)
        # self.assertEqual(out, INPUT)
        # self.assertEqual(lzd.check, lzma.CHECK_CRC64)
        # self.assertTrue(lzd.eof)
        # self.assertEqual(lzd.unused_data, b"")

    # def test_decompressor_chunks_empty(self):
        # lzd = LZMADecompressor()
        # out = []
        # for i in range(0, len(COMPRESSED_XZ), 10):
            # self.assertFalse(lzd.eof)
            # out.append(lzd.decompress(b''))
            # out.append(lzd.decompress(b''))
            # out.append(lzd.decompress(b''))
            # out.append(lzd.decompress(COMPRESSED_XZ[i:i+10]))
        # out = b"".join(out)
        # self.assertEqual(out, INPUT)
        # self.assertEqual(lzd.check, lzma.CHECK_CRC64)
        # self.assertTrue(lzd.eof)
        # self.assertEqual(lzd.unused_data, b"")

    # def test_decompressor_chunks_maxsize(self):
        # lzd = LZMADecompressor()
        # max_length = 100
        # out = []

        # # Feed first half the input
        # len_ = len(COMPRESSED_XZ) // 2
        # out.append(lzd.decompress(COMPRESSED_XZ[:len_],
                                  # max_length=max_length))
        # self.assertFalse(lzd.needs_input)
        # self.assertEqual(len(out[-1]), max_length)

        # # Retrieve more data without providing more input
        # out.append(lzd.decompress(b'', max_length=max_length))
        # self.assertFalse(lzd.needs_input)
        # self.assertEqual(len(out[-1]), max_length)

        # # Retrieve more data while providing more input
        # out.append(lzd.decompress(COMPRESSED_XZ[len_:],
                                  # max_length=max_length))
        # self.assertLessEqual(len(out[-1]), max_length)

        # # Retrieve remaining uncompressed data
        # while not lzd.eof:
            # out.append(lzd.decompress(b'', max_length=max_length))
            # self.assertLessEqual(len(out[-1]), max_length)

        # out = b"".join(out)
        # self.assertEqual(out, INPUT)
        # self.assertEqual(lzd.check, lzma.CHECK_CRC64)
        # self.assertEqual(lzd.unused_data, b"")

    # def test_decompressor_inputbuf_1(self):
        # # Test reusing input buffer after moving existing
        # # contents to beginning
        # lzd = LZMADecompressor()
        # out = []

        # # Create input buffer and fill it
        # self.assertEqual(lzd.decompress(COMPRESSED_XZ[:100],
                                        # max_length=0), b'')

        # # Retrieve some results, freeing capacity at beginning
        # # of input buffer
        # out.append(lzd.decompress(b'', 2))

        # # Add more data that fits into input buffer after
        # # moving existing data to beginning
        # out.append(lzd.decompress(COMPRESSED_XZ[100:105], 15))

        # # Decompress rest of data
        # out.append(lzd.decompress(COMPRESSED_XZ[105:]))
        # self.assertEqual(b''.join(out), INPUT)

    # def test_decompressor_inputbuf_2(self):
        # # Test reusing input buffer by appending data at the
        # # end right away
        # lzd = LZMADecompressor()
        # out = []

        # # Create input buffer and empty it
        # self.assertEqual(lzd.decompress(COMPRESSED_XZ[:200],
                                        # max_length=0), b'')
        # out.append(lzd.decompress(b''))

        # # Fill buffer with new data
        # out.append(lzd.decompress(COMPRESSED_XZ[200:280], 2))

        # # Append some more data, not enough to require resize
        # out.append(lzd.decompress(COMPRESSED_XZ[280:300], 2))

        # # Decompress rest of data
        # out.append(lzd.decompress(COMPRESSED_XZ[300:]))
        # self.assertEqual(b''.join(out), INPUT)

    # def test_decompressor_inputbuf_3(self):
        # # Test reusing input buffer after extending it

        # lzd = LZMADecompressor()
        # out = []

        # # Create almost full input buffer
        # out.append(lzd.decompress(COMPRESSED_XZ[:200], 5))

        # # Add even more data to it, requiring resize
        # out.append(lzd.decompress(COMPRESSED_XZ[200:300], 5))

        # # Decompress rest of data
        # out.append(lzd.decompress(COMPRESSED_XZ[300:]))
        # self.assertEqual(b''.join(out), INPUT)

    # def test_decompressor_unused_data(self):
        # lzd = LZMADecompressor()
        # extra = b"fooblibar"
        # self._test_decompressor(lzd, COMPRESSED_XZ + extra, lzma.CHECK_CRC64,
                                # unused_data=extra)

    # def test_decompressor_bad_input(self):
        # lzd = LZMADecompressor()
        # self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_RAW_1)

        # lzd = LZMADecompressor(lzma.FORMAT_XZ)
        # self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_ALONE)

        # lzd = LZMADecompressor(lzma.FORMAT_ALONE)
        # self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_XZ)

        # lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_1)
        # self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_XZ)

    # def test_decompressor_bug_28275(self):
        # # Test coverage for Issue 28275
        # lzd = LZMADecompressor()
        # self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_RAW_1)
        # # Previously, a second call could crash due to internal inconsistency
        # self.assertRaises(LZMAError, lzd.decompress, COMPRESSED_RAW_1)

    # # Test that LZMACompressor->LZMADecompressor preserves the input data.

    # def test_roundtrip_xz(self):
        # lzc = LZMACompressor()
        # cdata = lzc.compress(INPUT) + lzc.flush()
        # lzd = LZMADecompressor()
        # self._test_decompressor(lzd, cdata, lzma.CHECK_CRC64)

    # def test_roundtrip_alone(self):
        # lzc = LZMACompressor(lzma.FORMAT_ALONE)
        # cdata = lzc.compress(INPUT) + lzc.flush()
        # lzd = LZMADecompressor()
        # self._test_decompressor(lzd, cdata, lzma.CHECK_NONE)

    # def test_roundtrip_raw(self):
        # lzc = LZMACompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        # cdata = lzc.compress(INPUT) + lzc.flush()
        # lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        # self._test_decompressor(lzd, cdata, lzma.CHECK_NONE)

    # def test_roundtrip_raw_empty(self):
        # lzc = LZMACompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        # cdata = lzc.compress(INPUT)
        # cdata += lzc.compress(b'')
        # cdata += lzc.compress(b'')
        # cdata += lzc.compress(b'')
        # cdata += lzc.flush()
        # lzd = LZMADecompressor(lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        # self._test_decompressor(lzd, cdata, lzma.CHECK_NONE)

    # def test_roundtrip_chunks(self):
        # lzc = LZMACompressor()
        # cdata = []
        # for i in range(0, len(INPUT), 10):
            # cdata.append(lzc.compress(INPUT[i:i+10]))
        # cdata.append(lzc.flush())
        # cdata = b"".join(cdata)
        # lzd = LZMADecompressor()
        # self._test_decompressor(lzd, cdata, lzma.CHECK_CRC64)

    # def test_roundtrip_empty_chunks(self):
        # lzc = LZMACompressor()
        # cdata = []
        # for i in range(0, len(INPUT), 10):
            # cdata.append(lzc.compress(INPUT[i:i+10]))
            # cdata.append(lzc.compress(b''))
            # cdata.append(lzc.compress(b''))
            # cdata.append(lzc.compress(b''))
        # cdata.append(lzc.flush())
        # cdata = b"".join(cdata)
        # lzd = LZMADecompressor()
        # self._test_decompressor(lzd, cdata, lzma.CHECK_CRC64)

    # # LZMADecompressor intentionally does not handle concatenated streams.

    # def test_decompressor_multistream(self):
        # lzd = LZMADecompressor()
        # self._test_decompressor(lzd, COMPRESSED_XZ + COMPRESSED_ALONE,
                                # lzma.CHECK_CRC64, unused_data=COMPRESSED_ALONE)

    # # Test with inputs larger than 4GiB.

    # @support.skip_if_pgo_task
    # @bigmemtest(size=_4G + 100, memuse=2)
    # def test_compressor_bigmem(self, size):
        # lzc = LZMACompressor()
        # cdata = lzc.compress(b"x" * size) + lzc.flush()
        # ddata = lzma.decompress(cdata)
        # try:
            # self.assertEqual(len(ddata), size)
            # self.assertEqual(len(ddata.strip(b"x")), 0)
        # finally:
            # ddata = None

    # @support.skip_if_pgo_task
    # @bigmemtest(size=_4G + 100, memuse=3)
    # def test_decompressor_bigmem(self, size):
        # lzd = LZMADecompressor()
        # blocksize = 10 * 1024 * 1024
        # block = random.randbytes(blocksize)
        # try:
            # input = block * (size // blocksize + 1)
            # cdata = lzma.compress(input)
            # ddata = lzd.decompress(cdata)
            # self.assertEqual(ddata, input)
        # finally:
            # input = cdata = ddata = None

    # # Pickling raises an exception; there's no way to serialize an lzma_stream.

    # def test_pickle(self):
        # for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            # with self.assertRaises(TypeError):
                # pickle.dumps(LZMACompressor(), proto)
            # with self.assertRaises(TypeError):
                # pickle.dumps(LZMADecompressor(), proto)

    # @support.refcount_test
    # def test_refleaks_in_decompressor___init__(self):
        # gettotalrefcount = support.get_attribute(sys, 'gettotalrefcount')
        # lzd = LZMADecompressor()
        # refs_before = gettotalrefcount()
        # for i in range(100):
            # lzd.__init__()
        # self.assertAlmostEqual(gettotalrefcount() - refs_before, 0, delta=10)


# class CompressDecompressFunctionTestCase(unittest.TestCase):

    # # Test error cases:

    # def test_bad_args(self):
        # self.assertRaises(TypeError, lzma.compress)
        # self.assertRaises(TypeError, lzma.compress, [])
        # self.assertRaises(TypeError, lzma.compress, b"", format="xz")
        # self.assertRaises(TypeError, lzma.compress, b"", check="none")
        # self.assertRaises(TypeError, lzma.compress, b"", preset="blah")
        # self.assertRaises(TypeError, lzma.compress, b"", filters=1024)
        # # Can't specify a preset and a custom filter chain at the same time.
        # with self.assertRaises(ValueError):
            # lzma.compress(b"", preset=3, filters=[{"id": lzma.FILTER_LZMA2}])

        # self.assertRaises(TypeError, lzma.decompress)
        # self.assertRaises(TypeError, lzma.decompress, [])
        # self.assertRaises(TypeError, lzma.decompress, b"", format="lzma")
        # self.assertRaises(TypeError, lzma.decompress, b"", memlimit=7.3e9)
        # with self.assertRaises(TypeError):
            # lzma.decompress(b"", format=lzma.FORMAT_RAW, filters={})
        # # Cannot specify a memory limit with FILTER_RAW.
        # with self.assertRaises(ValueError):
            # lzma.decompress(b"", format=lzma.FORMAT_RAW, memlimit=0x1000000)
        # # Can only specify a custom filter chain with FILTER_RAW.
        # with self.assertRaises(ValueError):
            # lzma.decompress(b"", filters=FILTERS_RAW_1)
        # with self.assertRaises(ValueError):
            # lzma.decompress(b"", format=lzma.FORMAT_XZ, filters=FILTERS_RAW_1)
        # with self.assertRaises(ValueError):
            # lzma.decompress(
                    # b"", format=lzma.FORMAT_ALONE, filters=FILTERS_RAW_1)

    # def test_decompress_memlimit(self):
        # with self.assertRaises(LZMAError):
            # lzma.decompress(COMPRESSED_XZ, memlimit=1024)
        # with self.assertRaises(LZMAError):
            # lzma.decompress(
                    # COMPRESSED_XZ, format=lzma.FORMAT_XZ, memlimit=1024)
        # with self.assertRaises(LZMAError):
            # lzma.decompress(
                    # COMPRESSED_ALONE, format=lzma.FORMAT_ALONE, memlimit=1024)

    # # Test LZMADecompressor on known-good input data.

    # def test_decompress_good_input(self):
        # ddata = lzma.decompress(COMPRESSED_XZ)
        # self.assertEqual(ddata, INPUT)

        # ddata = lzma.decompress(COMPRESSED_ALONE)
        # self.assertEqual(ddata, INPUT)

        # ddata = lzma.decompress(COMPRESSED_XZ, lzma.FORMAT_XZ)
        # self.assertEqual(ddata, INPUT)

        # ddata = lzma.decompress(COMPRESSED_ALONE, lzma.FORMAT_ALONE)
        # self.assertEqual(ddata, INPUT)

        # ddata = lzma.decompress(
                # COMPRESSED_RAW_1, lzma.FORMAT_RAW, filters=FILTERS_RAW_1)
        # self.assertEqual(ddata, INPUT)

        # ddata = lzma.decompress(
                # COMPRESSED_RAW_2, lzma.FORMAT_RAW, filters=FILTERS_RAW_2)
        # self.assertEqual(ddata, INPUT)

        # ddata = lzma.decompress(
                # COMPRESSED_RAW_3, lzma.FORMAT_RAW, filters=FILTERS_RAW_3)
        # self.assertEqual(ddata, INPUT)

        # ddata = lzma.decompress(
                # COMPRESSED_RAW_4, lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        # self.assertEqual(ddata, INPUT)

    # def test_decompress_incomplete_input(self):
        # self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_XZ[:128])
        # self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_ALONE[:128])
        # self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_RAW_1[:128],
                          # format=lzma.FORMAT_RAW, filters=FILTERS_RAW_1)
        # self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_RAW_2[:128],
                          # format=lzma.FORMAT_RAW, filters=FILTERS_RAW_2)
        # self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_RAW_3[:128],
                          # format=lzma.FORMAT_RAW, filters=FILTERS_RAW_3)
        # self.assertRaises(LZMAError, lzma.decompress, COMPRESSED_RAW_4[:128],
                          # format=lzma.FORMAT_RAW, filters=FILTERS_RAW_4)

    # def test_decompress_bad_input(self):
        # with self.assertRaises(LZMAError):
            # lzma.decompress(COMPRESSED_BOGUS)
        # with self.assertRaises(LZMAError):
            # lzma.decompress(COMPRESSED_RAW_1)
        # with self.assertRaises(LZMAError):
            # lzma.decompress(COMPRESSED_ALONE, format=lzma.FORMAT_XZ)
        # with self.assertRaises(LZMAError):
            # lzma.decompress(COMPRESSED_XZ, format=lzma.FORMAT_ALONE)
        # with self.assertRaises(LZMAError):
            # lzma.decompress(COMPRESSED_XZ, format=lzma.FORMAT_RAW,
                            # filters=FILTERS_RAW_1)

    # # Test that compress()->decompress() preserves the input data.

    # def test_roundtrip(self):
        # cdata = lzma.compress(INPUT)
        # ddata = lzma.decompress(cdata)
        # self.assertEqual(ddata, INPUT)

        # cdata = lzma.compress(INPUT, lzma.FORMAT_XZ)
        # ddata = lzma.decompress(cdata)
        # self.assertEqual(ddata, INPUT)

        # cdata = lzma.compress(INPUT, lzma.FORMAT_ALONE)
        # ddata = lzma.decompress(cdata)
        # self.assertEqual(ddata, INPUT)

        # cdata = lzma.compress(INPUT, lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        # ddata = lzma.decompress(cdata, lzma.FORMAT_RAW, filters=FILTERS_RAW_4)
        # self.assertEqual(ddata, INPUT)

    # # Unlike LZMADecompressor, decompress() *does* handle concatenated streams.

    # def test_decompress_multistream(self):
        # ddata = lzma.decompress(COMPRESSED_XZ + COMPRESSED_ALONE)
        # self.assertEqual(ddata, INPUT * 2)

    # # Test robust handling of non-LZMA data following the compressed stream(s).

    # def test_decompress_trailing_junk(self):
        # ddata = lzma.decompress(COMPRESSED_XZ + COMPRESSED_BOGUS)
        # self.assertEqual(ddata, INPUT)

    # def test_decompress_multistream_trailing_junk(self):
        # ddata = lzma.decompress(COMPRESSED_XZ * 3 + COMPRESSED_BOGUS)
        # self.assertEqual(ddata, INPUT * 3)


# class TempFile:
    # """Context manager - creates a file, and deletes it on __exit__."""

    # def __init__(self, filename, data=b""):
        # self.filename = filename
        # self.data = data

    # def __enter__(self):
        # with open(self.filename, "wb") as f:
            # f.write(self.data)

    # def __exit__(self, *args):
        # unlink(self.filename)


# class FileTestCase(unittest.TestCase):

    # def test_init(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # pass
        # with LZMAFile(BytesIO(), "w") as f:
            # pass
        # with LZMAFile(BytesIO(), "x") as f:
            # pass
        # with LZMAFile(BytesIO(), "a") as f:
            # pass

    # def test_init_with_PathLike_filename(self):
        # filename = pathlib.Path(TESTFN)
        # with TempFile(filename, COMPRESSED_XZ):
            # with LZMAFile(filename) as f:
                # self.assertEqual(f.read(), INPUT)
            # with LZMAFile(filename, "a") as f:
                # f.write(INPUT)
            # with LZMAFile(filename) as f:
                # self.assertEqual(f.read(), INPUT * 2)

    # def test_init_with_filename(self):
        # with TempFile(TESTFN, COMPRESSED_XZ):
            # with LZMAFile(TESTFN) as f:
                # pass
            # with LZMAFile(TESTFN, "w") as f:
                # pass
            # with LZMAFile(TESTFN, "a") as f:
                # pass

    # def test_init_mode(self):
        # with TempFile(TESTFN):
            # with LZMAFile(TESTFN, "r"):
                # pass
            # with LZMAFile(TESTFN, "rb"):
                # pass
            # with LZMAFile(TESTFN, "w"):
                # pass
            # with LZMAFile(TESTFN, "wb"):
                # pass
            # with LZMAFile(TESTFN, "a"):
                # pass
            # with LZMAFile(TESTFN, "ab"):
                # pass

    # def test_init_with_x_mode(self):
        # self.addCleanup(unlink, TESTFN)
        # for mode in ("x", "xb"):
            # unlink(TESTFN)
            # with LZMAFile(TESTFN, mode):
                # pass
            # with self.assertRaises(FileExistsError):
                # with LZMAFile(TESTFN, mode):
                    # pass

    # def test_init_bad_mode(self):
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), (3, "x"))
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), "")
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), "xt")
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), "x+")
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), "rx")
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), "wx")
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), "rt")
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), "r+")
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), "wt")
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), "w+")
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), "rw")

    # def test_init_bad_check(self):
        # with self.assertRaises(TypeError):
            # LZMAFile(BytesIO(), "w", check=b"asd")
        # # CHECK_UNKNOWN and anything above CHECK_ID_MAX should be invalid.
        # with self.assertRaises(LZMAError):
            # LZMAFile(BytesIO(), "w", check=lzma.CHECK_UNKNOWN)
        # with self.assertRaises(LZMAError):
            # LZMAFile(BytesIO(), "w", check=lzma.CHECK_ID_MAX + 3)
        # # Cannot specify a check with mode="r".
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), check=lzma.CHECK_NONE)
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), check=lzma.CHECK_CRC32)
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), check=lzma.CHECK_CRC64)
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), check=lzma.CHECK_SHA256)
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), check=lzma.CHECK_UNKNOWN)

    # def test_init_bad_preset(self):
        # with self.assertRaises(TypeError):
            # LZMAFile(BytesIO(), "w", preset=4.39)
        # with self.assertRaises(LZMAError):
            # LZMAFile(BytesIO(), "w", preset=10)
        # with self.assertRaises(LZMAError):
            # LZMAFile(BytesIO(), "w", preset=23)
        # with self.assertRaises(OverflowError):
            # LZMAFile(BytesIO(), "w", preset=-1)
        # with self.assertRaises(OverflowError):
            # LZMAFile(BytesIO(), "w", preset=-7)
        # with self.assertRaises(TypeError):
            # LZMAFile(BytesIO(), "w", preset="foo")
        # # Cannot specify a preset with mode="r".
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(COMPRESSED_XZ), preset=3)

    # def test_init_bad_filter_spec(self):
        # with self.assertRaises(TypeError):
            # LZMAFile(BytesIO(), "w", filters=[b"wobsite"])
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(), "w", filters=[{"xyzzy": 3}])
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(), "w", filters=[{"id": 98765}])
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(), "w",
                     # filters=[{"id": lzma.FILTER_LZMA2, "foo": 0}])
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(), "w",
                     # filters=[{"id": lzma.FILTER_DELTA, "foo": 0}])
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(), "w",
                     # filters=[{"id": lzma.FILTER_X86, "foo": 0}])

    # def test_init_with_preset_and_filters(self):
        # with self.assertRaises(ValueError):
            # LZMAFile(BytesIO(), "w", format=lzma.FORMAT_RAW,
                     # preset=6, filters=FILTERS_RAW_1)

    # def test_close(self):
        # with BytesIO(COMPRESSED_XZ) as src:
            # f = LZMAFile(src)
            # f.close()
            # # LZMAFile.close() should not close the underlying file object.
            # self.assertFalse(src.closed)
            # # Try closing an already-closed LZMAFile.
            # f.close()
            # self.assertFalse(src.closed)

        # # Test with a real file on disk, opened directly by LZMAFile.
        # with TempFile(TESTFN, COMPRESSED_XZ):
            # f = LZMAFile(TESTFN)
            # fp = f._fp
            # f.close()
            # # Here, LZMAFile.close() *should* close the underlying file object.
            # self.assertTrue(fp.closed)
            # # Try closing an already-closed LZMAFile.
            # f.close()

    # def test_closed(self):
        # f = LZMAFile(BytesIO(COMPRESSED_XZ))
        # try:
            # self.assertFalse(f.closed)
            # f.read()
            # self.assertFalse(f.closed)
        # finally:
            # f.close()
        # self.assertTrue(f.closed)

        # f = LZMAFile(BytesIO(), "w")
        # try:
            # self.assertFalse(f.closed)
        # finally:
            # f.close()
        # self.assertTrue(f.closed)

    # def test_fileno(self):
        # f = LZMAFile(BytesIO(COMPRESSED_XZ))
        # try:
            # self.assertRaises(UnsupportedOperation, f.fileno)
        # finally:
            # f.close()
        # self.assertRaises(ValueError, f.fileno)
        # with TempFile(TESTFN, COMPRESSED_XZ):
            # f = LZMAFile(TESTFN)
            # try:
                # self.assertEqual(f.fileno(), f._fp.fileno())
                # self.assertIsInstance(f.fileno(), int)
            # finally:
                # f.close()
        # self.assertRaises(ValueError, f.fileno)

    # def test_seekable(self):
        # f = LZMAFile(BytesIO(COMPRESSED_XZ))
        # try:
            # self.assertTrue(f.seekable())
            # f.read()
            # self.assertTrue(f.seekable())
        # finally:
            # f.close()
        # self.assertRaises(ValueError, f.seekable)

        # f = LZMAFile(BytesIO(), "w")
        # try:
            # self.assertFalse(f.seekable())
        # finally:
            # f.close()
        # self.assertRaises(ValueError, f.seekable)

        # src = BytesIO(COMPRESSED_XZ)
        # src.seekable = lambda: False
        # f = LZMAFile(src)
        # try:
            # self.assertFalse(f.seekable())
        # finally:
            # f.close()
        # self.assertRaises(ValueError, f.seekable)

    # def test_readable(self):
        # f = LZMAFile(BytesIO(COMPRESSED_XZ))
        # try:
            # self.assertTrue(f.readable())
            # f.read()
            # self.assertTrue(f.readable())
        # finally:
            # f.close()
        # self.assertRaises(ValueError, f.readable)

        # f = LZMAFile(BytesIO(), "w")
        # try:
            # self.assertFalse(f.readable())
        # finally:
            # f.close()
        # self.assertRaises(ValueError, f.readable)

    # def test_writable(self):
        # f = LZMAFile(BytesIO(COMPRESSED_XZ))
        # try:
            # self.assertFalse(f.writable())
            # f.read()
            # self.assertFalse(f.writable())
        # finally:
            # f.close()
        # self.assertRaises(ValueError, f.writable)

        # f = LZMAFile(BytesIO(), "w")
        # try:
            # self.assertTrue(f.writable())
        # finally:
            # f.close()
        # self.assertRaises(ValueError, f.writable)

    # def test_read(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # self.assertEqual(f.read(), INPUT)
            # self.assertEqual(f.read(), b"")
        # with LZMAFile(BytesIO(COMPRESSED_ALONE)) as f:
            # self.assertEqual(f.read(), INPUT)
        # with LZMAFile(BytesIO(COMPRESSED_XZ), format=lzma.FORMAT_XZ) as f:
            # self.assertEqual(f.read(), INPUT)
            # self.assertEqual(f.read(), b"")
        # with LZMAFile(BytesIO(COMPRESSED_ALONE), format=lzma.FORMAT_ALONE) as f:
            # self.assertEqual(f.read(), INPUT)
            # self.assertEqual(f.read(), b"")
        # with LZMAFile(BytesIO(COMPRESSED_RAW_1),
                      # format=lzma.FORMAT_RAW, filters=FILTERS_RAW_1) as f:
            # self.assertEqual(f.read(), INPUT)
            # self.assertEqual(f.read(), b"")
        # with LZMAFile(BytesIO(COMPRESSED_RAW_2),
                      # format=lzma.FORMAT_RAW, filters=FILTERS_RAW_2) as f:
            # self.assertEqual(f.read(), INPUT)
            # self.assertEqual(f.read(), b"")
        # with LZMAFile(BytesIO(COMPRESSED_RAW_3),
                      # format=lzma.FORMAT_RAW, filters=FILTERS_RAW_3) as f:
            # self.assertEqual(f.read(), INPUT)
            # self.assertEqual(f.read(), b"")
        # with LZMAFile(BytesIO(COMPRESSED_RAW_4),
                      # format=lzma.FORMAT_RAW, filters=FILTERS_RAW_4) as f:
            # self.assertEqual(f.read(), INPUT)
            # self.assertEqual(f.read(), b"")

    # def test_read_0(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # self.assertEqual(f.read(0), b"")
        # with LZMAFile(BytesIO(COMPRESSED_ALONE)) as f:
            # self.assertEqual(f.read(0), b"")
        # with LZMAFile(BytesIO(COMPRESSED_XZ), format=lzma.FORMAT_XZ) as f:
            # self.assertEqual(f.read(0), b"")
        # with LZMAFile(BytesIO(COMPRESSED_ALONE), format=lzma.FORMAT_ALONE) as f:
            # self.assertEqual(f.read(0), b"")

    # def test_read_10(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # chunks = []
            # while True:
                # result = f.read(10)
                # if not result:
                    # break
                # self.assertLessEqual(len(result), 10)
                # chunks.append(result)
            # self.assertEqual(b"".join(chunks), INPUT)

    # def test_read_multistream(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ * 5)) as f:
            # self.assertEqual(f.read(), INPUT * 5)
        # with LZMAFile(BytesIO(COMPRESSED_XZ + COMPRESSED_ALONE)) as f:
            # self.assertEqual(f.read(), INPUT * 2)
        # with LZMAFile(BytesIO(COMPRESSED_RAW_3 * 4),
                      # format=lzma.FORMAT_RAW, filters=FILTERS_RAW_3) as f:
            # self.assertEqual(f.read(), INPUT * 4)

    # def test_read_multistream_buffer_size_aligned(self):
        # # Test the case where a stream boundary coincides with the end
        # # of the raw read buffer.
        # saved_buffer_size = _compression.BUFFER_SIZE
        # _compression.BUFFER_SIZE = len(COMPRESSED_XZ)
        # try:
            # with LZMAFile(BytesIO(COMPRESSED_XZ *  5)) as f:
                # self.assertEqual(f.read(), INPUT * 5)
        # finally:
            # _compression.BUFFER_SIZE = saved_buffer_size

    # def test_read_trailing_junk(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ + COMPRESSED_BOGUS)) as f:
            # self.assertEqual(f.read(), INPUT)

    # def test_read_multistream_trailing_junk(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ * 5 + COMPRESSED_BOGUS)) as f:
            # self.assertEqual(f.read(), INPUT * 5)

    # def test_read_from_file(self):
        # with TempFile(TESTFN, COMPRESSED_XZ):
            # with LZMAFile(TESTFN) as f:
                # self.assertEqual(f.read(), INPUT)
                # self.assertEqual(f.read(), b"")

    # def test_read_from_file_with_bytes_filename(self):
        # try:
            # bytes_filename = TESTFN.encode("ascii")
        # except UnicodeEncodeError:
            # self.skipTest("Temporary file name needs to be ASCII")
        # with TempFile(TESTFN, COMPRESSED_XZ):
            # with LZMAFile(bytes_filename) as f:
                # self.assertEqual(f.read(), INPUT)
                # self.assertEqual(f.read(), b"")

    # def test_read_incomplete(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ[:128])) as f:
            # self.assertRaises(EOFError, f.read)

    # def test_read_truncated(self):
        # # Drop stream footer: CRC (4 bytes), index size (4 bytes),
        # # flags (2 bytes) and magic number (2 bytes).
        # truncated = COMPRESSED_XZ[:-12]
        # with LZMAFile(BytesIO(truncated)) as f:
            # self.assertRaises(EOFError, f.read)
        # with LZMAFile(BytesIO(truncated)) as f:
            # self.assertEqual(f.read(len(INPUT)), INPUT)
            # self.assertRaises(EOFError, f.read, 1)
        # # Incomplete 12-byte header.
        # for i in range(12):
            # with LZMAFile(BytesIO(truncated[:i])) as f:
                # self.assertRaises(EOFError, f.read, 1)

    # def test_read_bad_args(self):
        # f = LZMAFile(BytesIO(COMPRESSED_XZ))
        # f.close()
        # self.assertRaises(ValueError, f.read)
        # with LZMAFile(BytesIO(), "w") as f:
            # self.assertRaises(ValueError, f.read)
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # self.assertRaises(TypeError, f.read, float())

    # def test_read_bad_data(self):
        # with LZMAFile(BytesIO(COMPRESSED_BOGUS)) as f:
            # self.assertRaises(LZMAError, f.read)

    # def test_read1(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # blocks = []
            # while True:
                # result = f.read1()
                # if not result:
                    # break
                # blocks.append(result)
            # self.assertEqual(b"".join(blocks), INPUT)
            # self.assertEqual(f.read1(), b"")

    # def test_read1_0(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # self.assertEqual(f.read1(0), b"")

    # def test_read1_10(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # blocks = []
            # while True:
                # result = f.read1(10)
                # if not result:
                    # break
                # blocks.append(result)
            # self.assertEqual(b"".join(blocks), INPUT)
            # self.assertEqual(f.read1(), b"")

    # def test_read1_multistream(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ * 5)) as f:
            # blocks = []
            # while True:
                # result = f.read1()
                # if not result:
                    # break
                # blocks.append(result)
            # self.assertEqual(b"".join(blocks), INPUT * 5)
            # self.assertEqual(f.read1(), b"")

    # def test_read1_bad_args(self):
        # f = LZMAFile(BytesIO(COMPRESSED_XZ))
        # f.close()
        # self.assertRaises(ValueError, f.read1)
        # with LZMAFile(BytesIO(), "w") as f:
            # self.assertRaises(ValueError, f.read1)
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # self.assertRaises(TypeError, f.read1, None)

    # def test_peek(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # result = f.peek()
            # self.assertGreater(len(result), 0)
            # self.assertTrue(INPUT.startswith(result))
            # self.assertEqual(f.read(), INPUT)
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # result = f.peek(10)
            # self.assertGreater(len(result), 0)
            # self.assertTrue(INPUT.startswith(result))
            # self.assertEqual(f.read(), INPUT)

    # def test_peek_bad_args(self):
        # with LZMAFile(BytesIO(), "w") as f:
            # self.assertRaises(ValueError, f.peek)

    # def test_iterator(self):
        # with BytesIO(INPUT) as f:
            # lines = f.readlines()
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # self.assertListEqual(list(iter(f)), lines)
        # with LZMAFile(BytesIO(COMPRESSED_ALONE)) as f:
            # self.assertListEqual(list(iter(f)), lines)
        # with LZMAFile(BytesIO(COMPRESSED_XZ), format=lzma.FORMAT_XZ) as f:
            # self.assertListEqual(list(iter(f)), lines)
        # with LZMAFile(BytesIO(COMPRESSED_ALONE), format=lzma.FORMAT_ALONE) as f:
            # self.assertListEqual(list(iter(f)), lines)
        # with LZMAFile(BytesIO(COMPRESSED_RAW_2),
                      # format=lzma.FORMAT_RAW, filters=FILTERS_RAW_2) as f:
            # self.assertListEqual(list(iter(f)), lines)

    # def test_readline(self):
        # with BytesIO(INPUT) as f:
            # lines = f.readlines()
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # for line in lines:
                # self.assertEqual(f.readline(), line)

    # def test_readlines(self):
        # with BytesIO(INPUT) as f:
            # lines = f.readlines()
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # self.assertListEqual(f.readlines(), lines)

    # def test_decompress_limited(self):
        # """Decompressed data buffering should be limited"""
        # bomb = lzma.compress(b'\0' * int(2e6), preset=6)
        # self.assertLess(len(bomb), _compression.BUFFER_SIZE)

        # decomp = LZMAFile(BytesIO(bomb))
        # self.assertEqual(decomp.read(1), b'\0')
        # max_decomp = 1 + DEFAULT_BUFFER_SIZE
        # self.assertLessEqual(decomp._buffer.raw.tell(), max_decomp,
            # "Excessive amount of data was decompressed")

    # def test_write(self):
        # with BytesIO() as dst:
            # with LZMAFile(dst, "w") as f:
                # f.write(INPUT)
            # expected = lzma.compress(INPUT)
            # self.assertEqual(dst.getvalue(), expected)
        # with BytesIO() as dst:
            # with LZMAFile(dst, "w", format=lzma.FORMAT_XZ) as f:
                # f.write(INPUT)
            # expected = lzma.compress(INPUT, format=lzma.FORMAT_XZ)
            # self.assertEqual(dst.getvalue(), expected)
        # with BytesIO() as dst:
            # with LZMAFile(dst, "w", format=lzma.FORMAT_ALONE) as f:
                # f.write(INPUT)
            # expected = lzma.compress(INPUT, format=lzma.FORMAT_ALONE)
            # self.assertEqual(dst.getvalue(), expected)
        # with BytesIO() as dst:
            # with LZMAFile(dst, "w", format=lzma.FORMAT_RAW,
                          # filters=FILTERS_RAW_2) as f:
                # f.write(INPUT)
            # expected = lzma.compress(INPUT, format=lzma.FORMAT_RAW,
                                     # filters=FILTERS_RAW_2)
            # self.assertEqual(dst.getvalue(), expected)

    # def test_write_10(self):
        # with BytesIO() as dst:
            # with LZMAFile(dst, "w") as f:
                # for start in range(0, len(INPUT), 10):
                    # f.write(INPUT[start:start+10])
            # expected = lzma.compress(INPUT)
            # self.assertEqual(dst.getvalue(), expected)

    # def test_write_append(self):
        # part1 = INPUT[:1024]
        # part2 = INPUT[1024:1536]
        # part3 = INPUT[1536:]
        # expected = b"".join(lzma.compress(x) for x in (part1, part2, part3))
        # with BytesIO() as dst:
            # with LZMAFile(dst, "w") as f:
                # f.write(part1)
            # with LZMAFile(dst, "a") as f:
                # f.write(part2)
            # with LZMAFile(dst, "a") as f:
                # f.write(part3)
            # self.assertEqual(dst.getvalue(), expected)

    # def test_write_to_file(self):
        # try:
            # with LZMAFile(TESTFN, "w") as f:
                # f.write(INPUT)
            # expected = lzma.compress(INPUT)
            # with open(TESTFN, "rb") as f:
                # self.assertEqual(f.read(), expected)
        # finally:
            # unlink(TESTFN)

    # def test_write_to_file_with_bytes_filename(self):
        # try:
            # bytes_filename = TESTFN.encode("ascii")
        # except UnicodeEncodeError:
            # self.skipTest("Temporary file name needs to be ASCII")
        # try:
            # with LZMAFile(bytes_filename, "w") as f:
                # f.write(INPUT)
            # expected = lzma.compress(INPUT)
            # with open(TESTFN, "rb") as f:
                # self.assertEqual(f.read(), expected)
        # finally:
            # unlink(TESTFN)

    # def test_write_append_to_file(self):
        # part1 = INPUT[:1024]
        # part2 = INPUT[1024:1536]
        # part3 = INPUT[1536:]
        # expected = b"".join(lzma.compress(x) for x in (part1, part2, part3))
        # try:
            # with LZMAFile(TESTFN, "w") as f:
                # f.write(part1)
            # with LZMAFile(TESTFN, "a") as f:
                # f.write(part2)
            # with LZMAFile(TESTFN, "a") as f:
                # f.write(part3)
            # with open(TESTFN, "rb") as f:
                # self.assertEqual(f.read(), expected)
        # finally:
            # unlink(TESTFN)

    # def test_write_bad_args(self):
        # f = LZMAFile(BytesIO(), "w")
        # f.close()
        # self.assertRaises(ValueError, f.write, b"foo")
        # with LZMAFile(BytesIO(COMPRESSED_XZ), "r") as f:
            # self.assertRaises(ValueError, f.write, b"bar")
        # with LZMAFile(BytesIO(), "w") as f:
            # self.assertRaises(TypeError, f.write, None)
            # self.assertRaises(TypeError, f.write, "text")
            # self.assertRaises(TypeError, f.write, 789)

    # def test_writelines(self):
        # with BytesIO(INPUT) as f:
            # lines = f.readlines()
        # with BytesIO() as dst:
            # with LZMAFile(dst, "w") as f:
                # f.writelines(lines)
            # expected = lzma.compress(INPUT)
            # self.assertEqual(dst.getvalue(), expected)

    # def test_seek_forward(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # f.seek(555)
            # self.assertEqual(f.read(), INPUT[555:])

    # def test_seek_forward_across_streams(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ * 2)) as f:
            # f.seek(len(INPUT) + 123)
            # self.assertEqual(f.read(), INPUT[123:])

    # def test_seek_forward_relative_to_current(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # f.read(100)
            # f.seek(1236, 1)
            # self.assertEqual(f.read(), INPUT[1336:])

    # def test_seek_forward_relative_to_end(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # f.seek(-555, 2)
            # self.assertEqual(f.read(), INPUT[-555:])

    # def test_seek_backward(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # f.read(1001)
            # f.seek(211)
            # self.assertEqual(f.read(), INPUT[211:])

    # def test_seek_backward_across_streams(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ * 2)) as f:
            # f.read(len(INPUT) + 333)
            # f.seek(737)
            # self.assertEqual(f.read(), INPUT[737:] + INPUT)

    # def test_seek_backward_relative_to_end(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # f.seek(-150, 2)
            # self.assertEqual(f.read(), INPUT[-150:])

    # def test_seek_past_end(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # f.seek(len(INPUT) + 9001)
            # self.assertEqual(f.tell(), len(INPUT))
            # self.assertEqual(f.read(), b"")

    # def test_seek_past_start(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # f.seek(-88)
            # self.assertEqual(f.tell(), 0)
            # self.assertEqual(f.read(), INPUT)

    # def test_seek_bad_args(self):
        # f = LZMAFile(BytesIO(COMPRESSED_XZ))
        # f.close()
        # self.assertRaises(ValueError, f.seek, 0)
        # with LZMAFile(BytesIO(), "w") as f:
            # self.assertRaises(ValueError, f.seek, 0)
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # self.assertRaises(ValueError, f.seek, 0, 3)
            # # io.BufferedReader raises TypeError instead of ValueError
            # self.assertRaises((TypeError, ValueError), f.seek, 9, ())
            # self.assertRaises(TypeError, f.seek, None)
            # self.assertRaises(TypeError, f.seek, b"derp")

    # def test_tell(self):
        # with LZMAFile(BytesIO(COMPRESSED_XZ)) as f:
            # pos = 0
            # while True:
                # self.assertEqual(f.tell(), pos)
                # result = f.read(183)
                # if not result:
                    # break
                # pos += len(result)
            # self.assertEqual(f.tell(), len(INPUT))
        # with LZMAFile(BytesIO(), "w") as f:
            # for pos in range(0, len(INPUT), 144):
                # self.assertEqual(f.tell(), pos)
                # f.write(INPUT[pos:pos+144])
            # self.assertEqual(f.tell(), len(INPUT))

    # def test_tell_bad_args(self):
        # f = LZMAFile(BytesIO(COMPRESSED_XZ))
        # f.close()
        # self.assertRaises(ValueError, f.tell)

    # def test_issue21872(self):
        # # sometimes decompress data incompletely

        # # ---------------------
        # # when max_length == -1
        # # ---------------------
        # d1 = LZMADecompressor()
        # entire = d1.decompress(ISSUE_21872_DAT, max_length=-1)
        # self.assertEqual(len(entire), 13160)
        # self.assertTrue(d1.eof)

        # # ---------------------
        # # when max_length > 0
        # # ---------------------
        # d2 = LZMADecompressor()

        # # When this value of max_length is used, the input and output
        # # buffers are exhausted at the same time, and lzs's internal
        # # state still have 11 bytes can be output.
        # out1 = d2.decompress(ISSUE_21872_DAT, max_length=13149)
        # self.assertFalse(d2.needs_input) # ensure needs_input mechanism works
        # self.assertFalse(d2.eof)

        # # simulate needs_input mechanism
        # # output internal state's 11 bytes
        # out2 = d2.decompress(b'')
        # self.assertEqual(len(out2), 11)
        # self.assertTrue(d2.eof)
        # self.assertEqual(out1 + out2, entire)


# class OpenTestCase(unittest.TestCase):

    # def test_binary_modes(self):
        # with lzma.open(BytesIO(COMPRESSED_XZ), "rb") as f:
            # self.assertEqual(f.read(), INPUT)
        # with BytesIO() as bio:
            # with lzma.open(bio, "wb") as f:
                # f.write(INPUT)
            # file_data = lzma.decompress(bio.getvalue())
            # self.assertEqual(file_data, INPUT)
            # with lzma.open(bio, "ab") as f:
                # f.write(INPUT)
            # file_data = lzma.decompress(bio.getvalue())
            # self.assertEqual(file_data, INPUT * 2)

    # def test_text_modes(self):
        # uncompressed = INPUT.decode("ascii")
        # uncompressed_raw = uncompressed.replace("\n", os.linesep)
        # with lzma.open(BytesIO(COMPRESSED_XZ), "rt") as f:
            # self.assertEqual(f.read(), uncompressed)
        # with BytesIO() as bio:
            # with lzma.open(bio, "wt") as f:
                # f.write(uncompressed)
            # file_data = lzma.decompress(bio.getvalue()).decode("ascii")
            # self.assertEqual(file_data, uncompressed_raw)
            # with lzma.open(bio, "at") as f:
                # f.write(uncompressed)
            # file_data = lzma.decompress(bio.getvalue()).decode("ascii")
            # self.assertEqual(file_data, uncompressed_raw * 2)

    # def test_filename(self):
        # with TempFile(TESTFN):
            # with lzma.open(TESTFN, "wb") as f:
                # f.write(INPUT)
            # with open(TESTFN, "rb") as f:
                # file_data = lzma.decompress(f.read())
                # self.assertEqual(file_data, INPUT)
            # with lzma.open(TESTFN, "rb") as f:
                # self.assertEqual(f.read(), INPUT)
            # with lzma.open(TESTFN, "ab") as f:
                # f.write(INPUT)
            # with lzma.open(TESTFN, "rb") as f:
                # self.assertEqual(f.read(), INPUT * 2)

    # def test_with_pathlike_filename(self):
        # filename = pathlib.Path(TESTFN)
        # with TempFile(filename):
            # with lzma.open(filename, "wb") as f:
                # f.write(INPUT)
            # with open(filename, "rb") as f:
                # file_data = lzma.decompress(f.read())
                # self.assertEqual(file_data, INPUT)
            # with lzma.open(filename, "rb") as f:
                # self.assertEqual(f.read(), INPUT)

    # def test_bad_params(self):
        # # Test invalid parameter combinations.
        # with self.assertRaises(ValueError):
            # lzma.open(TESTFN, "")
        # with self.assertRaises(ValueError):
            # lzma.open(TESTFN, "rbt")
        # with self.assertRaises(ValueError):
            # lzma.open(TESTFN, "rb", encoding="utf-8")
        # with self.assertRaises(ValueError):
            # lzma.open(TESTFN, "rb", errors="ignore")
        # with self.assertRaises(ValueError):
            # lzma.open(TESTFN, "rb", newline="\n")

    # def test_format_and_filters(self):
        # # Test non-default format and filter chain.
        # options = {"format": lzma.FORMAT_RAW, "filters": FILTERS_RAW_1}
        # with lzma.open(BytesIO(COMPRESSED_RAW_1), "rb", **options) as f:
            # self.assertEqual(f.read(), INPUT)
        # with BytesIO() as bio:
            # with lzma.open(bio, "wb", **options) as f:
                # f.write(INPUT)
            # file_data = lzma.decompress(bio.getvalue(), **options)
            # self.assertEqual(file_data, INPUT)

    # def test_encoding(self):
        # # Test non-default encoding.
        # uncompressed = INPUT.decode("ascii")
        # uncompressed_raw = uncompressed.replace("\n", os.linesep)
        # with BytesIO() as bio:
            # with lzma.open(bio, "wt", encoding="utf-16-le") as f:
                # f.write(uncompressed)
            # file_data = lzma.decompress(bio.getvalue()).decode("utf-16-le")
            # self.assertEqual(file_data, uncompressed_raw)
            # bio.seek(0)
            # with lzma.open(bio, "rt", encoding="utf-16-le") as f:
                # self.assertEqual(f.read(), uncompressed)

    # def test_encoding_error_handler(self):
        # # Test with non-default encoding error handler.
        # with BytesIO(lzma.compress(b"foo\xffbar")) as bio:
            # with lzma.open(bio, "rt", encoding="ascii", errors="ignore") as f:
                # self.assertEqual(f.read(), "foobar")

    # def test_newline(self):
        # # Test with explicit newline (universal newline mode disabled).
        # text = INPUT.decode("ascii")
        # with BytesIO() as bio:
            # with lzma.open(bio, "wt", newline="\n") as f:
                # f.write(text)
            # bio.seek(0)
            # with lzma.open(bio, "rt", newline="\r") as f:
                # self.assertEqual(f.readlines(), [text])

    # def test_x_mode(self):
        # self.addCleanup(unlink, TESTFN)
        # for mode in ("x", "xb", "xt"):
            # unlink(TESTFN)
            # with lzma.open(TESTFN, mode):
                # pass
            # with self.assertRaises(FileExistsError):
                # with lzma.open(TESTFN, mode):
                    # pass


# class MiscellaneousTestCase(unittest.TestCase):

    # def test_is_check_supported(self):
        # # CHECK_NONE and CHECK_CRC32 should always be supported,
        # # regardless of the options liblzma was compiled with.
        # self.assertTrue(lzma.is_check_supported(lzma.CHECK_NONE))
        # self.assertTrue(lzma.is_check_supported(lzma.CHECK_CRC32))

        # # The .xz format spec cannot store check IDs above this value.
        # self.assertFalse(lzma.is_check_supported(lzma.CHECK_ID_MAX + 1))

        # # This value should not be a valid check ID.
        # self.assertFalse(lzma.is_check_supported(lzma.CHECK_UNKNOWN))

    # def test__encode_filter_properties(self):
        # with self.assertRaises(TypeError):
            # lzma._encode_filter_properties(b"not a dict")
        # with self.assertRaises(ValueError):
            # lzma._encode_filter_properties({"id": 0x100})
        # with self.assertRaises(ValueError):
            # lzma._encode_filter_properties({"id": lzma.FILTER_LZMA2, "junk": 12})
        # with self.assertRaises(lzma.LZMAError):
            # lzma._encode_filter_properties({"id": lzma.FILTER_DELTA,
                                           # "dist": 9001})

        # # Test with parameters used by zipfile module.
        # props = lzma._encode_filter_properties({
                # "id": lzma.FILTER_LZMA1,
                # "pb": 2,
                # "lp": 0,
                # "lc": 3,
                # "dict_size": 8 << 20,
            # })
        # self.assertEqual(props, b"]\x00\x00\x80\x00")

    # def test__decode_filter_properties(self):
        # with self.assertRaises(TypeError):
            # lzma._decode_filter_properties(lzma.FILTER_X86, {"should be": bytes})
        # with self.assertRaises(lzma.LZMAError):
            # lzma._decode_filter_properties(lzma.FILTER_DELTA, b"too long")

        # # Test with parameters used by zipfile module.
        # filterspec = lzma._decode_filter_properties(
                # lzma.FILTER_LZMA1, b"]\x00\x00\x80\x00")
        # self.assertEqual(filterspec["id"], lzma.FILTER_LZMA1)
        # self.assertEqual(filterspec["pb"], 2)
        # self.assertEqual(filterspec["lp"], 0)
        # self.assertEqual(filterspec["lc"], 3)
        # self.assertEqual(filterspec["dict_size"], 8 << 20)

    # def test_filter_properties_roundtrip(self):
        # spec1 = lzma._decode_filter_properties(
                # lzma.FILTER_LZMA1, b"]\x00\x00\x80\x00")
        # reencoded = lzma._encode_filter_properties(spec1)
        # spec2 = lzma._decode_filter_properties(lzma.FILTER_LZMA1, reencoded)
        # self.assertEqual(spec1, spec2)


# # Test data:


def test_main():
    run_unittest(
        CompressorDecompressorTestCase,
        # CompressDecompressFunctionTestCase,
        # FileTestCase,
        # OpenTestCase,
        # MiscellaneousTestCase,
    )

if __name__ == "__main__":
    test_main()


#include "GZipCodec.h"
#include "zlib.h"

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#define CHUNK 16384
#define windowBits 15
#define GZIP_ENCODING 16

bool GZipCodec::Compress(const std::string& data, std::string& compressedData, int level)
{
  unsigned char out[CHUNK];
  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  if (deflateInit2(&strm, level, Z_DEFLATED, windowBits | GZIP_ENCODING, 8, Z_DEFAULT_STRATEGY) != Z_OK)
  {
    return false;
  }
  strm.next_in = (unsigned char*)data.c_str();
  strm.avail_in = data.size();
  do {
    int have;
    strm.avail_out = CHUNK;
    strm.next_out = out;
    if (deflate(&strm, Z_FINISH) == Z_STREAM_ERROR)
    {
      return false;
    }
    have = CHUNK - strm.avail_out;
    compressedData.append((char*)out, have);
  } while (strm.avail_out == 0);
  if (deflateEnd(&strm) != Z_OK)
  {
    return false;
  }
  return true;
}

bool GZipCodec::Uncompress(const std::string& compressedData, std::string& data)
{
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char out[CHUNK];

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  if (inflateInit2(&strm, 16 + MAX_WBITS) != Z_OK)
  {
    return false;
  }

  strm.avail_in = compressedData.size();
  strm.next_in = (unsigned char*)compressedData.c_str();
  do {
    strm.avail_out = CHUNK;
    strm.next_out = out;
    ret = inflate(&strm, Z_NO_FLUSH);
    switch (ret) {
    case Z_NEED_DICT:
    case Z_DATA_ERROR:
    case Z_MEM_ERROR:
      inflateEnd(&strm);
      return false;
    }
    have = CHUNK - strm.avail_out;
    data.append((char*)out, have);
  } while (strm.avail_out == 0);

  if (inflateEnd(&strm) != Z_OK) {
    return false;
  }
  
  return true;
}

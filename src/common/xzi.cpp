
// 2019 Team AobaZero
// This source code is in the public domain.
#include "err.hpp"
#include "iobase.hpp"
#include "xzi.hpp"
#include <algorithm>
#include <fstream>
#include <cassert>
#include <cstring>
using std::ifstream;
using std::min;
using std::ofstream;
using std::ios;
using ErrAux::die;

static bool is_delim(int ch, const char *delims) noexcept {
  for (; *delims != '\0'; ++delims)
    if (ch == *delims) return true;
  return false; }

uint64_t XZAux::crc64(const FName & fname) noexcept {
  assert(fname.ok());
  uint64_t crc64_value = 0;
  ifstream ifs(fname.get_fname(), ios::binary);
  if (!ifs) die(ERR_INT("cannot open %s", fname.get_fname()));
  do {
    char buf[BUFSIZ];
    size_t size = ifs.read(buf, sizeof(buf)).gcount();
    crc64_value = XZAux::crc64(buf, size, crc64_value);
  } while(!ifs.eof());

  return crc64_value; }

uint64_t XZAux::crc64(const char *p, size_t len, uint64_t crc64) noexcept {
  return lzma_crc64(reinterpret_cast<const uint8_t *>(p), len, crc64); }

uint64_t XZAux::crc64(const char *p, uint64_t crc64) noexcept {
  assert(p);
  return lzma_crc64(reinterpret_cast<const uint8_t *>(p), strlen(p), crc64); }

/*
void XZBase::xzwrite(int *fd, size_t len) const noexcept {
  ssize_t ret = write(*fd, _outbuf, len);
  if (ret < static_cast<ssize_t>(len)) die(ERR_CLL("write")); } */

void XZBase::xzwrite(ofstream *pofs, size_t len) const noexcept {
  pofs->write(reinterpret_cast<const char *>(_outbuf), len); }

void XZBase::xzwrite(PtrLen<char> *out, size_t len) const noexcept {
  memcpy(out->p + out->len, _outbuf, len);
  out->len += len; }

void XZBase::xzwrite(DevNul *, size_t) const noexcept {}

size_t XZBase::xzread(PtrLen<const char> *pl) noexcept {
  assert(pl->ok());
  size_t len(pl->len);
  len = min(sizeof(_inbuf), len);
  memcpy(_inbuf, pl->p, len);
  pl->p   += len;
  pl->len -= len;
  return len; }

size_t XZBase::xzread(ifstream *pifs) noexcept {
  pifs->read(reinterpret_cast<char *>(_inbuf), sizeof(_inbuf));
  return pifs->gcount(); }

/*
size_t XZBase::xzread(int *fd) noexcept {
  ssize_t ret(read(*fd, _inbuf, sizeof(_inbuf)));
  if (ret < 0) die(ERR_CLL("read"));
  return ret; } */

template <typename T_IN, typename T_OUT>
bool XZEncode<T_IN, T_OUT>::encode(const lzma_action &a, lzma_ret &ret)
  noexcept {

  ret = lzma_code(&_strm, a);
  if (_strm.avail_out == 0 || ret == LZMA_STREAM_END) {
    size_t len = sizeof(_outbuf) - _strm.avail_out;
    if (_maxlen_out_tot < _len_out_tot + len) return false;
    xzwrite(_out, len);
    _len_out_tot    += len;
    _strm.next_out   = _outbuf;
    _strm.avail_out  = sizeof(_outbuf); }
  assert(a != LZMA_RUN || ret != LZMA_STREAM_END);
  if (ret == LZMA_STREAM_END || ret == LZMA_OK) return true;
  
  const char *msg;
  switch (ret) {
  case LZMA_MEM_ERROR:  msg = "alloc"; break;
  case LZMA_DATA_ERROR: msg = "size";  break;
  default: msg = "class XZEncode";     break; }
  
  die(ERR_INT("XZEncode::encode() failed: bad %s", msg));
  return false; }

template <typename T_IN, typename T_OUT>
void XZEncode<T_IN, T_OUT>::start(T_OUT *out, size_t maxlen_out_tot,
				  uint32_t level, bool bExt) noexcept {
  assert(out);
  assert(0 <= level && level <= 9);
  
  if (bExt) level |= LZMA_PRESET_EXTREME;
  
  const char *msg;
  switch (lzma_easy_encoder(&_strm, level, LZMA_CHECK_CRC64)) {
  case LZMA_OK:
    _out            = out;
    _len_out_tot    = 0;
    _maxlen_out_tot = maxlen_out_tot;
    _strm.next_in   = _inbuf;
    _strm.avail_in  = 0;
    _strm.next_out  = _outbuf;
    _strm.avail_out = sizeof(_outbuf);
    return;
  case LZMA_MEM_ERROR:         msg = "allocation";      break;
  case LZMA_OPTIONS_ERROR:     msg = "presetd";         break;
  case LZMA_UNSUPPORTED_CHECK: msg = "integrity check"; break;
  default:                     msg = "XZEncode";        break;
  }

  die(ERR_INT("XZEncode::start() failed: bad %s", msg));
}

template <typename T_IN, typename T_OUT>
bool XZEncode<T_IN, T_OUT>::append(T_IN *in) noexcept {
  assert(in);
  lzma_ret ret;
  while (true) {
    if (_strm.avail_in == 0) {
      size_t len(xzread(in));
      if (len == 0) return true;

      _strm.next_in  = _inbuf;
      _strm.avail_in = len;
    }
    
    if (!encode(LZMA_RUN, ret)) return false;
  }
}

template <typename T_IN, typename T_OUT>
bool XZEncode<T_IN, T_OUT>::end() noexcept {
  lzma_ret ret;

  do { if (!encode(LZMA_FINISH, ret)) return false; }
  while (ret != LZMA_STREAM_END);
  return true;
}

template <typename T_IN, typename T_OUT>
void XZDecode<T_IN, T_OUT>::init() noexcept {
  const char *msg;
  
  _len_out_tot = 0;
  _crc64       = 0;
  switch (lzma_stream_decoder(&_strm, UINT64_MAX, LZMA_CONCATENATED)) {
  case LZMA_OK:
    _strm.next_in   = _inbuf;
    _strm.avail_in  = 0;
    _strm.next_out  = _outbuf;
    _strm.avail_out = sizeof(_outbuf);
    return;
  case LZMA_MEM_ERROR:     msg = "allocation"; break;
  case LZMA_OPTIONS_ERROR: msg = "flags";      break;
  default:                 msg = "XZDecode";   break;
  }

  die(ERR_INT("XZDecode::init() failed: bad %s", msg)); }

template <typename T_IN, typename T_OUT>
bool XZDecode<T_IN, T_OUT>::decode(T_IN *in, T_OUT *out, size_t len_limit)
  noexcept {
  assert(in);
  assert(out);
  lzma_action action(LZMA_RUN);
  lzma_ret ret;
  size_t len, len_out_tot(0);
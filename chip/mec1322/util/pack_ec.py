#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# A script to pack EC binary into SPI flash image for MEC1322
# Based on MEC1322_ROM_Doc_Rev0.5.pdf.

import argparse
import hashlib
import os
import struct
import subprocess
import tempfile

LOAD_ADDR = 0x100000
HEADER_SIZE = 0x140
SPI_CLOCK_LIST = [48, 24, 12, 8]
SPI_READ_CMD_LIST = [0x3, 0xb, 0x3b]

CRC_TABLE = [0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
             0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d]

def Crc8(crc, data):
  """Update CRC8 value."""
  data_bytes = map(lambda b: ord(b) if isinstance(b, str) else b, data)
  for v in data_bytes:
    crc = ((crc << 4) & 0xff) ^ (CRC_TABLE[(crc >> 4) ^ (v >> 4)]);
    crc = ((crc << 4) & 0xff) ^ (CRC_TABLE[(crc >> 4) ^ (v & 0xf)]);
  return crc ^ 0x55

def GetEntryPoint(payload_file):
  """Read entry point from payload EC image."""
  with open(payload_file, 'rb') as f:
    f.seek(4)
    s = f.read(4)
  return struct.unpack('<I', s)[0]

def GetPayload(payload_file):
  """Read payload and pad it to 64-byte aligned."""
  with open(payload_file, 'rb') as f:
    payload = bytearray(f.read())
  rem_len = len(payload) % 64
  if rem_len:
    payload += '\0' * (64 - rem_len)
  return payload

def GetPublicKey(pem_file):
  """Extract public exponent and modulus from PEM file."""
  s = subprocess.check_output(['openssl', 'rsa', '-in', pem_file,
                               '-text', '-noout'])
  modulus_raw = []
  in_modulus = False
  for line in s.split('\n'):
    if line.startswith('modulus'):
      in_modulus = True
    elif not line.startswith(' '):
      in_modulus = False
    elif in_modulus:
      modulus_raw.extend(line.strip().strip(':').split(':'))
    if line.startswith('publicExponent'):
      exp = int(line.split(' ')[1], 10)
  modulus_raw.reverse()
  modulus = bytearray(''.join(map(lambda x: chr(int(x, 16)),
                                  modulus_raw[0:256])))
  return struct.pack('<Q', exp), modulus

def GetSpiClockParameter(args):
  assert args.spi_clock in SPI_CLOCK_LIST, \
         "Unsupported SPI clock speed %d MHz" % args.spi_clock
  return SPI_CLOCK_LIST.index(args.spi_clock)

def GetSpiReadCmdParameter(args):
  assert args.spi_read_cmd in SPI_READ_CMD_LIST, \
         "Unsupported SPI read command 0x%x" % args.spi_read_cmd
  return SPI_READ_CMD_LIST.index(args.spi_read_cmd)

def PadZeroTo(data, size):
  data.extend('\0' * (size - len(data)))

def BuildHeader(args, payload_len):
  # Identifier and header version
  header = bytearray(['C', 'S', 'M', 'S', '\0'])

  PadZeroTo(header, 0x6)
  header.append(GetSpiClockParameter(args))
  header.append(GetSpiReadCmdParameter(args))

  header.extend(struct.pack('<I', LOAD_ADDR))
  header.extend(struct.pack('<I', GetEntryPoint(args.input)))
  header.append((payload_len >> 6) & 0xff)
  header.append((payload_len >> 14) & 0xff)
  PadZeroTo(header, 0x14)
  header.extend(struct.pack('<I', args.payload_offset))

  exp, modulus = GetPublicKey(args.payload_key)
  PadZeroTo(header, 0x20)
  header.extend(exp)
  PadZeroTo(header, 0x30)
  header.extend(modulus)
  PadZeroTo(header, HEADER_SIZE)

  return header

def SignByteArray(data, pem_file):
  hash_file = tempfile.mkstemp()[1]
  sign_file = tempfile.mkstemp()[1]
  with open(hash_file, 'wb') as f:
    hasher = hashlib.sha256()
    hasher.update(data)
    f.write(hasher.digest())
  subprocess.check_call(['openssl', 'rsautl', '-sign', '-inkey', pem_file,
                         '-keyform', 'PEM', '-in', hash_file,
                         '-out', sign_file])
  with open(sign_file, 'rb') as f:
    signed = list(f.read())
    signed.reverse()
    return bytearray(''.join(signed))

def BuildTag(args):
  tag = bytearray([(args.header_loc >> 8) & 0xff,
                   (args.header_loc >> 16) & 0xff,
                   (args.header_loc >> 24) & 0xff])
  if args.chip_select != 0:
    tag[2] |= 0x80
  tag.append(Crc8(0, tag))
  return tag


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("-i", "--input",
                      help="EC binary to pack, usually ec.bin or ec.RO.flat.",
                      metavar="EC_BIN", default="ec.bin")
  parser.add_argument("-o", "--output",
                      help="Output flash binary file",
                      metavar="EC_SPI_FLASH", default="ec.packed.bin")
  parser.add_argument("--header_key",
                      help="PEM key file for signing header",
                      default="rsakey_sign_header.pem")
  parser.add_argument("--payload_key",
                      help="PEM key file for signing payload",
                      default="rsakey_sign_payload.pem")
  parser.add_argument("-s", "--spi_size", type=int,
                      help="Size of the SPI flash in KB",
                      default=4096)
  parser.add_argument("-l", "--header_loc", type=int,
                      help="Location of header in SPI flash",
                      default=0)
  parser.add_argument("-p", "--payload_offset", type=int,
                      help="The offset of payload from the header",
                      default=0x240)
  parser.add_argument("-c", "--chip_select", type=int,
                      help="Chip select signal to use, either 0 or 1.",
                      default=0)
  parser.add_argument("--spi_clock", type=int,
                      help="SPI clock speed. 8, 12, 24, or 48 MHz.",
                      default=24)
  parser.add_argument("--spi_read_cmd", type=int,
                      help="SPI read command. 0x3, 0xB, or 0x3B.",
                      default=0xb)
  args = parser.parse_args()

  spi_size = args.spi_size * 1024
  spi_list = []

  payload = GetPayload(args.input)
  payload_len = len(payload)
  payload_signature = SignByteArray(payload, args.payload_key)
  header = BuildHeader(args, payload_len)
  header_signature = SignByteArray(header, args.header_key)
  tag = BuildTag(args)

  spi_list.append((args.header_loc, header))
  spi_list.append((args.header_loc + HEADER_SIZE, header_signature))
  spi_list.append((args.header_loc + args.payload_offset, payload))
  spi_list.append((args.header_loc + args.payload_offset + payload_len,
                   payload_signature))
  spi_list.append((spi_size - 256, tag))

  spi_list = sorted(spi_list)

  with open(args.output, 'wb') as f:
    addr = 0
    for s in spi_list:
      assert addr <= s[0]
      if addr < s[0]:
        f.write('\xff' * (s[0] - addr))
        addr = s[0]
      f.write(s[1])
      addr += len(s[1])
    if addr < spi_size:
      f.write('\xff' * (spi_size - addr))

if __name__ == '__main__':
  main()

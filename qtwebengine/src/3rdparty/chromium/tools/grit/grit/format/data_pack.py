#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Support for formatting a data pack file used for platform agnostic resource
files.
"""

from __future__ import print_function

import collections
import os
import struct
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))

from grit import util
from grit.node import include
from grit.node import message
from grit.node import structure


PACK_FILE_VERSION = 5
BINARY, UTF8, UTF16 = range(3)


class WrongFileVersion(Exception):
  pass


class CorruptDataPack(Exception):
  pass


class DataPackSizes(object):
  def __init__(self, header, id_table, alias_table, data):
    self.header = header
    self.id_table = id_table
    self.alias_table = alias_table
    self.data = data

  @property
  def total(self):
    return sum(v for v in self.__dict__.values())

  def __iter__(self):
    yield ('header', self.header)
    yield ('id_table', self.id_table)
    yield ('alias_table', self.alias_table)
    yield ('data', self.data)

  def __eq__(self, other):
    return self.__dict__ == other.__dict__

  def __repr__(self):
    return self.__class__.__name__ + repr(self.__dict__)


class DataPackContents(object):
  def __init__(self, resources, encoding, version, aliases, sizes):
    # Map of resource_id -> str.
    self.resources = resources
    # Encoding (int).
    self.encoding = encoding
    # Version (int).
    self.version = version
    # Map of resource_id->canonical_resource_id
    self.aliases = aliases
    # DataPackSizes instance.
    self.sizes = sizes


def Format(root, lang='en', output_dir='.'):
  """Writes out the data pack file format (platform agnostic resource file)."""
  id_map = root.GetIdMap()
  data = {}
  root.info = []
  for node in root.ActiveDescendants():
    with node:
      if isinstance(node, (include.IncludeNode, message.MessageNode,
                           structure.StructureNode)):
        value = node.GetDataPackValue(lang, UTF8)
        if value is not None:
          resource_id = id_map[node.GetTextualIds()[0]]
          data[resource_id] = value
          root.info.append('{},{},{}'.format(
              node.attrs.get('name'), resource_id, node.source))
  return WriteDataPackToString(data, UTF8)


def ReadDataPack(input_file):
  return ReadDataPackFromString(util.ReadFile(input_file, util.BINARY))


def ReadDataPackFromString(data):
  """Reads a data pack file and returns a dictionary."""
  # Read the header.
  version = struct.unpack('<I', data[:4])[0]
  if version == 4:
    resource_count, encoding = struct.unpack('<IB', data[4:9])
    alias_count = 0
    header_size = 9
  elif version == 5:
    encoding, resource_count, alias_count = struct.unpack('<BxxxHH', data[4:12])
    header_size = 12
  else:
    raise WrongFileVersion('Found version: ' + str(version))

  resources = {}
  kIndexEntrySize = 2 + 4  # Each entry is a uint16 and a uint32.
  def entry_at_index(idx):
    offset = header_size + idx * kIndexEntrySize
    return struct.unpack('<HI', data[offset:offset + kIndexEntrySize])

  prev_resource_id, prev_offset = entry_at_index(0)
  for i in range(1, resource_count + 1):
    resource_id, offset = entry_at_index(i)
    resources[prev_resource_id] = data[prev_offset:offset]
    prev_resource_id, prev_offset = resource_id, offset

  id_table_size = (resource_count + 1) * kIndexEntrySize
  # Read the alias table.
  kAliasEntrySize = 2 + 2  # uint16, uint16
  def alias_at_index(idx):
    offset = header_size + id_table_size + idx * kAliasEntrySize
    return struct.unpack('<HH', data[offset:offset + kAliasEntrySize])

  aliases = {}
  for i in range(alias_count):
    resource_id, index = alias_at_index(i)
    aliased_id = entry_at_index(index)[0]
    aliases[resource_id] = aliased_id
    resources[resource_id] = resources[aliased_id]

  alias_table_size = kAliasEntrySize * alias_count
  sizes = DataPackSizes(
      header_size, id_table_size, alias_table_size,
      len(data) - header_size - id_table_size - alias_table_size)
  assert sizes.total == len(data), 'original={} computed={}'.format(
      len(data), sizes.total)
  return DataPackContents(resources, encoding, version, aliases, sizes)


def WriteDataPackToString(resources, encoding):
  """Returns a string with a map of id=>data in the data pack format."""
  ret = []

  # Compute alias map.
  resource_ids = sorted(resources)
  # Use reversed() so that for duplicates lower IDs clobber higher ones.
  id_by_data = {resources[k]: k for k in reversed(resource_ids)}
  # Map of resource_id -> resource_id, where value < key.
  alias_map = {k: id_by_data[v] for k, v in resources.items()
               if id_by_data[v] != k}

  # Write file header.
  resource_count = len(resources) - len(alias_map)
  # Padding bytes added for alignment.
  ret.append(struct.pack('<IBxxxHH', PACK_FILE_VERSION, encoding,
                         resource_count, len(alias_map)))
  HEADER_LENGTH = 4 + 4 + 2 + 2

  # Each main table entry is: uint16 + uint32 (and an extra entry at the end).
  # Each alias table entry is: uint16 + uint16.
  data_offset = HEADER_LENGTH + (resource_count + 1) * 6 + len(alias_map) * 4

  # Write main table.
  index_by_id = {}
  deduped_data = []
  index = 0
  for resource_id in resource_ids:
    if resource_id in alias_map:
      continue
    data = resources[resource_id]
    index_by_id[resource_id] = index
    ret.append(struct.pack('<HI', resource_id, data_offset))
    data_offset += len(data)
    deduped_data.append(data)
    index += 1

  assert index == resource_count
  # Add an extra entry at the end.
  ret.append(struct.pack('<HI', 0, data_offset))

  # Write alias table.
  for resource_id in sorted(alias_map):
    index = index_by_id[alias_map[resource_id]]
    ret.append(struct.pack('<HH', resource_id, index))

  # Write data.
  ret.extend(deduped_data)
  return ''.join(ret)


def WriteDataPack(resources, output_file, encoding):
  """Writes a map of id=>data into output_file as a data pack."""
  content = WriteDataPackToString(resources, encoding)
  with open(output_file, 'wb') as file:
    file.write(content)


def RePack(output_file, input_files, whitelist_file=None,
           suppress_removed_key_output=False):
  """Write a new data pack file by combining input pack files.

  Args:
      output_file: path to the new data pack file.
      input_files: a list of paths to the data pack files to combine.
      whitelist_file: path to the file that contains the list of resource IDs
                      that should be kept in the output file or None to include
                      all resources.
      suppress_removed_key_output: allows the caller to suppress the output from
                                   RePackFromDataPackStrings.

  Raises:
      KeyError: if there are duplicate keys or resource encoding is
      inconsistent.
  """
  input_data_packs = [ReadDataPack(filename) for filename in input_files]
  input_info_files = [filename + '.info' for filename in input_files]
  whitelist = None
  if whitelist_file:
    lines = util.ReadFile(whitelist_file, util.RAW_TEXT).strip().splitlines()
    if not lines:
      raise Exception('Whitelist file should not be empty')
    whitelist = set(int(x) for x in lines)
  inputs = [(p.resources, p.encoding) for p in input_data_packs]
  resources, encoding = RePackFromDataPackStrings(
      inputs, whitelist, suppress_removed_key_output)
  WriteDataPack(resources, output_file, encoding)
  with open(output_file + '.info', 'w') as output_info_file:
    for filename in input_info_files:
      with open(filename, 'r') as info_file:
        output_info_file.writelines(info_file.readlines())


def RePackFromDataPackStrings(inputs, whitelist,
                              suppress_removed_key_output=False):
  """Combines all inputs into one.

  Args:
      inputs: a list of (resources_by_id, encoding) tuples to be combined.
      whitelist: a list of resource IDs that should be kept in the output string
                 or None to include all resources.
      suppress_removed_key_output: Do not print removed keys.

  Returns:
      Returns (resources_by_id, encoding).

  Raises:
      KeyError: if there are duplicate keys or resource encoding is
      inconsistent.
  """
  resources = {}
  encoding = None
  for input_resources, input_encoding in inputs:
    # Make sure we have no dups.
    duplicate_keys = set(input_resources.keys()) & set(resources.keys())
    if duplicate_keys:
      raise KeyError('Duplicate keys: ' + str(list(duplicate_keys)))

    # Make sure encoding is consistent.
    if encoding in (None, BINARY):
      encoding = input_encoding
    elif input_encoding not in (BINARY, encoding):
      raise KeyError('Inconsistent encodings: ' + str(encoding) +
                     ' vs ' + str(input_encoding))

    if whitelist:
      whitelisted_resources = dict([(key, input_resources[key])
                                    for key in input_resources.keys()
                                    if key in whitelist])
      resources.update(whitelisted_resources)
      removed_keys = [key for key in input_resources.keys()
                      if key not in whitelist]
      if not suppress_removed_key_output:
        for key in removed_keys:
          print('RePackFromDataPackStrings Removed Key:', key)
    else:
      resources.update(input_resources)

  # Encoding is 0 for BINARY, 1 for UTF8 and 2 for UTF16
  if encoding is None:
    encoding = BINARY
  return resources, encoding


def main():
  # Write a simple file.
  data = {1: '', 4: 'this is id 4', 6: 'this is id 6', 10: ''}
  WriteDataPack(data, 'datapack1.pak', UTF8)
  data2 = {1000: 'test', 5: 'five'}
  WriteDataPack(data2, 'datapack2.pak', UTF8)
  print('wrote datapack1 and datapack2 to current directory.')


if __name__ == '__main__':
  main()

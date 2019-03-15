# Copyright 2018 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Writes or reads Riegeli/records files."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

from riegeli.base import riegeli_error
from riegeli.records import record_position
from riegeli.records import record_reader
from riegeli.records import record_writer
from riegeli.records import skipped_region

from riegeli.records import records_metadata_pb2

__all__ = ('RiegeliError', 'FlushType', 'RecordPosition', 'SkippedRegion',
           'RecordsMetadata', 'set_record_type', 'RecordWriter',
           'get_record_type', 'RecordReader')

# pylint: disable=invalid-name
RiegeliError = riegeli_error.RiegeliError
RecordPosition = record_position.RecordPosition
SkippedRegion = skipped_region.SkippedRegion
RecordsMetadata = records_metadata_pb2.RecordsMetadata
FlushType = record_writer.FlushType
set_record_type = record_writer.set_record_type
RecordWriter = record_writer.RecordWriter
get_record_type = record_reader.get_record_type
RecordReader = record_reader.RecordReader

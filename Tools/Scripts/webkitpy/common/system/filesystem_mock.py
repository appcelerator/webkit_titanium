# Copyright (C) 2009 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import errno
import os
import re

from webkitpy.common.system import path
from webkitpy.common.system import ospath


class MockFileSystem(object):
    def __init__(self, files=None):
        """Initializes a "mock" filesystem that can be used to completely
        stub out a filesystem.

        Args:
            files: a dict of filenames -> file contents. A file contents
                value of None is used to indicate that the file should
                not exist.
        """
        self.files = files or {}
        self.written_files = {}
        self._sep = '/'
        self.current_tmpno = 0

    def _get_sep(self):
        return self._sep

    sep = property(_get_sep, doc="pathname separator")

    def _raise_not_found(self, path):
        raise IOError(errno.ENOENT, path, os.strerror(errno.ENOENT))

    def _split(self, path):
        return path.rsplit(self.sep, 1)

    def abspath(self, path):
        if path.endswith(self.sep):
            return path[:-1]
        return path

    def basename(self, path):
        return self._split(path)[1]

    def copyfile(self, source, destination):
        if not self.exists(source):
            self._raise_not_found(source)
        if self.isdir(source):
            raise IOError(errno.EISDIR, source, os.strerror(errno.ISDIR))
        if self.isdir(destination):
            raise IOError(errno.EISDIR, destination, os.strerror(errno.ISDIR))

        self.files[destination] = self.files[source]
        self.written_files[destination] = self.files[source]

    def dirname(self, path):
        return self._split(path)[0]

    def exists(self, path):
        return self.isfile(path) or self.isdir(path)

    def files_under(self, path, dirs_to_skip=[], file_filter=None):
        def filter_all(fs, dirpath, basename):
            return True

        file_filter = file_filter or filter_all
        files = []
        if self.isfile(path):
            if file_filter(self, self.dirname(path), self.basename(path)):
                files.append(path)
            return files

        if self.basename(path) in dirs_to_skip:
            return []

        if not path.endswith(self.sep):
            path += self.sep

        dir_substrings = [self.sep + d + self.sep for d in dirs_to_skip]
        for filename in self.files:
            if not filename.startswith(path):
                continue

            suffix = filename[len(path) - 1:]
            if any(dir_substring in suffix for dir_substring in dir_substrings):
                continue

            dirpath, basename = self._split(filename)
            if file_filter(self, dirpath, basename):
                files.append(filename)

        return files

    def glob(self, path):
        # FIXME: This only handles a wildcard '*' at the end of the path.
        # Maybe it should handle more?
        if path[-1] == '*':
            return [f for f in self.files if f.startswith(path[:-1])]
        else:
            return [f for f in self.files if f == path]

    def isabs(self, path):
        return path.startswith(self.sep)

    def isfile(self, path):
        return path in self.files and self.files[path] is not None

    def isdir(self, path):
        if path in self.files:
            return False
        if not path.endswith(self.sep):
            path += self.sep

        # We need to use a copy of the keys here in order to avoid switching
        # to a different thread and potentially modifying the dict in
        # mid-iteration.
        files = self.files.keys()[:]
        return any(f.startswith(path) for f in files)

    def join(self, *comps):
        # FIXME: might want tests for this and/or a better comment about how
        # it works.
        return re.sub(re.escape(os.path.sep), self.sep, os.path.join(*comps))

    def listdir(self, path):
        if not self.isdir(path):
            raise OSError("%s is not a directory" % path)

        if not path.endswith(self.sep):
            path += self.sep

        dirs = []
        files = []
        for f in self.files:
            if self.exists(f) and f.startswith(path):
                remaining = f[len(path):]
                if self.sep in remaining:
                    dir = remaining[:remaining.index(self.sep)]
                    if not dir in dirs:
                        dirs.append(dir)
                else:
                    files.append(remaining)
        return dirs + files

    def mtime(self, path):
        if self.exists(path):
            return 0
        self._raise_not_found(path)

    def _mktemp(self, suffix='', prefix='tmp', dir=None, **kwargs):
        if dir is None:
            dir = self.sep + '__im_tmp'
        curno = self.current_tmpno
        self.current_tmpno += 1
        return self.join(dir, "%s_%u_%s" % (prefix, curno, suffix))

    def mkdtemp(self, **kwargs):
        class TemporaryDirectory(object):
            def __init__(self, fs, **kwargs):
                self._kwargs = kwargs
                self._filesystem = fs
                self._directory_path = fs._mktemp(**kwargs)
                fs.maybe_make_directory(self._directory_path)

            def __str__(self):
                return self._directory_path

            def __enter__(self):
                return self._directory_path

            def __exit__(self, type, value, traceback):
                # Only self-delete if necessary.

                # FIXME: Should we delete non-empty directories?
                if self._filesystem.exists(self._directory_path):
                    self._filesystem.rmtree(self._directory_path)

        return TemporaryDirectory(fs=self, **kwargs)

    def maybe_make_directory(self, *path):
        # FIXME: Implement such that subsequent calls to isdir() work?
        pass

    def move(self, source, destination):
        if self.files[source] is None:
            self._raise_not_found(source)
        self.files[destination] = self.files[source]
        self.written_files[destination] = self.files[destination]
        self.files[source] = None
        self.written_files[source] = None

    def normpath(self, path):
        return path

    def open_binary_tempfile(self, suffix=''):
        path = self._mktemp(suffix)
        return (WritableFileObject(self, path), path)

    def open_text_file_for_writing(self, path, append=False):
        return WritableFileObject(self, path, append)

    def read_text_file(self, path):
        return self.read_binary_file(path).decode('utf-8')

    def open_binary_file_for_reading(self, path):
        if self.files[path] is None:
            self._raise_not_found(path)
        return ReadableFileObject(self, path, self.files[path])

    def read_binary_file(self, path):
        # Intentionally raises KeyError if we don't recognize the path.
        if self.files[path] is None:
            self._raise_not_found(path)
        return self.files[path]

    def relpath(self, path, start='.'):
        return ospath.relpath(path, start, self.abspath, self.sep)

    def remove(self, path):
        if self.files[path] is None:
            self._raise_not_found(path)
        self.files[path] = None
        self.written_files[path] = None

    def rmtree(self, path):
        if not path.endswith(self.sep):
            path += self.sep

        for f in self.files:
            if f.startswith(path):
                self.files[f] = None

    def splitext(self, path):
        idx = path.rfind('.')
        if idx == -1:
            idx = 0
        return (path[0:idx], path[idx:])

    def write_text_file(self, path, contents):
        return self.write_binary_file(path, contents.encode('utf-8'))

    def write_binary_file(self, path, contents):
        self.files[path] = contents
        self.written_files[path] = contents


class WritableFileObject(object):
    def __init__(self, fs, path, append=False, encoding=None):
        self.fs = fs
        self.path = path
        self.closed = False
        if path not in self.fs.files or not append:
            self.fs.files[path] = ""

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def close(self):
        self.closed = True

    def write(self, str):
        self.fs.files[self.path] += str
        self.fs.written_files[self.path] = self.fs.files[self.path]


class ReadableFileObject(object):
    def __init__(self, fs, path, data=""):
        self.fs = fs
        self.path = path
        self.closed = False
        self.data = data
        self.offset = 0

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        self.close()

    def close(self):
        self.closed = True

    def read(self, bytes=None):
        if not bytes:
            return self.data[self.offset:]
        start = self.offset
        self.offset += bytes
        return self.data[start:self.offset]

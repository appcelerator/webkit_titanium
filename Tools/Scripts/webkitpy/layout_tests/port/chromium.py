#!/usr/bin/env python
# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
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

"""Chromium implementations of the Port interface."""

import errno
import logging
import re
import signal
import subprocess
import sys
import time
import webbrowser

from webkitpy.common.system import executive
from webkitpy.common.system.path import cygpath
from webkitpy.layout_tests.layout_package import test_expectations
from webkitpy.layout_tests.port import base
from webkitpy.layout_tests.port import http_server
from webkitpy.layout_tests.port import websocket_server

_log = logging.getLogger("webkitpy.layout_tests.port.chromium")


# FIXME: This function doesn't belong in this package.
class ChromiumPort(base.Port):
    """Abstract base class for Chromium implementations of the Port class."""
    ALL_BASELINE_VARIANTS = [
        'chromium-mac-snowleopard', 'chromium-mac-leopard',
        'chromium-win-win7', 'chromium-win-vista', 'chromium-win-xp',
        'chromium-linux', 'chromium-linux-x86_64',
        'chromium-gpu-mac-leopard', 'chromium-gpu-win-xp', 'chromium-gpu-linux',
    ]

    def __init__(self, **kwargs):
        base.Port.__init__(self, **kwargs)
        self._chromium_base_dir = None

    def _check_file_exists(self, path_to_file, file_description,
                           override_step=None, logging=True):
        """Verify the file is present where expected or log an error.

        Args:
            file_name: The (human friendly) name or description of the file
                you're looking for (e.g., "HTTP Server"). Used for error logging.
            override_step: An optional string to be logged if the check fails.
            logging: Whether or not log the error messages."""
        if not self._filesystem.exists(path_to_file):
            if logging:
                _log.error('Unable to find %s' % file_description)
                _log.error('    at %s' % path_to_file)
                if override_step:
                    _log.error('    %s' % override_step)
                    _log.error('')
            return False
        return True


    def baseline_path(self):
        return self._webkit_baseline_path(self._name)

    def check_build(self, needs_http):
        result = True

        dump_render_tree_binary_path = self._path_to_driver()
        result = self._check_file_exists(dump_render_tree_binary_path,
                                         'test driver') and result
        if result and self.get_option('build'):
            result = self._check_driver_build_up_to_date(
                self.get_option('configuration'))
        else:
            _log.error('')

        helper_path = self._path_to_helper()
        if helper_path:
            result = self._check_file_exists(helper_path,
                                             'layout test helper') and result

        if self.get_option('pixel_tests'):
            result = self.check_image_diff(
                'To override, invoke with --no-pixel-tests') and result

        # It's okay if pretty patch isn't available, but we will at
        # least log a message.
        self.check_pretty_patch()

        return result

    def check_sys_deps(self, needs_http):
        cmd = [self._path_to_driver(), '--check-layout-test-sys-deps']

        local_error = executive.ScriptError()

        def error_handler(script_error):
            local_error.exit_code = script_error.exit_code

        output = self._executive.run_command(cmd, error_handler=error_handler)
        if local_error.exit_code:
            _log.error('System dependencies check failed.')
            _log.error('To override, invoke with --nocheck-sys-deps')
            _log.error('')
            _log.error(output)
            return False
        return True

    def check_image_diff(self, override_step=None, logging=True):
        image_diff_path = self._path_to_image_diff()
        return self._check_file_exists(image_diff_path, 'image diff exe',
                                       override_step, logging)

    def diff_image(self, expected_contents, actual_contents,
                   diff_filename=None):
        executable = self._path_to_image_diff()

        tempdir = self._filesystem.mkdtemp()
        expected_filename = self._filesystem.join(str(tempdir), "expected.png")
        self._filesystem.write_binary_file(expected_filename, expected_contents)
        actual_filename = self._filesystem.join(str(tempdir), "actual.png")
        self._filesystem.write_binary_file(actual_filename, actual_contents)

        if diff_filename:
            cmd = [executable, '--diff', expected_filename,
                   actual_filename, diff_filename]
        else:
            cmd = [executable, expected_filename, actual_filename]

        result = True
        try:
            exit_code = self._executive.run_command(cmd, return_exit_code=True)
            if exit_code == 0:
                # The images are the same.
                result = False
            elif exit_code != 1:
                _log.error("image diff returned an exit code of "
                           + str(exit_code))
                # Returning False here causes the script to think that we
                # successfully created the diff even though we didn't.  If
                # we return True, we think that the images match but the hashes
                # don't match.
                # FIXME: Figure out why image_diff returns other values.
                result = False
        except OSError, e:
            if e.errno == errno.ENOENT or e.errno == errno.EACCES:
                _compare_available = False
            else:
                raise e
        finally:
            self._filesystem.rmtree(str(tempdir))
        return result

    def driver_name(self):
        return "DumpRenderTree"

    def path_from_chromium_base(self, *comps):
        """Returns the full path to path made by joining the top of the
        Chromium source tree and the list of path components in |*comps|."""
        if not self._chromium_base_dir:
            abspath = self._filesystem.abspath(__file__)
            offset = abspath.find('third_party')
            if offset == -1:
                self._chromium_base_dir = self._filesystem.join(
                    abspath[0:abspath.find('Tools')],
                    'Source', 'WebKit', 'chromium')
            else:
                self._chromium_base_dir = abspath[0:offset]
        return self._filesystem.join(self._chromium_base_dir, *comps)

    def path_to_test_expectations_file(self):
        return self.path_from_webkit_base('LayoutTests', 'platform',
            'chromium', 'test_expectations.txt')

    def results_directory(self):
        try:
            return self.path_from_chromium_base('webkit',
                self.get_option('configuration'),
                self.get_option('results_directory'))
        except AssertionError:
            return self._build_path(self.get_option('configuration'),
                                    self.get_option('results_directory'))

    def setup_test_run(self):
        # Delete the disk cache if any to ensure a clean test run.
        dump_render_tree_binary_path = self._path_to_driver()
        cachedir = self._filesystem.dirname(dump_render_tree_binary_path)
        cachedir = self._filesystem.join(cachedir, "cache")
        if self._filesystem.exists(cachedir):
            self._filesystem.rmtree(cachedir)

    def create_driver(self, worker_number):
        """Starts a new Driver and returns a handle to it."""
        return ChromiumDriver(self, worker_number)

    def start_helper(self):
        helper_path = self._path_to_helper()
        if helper_path:
            _log.debug("Starting layout helper %s" % helper_path)
            # Note: Not thread safe: http://bugs.python.org/issue2320
            self._helper = subprocess.Popen([helper_path],
                stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=None)
            is_ready = self._helper.stdout.readline()
            if not is_ready.startswith('ready'):
                _log.error("layout_test_helper failed to be ready")

    def stop_helper(self):
        if self._helper:
            _log.debug("Stopping layout test helper")
            self._helper.stdin.write("x\n")
            self._helper.stdin.close()
            # wait() is not threadsafe and can throw OSError due to:
            # http://bugs.python.org/issue1731717
            self._helper.wait()

    def all_baseline_variants(self):
        return self.ALL_BASELINE_VARIANTS

    def test_expectations(self):
        """Returns the test expectations for this port.

        Basically this string should contain the equivalent of a
        test_expectations file. See test_expectations.py for more details."""
        expectations_path = self.path_to_test_expectations_file()
        return self._filesystem.read_text_file(expectations_path)

    def test_expectations_overrides(self):
        try:
            overrides_path = self.path_from_chromium_base('webkit', 'tools',
                'layout_tests', 'test_expectations.txt')
        except AssertionError:
            return None
        if not self._filesystem.exists(overrides_path):
            return None
        return self._filesystem.read_text_file(overrides_path)

    def skipped_layout_tests(self, extra_test_files=None):
        expectations_str = self.test_expectations()
        overrides_str = self.test_expectations_overrides()
        is_debug_mode = False

        all_test_files = self.tests([])
        if extra_test_files:
            all_test_files.update(extra_test_files)

        expectations = test_expectations.TestExpectations(
            self, all_test_files, expectations_str, self.test_configuration(),
            is_lint_mode=False, overrides=overrides_str)
        tests_dir = self.layout_tests_dir()
        return [self.relative_test_filename(test)
                for test in expectations.get_tests_with_result_type(test_expectations.SKIP)]

    def test_repository_paths(self):
        # Note: for JSON file's backward-compatibility we use 'chrome' rather
        # than 'chromium' here.
        repos = super(ChromiumPort, self).test_repository_paths()
        repos.append(('chrome', self.path_from_chromium_base()))
        return repos

    #
    # PROTECTED METHODS
    #
    # These routines should only be called by other methods in this file
    # or any subclasses.
    #

    def _check_driver_build_up_to_date(self, configuration):
        if configuration in ('Debug', 'Release'):
            try:
                debug_path = self._path_to_driver('Debug')
                release_path = self._path_to_driver('Release')

                debug_mtime = self._filesystem.mtime(debug_path)
                release_mtime = self._filesystem.mtime(release_path)

                if (debug_mtime > release_mtime and configuration == 'Release' or
                    release_mtime > debug_mtime and configuration == 'Debug'):
                    _log.warning('You are not running the most '
                                 'recent DumpRenderTree binary. You need to '
                                 'pass --debug or not to select between '
                                 'Debug and Release.')
                    _log.warning('')
            # This will fail if we don't have both a debug and release binary.
            # That's fine because, in this case, we must already be running the
            # most up-to-date one.
            except OSError:
                pass
        return True

    def _chromium_baseline_path(self, platform):
        if platform is None:
            platform = self.name()
        return self.path_from_webkit_base('LayoutTests', 'platform', platform)

    def _convert_path(self, path):
        """Handles filename conversion for subprocess command line args."""
        # See note above in diff_image() for why we need this.
        if sys.platform == 'cygwin':
            return cygpath(path)
        return path

    def _path_to_image_diff(self):
        binary_name = 'ImageDiff'
        return self._build_path(self.get_option('configuration'), binary_name)


class ChromiumDriver(base.Driver):
    """Abstract interface for DRT."""

    def __init__(self, port, worker_number):
        self._port = port
        self._worker_number = worker_number
        self._image_path = None
        if self._port.get_option('pixel_tests'):
            self._image_path = self._port._filesystem.join(
                self._port.get_option('results_directory'),
                'png_result%s.png' % self._worker_number)

    def cmd_line(self):
        cmd = self._command_wrapper(self._port.get_option('wrapper'))
        cmd.append(self._port._path_to_driver())
        if self._port.get_option('pixel_tests'):
            # See note above in diff_image() for why we need _convert_path().
            cmd.append("--pixel-tests=" +
                       self._port._convert_path(self._image_path))

        cmd.append('--test-shell')

        if self._port.get_option('startup_dialog'):
            cmd.append('--testshell-startup-dialog')

        if self._port.get_option('gp_fault_error_box'):
            cmd.append('--gp-fault-error-box')

        if self._port.get_option('js_flags') is not None:
            cmd.append('--js-flags="' + self._port.get_option('js_flags') + '"')

        if self._port.get_option('stress_opt'):
            cmd.append('--stress-opt')

        if self._port.get_option('stress_deopt'):
            cmd.append('--stress-deopt')

        if self._port.get_option('accelerated_compositing'):
            cmd.append('--enable-accelerated-compositing')
        if self._port.get_option('accelerated_2d_canvas'):
            cmd.append('--enable-accelerated-2d-canvas')
        if self._port.get_option('enable_hardware_gpu'):
            cmd.append('--enable-hardware-gpu')
        return cmd

    def start(self):
        # FIXME: Should be an error to call this method twice.
        cmd = self.cmd_line()

        # We need to pass close_fds=True to work around Python bug #2320
        # (otherwise we can hang when we kill DumpRenderTree when we are running
        # multiple threads). See http://bugs.python.org/issue2320 .
        # Note that close_fds isn't supported on Windows, but this bug only
        # shows up on Mac and Linux.
        close_flag = sys.platform not in ('win32', 'cygwin')
        self._proc = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT,
                                      close_fds=close_flag)

    def poll(self):
        # poll() is not threadsafe and can throw OSError due to:
        # http://bugs.python.org/issue1731717
        return self._proc.poll()

    def _write_command_and_read_line(self, input=None):
        """Returns a tuple: (line, did_crash)"""
        try:
            if input:
                if isinstance(input, unicode):
                    # DRT expects utf-8
                    input = input.encode("utf-8")
                self._proc.stdin.write(input)
            # DumpRenderTree text output is always UTF-8.  However some tests
            # (e.g. webarchive) may spit out binary data instead of text so we
            # don't bother to decode the output.
            line = self._proc.stdout.readline()
            # We could assert() here that line correctly decodes as UTF-8.
            return (line, False)
        except IOError, e:
            _log.error("IOError communicating w/ DRT: " + str(e))
            return (None, True)

    def _test_shell_command(self, uri, timeoutms, checksum):
        cmd = uri
        if timeoutms:
            cmd += ' ' + str(timeoutms)
        if checksum:
            cmd += ' ' + checksum
        cmd += "\n"
        return cmd

    def _output_image(self):
        """Returns the image output which driver generated."""
        png_path = self._image_path
        if png_path and self._port._filesystem.exists(png_path):
            return self._port._filesystem.read_binary_file(png_path)
        else:
            return ''

    def _output_image_with_retry(self):
        # Retry a few more times because open() sometimes fails on Windows,
        # raising "IOError: [Errno 13] Permission denied:"
        retry_num = 50
        timeout_seconds = 5.0
        for i in range(retry_num):
            try:
                return self._output_image()
            except IOError, e:
                if e.errno == errno.EACCES:
                    time.sleep(timeout_seconds / retry_num)
                else:
                    raise e
        return self._output_image()

    def _clear_output_image(self):
        png_path = self._image_path
        if png_path and self._port._filesystem.exists(png_path):
            self._port._filesystem.remove(png_path)

    def run_test(self, driver_input):
        output = []
        error = []
        crash = False
        timeout = False
        actual_uri = None
        actual_checksum = None
        self._clear_output_image()
        start_time = time.time()

        uri = self._port.filename_to_uri(driver_input.filename)
        cmd = self._test_shell_command(uri, driver_input.timeout,
                                       driver_input.image_hash)
        (line, crash) = self._write_command_and_read_line(input=cmd)

        while not crash and line.rstrip() != "#EOF":
            # Make sure we haven't crashed.
            if line == '' and self.poll() is not None:
                # This is hex code 0xc000001d, which is used for abrupt
                # termination. This happens if we hit ctrl+c from the prompt
                # and we happen to be waiting on DRT.
                # sdoyon: Not sure for which OS and in what circumstances the
                # above code is valid. What works for me under Linux to detect
                # ctrl+c is for the subprocess returncode to be negative
                # SIGINT. And that agrees with the subprocess documentation.
                if (-1073741510 == self._proc.returncode or
                    - signal.SIGINT == self._proc.returncode):
                    raise KeyboardInterrupt
                crash = True
                break

            # Don't include #URL lines in our output
            if line.startswith("#URL:"):
                actual_uri = line.rstrip()[5:]
                if uri != actual_uri:
                    # GURL capitalizes the drive letter of a file URL.
                    if (not re.search("^file:///[a-z]:", uri) or
                        uri.lower() != actual_uri.lower()):
                        _log.fatal("Test got out of sync:\n|%s|\n|%s|" %
                                   (uri, actual_uri))
                        raise AssertionError("test out of sync")
            elif line.startswith("#MD5:"):
                actual_checksum = line.rstrip()[5:]
            elif line.startswith("#TEST_TIMED_OUT"):
                timeout = True
                # Test timed out, but we still need to read until #EOF.
            elif actual_uri:
                output.append(line)
            else:
                error.append(line)

            (line, crash) = self._write_command_and_read_line(input=None)

        run_time = time.time() - start_time
        output_image = self._output_image_with_retry()
        assert output_image is not None
        return base.DriverOutput(''.join(output), output_image, actual_checksum,
                                 crash, run_time, timeout, ''.join(error))

    def stop(self):
        if self._proc:
            self._proc.stdin.close()
            self._proc.stdout.close()
            if self._proc.stderr:
                self._proc.stderr.close()
            if sys.platform not in ('win32', 'cygwin'):
                # Closing stdin/stdout/stderr hangs sometimes on OS X,
                # (see __init__(), above), and anyway we don't want to hang
                # the harness if DRT is buggy, so we wait a couple
                # seconds to give DRT a chance to clean up, but then
                # force-kill the process if necessary.
                KILL_TIMEOUT = 3.0
                timeout = time.time() + KILL_TIMEOUT
                # poll() is not threadsafe and can throw OSError due to:
                # http://bugs.python.org/issue1731717
                while self._proc.poll() is None and time.time() < timeout:
                    time.sleep(0.1)
                # poll() is not threadsafe and can throw OSError due to:
                # http://bugs.python.org/issue1731717
                if self._proc.poll() is None:
                    _log.warning('stopping test driver timed out, '
                                 'killing it')
                    self._port._executive.kill_process(self._proc.pid)

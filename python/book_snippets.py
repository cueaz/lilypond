# book_snippets.py
# -*- coding: utf-8 -*-
#
# This file is part of LilyPond, the GNU music typesetter.
#
# Copyright (C) 2010--2020 Reinhold Kainhofer <reinhold@kainhofer.com>
#
# LilyPond is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LilyPond is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LilyPond.  If not, see <http://www.gnu.org/licenses/>.


import copy
import hashlib
import os
import re
import shutil
import subprocess
import sys

import book_base
import lilylib as ly

progress = ly.progress
warning = ly.warning
error = ly.error
debug = ly.debug_output


####################################################################
# Snippet option handling
####################################################################


#
# Is this pythonic?  Personally, I find this rather #define-nesque. --hwn
#
# Global definitions:
AFTER = 'after'
ALT = 'alt'
BEFORE = 'before'
DOCTITLE = 'doctitle'
EXAMPLEINDENT = 'exampleindent'
FILENAME = 'filename'
FILTER = 'filter'
FRAGMENT = 'fragment'
LAYOUT = 'layout'
LINE_WIDTH = 'line-width'
NOFRAGMENT = 'nofragment'
NOGETTEXT = 'nogettext'
NOINDENT = 'noindent'
INDENT = 'indent'
NORAGGED_RIGHT = 'noragged-right'
NOTES = 'body'
NOTIME = 'notime'
OUTPUT = 'output'
OUTPUTIMAGE = 'outputimage'
PAPER = 'paper'
PAPERSIZE = 'papersize'
PREAMBLE = 'preamble'
PRINTFILENAME = 'printfilename'
QUOTE = 'quote'
RAGGED_RIGHT = 'ragged-right'
RELATIVE = 'relative'
STAFFSIZE = 'staffsize'
TEXIDOC = 'texidoc'
VERBATIM = 'verbatim'
VERSION = 'lilypondversion'


# NOTIME and NOGETTEXT have no opposite so they aren't part of this
# dictionary.
no_options = {
    NOFRAGMENT: FRAGMENT,
    NOINDENT: INDENT,
}

# Options that have no impact on processing by lilypond (or --process
# argument)
PROCESSING_INDEPENDENT_OPTIONS = (
    ALT, NOGETTEXT, VERBATIM,
    TEXIDOC, DOCTITLE, VERSION, PRINTFILENAME)


# Options without a pattern in snippet_options.
simple_options = [
    EXAMPLEINDENT,
    FRAGMENT,
    NOFRAGMENT,
    NOGETTEXT,
    NOINDENT,
    PRINTFILENAME,
    DOCTITLE,
    TEXIDOC,
    VERBATIM,
    FILENAME,
    ALT
]


####################################################################
# LilyPond templates for the snippets
####################################################################

snippet_options = {
    ##
    NOTES: {
        RELATIVE: r'''\relative c%(relative_quotes)s''',
    },

    ##
    # TODO: Remove the 1mm additional padding in the line-width
    #       once lilypond creates tighter cropped images!
    PAPER: {
        PAPERSIZE: r'''#(set-paper-size "%(papersize)s")''',
        INDENT: r'''indent = %(indent)s''',
        LINE_WIDTH: r'''line-width = %(line-width)s
  %% offset the left padding, also add 1mm as lilypond creates cropped
  %% images with a little space on the right
  line-width = #(- line-width (* mm  %(padding_mm)f) (* mm 1))''',
        QUOTE: r'''line-width = %(line-width)s - 2.0 * %(exampleindent)s
  %% offset the left padding, also add 1mm as lilypond creates cropped
  %% images with a little space on the right
  line-width = #(- line-width (* mm  %(padding_mm)f) (* mm 1))''',
        RAGGED_RIGHT: r'''ragged-right = ##t''',
        NORAGGED_RIGHT: r'''ragged-right = ##f''',
    },

    ##
    LAYOUT: {
        NOTIME: r'''
 \context {
   \Score
   timing = ##f
 }
 \context {
   \Staff
   \remove "Time_signature_engraver"
 }''',
    },

    ##
    PREAMBLE: {
        STAFFSIZE: r'''#(set-global-staff-size %(staffsize)s)''',
    },
}


def classic_lilypond_book_compatibility(key, value):
    if key == 'lilyquote':
        return (QUOTE, value)
    if key == 'singleline' and value is None:
        return (RAGGED_RIGHT, None)

    m = re.search(r'relative\s*([-0-9])', key)
    if m:
        return ('relative', m.group(1))

    m = re.match('([0-9]+)pt', key)
    if m:
        return ('staffsize', m.group(1))

    if key == 'indent' or key == 'line-width':
        m = re.match('([-.0-9]+)(cm|in|mm|pt|staffspace)', value)
        if m:
            f = float(m.group(1))
            return (key, '%f\\%s' % (f, m.group(2)))

    return (None, None)


PREAMBLE_LY = r'''%%%% Generated by lilypond-book
%%%% Options: [%(option_string)s]
\include "lilypond-book-preamble.ly"


%% ****************************************************************
%% Start cut-&-pastable-section
%% ****************************************************************

%(preamble_string)s

\paper {
  %(paper_string)s
}

\layout {
  %(layout_string)s
}

%(safe_mode_string)s
'''


FULL_LY = '''


%% ****************************************************************
%% ly snippet:
%% ****************************************************************
%(code)s


%% ****************************************************************
%% end ly snippet
%% ****************************************************************
'''

FRAGMENT_LY = r'''
%(notes_string)s
{


%% ****************************************************************
%% ly snippet contents follows:
%% ****************************************************************
%(code)s


%% ****************************************************************
%% end ly snippet
%% ****************************************************************
}
'''


####################################################################
# Helper functions
####################################################################

def ps_page_count(ps_name):
    # Open .ps file in binary mode, it might contain embedded fonts.
    header = open(ps_name, 'rb').read(1024)
    m = re.search(b'\n%%Pages: ([0-9]+)', header)
    if m:
        return int(m.group(1))
    return 0


ly_var_def_re = re.compile(r'^([a-zA-Z]+)[\t ]*=', re.M)
ly_comment_re = re.compile(r'(%+[\t ]*)(.*)$', re.M)
ly_context_id_re = re.compile('\\\\(?:new|context)\\s+(?:[a-zA-Z]*?(?:Staff\
(?:Group)?|Voice|FiguredBass|FretBoards|Names|Devnull))\\s+=\\s+"?([a-zA-Z]+)"?\\s+')


def ly_comment_gettext(t, m):
    return m.group(1) + t(m.group(2))


class CompileError(Exception):
    pass


####################################################################
# Snippet classes
####################################################################

class Chunk:
    def replacement_text(self):
        return ''

    def filter_text(self):
        return self.replacement_text()

    def is_plain(self):
        return False

    def __init__(self):
        self._input_fullpath = ''
        self._output_fullpath = ''

    def set_document_fullpaths(self, in_fp: str, out_fp: str):
        self._input_fullpath = in_fp
        self._output_fullpath = out_fp

    def input_fullpath(self) -> str:
        """The input file path where this chunk comes from."""
        return self._input_fullpath

    def output_fullpath(self) -> str:
        """The output file path that this chunk belongs to."""
        return self._output_fullpath


class Substring (Chunk):
    """A string that does not require extra memory."""

    def __init__(self, source, start, end, line_number):
        self.source = source
        self.start = start
        self.end = end
        self.line_number = line_number
        self.override_text = None

    def is_plain(self):
        return True

    def replacement_text(self):
        if self.override_text:
            return self.override_text
        else:
            return self.source[self.start:self.end]


class Snippet (Chunk):
    def __init__(self, type, match, formatter, line_number, global_options):
        self.type = type
        self.match = match
        self.checksum = 0
        self.option_dict = {}
        self.formatter = formatter
        self.line_number = line_number
        self.global_options = global_options
        self.replacements = {'program_version': global_options.information["program_version"],
                             'program_name': ly.program_name}

    # return a shallow copy of the replacements, so the caller can modify
    # it locally without interfering with other snippet operations
    def get_replacements(self):
        return copy.copy(self.replacements)

    def replacement_text(self):
        return self.match.group('match')

    def substring(self, s):
        return self.match.group(s)

    def __repr__(self):
        return repr(self.__class__) + ' type = ' + self.type


class IncludeSnippet (Snippet):
    def processed_filename(self):
        f = self.substring('filename')
        return os.path.splitext(f)[0] + self.formatter.default_extension

    def replacement_text(self):
        s = self.match.group('match')
        f = self.substring('filename')
        return re.sub(f, self.processed_filename(), s)


class LilypondSnippet (Snippet):
    def __init__(self, type, match, formatter, line_number, global_options):
        Snippet.__init__(self, type, match, formatter,
                         line_number, global_options)
        self.filename = ''
        self.ext = '.ly'
        os = match.group('options')
        self.parse_snippet_options(os, self.type)

    def snippet_options(self):
        return []

    def verb_ly_gettext(self, s):
        lang = self.formatter.document_language
        if not lang:
            return s
        try:
            t = langdefs.translation[lang]
        except:
            return s
        # TODO: this part is flawed. langdefs is not imported,
        # so the line under `try:` raises a NameError, which is
        # catched by the too broad `except:` that was likely meant
        # only to except KeyError. As a result, this function
        # always returns `s` and the below code is never executed.
        # Investigate what the intent was and change the code accordingly
        # if possible.   --jas
        s = ly_comment_re.sub(lambda m: ly_comment_gettext(t, m), s)

        if langdefs.LANGDICT[lang].enable_ly_identifier_l10n:
            for v in ly_var_def_re.findall(s):
                s = re.sub(r"(?m)(?<!\\clef)(^|[' \\#])%s([^a-zA-Z])" % v,
                           "\\1" + t(v) + "\\2",
                           s)
            for id in ly_context_id_re.findall(s):
                s = re.sub(r'(\s+|")%s(\s+|")' % id,
                           "\\1" + t(id) + "\\2",
                           s)
        return s

    def verb_ly(self):
        verb_text = self.substring('code')
        if not NOGETTEXT in self.option_dict:
            verb_text = self.verb_ly_gettext(verb_text)
        if not verb_text.endswith('\n'):
            verb_text += '\n'
        return verb_text

    def ly(self):
        contents = self.substring('code')
        return ('\\sourcefileline %d\n%s'
                % (self.line_number - 1, contents))

    def full_ly(self):
        s = self.ly()
        if s:
            return self.compose_ly(s)
        return ''

    def split_options(self, option_string):
        return self.formatter.split_snippet_options(option_string)

    def parse_snippet_options(self, option_string, type):
        self.snippet_option_dict = {}

        # Split option string and create raw option_dict from it
        options = self.split_options(option_string)

        for option in options:
            (key, value) = (option, None)
            if '=' in option:
                (key, value) = re.split(r'\s*=\s*', option)
            else:
                # a no... option removes a previous option if present!
                if key in no_options:
                    if no_options[key] in self.option_dict:
                        del self.snippet_option_dict[no_options[key]]
                        key = None
            # Check for deprecated options, replace them by new ones
            (c_key, c_value) = classic_lilypond_book_compatibility(key, value)
            if c_key:
                if c_value:
                    warning(
                        _("deprecated ly-option used: %s=%s") % (key, value))
                    warning(
                        _("compatibility mode translation: %s=%s") % (c_key, c_value))
                else:
                    warning(
                        _("deprecated ly-option used: %s") % key)
                    warning(
                        _("compatibility mode translation: %s") % c_key)
                (key, value) = (c_key, c_value)
            # Finally, insert the option:
            if key:
                self.snippet_option_dict[key] = value

        # If LINE_WIDTH is used without parameter, set it to default.
        has_line_width = LINE_WIDTH in self.snippet_option_dict
        if has_line_width and self.snippet_option_dict[LINE_WIDTH] is None:
            del self.snippet_option_dict[LINE_WIDTH]

        # RELATIVE does not work without FRAGMENT, so imply that
        if RELATIVE in self.snippet_option_dict:
            self.snippet_option_dict[FRAGMENT] = None

        # Now get the default options from the formatter object (HTML, latex,
        # texinfo, etc.) and insert the explicit snippet options to get the
        # list of all options for this snippet
        # first, make sure we have an INDENT value as a fallback
        self.option_dict = {INDENT: '0\\mm'}
        self.option_dict.update(self.formatter.default_snippet_options)
        self.option_dict.update(self.snippet_option_dict)

        # also construct a list of all options (as strings) that influence the
        # visual appearance of the snippet
        lst = [x_y for x_y in iter(self.option_dict.items(
        )) if x_y[0] not in PROCESSING_INDEPENDENT_OPTIONS]
        option_list = []
        for (key, value) in lst:
            if value is None:
                option_list.append(key)
            else:
                option_list.append(key + "=" + value)
        option_list.sort()
        self.outputrelevant_option_list = option_list
        #print ("self.outputrelevant_option_list: %s\n" % self.outputrelevant_option_list);

        # Set a default line-width if there is none. We need this, because
        # lilypond-book has set left-padding by default and therefore does
        # #(define line-width (- line-width (* 3 mm)))
        # TODO: Junk this ugly hack if the code gets rewritten to concatenate
        # all settings before writing them in the \paper block.
        # if not LINE_WIDTH in self.option_dict:
        # if not QUOTE in self.option_dict:
        # self.option_dict[LINE_WIDTH] = "#(- paper-width \
# left-margin-default right-margin-default)"

    # Get a list of all options (as string) that influence the snippet appearance

    def get_outputrelevant_option_strings(self):
        return self.outputrelevant_option_list

    def compose_ly(self, code):

        # Defaults.
        relative = 1
        override = {}
        # The original concept of the `exampleindent' option is broken.
        # It is not possible to get a sane value for @exampleindent at all
        # without processing the document itself.  Saying
        #
        #   @exampleindent 0
        #   @example
        #   ...
        #   @end example
        #   @exampleindent 5
        #
        # causes ugly results with the TeX backend of texinfo since the
        # default value for @exampleindent isn't 5em but 0.4in (or a smaller
        # value).  Executing the above code changes the environment
        # indentation to an unknown value because we don't know the amount
        # of 1em in advance since it is font-dependent.  Modifying
        # @exampleindent in the middle of a document is simply not
        # supported within texinfo.
        #
        # As a consequence, the only function of @exampleindent is now to
        # specify the amount of indentation for the `quote' option.
        #
        # To set @exampleindent locally to zero, we use the @format
        # environment for non-quoted snippets.
        #
        # Update: since July 2011 we are running texinfo on a test file
        #         to detect the default exampleindent, so we might reintroduce
        #         further usage of exampleindent again.
        #
        # First, make sure we have some falback default value, auto-detected
        # values and explicitly specified values will always override them:
        override[EXAMPLEINDENT] = r'0.4\in'
        override[LINE_WIDTH] = '5\\in'
        override.update(self.formatter.default_snippet_options)
        override['padding_mm'] = self.global_options.padding_mm

        option_string = ','.join(self.get_outputrelevant_option_strings())
        compose_dict = {}
        compose_types = [NOTES, PREAMBLE, LAYOUT, PAPER]
        for a in compose_types:
            compose_dict[a] = []

        option_names = sorted(self.option_dict.keys())
        for key in option_names:
            value = self.option_dict[key]

            if value:
                override[key] = value
            else:
                if key not in override:
                    override[key] = None

            found = 0
            for typ in compose_types:
                if key in snippet_options[typ]:
                    compose_dict[typ].append(snippet_options[typ][key])
                    found = 1
                    break

            if not found and key not in simple_options and key not in self.snippet_options():
                warning(_("ignoring unknown ly option: %s") % key)

        # URGS
        if RELATIVE in override and override[RELATIVE]:
            relative = int(override[RELATIVE])

        relative_quotes = ''

        # 1 = central C
        if relative < 0:
            relative_quotes += ',' * (- relative)
        elif relative > 0:
            relative_quotes += "'" * relative

        # put paper-size first, if it exists
        for i, elem in enumerate(compose_dict[PAPER]):
            if elem.startswith("#(set-paper-size"):
                compose_dict[PAPER].insert(0, compose_dict[PAPER].pop(i))
                break

        paper_string = '\n  '.join(compose_dict[PAPER]) % override
        layout_string = '\n  '.join(compose_dict[LAYOUT]) % override
        notes_string = '\n  '.join(compose_dict[NOTES]) % vars()
        preamble_string = '\n  '.join(compose_dict[PREAMBLE]) % override
        padding_mm = self.global_options.padding_mm
        if self.global_options.safe_mode:
            safe_mode_string = "#(ly:set-option 'safe #t)"
        else:
            safe_mode_string = ""

        d = globals().copy()
        d.update(locals())
        d.update(self.global_options.information)
        if FRAGMENT in self.option_dict:
            body = FRAGMENT_LY
        else:
            body = FULL_LY
        return (PREAMBLE_LY + body) % d

    def get_checksum(self):
        if not self.checksum:
            # We only want to calculate the hash based on the snippet
            # code plus fragment options relevant to processing by
            # lilypond, not the snippet + preamble
            hash = hashlib.md5(self.relevant_contents(
                self.ly()).encode('utf-8'))
            for option in self.get_outputrelevant_option_strings():
                hash.update(option.encode('utf-8'))

            # let's not create too long names.
            self.checksum = hash.hexdigest()[:10]

        return self.checksum

    def basename(self):
        cs = self.get_checksum()
        name = os.path.join(cs[:2], 'lily-%s' % cs[2:])
        return name

    final_basename = basename

    def write_ly(self):
        base = self.basename()
        path = os.path.join(self.global_options.lily_output_dir, base)
        directory = os.path.split(path)[0]
        os.makedirs(directory, exist_ok=True)
        filename = path + '.ly'
        if os.path.exists(filename):
            existing = open(filename, 'r', encoding='utf-8').read()

            if self.relevant_contents(existing) != self.relevant_contents(self.full_ly()):
                warning("%s: duplicate filename but different contents of original file,\n\
printing diff against existing file." % filename)
                encoded = self.full_ly().encode('utf-8')
                cmd = 'diff -u %s -' % filename
                sys.stderr.write(self.filter_pipe(
                    encoded, cmd).decode('utf-8'))
        else:
            out = open(filename, 'w', encoding='utf-8')
            out.write(self.full_ly())

    def relevant_contents(self, ly):
        return re.sub(r'\\(version|sourcefileline|sourcefilename)[^\n]*\n', '', ly)

    def link_all_output_files(self, output_dir, destination):
        existing, missing = self.all_output_files(output_dir)
        if missing:
            error(_('Missing files: %s') % ', '.join(missing))
            raise CompileError(self.basename())
        for name in existing:
            if (self.global_options.use_source_file_names
                    and isinstance(self, LilypondFileSnippet)):
                base, ext = os.path.splitext(name)
                components = base.split('-')
                # ugh, assume filenames with prefix with one dash (lily-xxxx)
                if len(components) > 2:
                    base_suffix = '-' + components[-1]
                else:
                    base_suffix = ''
                final_name = self.final_basename() + base_suffix + ext
            else:
                final_name = name
            try:
                os.unlink(os.path.join(destination, final_name))
            except OSError:
                pass

            src = os.path.join(output_dir, name)
            dst = os.path.join(destination, final_name)
            dst_path = os.path.split(dst)[0]
            os.makedirs(dst_path, exist_ok=True)
            try:
                if (self.global_options.use_source_file_names
                        and isinstance(self, LilypondFileSnippet)):
                    content = open(src, 'rb').read()
                    basename = self.basename().encode('utf-8')
                    final_basename = self.final_basename().encode('utf-8')
                    content = content.replace(basename, final_basename)
                    open(dst, 'wb').write(content)
                else:
                    try:
                        os.link(src, dst)
                    except AttributeError:
                        shutil.copyfile(src, dst)
            except (IOError, OSError):
                error(_('Could not overwrite file %s') % dst)
                raise CompileError(self.basename())

    def additional_files_to_consider(self, base, full):
        return []

    def additional_files_required(self, base, full):
        result = []
        if self.ext != '.ly':
            result.append(base + self.ext)
        return result

    def all_output_files(self, output_dir):
        """Return all files generated in lily_output_dir, a set.

        output_dir_files is the list of files in the output directory.
        """
        result = set()
        missing = set()
        base = self.basename()
        full = os.path.join(output_dir, base)

        def consider_file(name):
            if os.path.isfile(os.path.join(output_dir, name)):
                result.add(name)

        def require_file(name):
            if os.path.isfile(os.path.join(output_dir, name)):
                result.add(name)
            else:
                missing.add(name)

        # UGH - junk self.global_options
        skip_lily = self.global_options.skip_lilypond_run
        require_file(base + '.ly')
        if not skip_lily:
            require_file(base + '-systems.count')

        if 'dseparate-log-file' in self.global_options.process_cmd:
            require_file(base + '.log')

        for f in [base + '.tex',
                  base + '.eps',
                  base + '.pdf',
                  base + '.texidoc',
                  base + '.doctitle',
                  base + '-systems.texi',
                  base + '-systems.tex',
                  base + '-systems.pdftexi']:
            consider_file(f)
        if self.formatter.document_language:
            for f in [base + '.texidoc' + self.formatter.document_language,
                      base + '.doctitle' + self.formatter.document_language]:
                consider_file(f)


        required_files = self.formatter.required_files(
            self, base, full, result)
        for f in required_files:
            require_file(f)

        system_count = 0
        if not skip_lily and not missing:
            system_count = int(open(full + '-systems.count', encoding="utf8").read())

        for number in range(1, system_count + 1):
            systemfile = '%s-%d' % (base, number)
            require_file(systemfile + '.eps')
            consider_file(systemfile + '.pdf')

            # We can't require signatures, since books and toplevel
            # markups do not output a signature.
            if 'ddump-signature' in self.global_options.process_cmd:
                consider_file(systemfile + '.signature')

        for f in  self.additional_files_to_consider(base, full):
            consider_file(f)

        for f in self.additional_files_required(base, full):
            require_file(f)

        return (result, missing)

    def is_outdated(self, output_dir):
        found, missing = self.all_output_files(output_dir)
        return missing

    def filter_pipe(self, input: bytes, cmd: str) -> bytes:
        """Pass input through cmd, and return the result.

        Args:
          input: the input
          cmd: a shell command

        Returns:
          the filtered result
        """
        debug(_("Running through filter `%s'") % cmd, True)

        closefds = True
        if sys.platform == "mingw32":
            closefds = False

        p = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=closefds)
        (stdin, stdout, stderr) = (p.stdin, p.stdout, p.stderr)
        stdin.write(input)
        status = stdin.close()

        if not status:
            status = 0
            output = stdout.read()
            status = stdout.close()

        # assume stderr always is text
        err = stderr.read().decode('utf-8')

        if not status:
            status = 0
        signal = 0x0f & status
        if status or (not output and err):
            exit_status = status >> 8
            ly.error(_("`%s' failed (%d)") % (cmd, exit_status))
            ly.error(_("The error log is as follows:"))
            sys.stderr.write(err)
            exit(status)

        debug('\n')

        return output

    def get_snippet_code(self) -> str:
        return self.substring('code')

    def filter_text(self):
        """Run snippet bodies through a command (say: convert-ly).
        """
        code = self.get_snippet_code().encode('utf-8')
        output = self.filter_pipe(code, self.global_options.filter_cmd)
        options = self.match.group('options')
        if options is None:
            options = ''
        d = {
            'code': output.decode('utf-8'),
            'options': options,
        }
        return self.formatter.output_simple_replacements(FILTER, d)

    def replacement_text(self):
        base = self.final_basename()
        return self.formatter.snippet_output(base, self)

    def get_images(self):
        base = self.final_basename()

        outdir = self.global_options.lily_output_dir
        single_base= '%s.png' % base
        single = os.path.join(outdir, single_base)
        multiple = os.path.join(outdir, '%s-page1.png' % base)
        images = (single_base,)
        if (os.path.exists(multiple)
            and (not os.path.exists(single)
                 or (os.stat(multiple)[stat.ST_MTIME]
                     > os.stat(single)[stat.ST_MTIME]))):
            count = ps_page_count(os.path.join(outdir, '%s.eps' % base))
            images = ['%s-page%d.png' % (base, page)
                      for page in range(1, count+1)]
            images = tuple(images)

        return images


re_begin_verbatim = re.compile(r'\s+%.*?begin verbatim.*\n*', re.M)
re_end_verbatim = re.compile(r'\s+%.*?end verbatim.*$', re.M)


class LilypondFileSnippet (LilypondSnippet):
    def __init__(self, type, match, formatter, line_number, global_options):
        LilypondSnippet.__init__(
            self, type, match, formatter, line_number, global_options)
        self.filename = self.substring('filename')
        self.contents = None

    def get_contents(self) -> bytes:
        if not self.contents:
            self.contents = open(book_base.find_file(self.filename,
                                                     self.global_options.include_path, self.global_options.original_dir), 'rb').read()
        return self.contents

    def get_snippet_code(self) -> str:
        return self.get_contents().decode('utf-8')

    def verb_ly(self):
        s = self.get_snippet_code()
        s = re_begin_verbatim.split(s)[-1]
        s = re_end_verbatim.split(s)[0]
        if not NOGETTEXT in self.option_dict:
            s = self.verb_ly_gettext(s)
        if not s.endswith('\n'):
            s += '\n'
        return s

    def ly(self):
        name = self.filename
        return ('\\sourcefilename \"%s\"\n\\sourcefileline 0\n%s'
                % (name, self.get_snippet_code()))

    def final_basename(self):
        if self.global_options.use_source_file_names:
            base = os.path.splitext(os.path.basename(self.filename))[0]
            return base
        else:
            return self.basename()


class MusicXMLFileSnippet (LilypondFileSnippet):
    def __init__(self, type, match, formatter, line_number, global_options):
        LilypondFileSnippet.__init__(
            self, type, match, formatter, line_number, global_options)
        self.compressed = False
        self.converted_ly = None
        self.ext = os.path.splitext(os.path.basename(self.filename))[1]
        self.musicxml_options_dict = {
            'verbose': '--verbose',
            'lxml': '--lxml',
            'compressed': '--compressed',
            'relative': '--relative',
            'absolute': '--absolute',
            'no-articulation-directions': '--no-articulation-directions',
            'no-rest-positions': '--no-rest-positions',
            'no-page-layout': '--no-page-layout',
            'no-beaming': '--no-beaming',
            'language': '--language',
        }

    def snippet_options(self):
        return list(self.musicxml_options_dict.keys())

    def convert_from_musicxml(self):
        name = self.filename
        xml2ly_option_list = []
        for (key, value) in list(self.option_dict.items()):
            cmd_key = self.musicxml_options_dict.get(key, None)
            if cmd_key is None:
                continue
            if value is None:
                xml2ly_option_list.append(cmd_key)
            else:
                xml2ly_option_list.append(cmd_key + '=' + value)
        if ('.mxl' in name) and ('--compressed' not in xml2ly_option_list):
            xml2ly_option_list.append('--compressed')
            self.compressed = True
        opts = " ".join(xml2ly_option_list)
        progress(_("Converting MusicXML file `%s'...") % self.filename)

        cmd = 'musicxml2ly %s --out=- - ' % opts
        ly_code = self.filter_pipe(self.get_contents(), cmd).decode('utf-8')
        return ly_code

    def ly(self):
        if self.converted_ly is None:
            self.converted_ly = self.convert_from_musicxml()
        name = self.filename
        return ('\\sourcefilename \"%s\"\n\\sourcefileline 0\n%s'
                % (name, self.converted_ly))

    def write_ly(self):
        base = self.basename()
        path = os.path.join(self.global_options.lily_output_dir, base)
        directory = os.path.split(path)[0]
        os.makedirs(directory, exist_ok=True)

        # First write the XML to a file (so we can link it!)
        if self.compressed:
            xmlfilename = path + '.mxl'
        else:
            xmlfilename = path + '.xml'
        if os.path.exists(xmlfilename):
            diff_against_existing = self.filter_pipe(
                self.get_contents(), 'diff -u %s - ' % xmlfilename)
            if diff_against_existing:
                warning(_("%s: duplicate filename but different contents of original file,\n\
printing diff against existing file.") % xmlfilename)
                sys.stderr.write(diff_against_existing.decode('utf-8'))
        else:
            out = open(xmlfilename, 'wb')
            out.write(self.get_contents())
            out.close()

        # also write the converted lilypond
        filename = path + '.ly'
        if os.path.exists(filename):
            encoded = self.full_ly().encode('utf-8')
            cmd = 'diff -u %s -' % filename
            diff_against_existing = self.filter_pipe(
                encoded, cmd).decode('utf-8')
            if diff_against_existing:
                warning(_("%s: duplicate filename but different contents of converted lilypond file,\n\
printing diff against existing file.") % filename)
                sys.stderr.write(diff_against_existing.decode('utf-8'))
        else:
            out = open(filename, 'w', encoding='utf-8')
            out.write(self.full_ly())
            out.close()


class LilyPondVersionString (Snippet):
    """A string that does not require extra memory."""

    def __init__(self, type, match, formatter, line_number, global_options):
        Snippet.__init__(self, type, match, formatter,
                         line_number, global_options)

    def replacement_text(self):
        return self.formatter.output_simple(self.type, self)


snippet_type_to_class = {
    'lilypond_file': LilypondFileSnippet,
    'lilypond_block': LilypondSnippet,
    'lilypond': LilypondSnippet,
    'include': IncludeSnippet,
    'lilypondversion': LilyPondVersionString,
    'musicxml_file': MusicXMLFileSnippet,
}

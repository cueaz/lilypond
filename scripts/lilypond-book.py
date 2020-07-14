#!@TARGET_PYTHON@
# -*- coding: utf-8 -*-

# This file is part of LilyPond, the GNU music typesetter.
#
# Copyright (C) 1998--2020  Han-Wen Nienhuys <hanwen@xs4all.nl>
#                           Jan Nieuwenhuizen <janneke@gnu.org>
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

r'''
Example usage:

test:
  lilypond-book --filter="tr '[a-z]' '[A-Z]'" BOOK

convert-ly on book:
  lilypond-book --filter="convert-ly --no-version --from=1.6.11 -" BOOK

classic lilypond-book:
  lilypond-book --process="lilypond" BOOK.tely

TODO:

  *  ly-options: intertext?
  *  --line-width?
  *  eps in latex / eps by lilypond -b ps?
  *  check latex parameters, twocolumn, multicolumn?
  *  use --png --ps --pdf for making images?

  *  Converting from lilypond-book source, substitute:
   @mbinclude foo.itely -> @include foo.itely
   \mbinput -> \input

'''


# TODO: Better solve the global_options copying to the snippets...

import codecs
import fcntl
from functools import reduce
import gettext
import glob
import hashlib
from optparse import OptionGroup
import os
import pipes
import re
import stat
import subprocess
import sys
import tempfile

import book_base
import book_docbook
import book_html
import book_latex
import book_texinfo
import book_snippets
import fontextract
import langdefs


"""
@relocate-preamble@
"""

# Load translation and install _() into Python's builtins namespace.
gettext.install('lilypond', '@localedir@')

import lilylib as ly

original_dir = os.getcwd()
backend = 'ps'

help_summary = (
    _("Process LilyPond snippets in hybrid HTML, LaTeX, texinfo or DocBook document.")
    + '\n\n'
    + _("Examples:")
    + '''
 $ lilypond-book --filter="tr '[a-z]' '[A-Z]'" %(BOOK)s
 $ lilypond-book -F "convert-ly --no-version --from=2.0.0 -" %(BOOK)s
 $ lilypond-book --process='lilypond -I include' %(BOOK)s
''' % {'BOOK': _("BOOK")})

authors = ('Jan Nieuwenhuizen <janneke@gnu.org>',
           'Han-Wen Nienhuys <hanwen@xs4all.nl>')

################################################################


def exit(i):
    if ly.is_verbose():
        raise Exception(_('Exiting (%d)...') % i)
    else:
        sys.exit(i)


progress = ly.progress
warning = ly.warning
error = ly.error

program_version = '@TOPLEVEL_VERSION@'
if program_version.startswith("@"):
    # '@' in lilypond-book output confuses texinfo
    program_version = "dev"


def identify():
    progress('%s (GNU LilyPond) %s' % (ly.program_name, program_version))


def warranty():
    identify()
    sys.stdout.write('''
%s

  %s

%s
%s
''' % (_('Copyright (c) %s by') % '2001--2020',
        '\n  '.join(authors),
        _("Distributed under terms of the GNU General Public License."),
        _("It comes with NO WARRANTY.")))


def get_option_parser():
    p = ly.get_option_parser(usage=_("%s [OPTION]... FILE") % 'lilypond-book',
                             description=help_summary,
                             conflict_handler="resolve",
                             add_help_option=False)

    p.add_option('-F', '--filter', metavar=_("FILTER"),
                 action="store",
                 dest="filter_cmd",
                 help=_(
                     "pipe snippets through FILTER [default: `convert-ly -n -']"),
                 default=None)

    p.add_option('-f', '--format',
                 help=_(
                     "use output format FORMAT (texi [default], texi-html, latex, html, docbook)"),
                 metavar=_("FORMAT"),
                 action='store')

    p.add_option("-h", "--help",
                 action="help",
                 help=_("show this help and exit"))

    p.add_option("-I", '--include', help=_("add DIR to include path"),
                 metavar=_("DIR"),
                 action='append', dest='include_path',
                 default=[])

    p.add_option('--info-images-dir',
                 help=_("format Texinfo output so that Info will "
                        "look for images of music in DIR"),
                 metavar=_("DIR"),
                 action='store', dest='info_images_dir',
                 default='')

    p.add_option('--left-padding',
                 metavar=_("PAD"),
                 dest="padding_mm",
                 help=_(
                     "pad left side of music to align music in spite of uneven bar numbers (in mm)"),
                 type="float",
                 default=3.0)

    p.add_option('--lily-loglevel',
                 help=_("Print lilypond log messages according to LOGLEVEL"),
                 metavar=_("LOGLEVEL"),
                 action='store', dest='lily_loglevel',
                 default=os.environ.get("LILYPOND_LOGLEVEL", None))

    p.add_option('--lily-output-dir',
                 help=_("write lily-XXX files to DIR, link into --output dir"),
                 metavar=_("DIR"),
                 action='store', dest='lily_output_dir',
                 default=None)

    p.add_option("-l", "--loglevel",
                 help=_("Print log messages according to LOGLEVEL "
                        "(NONE, ERROR, WARNING, PROGRESS (default), DEBUG)"),
                 metavar=_("LOGLEVEL"),
                 action='callback',
                 callback=ly.handle_loglevel_option,
                 type='string')

    p.add_option("-o", '--output', help=_("write output to DIR"),
                 metavar=_("DIR"),
                 action='store', dest='output_dir',
                 default='')

    p.add_option('-P', '--process', metavar=_("COMMAND"),
                 help=_("process ly_files using COMMAND FILE..."),
                 action='store',
                 dest='process_cmd', default='')

    p.add_option('--redirect-lilypond-output',
                 help=_("Redirect the lilypond output"),
                 action='store_true',
                 dest='redirect_output', default=False)

    p.add_option('-s', '--safe', help=_("Compile snippets in safe mode"),
                 action="store_true",
                 default=False,
                 dest="safe_mode")

    p.add_option('--skip-lily-check',
                 help=_("do not fail if no lilypond output is found"),
                 metavar=_("DIR"),
                 action='store_true', dest='skip_lilypond_run',
                 default=False)

    p.add_option('--skip-png-check',
                 help=_("do not fail if no PNG images are found for EPS files"),
                 metavar=_("DIR"),
                 action='store_true', dest='skip_png_check',
                 default=False)

    p.add_option('--use-source-file-names',
                 help=_(
                     "write snippet output files with the same base name as their source file"),
                 action='store_true', dest='use_source_file_names',
                 default=False)

    p.add_option('-V', '--verbose', help=_("be verbose"),
                 action="callback",
                 callback=ly.handle_loglevel_option,
                 callback_args=("DEBUG",))

    p.version = "@TOPLEVEL_VERSION@"
    p.add_option("--version",
                 action="version",
                 help=_("show version number and exit"))

    p.add_option('-w', '--warranty',
                 help=_("show warranty and copyright"),
                 action='store_true')

    group = OptionGroup(p, "Options only for the latex and texinfo backends")
    group.add_option('--latex-program',
                     help=_("run executable PROG instead of latex, or in\n\
case --pdf option is set instead of pdflatex"),
                     metavar=_("PROG"),
                     action='store', dest='latex_program',
                     default='latex')
    group.add_option('--texinfo-program',
                     help=_("run executable PROG instead of texi2pdf"),
                     metavar=_("PROG"),
                     action='store', dest='texinfo_program',
                     default='texi2pdf')
    group.add_option('--pdf',
                     action="store_true",
                     dest="create_pdf",
                     help=_("create PDF files for use with PDFTeX"),
                     default=False)
    p.add_option_group(group)

    p.add_option_group('',
                       description=(
                           _("Report bugs via %s")
                           % 'bug-lilypond@gnu.org') + '\n')

    for formatter in book_base.all_formats:
        formatter.add_options(p)

    return p


lilypond_binary = os.path.join('@bindir@', 'lilypond')

# If we are called with full path, try to use lilypond binary
# installed in the same path; this is needed in GUB binaries, where
# @bindir is always different from the installed binary path.
if 'bindir' in globals() and bindir:
    lilypond_binary = os.path.join(bindir, 'lilypond')

# Only use installed binary when we are installed too.
if '@bindir@' == ('@' + 'bindir@') or not os.path.exists(lilypond_binary):
    lilypond_binary = 'lilypond'

# Need to shell-quote, issue 3468

lilypond_binary = pipes.quote(lilypond_binary)

global_options = None


def command_name(cmd):
    # Strip all stuf after command,
    # deal with "((latex ) >& 1 ) .." too
    cmd = re.match(r'([\(\)]*)([^\\ ]*)', cmd).group(2)
    return os.path.basename(cmd)


def system_in_directory(cmd_str, directory, log_file):
    """Execute a command in a different directory."""

    if global_options.redirect_output:
        ly.progress(_("Processing %s.ly") % log_file)
    else:
        if ly.is_verbose():
            ly.progress(_("Invoking `%s\'") % cmd_str)
        else:
            name = command_name(cmd_str)
            ly.progress(_("Running %s...") % name)

    output_location = None
    if global_options.redirect_output:
        output_location = open(log_file + '.log', 'w')

    try:
        subprocess.run(cmd_str, stdout=output_location,
                       stderr=output_location, cwd=directory,
                       shell=True, check=True)
    except subprocess.CalledProcessError as e:
        sys.stderr.write("%s\n" % e)
        sys.exit(1)


def process_snippets(input_name, cmd, basenames,
                     formatter, lily_output_dir):
    """Run cmd on all of the .ly files from snippets."""

    # No need for a secure hash function, just need a digest.
    checksum = hashlib.md5()
    for name in basenames:
        checksum.update(name.encode('ascii'))
    checksum = checksum.hexdigest()

    lily_output_dir = global_options.lily_output_dir
    snippet_map_file = 'snippet-map-%s.ly' % checksum
    snippet_map_path = os.path.join(lily_output_dir, snippet_map_file)

    # Write snippet map.
    with open(snippet_map_path, 'w') as snippet_map:
        snippet_map.write("""

#(define version-seen #t)
#(define output-empty-score-list #f)
""")

        snippet_map.write("#(ly:add-file-name-alist '(\n")
        for name in basenames:
            snippet_map.write('("%s.ly" . "%s")\n' % (
                name.replace('\\', '/'), input_name))
        snippet_map.write('))\n')

    # Write list of snippet names.
    snippet_names_file = 'snippet-names-%s.ly' % checksum
    snippet_names_path = os.path.join(lily_output_dir, snippet_names_file)
    with open(snippet_names_path, 'w') as snippet_names:
        snippet_names.write('\n'.join(
            [snippet_map_file] + [name + '.ly' for name in basenames]))

    # Run command.
    cmd = formatter.adjust_snippet_command(cmd)
    # Remove .ly ending.
    logfile = os.path.splitext(snippet_names_path)[0]
    snippet_names_arg = mkarg(snippet_names_path.replace(os.path.sep, '/'))
    system_in_directory(' '.join([cmd, snippet_names_arg]),
                        lily_output_dir,
                        logfile)
    os.unlink(snippet_map_path)
    os.unlink(snippet_names_path)


def lock_path(name):
    if os.name != 'posix':
        return None

    fp = open(name, 'w')
    fcntl.lockf(fp, fcntl.LOCK_EX)
    return fp


def unlock_path(lock):
    if os.name != 'posix':
        return None
    fcntl.lockf(lock, fcntl.LOCK_UN)
    lock.close()


def do_process_cmd(chunks, input_name, options):
    """Wrap do_process_cmd_locked in a filesystem lock"""
    snippets = [c for c in chunks if isinstance(
        c, book_snippets.LilypondSnippet)]

    # calculate checksums eagerly
    for s in snippets:
        s.get_checksum()

    os.makedirs(options.lily_output_dir, exist_ok=True)
    lock_file = os.path.join(options.lily_output_dir, "lock")
    lock = None
    try:
        lock = lock_path(lock_file)
        do_process_cmd_locked(snippets, input_name, options)
    finally:
        if lock:
            unlock_path(lock)


def do_process_cmd_locked(snippets, input_name, options):
    """Look at all snippets, write the outdated ones, and compile them."""
    outdated = [c for c in snippets if c.is_outdated(options.lily_output_dir)]

    if outdated:
        # First unique the list based on the basename, by using them as keys
        # in a dict.
        outdated_dict = dict()
        for snippet in outdated:
            outdated_dict[snippet.basename()] = snippet

        # Next call write_ly() for each snippet once.
        progress(_("Writing snippets..."))
        for snippet in outdated_dict.values():
            snippet.write_ly()

        # Sort the keys / basenames to get a stable order.
        basenames = sorted(outdated_dict.keys())
        progress(_("Processing..."))
        process_snippets(input_name, options.process_cmd, basenames,
                         options.formatter, options.lily_output_dir)

    else:
        progress(_("All snippets are up to date..."))

    progress(_("Linking files..."))
    abs_lily_output_dir = os.path.join(
        options.original_dir, options.lily_output_dir)
    abs_output_dir = os.path.join(options.original_dir, options.output_dir)
    if abs_lily_output_dir != abs_output_dir:
        for snippet in snippets:
            snippet.link_all_output_files(abs_lily_output_dir,
                                          abs_output_dir)


###
# Format guessing data

def guess_format(input_filename):
    format = None
    e = os.path.splitext(input_filename)[1]
    for formatter in book_base.all_formats:
        if formatter.can_handle_extension(e):
            return formatter
    error(_("cannot determine format for: %s" % input_filename))
    exit(1)


def write_if_updated(file_name, lines):
    try:
        with open(file_name, encoding='utf-8') as file:
            old_str = file.read()
    except FileNotFoundError:
        pass
    else:
        new_str = ''.join(lines)
        if old_str == new_str:
            progress(_("%s is up to date.") % file_name)

            # this prevents make from always rerunning lilypond-book:
            # output file must be touched in order to be up to date
            os.utime(file_name, None)
            return

    output_dir = os.path.dirname(file_name)
    os.makedirs(output_dir, exist_ok=True)

    progress(_("Writing `%s'...") % file_name)
    codecs.open(file_name, 'w', 'utf-8').writelines(lines)


def note_input_file(name, inputs=[]):
    # hack: inputs is mutable!
    inputs.append(name)
    return inputs


def samefile(f1, f2):
    try:
        return os.path.samefile(f1, f2)
    except AttributeError:                # Windoze
        f1 = re.sub("//*", "/", f1)
        f2 = re.sub("//*", "/", f2)
        return f1 == f2


def do_file(input_filename, included=False):
    # Ugh.
    input_absname = input_filename
    if not input_filename or input_filename == '-':
        in_handle = sys.stdin
        input_fullname = '<stdin>'
    else:
        if os.path.exists(input_filename):
            input_fullname = input_filename
        else:
            input_fullname = global_options.formatter.input_fullname(
                input_filename)
        # Normalize path to absolute path, since we will change cwd to the output dir!
        # Otherwise, "lilypond-book -o out test.tex" will complain that it is
        # overwriting the input file (which it is actually not), since the
        # input filename is relative to the CWD...
        input_absname = os.path.abspath(input_fullname)

        note_input_file(input_fullname)
        in_handle = codecs.open(input_fullname, 'r', 'utf-8')

    if input_filename == '-':
        global_options.input_dir = os.getcwd()
        input_base = 'stdin'
    elif included:
        input_base = os.path.splitext(input_filename)[0]
    else:
        global_options.input_dir = os.path.split(input_absname)[0]
        input_base = os.path.basename(
            os.path.splitext(input_filename)[0])

    # don't complain when global_options.output_dir is existing
    if not global_options.output_dir:
        global_options.output_dir = os.getcwd()
    else:
        global_options.output_dir = os.path.abspath(global_options.output_dir)
        os.makedirs(global_options.output_dir, 0o777, exist_ok=True)
        os.chdir(global_options.output_dir)

    output_filename = os.path.join(global_options.output_dir,
                                   input_base + global_options.formatter.default_extension)
    if (os.path.exists(input_filename)
        and os.path.exists(output_filename)
            and samefile(output_filename, input_absname)):
        error(
            _("Output would overwrite input file; use --output."))
        exit(2)

    try:
        progress(_("Reading `%s'") % input_absname)
        source = in_handle.read()

        if not included:
            global_options.formatter.init_default_snippet_options(source)

        progress(_("Dissecting..."))
        chunks = book_base.find_toplevel_snippets(
            source, global_options.formatter, global_options)

        # Let the formatter modify the chunks before further processing
        chunks = global_options.formatter.process_chunks(chunks)

        if global_options.filter_cmd:
            write_if_updated(output_filename,
                             [c.filter_text() for c in chunks])
        elif global_options.process_cmd:
            do_process_cmd(chunks, input_fullname, global_options)
            progress(_("Compiling `%s'...") % output_filename)
            write_if_updated(output_filename,
                             [s.replacement_text()
                              for s in chunks])

        def process_include(snippet):
            os.chdir(original_dir)
            name = snippet.substring('filename')
            progress(_("Processing include `%s'") % name)
            return do_file(name, included=True)

        include_chunks = list(map(process_include,
                                  [x for x in chunks if isinstance(x, book_snippets.IncludeSnippet)]))

        return chunks + reduce(lambda x, y: x + y, include_chunks, [])

    except book_snippets.CompileError:
        os.chdir(original_dir)
        progress(_("Removing `%s'") % output_filename)
        raise book_snippets.CompileError


def adjust_include_path(path, outpath):
    """Rewrite an include path relative to the dir where lilypond is launched.
    Always use forward slashes since this is what lilypond expects."""
    path = os.path.expanduser(path)
    path = os.path.expandvars(path)
    path = os.path.normpath(path)
    if os.path.isabs(outpath):
        return os.path.abspath(path).replace(os.path.sep, '/')
    if os.path.isabs(path):
        return path.replace(os.path.sep, '/')
    return os.path.join(inverse_relpath(original_dir, outpath), path).replace(os.path.sep, '/')


def inverse_relpath(path, relpath):
    """Given two paths, the second relative to the first,
    return the first path relative to the second.
    Always use forward slashes since this is what lilypond expects."""
    if os.path.isabs(relpath):
        return os.path.abspath(path).replace(os.path.sep, '/')
    relparts = ['']
    parts = os.path.normpath(path).split(os.path.sep)
    for part in os.path.normpath(relpath).split(os.path.sep):
        if part == '..':
            relparts.append(parts[-1])
            parts.pop()
        else:
            relparts.append('..')
            parts.append(part)
    return '/'.join(relparts[::-1])


def do_options():
    global global_options

    opt_parser = get_option_parser()
    (global_options, args) = opt_parser.parse_args()

    global_options.information = {
        'program_version': program_version, 'program_name': ly.program_name}
    global_options.original_dir = original_dir

    if global_options.lily_output_dir:
        global_options.lily_output_dir = os.path.expanduser(
            global_options.lily_output_dir)
        for i, path in enumerate(global_options.include_path):
            global_options.include_path[i] = adjust_include_path(
                path, global_options.lily_output_dir)
        global_options.include_path.insert(0, inverse_relpath(
            original_dir, global_options.lily_output_dir))

    elif global_options.output_dir:
        global_options.output_dir = os.path.expanduser(
            global_options.output_dir)
        for i, path in enumerate(global_options.include_path):
            global_options.include_path[i] = adjust_include_path(
                path, global_options.output_dir)
        global_options.include_path.insert(
            0, inverse_relpath(original_dir, global_options.output_dir))

    global_options.include_path.insert(0, "./")

    if global_options.warranty:
        warranty()
        exit(0)
    if not args or len(args) > 1:
        opt_parser.print_help()
        exit(2)

    return args


def mkarg(x):
    r"""
    A modified version of the commands.mkarg(x)

    Uses double quotes (since Windows can't handle the single quotes)
    and escapes the characters \, $, ", and ` for unix shells.
    """
    if os.name == 'nt':
        return ' "%s"' % x
    s = ' "'
    for c in x:
        if c in '\\$"`':
            s = s + '\\'
        s = s + c
    s = s + '"'
    return s


def main():
    if ("LILYPOND_BOOK_LOGLEVEL" in os.environ):
        ly.set_loglevel(os.environ["LILYPOND_BOOK_LOGLEVEL"])
    files = do_options()

    basename = os.path.splitext(files[0])[0]
    basename = os.path.split(basename)[1]

    if global_options.format:
        # Retrieve the formatter for the given format
        for formatter in book_base.all_formats:
            if formatter.can_handle_format(global_options.format):
                global_options.formatter = formatter
    else:
        global_options.formatter = guess_format(files[0])
        global_options.format = global_options.formatter.format

    # make the global options available to the formatters:
    global_options.formatter.global_options = global_options
    formats = global_options.formatter.image_formats

    if global_options.process_cmd == '':
        global_options.process_cmd = (lilypond_binary
                                      + ' --formats=%s -dbackend=eps ' % formats)

    if global_options.process_cmd:
        includes = global_options.include_path
        global_options.process_cmd += ' '.join([' -I %s' % mkarg(p)
                                                for p in includes])

    global_options.formatter.process_options(global_options)

    if global_options.lily_loglevel:
        ly.debug_output(_("Setting LilyPond's loglevel to %s") %
                        global_options.lily_loglevel, True)
        global_options.process_cmd += " --loglevel=%s" % global_options.lily_loglevel
    elif ly.is_verbose():
        if os.environ.get("LILYPOND_LOGLEVEL", None):
            ly.debug_output(_("Setting LilyPond's loglevel to %s (from environment variable LILYPOND_LOGLEVEL)") %
                            os.environ.get("LILYPOND_LOGLEVEL", None), True)
            global_options.process_cmd += " --loglevel=%s" % os.environ.get(
                "LILYPOND_LOGLEVEL", None)
        else:
            ly.debug_output(
                _("Setting LilyPond's output to --verbose, implied by lilypond-book's setting"), True)
            global_options.process_cmd += " --verbose"

    if global_options.padding_mm:
        global_options.process_cmd += " -deps-box-padding=%f " % global_options.padding_mm

    global_options.process_cmd += " -dread-file-list -dno-strip-output-dir"

    if global_options.lily_output_dir:
        global_options.lily_output_dir = os.path.abspath(
            global_options.lily_output_dir)
    else:
        global_options.lily_output_dir = os.path.abspath(
            global_options.output_dir)

    relative_output_dir = global_options.output_dir

    identify()
    try:
        chunks = do_file(files[0])
    except book_snippets.CompileError:
        exit(1)

    inputs = note_input_file('')
    inputs.pop()

    base_file_name = os.path.splitext(os.path.basename(files[0]))[0]
    dep_file = os.path.join(global_options.output_dir, base_file_name + '.dep')
    final_output_file = os.path.join(relative_output_dir,
                                     base_file_name + global_options.formatter.default_extension)

    os.chdir(original_dir)
    open(dep_file, 'w').write('%s: %s\n'
                              % (final_output_file, ' '.join(inputs)))


if __name__ == '__main__':
    main()

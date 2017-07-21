import sys
import os
import os.path

from waflib import Configure, Logs, TaskGen

APPNAME = 'handel'
VERSION = '1.0'

windows = os.name == 'nt'

top = '.'

# Allow overriding the configure/build directory by setting WAFLOCK=.lock-wafFOO.
if not os.getenv('WAFLOCK'):
    out = 'build-' + sys.platform

def options(opt):
    opt.add_option('--autoconfigure', action='callback', callback=set_autoconfigure,
                   help='Call configure automatically when needed.')
    opt.add_option('--sitoro', action='store', dest='sitoro_path', default='',
                   help='Set the path to the SiToro release.')
    opt.add_option('--release', action='store_true', dest='releasing', default=False,
                   help='Update xia_version.h with version control info.')
    opt.add_option('--show-commands', action='store_true', dest='show_commands', default=False,
                   help='Print the commands as strings.')
    opt.load('compiler_c')


#
# Configure the variants.
#
def configure(conf):

    #
    # Provide a way for the user to get the full traditional output.
    #
    conf.env.SHOW_COMMANDS = 'yes' if conf.options.show_commands else 'no'

    #
    # Check the user's sitoro option and, if set, get the use variables.
    #
    conf.read_sitoro()

    #
    # Find ruby. Configure our version script.
    #
    conf.env.RUBY = conf.find_program('ruby')
    conf.env.GVER_FLAGS = ['']
    conf.env.GVER = os.path.join(conf.srcnode.abspath(),
                                 'build_helpers/gen_version.rb')

    #
    # Set up some msvc-specific variables to make the compiler work like we need.
    #
    if windows:
        conf.env['MSVC_LAZY_AUTODETECT'] = True

        # We needed 12+ for C99-style inline initializers, now 14 for snprintf.
        conf.env['MSVC_VERSIONS'] = ['msvc 14.0']

        # Force x86 for linking with the DLL
        if 'LIB_sitoro' in conf.env:
            conf.env['MSVC_TARGETS'] = ['x86']

    #
    # Get the flags.
    #
    conf.load('compiler_c')
    flags = get_flags(conf)
    if windows:
        # Make winsock available to 'use'
        conf.check_cc(lib='ws2_32')
    else:
        flags = append_flags(flags, 'lib', ['m'])

    env = conf.env

    for name in ['debug', 'release']:
        conf.setenv(name, env)
        conf.msg('Variant', name, 'YELLOW')

        conf.env.BUILD = name

        conf.env.CFLAGS = flags['cflags'][name]
        conf.env.WARNINGS = flags['warnings'][name]
        conf.env.LINKFLAGS = flags['linkflags'][name]
        conf.env.LIB = flags['lib'][name]
        conf.env.LIBPATH = flags['libpath'][name]

#
# Create a tags file. Use "waf tags".
#
# I suspect this will not work on Windows due to the shell specifics.
#
def tags(ctx):
    ctx.exec_command('etags $(find . -name \*.[ch])', shell = True)

#
# Make a distribution package.
#
def dist(ctx):
        ctx.base_name = 'handel_1_0'
        ctx.algo      = 'tar.bz2'
        ctx.excl      = ' **/.waf-1* **/*~ **/*.hg* **/.lock-w*'

#
# Build the variants.
#
def build(bld):
    if bld.env.SHOW_COMMANDS == 'yes':
        output_command_line()

    # Normally dirs have to be set in configure, but we can check the
    # options here, too, for the install command.
    bld.env.BINDIR = bld.options.bindir or bld.env.BINDIR
    bld.env.LIBDIR = bld.options.libdir or bld.env.LIBDIR
    bld.env.IMPLIBDIR = getattr(bld.env, 'IMPLIBDIR', bld.env.LIBDIR)

    sinc_src = 'libsinc-c/'
    src = 'src/'

    if windows:
        threads = [src + 'handel_md_threads_windows.c']
        sockets = ['WS2_32']
    else:
        threads = [src + 'handel_md_threads_posix.c']
        sockets = []

    defines = ['HANDEL_MAKE_DLL=1']
    if windows:
        defines += ['_CRT_SECURE_NO_WARNINGS', 'WIN32']

    includes = ['.', 'inc', 'libsinc-c']

    bld.env.GVER_FLAGS = '--release' if bld.options.releasing else ''
    bld(rule      = '${RUBY} ${GVER} ${GVER_FLAGS} ${SRC} ${TGT}',
        target   = 'xia_version.h',
        source   = 'tools/releaser/version.yml')

    if 'LIB_sitoro' in bld.env:
        bld.install_files('${BINDIR}', ['bin/sitoro-3.dll', 'bin/libusb-1.0.dll'],
                          cwd=bld.root.find_dir(bld.env.SITORO_PATH))
    else:
        # Build the stub if we didn't get sitoro from the command line.
        bld(features = 'c cshlib',
            name     = 'sitoro',
            target   = 'sitoro-3',
            source   = [src + 'sitoro-stub.c'],
            defines  = defines + ['SITORO_EXPORTS'],
            includes = includes)

    bld(features = 'c cstlib',
        target   = 'sinc',
        source = [sinc_src + 'api.c',
                  sinc_src + 'blocking.c',
                  sinc_src + 'command.c',
                  sinc_src + 'decode.c',
                  # sinc_src + 'discovery.c',
                  sinc_src + 'encapsulation.c',
                  sinc_src + 'encode.c',
                  sinc_src + 'readmessage.c',
                  sinc_src + 'request.c',
                  sinc_src + 'sinc.pb-c.c',
                  sinc_src + 'socket.c',
                  sinc_src + 'protobuf-c.c'],
        defines  = defines,
        includes = includes,
        use      = sockets)

    bld(features = 'c cshlib',
        target   = 'handel',
        source   = [src + 'handel.c',
                    src + 'handel_detchan.c',
                    src + 'handel_dyn_detector.c',
                    src + 'handel_dyn_module.c',
                    src + 'handel_file.c',
                    src + 'handel_run_params.c',
                    src + 'handel_system.c',
                    src + 'handel_dbg.c',
                    src + 'handel_dyn_default.c',
                    src + 'handel_dyn_firmware.c',
                    src + 'handel_fdd_shim.c',
                    src + 'handel_log.c',
                    src + 'handel_run_control.c',
                    src + 'handel_sort.c',
                    src + 'xia_assert.c',
                    src + 'xia_file.c',
                    src + 'md_shim.c',
                    src + 'falconx_mm.c',
                    src + 'falconx_psl.c',
                    src + 'falconxn_psl.c',
                    src + 'psl.c'] + threads,
        cflags = bld.env['WARNINGS'],
        defines  = defines,
        includes = includes,
        use      = ['sitoro', 'sinc', 'xia_version.h'])

    tests = ['hd-board-info',
             'hd-serial-num',
             'hd-det-characterize',
             'hd-dc-pulses',
             'hd-adc-trace',
             'hd-mca',
             'hd-mca-preset',
             'hd-mm1',
             'hd-mm1-trace',
             'hd-run-spec',
             'hd-get-acq',
             'hd-set-acq',
             'hd-save-system',
             'hd-sca',
             'hd-connected']
    for t in tests:
        test(bld, includes, t, ['tests/c/%s.c' % (t)])

#
# Test program.
#
def test(bld, includes, target, source):
    if windows:
        defines = ['_CRT_SECURE_NO_WARNINGS', 'WIN32']
    else:
        defines = []

    bld(features = 'cprogram c',
        target   = target,
        source   = source,
        includes = includes,
        cflags   = bld.env['WARNINGS'],
        defines  = defines,
        use = ['handel'])

#
# Get the build flags.
#
def get_flags(ctx):
    if windows:
        return msvc_get_flags(ctx)
    return cc_get_flags(ctx)

#
#
#
def append_flags(flags, flag, opts):
    for name in ['debug', 'release']:
        flags[flag][name] += opts
    return flags

#
# Get the build flags.
#
def cc_get_flags(ctx):
    #
    # Generate the flags for the platforms and variants.
    #
    common_cflags = ['-pipe']

    pcf = []
    pcw = ['-Wall']
    plf = []
    plib = []
    plp = []

    if sys.platform == 'darwin':
        plf = ['-Wl,-framework,IOKit',
               '-Wl,-framework,CoreFoundation',
               '-Wl,-prebind']
    else:
        pcf = ['-fPIC']
        plf = ['-w']
        plib = ['util', 'rt', 'pthread', 'm']
        if sys.platform.startswith('linux'):
            plib += ['dl']

    if ctx.env['CC_NAME'] == 'clang':
        pcw += ['-Weverything',
                '-Wno-padded',
                '-Wno-documentation-unknown-command',
                '-Wno-float-equal',
                '-Wno-covered-switch-default']

    flags = { 'cflags'   : { 'debug'  : common_cflags + ['-g', '-O0'] + pcf,
                             'release': common_cflags + ['-O2'] + pcf },
              'warnings' : { 'debug'  : pcw,
                             'release': pcw },
              'linkflags': { 'debug'  : ['-g'] + plf,
                             'release': [] + plf },
              'lib'      :  { 'debug'  : plib,
                              'release': plib },
              'libpath'  :  { 'debug'  : plp,
                              'release': plp } }

    return flags

def msvc_get_flags(ctx):
    #
    # Generate the flags for the platforms and variants.
    #

    if ctx.env['CC_NAME'] and ctx.env['CC_NAME'] == 'gcc':
        common_cflags = []
        pcfd = ['-g', '-O0']
        pcfr = ['-O2']
        pcw = ['-Wall']
        plf = ['-g', '-w']
        plib = ['m']
        plp = []
    else:
        common_cflags = ['/Zi', '/nologo', '/FS']
        pcfd = ['/MDd']
        pcfr = ['/MD']
        pcw = ['/W4']
        plf = ['/MANIFEST', '/nologo', '/DEBUG', '/INCREMENTAL:NO']
        plib = []

        # Keep the tools libpath discovered by msvc autodetect, for system libs like ws2_32.
        plp = ctx.env['LIBPATH']

    flags = { 'cflags'   : { 'debug'  : common_cflags + pcfd,
                             'release': common_cflags + pcfr },
              'warnings' : { 'debug'  : pcw,
                             'release': pcw },
              'linkflags': { 'debug'  : plf,
                             'release': plf },
              'lib'      : { 'debug'  : plib,
                             'release': plib },
              'libpath'  : { 'debug'  : plp,
                             'release': plp } }

    return flags

#
# Custom optparse handler to set autoconfig from the command line options.
#
def set_autoconfigure(option, opt, value, parser):
    Configure.autoconfig = True

#
# Parse the sitoro option and set the cc uselib variables if the library
# enabled.
#
# Usage:
# bld(features = 'c cshlib',
#     target   = 'mylib',
#     source   = ['mylib.c'],
#     includes = ['inc'], # sitoro.h is patched here
#     use      = ['sitoro'])
#
@Configure.conf
def read_sitoro(ctx):
    ctx.start_msg('Checking for sitoro')
    if len(ctx.options.sitoro_path):
        if ctx.options.sitoro_path == 'disable':
            sitoro_path = None
            ctx.end_msg('disabled')
        else:
            sitoro_path = os.path.join(ctx.options.sitoro_path)
    elif windows:
        sitoro_path = os.path.join(ctx.srcnode.abspath(), 'redist')
    else:
        sitoro_path = None

    if sitoro_path is not None:
        # This isn't a 'use' variable, but we need it for installing the binaries
        ctx.env.SITORO_PATH = sitoro_path

        ctx.env.LIB_sitoro = 'sitoro-3'
        ctx.env.LIBPATH_sitoro = [os.path.join(sitoro_path, 'lib')]

        # OEM INCLUDES are not set since we are still tracking ./inc/sitoro.h.
        #ctx.env.INCLUDES_sitoro = [os.path.join(sitoro_path, 'include')]

    ctx.end_msg(sitoro_path)

@TaskGen.feature('c', 'cxx')
@TaskGen.after_method('process_source')
def apply_unique_pdb(self):
    # For msvc, specify a unique compile pdb per target, to work around
    # LNK4099 in case multiple independent targets build in the same
    # directory, e.g. test programs. CFLAGS are updated with a unique
    # /Fd flag based on the target name. This is separate from the link pdb.

    if self.env.CC_NAME != 'msvc':
        return

    debug = False
    fd = False
    for f in self.env['CFLAGS']:
        fl = f.lower()
        if fl[1:].lower() == 'zi': # /Zi, /ZI: yes, debug
            debug = True
        elif fl[1:3] == 'fd':      # /Fd, file or directory already provided
            fd = True

    if debug and not fd:
        pdb = "%s.vc.pdb" % self.get_name()
        self.env.append_value('CFLAGS', '/Fd%s' % pdb)

#
# From http://code.google.com/p/waf/source/browse/demos/variants/wscript
#
def init(ctx):
    from waflib.Build import BuildContext, CleanContext, InstallContext, UninstallContext

    for x in ['debug', 'release']:
        for y in (BuildContext, CleanContext, InstallContext, UninstallContext):
            name = y.__name__.replace('Context','').lower()
            class tmp(y):
                cmd = name + '_' + x
                variant = x

    def buildall(ctx):
        import waflib.Options
        for x in ['build_debug', 'build_release']:
            waflib.Options.commands.insert(0, x)

    ## if you work on "debug" 99% of the time, here is how to re-enable "waf build":
    for y in (BuildContext, CleanContext, InstallContext, UninstallContext):
        class tmp(y):
            variant = 'debug'

#
# From the demos. Use this to get the command to cut+paste to play.
#
def output_command_line():
    # first, display strings, people like them

    from waflib import Utils, Logs
    from waflib.Context import Context
    def exec_command(self, cmd, **kw):
        subprocess = Utils.subprocess
        kw['shell'] = isinstance(cmd, str)
        if isinstance(cmd, str):
            Logs.info('%s' % cmd)
        else:
            Logs.info('%s' % ' '.join(cmd)) # here is the change
        Logs.debug('runner_env: kw=%s' % kw)
        try:
            if self.logger:
                self.logger.info(cmd)
                kw['stdout'] = kw['stderr'] = subprocess.PIPE
                p = subprocess.Popen(cmd, **kw)
                (out, err) = p.communicate()
                if out:
                    self.logger.debug('out: %s' % out.decode(sys.stdout.encoding or 'iso8859-1'))
                if err:
                    self.logger.error('err: %s' % err.decode(sys.stdout.encoding or 'iso8859-1'))
                return p.returncode
            else:
                p = subprocess.Popen(cmd, **kw)
                return p.wait()
        except OSError:
            return -1
    Context.exec_command = exec_command

    # Change the outputs for tasks too

    from waflib.Task import Task
    def display(self):
        return '' # no output on empty strings

    Task.__str__ = display
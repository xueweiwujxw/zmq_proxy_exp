#!/usr/bin/python3

import glob
from os import path
from os import walk
from pathlib import Path
import git
import time

VERSION = '0.1.0'


def has_c_cpp_source_file(directory):
    c_cpp_source_ext = {'.c', '.cpp', '.cc', '.cxx'}
    file_paths = []
    for root, _, files in walk(directory):
        for file in files:
            if Path(file).suffix in c_cpp_source_ext:
                full_path = path.join(root, file)
                file_paths.append(full_path)

    return file_paths


def options(opt):
    opt.load('compiler_c')
    opt.load('compiler_cxx')
    repo = git.Repo(search_parent_directories=True)
    try:
        sha = repo.git.rev_parse(repo.head.commit.hexsha, short=8)
    except:
        sha = 0
    stamp = int(time.time())
    opt.add_option('--hash', action='store', default=sha,
                   help='git commit hash value, default: current commit hash')
    opt.add_option('--debug', action='store_true',
                   default=False, help='enable debug mode, default: False')
    opt.add_option('--stamp', action='store',
                   default=stamp, help='configure time, default: current timestamp')
    opt.add_option('--mode', action='store',
                   default='develop', help='test: test mode, develop: development mode, product: production mode, default: develop')


def build(bld):
    defines = [
        f'VERSION="{VERSION}"',
        f'STAMP="{bld.env.stamp}"',
        f'HASH="{bld.env.hash}"',
    ]
    if bld.env.debug:
        defines.append('DEBUG')

    built_libs = []

    for lib in glob.glob('lib/**'):
        if path.isdir(lib):
            lib_src_files = has_c_cpp_source_file(lib)
            lib_name = path.basename(lib)
            if lib_src_files.__len__() != 0:
                bld.stlib(
                    source=lib_src_files,
                    includes=[lib_header for lib_header in glob.glob(
                        f'{lib}/**/', recursive=True)],
                    vnum=VERSION,
                    defines=defines,
                    target=lib_name)
                built_libs.append(lib_name)

    includepath = [src_dir for src_dirs in ['src', 'lib']
                   for src_dir in glob.glob(f'{src_dirs}/**/', recursive=True)]

    src_files = has_c_cpp_source_file('src/')
    if src_files.__len__() != 0:
        bld.stlib(
            source=glob.glob('src/**/*.c*', recursive=True),
            use=built_libs,
            lib=['zmq', 'pthread'],
            includes=includepath,
            vnum=VERSION,
            defines=defines,
            target='srclib'
        )
        built_libs.append('srclib')

    apptargets = glob.glob('app/*.c*')
    for app in apptargets:
        appname = path.splitext(path.basename(app))[0]
        bld.program(
            source=app,
            use=built_libs,
            includes=includepath,
            rpath='$ORIGIN',
            lib=['zmq', 'pthread'],
            target=f'dist/app/{appname}',
            defines=defines
        )


def configure(ctx):
    ctx.load('g++ gcc')
    ctx.env.target = 'host'
    ctx.env.hash = ctx.options.hash
    ctx.env.debug = ctx.options.debug
    ctx.env.stamp = ctx.options.stamp
    ctx.env.mode = ctx.options.mode
    cxxflags = ['-Wall', '-Wextra', '-std=c++17', '-Wno-unused-parameter']
    if ctx.env.debug:
        cxxflags.append('-g')
    else:
        cxxflags.append('-O3')
    ctx.env.append_value('CXXFLAGS', cxxflags)

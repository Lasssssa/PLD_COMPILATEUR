#!/usr/bin/env python3

import argparse
import glob
import os
import shutil
import sys
import subprocess
import textwrap

################################################################################
## SETUP & ARGPARSE

width = shutil.get_terminal_size().columns - 2
twf = lambda text: textwrap.fill(text, width, initial_indent=' ' * 4, subsequent_indent=' ' * 6)

argparser = argparse.ArgumentParser(
    formatter_class=argparse.RawDescriptionHelpFormatter,
    description="Testing script for the ifcc compiler. operates in one of two modes:\n\n"
                + twf("- Multiple-files mode (by default): Compile several programs with both GCC and IFCC, run them, and compare the results.") + "\n\n"
                + twf("- Single-file mode (with options -o,-c and/or -S): Compile and/or assemble and/or link a single program."),
    epilog="examples:\n\n"
           + twf("python3 ifcc-test.py testfiles") + '\n'
           + twf("python3 ifcc-test.py path/to/some/dir/*.c path/to/some/other/dir") + '\n\n'
           + twf("python3 ifcc-test.py -o ./myprog path/to/some/source.c") + '\n'
           + twf("python3 ifcc-test.py -S -o truc.s truc.c"),
)

argparser.add_argument('input', metavar='PATH', nargs='+', help='Path(s) to .c files or directories containing them')
argparser.add_argument('-v', '--verbose', action="count", default=0, help='increase verbosity level')
argparser.add_argument('-d', '--debug', action="count", default=0, help='debug mode')
argparser.add_argument('-S', action="store_true", help='compile to assembly only')
argparser.add_argument('-c', action="store_true", help='compile to object file only')
argparser.add_argument('-o', '--output', metavar='OUTPUTNAME', help='name of output file')

args = argparser.parse_args()
orig_cwd = os.getcwd()

if args.debug >= 2:
    print('debug: command-line arguments', args)

# Resolve base dir
pld_base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
IFCC = os.path.join(pld_base_dir, 'compiler', 'ifcc')

if args.debug:
    print("Base dir =", pld_base_dir)
    print("IFCC path =", IFCC)

if not os.path.isfile(IFCC) or not os.access(IFCC, os.X_OK):
    print(f"error: compiler/ifcc not found or not executable at: {IFCC}")
    sys.exit(127)

def run_command(string, logfile=None, toscreen=False):
    """Execute `string` as shell command. Optionally write output to `logfile` and/or stdout."""
    if args.debug:
        print("CMD:", string)

    process = subprocess.Popen(string, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    with open(logfile, 'w') if logfile else open(os.devnull, 'w') as log:
        for line in process.stdout:
            log.write(line)
            if toscreen:
                sys.stdout.write(line)

        process.wait()
        log.write(f'\nexit status: {process.returncode}\n')

    return process.returncode

def dumpfile(name, quiet=False):
    with open(name, "rb") as f:
        data = f.read().decode('utf-8', errors='ignore')
    if not quiet:
        print(data, end='')
    return data

# Safety: don't run inside output dir
if "ifcc-test-output" in orig_cwd:
    print("error: cannot run ifcc-test.py from within its own output directory")
    sys.exit(1)

# Cleanup previous test output
test_output_dir = os.path.join(pld_base_dir, 'ifcc-test-output')
if os.path.isdir(test_output_dir):
    shutil.rmtree(test_output_dir)

# Rebuild ifcc
if run_command(f'cd {pld_base_dir}/compiler && make --question ifcc'):
    if run_command(f'cd {pld_base_dir}/compiler && make ifcc', toscreen=True):
        print("error: compilation of ifcc failed.")
        sys.exit(1)

############################################
## SINGLE-FILE MODE

if args.S or args.c or args.output:
    if args.S and args.c:
        print("error: options -S and -c are not compatible")
        sys.exit(1)
    if len(args.input) > 1:
        print("error: only one input file allowed in single-file mode")
        sys.exit(1)

    inputfile = args.input[0]
    if not inputfile.endswith(".c"):
        print("error: input file must have .c extension")
        sys.exit(1)
    if not os.path.exists(inputfile):
        print(f"error: input file does not exist: {inputfile}")
        sys.exit(1)

    if (args.S or args.c) and not args.output:
        print("error: -o is required when using -S or -c")
        sys.exit(1)

    if args.S:
        if not args.output.endswith(".s"):
            print("error: output file must end with .s")
            sys.exit(1)
        sys.exit(run_command(f'{IFCC} {inputfile} > {args.output}', toscreen=True))

    if args.c:
        if not args.output.endswith(".o"):
            print("error: output file must end with .o")
            sys.exit(1)
        asm = args.output.replace(".o", ".s")
        if run_command(f'{IFCC} {inputfile} > {asm}', toscreen=True):
            sys.exit(1)
        sys.exit(run_command(f'gcc -c -o {args.output} {asm}', toscreen=True))

    if args.output.endswith((".o", ".s", ".c")):
        print("error: executable cannot end with .o, .s, or .c")
        sys.exit(1)

    asm = args.output + ".s"
    if run_command(f'{IFCC} {inputfile} > {asm}', toscreen=True):
        sys.exit(1)
    sys.exit(run_command(f'gcc -o {args.output} {asm}', toscreen=True))

############################################
## MULTI-FILE MODE

inputfiles = []
for path in args.input:
    norm = os.path.normpath(path)
    if os.path.isfile(norm) and norm.endswith('.c'):
        inputfiles.append(norm)
    elif os.path.isdir(norm):
        for dirpath, _, filenames in os.walk(norm):
            if 'ifcc-test-output' in dirpath:
                continue
            for f in filenames:
                if f.endswith(".c"):
                    inputfiles.append(os.path.join(dirpath, f))
    else:
        print(f"error: cannot read input path {path}")
        sys.exit(1)

inputfiles = sorted(set(inputfiles))
if not inputfiles:
    print("error: no .c test-cases found.")
    sys.exit(1)

os.mkdir(test_output_dir)
jobs = []

for f in inputfiles:
    rel = os.path.relpath(f, start=pld_base_dir)
    jobname = rel.replace('..', '-').replace('./', '').replace('/', '-').replace('.c', '')
    jobdir = os.path.join(test_output_dir, jobname)
    os.mkdir(jobdir)
    shutil.copyfile(f, os.path.join(jobdir, "input.c"))
    jobs.append(jobname)

all_ok = True

for job in jobs:
    os.chdir(os.path.join(test_output_dir, job))
    print(f"TEST-CASE: {job}")

    gcc_ok = run_command("gcc -S -o asm-gcc.s input.c", "gcc-compile.txt") == 0
    if gcc_ok:
        gcc_ok = run_command("gcc -o exe-gcc asm-gcc.s", "gcc-link.txt") == 0
    if gcc_ok:
        run_command("./exe-gcc", "gcc-execute.txt")
        if args.verbose >= 2:
            dumpfile("gcc-execute.txt")

    ifcc_ok = run_command(f'{IFCC} input.c > asm-ifcc.s', "ifcc-compile.txt") == 0

    if not gcc_ok and not ifcc_ok:
        print("TEST OK")
        continue
    if not gcc_ok and ifcc_ok:
        print("TEST FAIL (your compiler accepts an invalid program)")
        all_ok = False
        continue
    if gcc_ok and not ifcc_ok:
        print("TEST FAIL (your compiler rejects a valid program)")
        all_ok = False
        if args.verbose:
            dumpfile("asm-ifcc.s")
            dumpfile("ifcc-compile.txt")
        continue

    if run_command("gcc -o exe-ifcc asm-ifcc.s", "ifcc-link.txt"):
        print("TEST FAIL (your compiler produced invalid assembly)")
        all_ok = False
        if args.verbose:
            dumpfile("asm-ifcc.s")
            dumpfile("ifcc-link.txt")
        continue

    run_command("./exe-ifcc", "ifcc-execute.txt")
    if open("gcc-execute.txt").read() != open("ifcc-execute.txt").read():
        print("TEST FAIL (different outputs at runtime)")
        all_ok = False
        if args.verbose:
            print("GCC:")
            dumpfile("gcc-execute.txt")
            print("You:")
            dumpfile("ifcc-execute.txt")
        continue

    print("TEST OK")

if not all_ok and not args.verbose:
    print("Some test-cases failed. Use --verbose for more information.")

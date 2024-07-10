#!/bin/bash
builddir=`dirname "$0"`
GCONV_PATH="${builddir}/iconvdata"

usage () {
  echo "usage: $0 [--tool=strace] PROGRAM [ARGUMENTS...]" 2>&1
  echo "       $0 --tool=valgrind PROGRAM [ARGUMENTS...]" 2>&1
  exit 1
}

toolname=default
while test $# -gt 0 ; do
  case "$1" in
    --tool=*)
      toolname="${1:7}"
      shift
      ;;
    --*)
      usage
      ;;
    *)
      break
      ;;
  esac
done

if test $# -eq 0 ; then
  usage
fi

case "$toolname" in
  default)
    exec   env GCONV_PATH="${builddir}"/iconvdata LOCPATH="${builddir}"/localedata LC_ALL=C  "${builddir}"/elf/ld-linux-x86-64.so.2 --library-path "${builddir}":"${builddir}"/math:"${builddir}"/elf:"${builddir}"/dlfcn:"${builddir}"/nss:"${builddir}"/nis:"${builddir}"/rt:"${builddir}"/resolv:"${builddir}"/mathvec:"${builddir}"/support:"${builddir}"/crypt:"${builddir}"/nptl ${1+"$@"}
    ;;
  strace)
    exec strace  -EGCONV_PATH=/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/iconvdata  -ELOCPATH=/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/localedata  -ELC_ALL=C  /home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/elf/ld-linux-x86-64.so.2 --library-path /home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/math:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/elf:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/dlfcn:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nss:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nis:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/rt:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/resolv:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/mathvec:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/support:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/crypt:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nptl ${1+"$@"}
    ;;
  valgrind)
    exec env GCONV_PATH=/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/iconvdata LOCPATH=/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/localedata LC_ALL=C valgrind  /home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/elf/ld-linux-x86-64.so.2 --library-path /home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/math:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/elf:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/dlfcn:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nss:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nis:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/rt:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/resolv:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/mathvec:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/support:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/crypt:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nptl ${1+"$@"}
    ;;
  container)
    exec env GCONV_PATH=/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/iconvdata LOCPATH=/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/localedata LC_ALL=C  /home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/elf/ld-linux-x86-64.so.2 --library-path /home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/math:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/elf:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/dlfcn:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nss:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nis:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/rt:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/resolv:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/mathvec:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/support:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/crypt:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nptl /home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/support/test-container env GCONV_PATH=/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/iconvdata LOCPATH=/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/localedata LC_ALL=C  /home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/elf/ld-linux-x86-64.so.2 --library-path /home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/math:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/elf:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/dlfcn:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nss:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nis:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/rt:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/resolv:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/mathvec:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/support:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/crypt:/home/LLVM_MYSQL/knob_profiler/glibcForPRELOAD/glibc-2.31/build/nptl ${1+"$@"}
    ;;
  *)
    usage
    ;;
esac

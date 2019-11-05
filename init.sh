#!/bin/sh

set -eu

BASE=$(cd "$(dirname "$0")"; pwd)

if [ ! -x "$BASE/bin/ebridge.exe" ]; then
  echo "ebridge.exe not found or is not executable: '$BASE/bin/ebridge.exe'" >&2
  exit 1
fi

case ${1:-sh} in

(sh) cat<<HERE
export EOPEN_ROOT="$BASE"
alias eopen='sh "$BASE/wsl/eopen.sh"'
alias ewd='sh "$BASE/wsl/ewd.sh"'
alias ecd='. "$BASE/wsl/ecd.sh"'
if type pushd >/dev/null 2>&1; then
  alias epushd='. "$BASE/wsl/epushd.sh"'
fi
HERE
;;

(tcsh) cat<<HERE
set EOPEN_ROOT="$BASE";
alias eopen 'sh "$BASE/wsl/eopen.sh"';
alias ewd 'sh "$BASE/wsl/ewd.sh"';
alias ecd 'source "$BASE/wsl/ecd.tcsh"';
alias epushd 'source "$BASE/wsl/epushd.tcsh"';
HERE
;;

(fish) cat<<HERE
set EOPEN_ROOT "$BASE";
alias eopen='sh "$BASE/wsl/eopen.sh"';
alias ewd='sh "$BASE/wsl/ewd.sh"';
alias ecd='source "$BASE/wsl/ecd.fish"';
alias epushd='source "$BASE/wsl/epushd.fish"';
HERE
;;

(*) echo "Invalid shell type" >&2

esac
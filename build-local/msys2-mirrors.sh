# Puts the mirrors listed in FA_MSYS2_MIRRORS (space separated base URLs)
# at the top of the pacman mirror lists, so they are tried first.
# Called by the msys64 stage of prepare.py right after MSYS2 is unpacked.
set -e

if [ -z "$FA_MSYS2_MIRRORS" ] || [ "$FA_MSYS2_MIRRORS" = "none" ]; then
    exit 0
fi

prepend() {
    file="/etc/pacman.d/$1"
    suffix="$2"
    [ -f "$file" ] || return 0
    tmp="$(mktemp)"
    for mirror in $FA_MSYS2_MIRRORS; do
        echo "Server = ${mirror%/}/$suffix"
    done > "$tmp"
    grep -v -x -F -f "$tmp" "$file" >> "$tmp" || true
    mv "$tmp" "$file"
}

prepend mirrorlist.msys 'msys/$arch/'
prepend mirrorlist.mingw 'mingw/$repo/'
echo "[msys2] Preferred mirrors: $FA_MSYS2_MIRRORS"

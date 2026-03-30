#!/bin/bash
#
# Firmware Build Script — Ulanzi TC001
# Builds 4 MB and/or 8 MB firmware using PlatformIO.
#
# Usage: ./build-firmware.sh [4mb|8mb|all] [normal|beta|both] [--no-push] [--no-commit]
#   Default: all sizes, both versions
#
# Output:
#   8 MB normal → firmware/update.bin        version.txt
#   8 MB beta   → firmware/update_beta.bin   version_beta.txt
#   4 MB normal → firmware/update_4mb.bin    version_4mb.txt
#   4 MB beta   → firmware/update_beta_4mb.bin  version_beta_4mb.txt
#

set -e
export PATH="$HOME/.platformio/penv/bin:$PATH"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; NC='\033[0m'

CONFIG_FILE="src/config.h"
NORMAL_SUFFIX="ULANZI"
BETA_SUFFIX="ULANZI_BETA"
NO_PUSH=false
NO_COMMIT=false
BUILD_SIZE="all"
BUILD_TYPE="both"

print_info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error()   { echo -e "${RED}[ERROR]${NC} $1"; }

get_version_date() { date +%Y%m%d; }

update_version() {
    local version_string="$1"
    cp "$CONFIG_FILE" "${CONFIG_FILE}.bak"
    print_info "Updating version to: $version_string"
    if [[ "$version_string" == *"BETA"* ]]; then
        sed -i.tmp \
            -e "/FIRMWARE_VERSION.*_${NORMAL_SUFFIX}\"$/s/^#define/\/\/#define/" \
            -e "/FIRMWARE_VERSION.*_${BETA_SUFFIX}\"$/s/^\/\///" \
            -e "/FIRMWARE_VERSION.*_${BETA_SUFFIX}\"$/s/\"[^\"]*\"/\"${version_string}\"/" \
            "$CONFIG_FILE"
    else
        sed -i.tmp \
            -e "/FIRMWARE_VERSION.*_${NORMAL_SUFFIX}\"$/s/^\/\///" \
            -e "/FIRMWARE_VERSION.*_${NORMAL_SUFFIX}\"$/s/\"[^\"]*\"/\"${version_string}\"/" \
            -e "/FIRMWARE_VERSION.*_${BETA_SUFFIX}\"$/s/^#define/\/\/#define/" \
            "$CONFIG_FILE"
    fi
    rm -f "${CONFIG_FILE}.tmp"
}

restore_config() { [ -f "${CONFIG_FILE}.bak" ] && mv "${CONFIG_FILE}.bak" "$CONFIG_FILE"; }

compile_firmware() {
    local env="$1"
    print_info "Compiling (env: $env)..."
    pio run -e "$env"
    print_success "Compilation done"
}

copy_binary() {
    local env="$1" dest="$2"
    mkdir -p firmware
    cp ".pio/build/${env}/firmware.bin" "$dest"
    print_success "Binary → $dest"
}

git_commit() {
    local bin="$1" ver="$2"
    [ "$NO_COMMIT" = true ] && return
    git add "$bin" "$ver"
    if ! git diff --cached --quiet; then
        git commit -m "firmware: $bin — $(cat $ver)"
        if [ "$NO_PUSH" = false ]; then git push && print_success "Pushed"; fi
    else
        print_warning "No changes to commit"
    fi
}

build_one() {
    local env="$1" suffix="$2" bin="$3" ver="$4"
    local version_string="$(get_version_date)_${suffix}"

    echo ""
    print_info "============================================="
    print_info "Building: $version_string  [$env]"
    print_info "============================================="
    echo ""

    update_version "$version_string"
    compile_firmware "$env"
    copy_binary "$env" "$bin"
    echo "$version_string" > "$ver" && print_success "Version → $ver"
    restore_config
    git_commit "$bin" "$ver"
    print_success "Done: $bin"
}

build_size() {
    local size="$1"   # 4mb or 8mb
    if [ "$size" = "8mb" ]; then
        local env="ulanzi-tc001-8mb"
        local norm_bin="firmware/update.bin"         norm_ver="version.txt"
        local beta_bin="firmware/update_beta.bin"    beta_ver="version_beta.txt"
    else
        local env="ulanzi-tc001-4mb"
        local norm_bin="firmware/update_4mb.bin"     norm_ver="version_4mb.txt"
        local beta_bin="firmware/update_beta_4mb.bin" beta_ver="version_beta_4mb.txt"
    fi

    case "$BUILD_TYPE" in
        normal) build_one "$env" "$NORMAL_SUFFIX" "$norm_bin" "$norm_ver" ;;
        beta)   build_one "$env" "$BETA_SUFFIX"   "$beta_bin" "$beta_ver" ;;
        both)
            build_one "$env" "$NORMAL_SUFFIX" "$norm_bin" "$norm_ver"
            echo ""
            build_one "$env" "$BETA_SUFFIX"   "$beta_bin" "$beta_ver"
            ;;
    esac
}

show_usage() {
    echo "Usage: $0 [4mb|8mb|all] [normal|beta|both] [--no-push] [--no-commit]"
    echo ""
    echo "  4mb     Build 4 MB flash version only"
    echo "  8mb     Build 8 MB flash version only"
    echo "  all     Build both flash sizes (default)"
    echo "  normal  Build stable release only"
    echo "  beta    Build beta release only"
    echo "  both    Build normal + beta (default)"
  echo ""
  echo "  --no-push    Commit locally, do not push to git"
  echo "  --no-commit  Copy files only, skip all git operations"
    echo ""
    echo "Output:"
    echo "  8 MB normal → firmware/update.bin"
    echo "  8 MB beta   → firmware/update_beta.bin"
    echo "  4 MB normal → firmware/update_4mb.bin"
    echo "  4 MB beta   → firmware/update_beta_4mb.bin"
}

trap 'print_error "Build failed!"; restore_config; exit 1' ERR

# Parse args
for arg in "$@"; do
    case "$arg" in
        4mb)        BUILD_SIZE="4mb" ;;
        8mb)        BUILD_SIZE="8mb" ;;
        all)        BUILD_SIZE="all" ;;
        normal)     BUILD_TYPE="normal" ;;
        beta)       BUILD_TYPE="beta" ;;
        both)       BUILD_TYPE="both" ;;
        --no-push)  NO_PUSH=true  && print_info "No-push mode" ;;
        --no-commit) NO_COMMIT=true && print_info "No-commit mode" ;;
        -h|--help|help) show_usage; exit 0 ;;
        *) print_error "Unknown argument: $arg"; show_usage; exit 1 ;;
    esac
done

case "$BUILD_SIZE" in
    8mb) build_size 8mb ;;
    4mb) build_size 4mb ;;
    all) build_size 8mb; echo ""; build_size 4mb ;;
esac

echo ""
print_success "============================================="
print_success "All builds completed!"
print_success "============================================="

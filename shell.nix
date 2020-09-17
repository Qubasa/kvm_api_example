  { pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    buildInputs = with pkgs; [
      clang_10
      llvmPackages_10.lld
      llvmPackages_10.bintools
      cmake
      gnumake
    ];
    shellHook = ''
      export HISTFILE=${toString ./.history}
      export UBSAN_OPTIONS=print_stacktrace=1
      '';
  }

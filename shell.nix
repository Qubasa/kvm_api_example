  { pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    buildInputs = with pkgs; [
      clang_10
      llvmPackages_10.lld
      cmake
      gnumake
    ];
    shellHook = "export HISTFILE=${toString ./.history}";
  }

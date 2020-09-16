  { pkgs ? import <nixpkgs> {} }:
  pkgs.mkShell {
    buildInputs = with pkgs; [
      clang_10
      cmake
      gnumake
    ];
    shellHook = "export HISTFILE=${toString ./.history}";
  }

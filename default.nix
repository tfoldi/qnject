{ pkgs ? (import <nixpkgs> {}), use_clang ? false, use_qt ? false, ...} :
{
  qnject = pkgs.stdenv.mkDerivation rec {
    name = "qnject";
    src = ./.;
    buildInputs = with pkgs; [ cmake vim ];
    cmakeFlags = (if ! use_clang then "" else ''
      -DCMAKE_C_COMPILER=${pkgs.clang}/bin/clang
      -DCMAKE_CXX_COMPILER=${pkgs.clang}/bin/clang++
    '') + (if ! use_qt then "" else ''
      -DCMAKE_PREFIX_PATH=${pkgs.qt5.full}
    '');
    preBuild = "ln -s build/config.h ../config.h";
    installPhase = ''
      mkdir -p $out/lib
      cp vaccine/libqnject.so $out/lib/
      ctest -V
    '';
  };
}

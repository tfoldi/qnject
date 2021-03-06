{ pkgs ? (import <nixpkgs> {}), use_clang ? false, use_qt ? false, ...} :
{
  qnject = pkgs.stdenv.mkDerivation rec {
    name = "qnject";
    src = ./.;
    
    buildInputs = with pkgs; [ cmake vim ];

    dontUseCmakeBuildDir = true;

    cmakeFlags = (if ! use_clang then "" else ''
      -DCMAKE_C_COMPILER=${pkgs.clang}/bin/clang
      -DCMAKE_CXX_COMPILER=${pkgs.clang}/bin/clang++
    '') + (if ! use_qt then "" else ''
      -DCMAKE_PREFIX_PATH=${pkgs.qt5.full}
    '');

    buildPhase = ''
        DISPLAY=:99.0 make all test ARGS=-V
      '';
  };
}

{
  stdenv,
  lib,
  applyPatches,
  callPackage,
  cmake,
  fetchFromGitHub,
  fetchpatch,
  darwin,
}@args:

stdenv.mkDerivation rec {
  pname = "plutovg";
  version = "3e6f922f453da1c9e7d1d7f66cac1d9724a18b47";
  outputs = [
    "out"
    "lib"
  ];

  src = fetchFromGitHub {
    owner = "sammycage";
    repo = "plutovg";
    rev = "3e6f922f453da1c9e7d1d7f66cac1d9724a18b47";
    hash = "sha256-ruwgZ+ZXXGDH/gi65hGhIF/NjuU4+S7uINNVh5ifOZY=";
  };

  nativeBuildInputs = [ cmake ];

  # buildInputs = lib.optionals stdenv.hostPlatform.isDarwin [
  #   darwin.apple_sdk.frameworks.ApplicationServices
  #   darwin.apple_sdk.frameworks.Cocoa
  #   darwin.apple_sdk.frameworks.GameController
  # ];

  meta = {
    description = "Tiny 2D vector graphics library in C";
    homepage = "https://github.com/sammycage/plutovg";
    license = lib.licenses.mit; # vcpkg licensed as MIT too
    maintainers = with lib.maintainers; [
      SomeoneSerge
    ];
    platforms = lib.platforms.all;
  };
}

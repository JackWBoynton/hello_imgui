{ stdenv
, lib
, callPackage
, cmake
, fetchFromGitHub
, pkg-config
,
}:

let
  plutovg = callPackage ./plutovg.nix { };
in

stdenv.mkDerivation rec {
  pname = "lunasvg";
  version = "3.2.0";
  outputs = [
    "out"
    "lib"
  ];

  src = fetchFromGitHub {
    owner = "sammycage";
    repo = "lunasvg";
    rev = "v${version}";
    hash = "sha256-/DEyiHlZJYctkNqjQECKRbMGwUYTJHtlQrO0aBXf+Oc=";
  };

  nativeBuildInputs = [ cmake plutovg pkg-config ];

  # buildInputs = lib.optionals stdenv.hostPlatform.isDarwin [
  #   darwin.apple_sdk.frameworks.ApplicationServices
  #   darwin.apple_sdk.frameworks.Cocoa
  #   darwin.apple_sdk.frameworks.GameController
  # ];

  cmakeFlags = [
    "-DCMAKE_REQUIRE_FIND_PACKAGE_plutovg=1"
    "-DLUNASVG_BUILD_EXAMPLES=OFF"
  ];

  meta = {
    description = "SVG rendering and manipulation library in C++";
    homepage = "https://github.com/sammycage/lunasvg";
    license = lib.licenses.mit; # vcpkg licensed as MIT too
    maintainers = with lib.maintainers; [
      jackwboynton
    ];
    platforms = lib.platforms.all;
  };
}


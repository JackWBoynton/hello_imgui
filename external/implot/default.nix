{ lib
, stdenv
, fetchFromGitHub
, callPackage
, cmake
, vcpkg
, imgui
,
}:

stdenv.mkDerivation rec {
  pname = "implot";
  version = "3aae9e2ab54874ff0e80479ad9ae7a3feefeafef";

  src = ./.;

  cmakeRules = "${vcpkg.src}/ports/implot";
  postPatch = ''
    cp "$cmakeRules"/CMakeLists.txt ./
  '';

  buildInputs = [ imgui ];
  nativeBuildInputs = [ cmake ];
  propagatedBuildInputs = [ imgui ];

  passthru.tests = {
    implot-demos = callPackage ./demos { };
  };

  meta = {
    description = "Immediate Mode Plotting";
    homepage = "https://github.com/jackwboynton/implot";
    license = lib.licenses.mit;
    maintainers = with lib.maintainers; [ JackWBoynton ];
    platforms = lib.platforms.all;
  };
}


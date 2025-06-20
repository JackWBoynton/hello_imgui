{ pkgs
, stdenv
, lib
, callPackage
, cmake
, darwin
, freetype
, glfw3
, libGLU
}:

let
  imgui = callPackage ./external/imgui {
    inherit glfw3;
  };
  lunasvg = callPackage ./external/lunasvg.nix { };
in

stdenv.mkDerivation {
  pname = "hello-imgui";
  version = "62a698994fc0ad2813bf38f08e8073a4cd7cf0c2";
  outputs = [
    "out"
  ];

  src = ./.;


  nativeBuildInputs = [ cmake ];

  buildInputs = [
    freetype
    lunasvg.lib
    imgui
    glfw3
    libGLU
  ] ++ lib.optionals stdenv.hostPlatform.isDarwin [
    darwin.apple_sdk.frameworks.ApplicationServices
    darwin.apple_sdk.frameworks.Cocoa
    darwin.apple_sdk.frameworks.GameController
  ] ++ lib.optionals stdenv.hostPlatform.isLinux [ pkgs.xorg.libX11 pkgs.xorg.libXrandr ];

  propagatedBuildInputs = [ imgui ];


  cmakeFlags = [
    "-DHELLOIMGUI_USE_GLFW_OPENGL3=ON" # TODO: make this configurable
    "-DHELLOIMGUI_FETCH_FORBIDDEN=ON"
    "-DHELLOIMGUI_USE_IMGUI_CMAKE_PACKAGE=ON"
    "-DHELLOIMGUI_BUILD_DEMOS=OFF"
    "-Dlunasvg_DIR=${lunasvg.lib}/cmake/lunasvg"
  ];


  meta = {
    description = "Hello, Dear ImGui: unleash your creativity in app development and prototyping";
    homepage = "https://github.com/jackwboynton/hello-imgui";
    license = lib.licenses.mit; # vcpkg licensed as MIT too
    maintainers = with lib.maintainers; [
      jackwboynton
    ];
    platforms = lib.platforms.all;
  };
}

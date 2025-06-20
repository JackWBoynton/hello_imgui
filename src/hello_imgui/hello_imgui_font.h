#pragma once
#include "imgui.h"
#include <string>
#include <vector>
#include <array>

namespace HelloImGui
{
    using ImWcharPair = std::array<ImWchar, 2>;

    // Utility to translate DearImGui common Unicode ranges to ImWcharPair (Python)
    //   (get_glyph_ranges_chinese_simplified_common, get_glyph_ranges_japanese, ...)
    std::vector<ImWcharPair> translate_common_glyph_ranges(const std::vector<ImWchar> & glyphRanges);

    // Utility to translate DearImGui common Unicode ranges to ImWcharPair (C++)
    //   (GetGlyphRangesChineseSimplifiedCommon, GetGlyphRangesJapanese, ...)
    std::vector<ImWcharPair> TranslateCommonGlyphRanges(const ImWchar* glyphRanges);

    // @@md#Fonts

    // When loading fonts, use
    //          HelloImGui::LoadFont(..)
    //      or
    //      	HelloImGui::LoadDpiResponsiveFont()
    //
    // Use these functions instead of ImGui::GetIO().Fonts->AddFontFromFileTTF(),
    // because they will automatically adjust the font size to account for HighDPI,
    // and will help you to get consistent font size across different OSes.

    //
    // Font loading parameters: several options are available (color, merging, range, ...)
    struct FontLoadingParams
    {
        // if true, the font size will be adjusted automatically to account for HighDPI
        bool adjustSizeToDpi = true;

        // if true, the font will be merged to the last font
        bool mergeToLastFont = false;

        // if true, the font will be loaded using colors
        // (requires freetype, enabled by IMGUI_ENABLE_FREETYPE)
        bool loadColor = false;

        // if true, the font will be loaded using HelloImGui asset system.
        // Otherwise, it will be loaded from the filesystem
        bool insideAssets = true;

        // ImGui native font config to use
        ImFontConfig fontConfig = ImFontConfig();

    };


    // Loads a font with the specified parameters
    // (this font will not adapt to DPI changes after startup)
    ImFont* LoadFont(
        const std::string & fontFilename, float fontSize,
        const FontLoadingParams & params = {});

    // @@md

    //
    // Deprecated API below, kept for compatibility (uses LoadFont internally)
    //
    ImFont* LoadFontTTF(
        const std::string & fontFilename,
        float fontSize,
        ImFontConfig config = ImFontConfig()
        );
    ImFont* LoadFontTTF_WithFontAwesomeIcons(
        const std::string & fontFilename,
        float fontSize,
        ImFontConfig configFont = ImFontConfig()
    );
}
# Single inventory for C++ asset IDs, Win32 RCDATA, staging, and installation.
# key | path relative to assets/ | FILE/EMBEDDED | Win32 ID (0 for FILE) |
# REQUIRED/OPTIONAL | legacy flat filename (or - for embedded assets)
tenhou_asset(Background images/_bg.bmp FILE 0 REQUIRED _bg.bmp)
tenhou_asset(BackgroundAlternate images/_bg1.bmp FILE 0 REQUIRED _bg1.bmp)
tenhou_asset(LobbyMusic audio/_bgm0.wav FILE 0 REQUIRED _bgm0.wav)
tenhou_asset(LobbyMusicAlternate audio/_bgm1.wav FILE 0 REQUIRED _bgm1.wav)
# The bundled display pack contains the seven original images and sound effects.
tenhou_asset(DisplayPack packs/_res2.dat FILE 0 REQUIRED _res2.dat)
# Four native 4:3 table object packs (1024, 1440, 1920, 2640).
tenhou_asset(TablePackSd packs/_obj0.dat FILE 0 REQUIRED _obj0.dat)
tenhou_asset(TableCompanionPackSd packs/_obj1.dat FILE 0 REQUIRED _obj1.dat)
tenhou_asset(TablePack packs/_obj2.dat FILE 0 REQUIRED _obj2.dat)
tenhou_asset(TableCompanionPack packs/_obj3.dat FILE 0 REQUIRED _obj3.dat)
tenhou_asset(TablePack1920 packs/_obj0_1920x1440.dat FILE 0 REQUIRED _obj0_1920x1440.dat)
tenhou_asset(TableCompanionPack1920 packs/_obj1_1920x1440.dat FILE 0 REQUIRED _obj1_1920x1440.dat)
tenhou_asset(TablePack2640 packs/_obj0_2640x1980.dat FILE 0 REQUIRED _obj0_2640x1980.dat)
tenhou_asset(TableCompanionPack2640 packs/_obj1_2640x1980.dat FILE 0 REQUIRED _obj1_2640x1980.dat)
# Exact original RT_DIALOG bytes; IDs also identify settings page handlers.
tenhou_asset(Dialog104 dialogs/104.bin EMBEDDED 104 REQUIRED -)
tenhou_asset(Dialog105 dialogs/105.bin EMBEDDED 105 REQUIRED -)
tenhou_asset(Dialog106 dialogs/106.bin EMBEDDED 106 REQUIRED -)
tenhou_asset(Dialog107 dialogs/107.bin EMBEDDED 107 REQUIRED -)
tenhou_asset(Dialog108 dialogs/108.bin EMBEDDED 108 REQUIRED -)
tenhou_asset(Dialog109 dialogs/109.bin EMBEDDED 109 REQUIRED -)
tenhou_asset(Dialog110 dialogs/110.bin EMBEDDED 110 REQUIRED -)
tenhou_asset(Dialog114 dialogs/114.bin EMBEDDED 114 REQUIRED -)
# Fixed original 0040c1d0 output (zlib-compressed sdw2 for 1440) and native sdw0 packs for 1024/1920/2640.
tenhou_asset(TileShadows shadows/shadow-hd.dat EMBEDDED 2000 REQUIRED -)
tenhou_asset(TileShadowsSd shadows/_sdw0_1024x768.dat FILE 0 REQUIRED _sdw0_1024x768.dat)
tenhou_asset(TileShadows1920 shadows/_sdw0_1920x1440.dat FILE 0 REQUIRED _sdw0_1920x1440.dat)
tenhou_asset(TileShadows2640 shadows/_sdw0_2640x1980.dat FILE 0 REQUIRED _sdw0_2640x1980.dat)

# Flowers retain their dedicated tile-face artwork.
tenhou_asset(FlowerMei images/variants/flowers/梅.png FILE 0 REQUIRED -)
tenhou_asset(FlowerLan images/variants/flowers/兰.png FILE 0 REQUIRED -)
tenhou_asset(FlowerZhu images/variants/flowers/竹.png FILE 0 REQUIRED -)
tenhou_asset(FlowerJu images/variants/flowers/菊.png FILE 0 REQUIRED -)
tenhou_asset(FlowerChun images/variants/flowers/春.png FILE 0 REQUIRED -)
tenhou_asset(FlowerXia images/variants/flowers/夏.png FILE 0 REQUIRED -)
tenhou_asset(FlowerQiu images/variants/flowers/秋.png FILE 0 REQUIRED -)
tenhou_asset(FlowerDong images/variants/flowers/冬.png FILE 0 REQUIRED -)

# Non-text original display images: 401/402 (8x) and 466/589/591/592 (4x) ship as HD PNG overrides;
# 440 stays on bundled _res2.dat runtime scaling unless an optional 440.png override is provided.
set(display_sprite_ids 401 402 440 466 589 591 592)
foreach(sprite_id IN LISTS display_sprite_ids)
    tenhou_sprite(${sprite_id})
endforeach()
# Private TrueType source already present in the repository; no system-font copying.
tenhou_asset(TextFont "fonts/HGSeikaiShotaiPRO.TTF" FILE 0 OPTIONAL -)

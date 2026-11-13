# Single inventory for C++ asset IDs, Win32 RCDATA, staging, and installation.
# key | path relative to assets/ | FILE/EMBEDDED | Win32 ID (0 for FILE) |
# REQUIRED/OPTIONAL | legacy flat filename (or - for embedded assets)
tenhou_asset(Background images/_bg.bmp FILE 0 REQUIRED _bg.bmp)
tenhou_asset(BackgroundAlternate images/_bg1.bmp FILE 0 REQUIRED _bg1.bmp)
tenhou_asset(LobbyMusic audio/_bgm0.wav FILE 0 REQUIRED _bgm0.wav)
tenhou_asset(LobbyMusicAlternate audio/_bgm1.wav FILE 0 REQUIRED _bgm1.wav)
tenhou_asset(DisplayPack packs/_res2.dat FILE 0 REQUIRED _res2.dat)
tenhou_asset(TablePack packs/_obj2.dat FILE 0 REQUIRED _obj2.dat)
tenhou_asset(TableCompanionPack packs/_obj3.dat FILE 0 REQUIRED _obj3.dat)
# The repository ships HD artwork. Keep optional SD packs usable via --resources.
tenhou_asset(TablePackSd packs/_obj0.dat FILE 0 OPTIONAL _obj0.dat)
tenhou_asset(TableCompanionPackSd packs/_obj1.dat FILE 0 OPTIONAL _obj1.dat)
# Exact original RT_DIALOG bytes; IDs also identify settings page handlers.
tenhou_asset(Dialog104 dialogs/104.bin EMBEDDED 104 REQUIRED -)
tenhou_asset(Dialog105 dialogs/105.bin EMBEDDED 105 REQUIRED -)
tenhou_asset(Dialog106 dialogs/106.bin EMBEDDED 106 REQUIRED -)
tenhou_asset(Dialog107 dialogs/107.bin EMBEDDED 107 REQUIRED -)
tenhou_asset(Dialog108 dialogs/108.bin EMBEDDED 108 REQUIRED -)
tenhou_asset(Dialog109 dialogs/109.bin EMBEDDED 109 REQUIRED -)
tenhou_asset(Dialog110 dialogs/110.bin EMBEDDED 110 REQUIRED -)
tenhou_asset(Dialog114 dialogs/114.bin EMBEDDED 114 REQUIRED -)
# Fixed original 0040c1d0 output, zlib-compressed sdw2 data.
tenhou_asset(TileShadows shadows/shadow-hd.dat EMBEDDED 2000 REQUIRED -)

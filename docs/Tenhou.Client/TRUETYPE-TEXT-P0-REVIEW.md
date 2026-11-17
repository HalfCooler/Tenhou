# TrueType P0 人工核对表

状态：用户已确认当前文案及下列规格；P1 效果参数仍待样张校准。

来源：`docs/Tenhou.Client/asset-reference/Sprites.xlsx`，工作表 `Sprites705`。

XLSX SHA-256：`b4be245fafd3b68d79c1a4fe784662cc7e790b1e958558cf61813d076ecc8bd0`。

机械检查：296 Text、7 Image；481 使用通常役种的完整 24 pt 样式；仅 Info 空白格式继承 566。

## 核对要点

- [x] 全部显示文案以当前版本为准，保留原先未使用文案，并更新代码回退。
- [x] 黑色、白色为正确描述；旧源表中的输入法错字只保留在来源记录中。
- [ ] 确认普通/悬停复用文案；自动选项的 checked/unchecked 含义正确。
- [x] 普通按钮黑边、悬停按钮橙边；441–449 / 595–598 为悬停，450–458 / 599–602 为普通。
- [x] 481 为 24 pt，其他设定与通常役种一致：近白、非粗体、无描边、无阴影、零额外字距。
- [x] 459 右下角小字为 12 pt，偏移留 P1 样张。
- [x] 566 三段独立渐变已逐像素确认；Alpha、边距和字宽基准留 P1。
- [ ] 核对 101 条规则匹配名、显示文字和缺省分值；规则匹配不使用显示名反推。
- [ ] P1 样张确定近白、深灰、棕、橙、红的 RGBA，以及描边/阴影参数。

## 样式

| 样式键 | pt | 字距 pt | 粗体 | 源颜色描述 | 源样式描述 | 核对 |
| --- | ---: | ---: | --- | --- | --- | --- |
| center_black | 22.0 | 0 | False | 黑色 |  |  |
| seat_near_white | 30.0 | 0 | False | 几乎纯白 |  |  |
| seat_red_outline | 30.0 | 0 | False | 白色填充；红色描边 |  |  |
| seat_black | 30.0 | 0 | False | 黑色 |  |  |
| option_checked | 18.0 | -0.5 | True | 白色填充；棕色描边 | 粗体；间距紧缩 0.5 磅 |  |
| option_unchecked | 18.0 | -0.5 | True | 深灰色填充；黑色描边 | 粗体；间距紧缩 0.5 磅 |  |
| limit | 22.0 | 1 | True | 几乎纯白 | 粗体；间距加宽 1 磅 |  |
| button_orange_outline | 24.0 | 1 | True | 白色填充；橙色描边 | 粗体；间距加宽 1 磅 |  |
| button_black_outline | 24.0 | 1 | True | 白色填充；黑色描边 | 粗体；间距加宽 1 磅 |  |
| title | 44.0 | 0 | False | 几乎纯白 |  |  |
| draw_reason | 40.0 | 1 | False | 几乎纯白 | 间距加宽 1 磅 |  |
| action_caption | 56.0 | 0 | True | 白色填充；阴影 | 粗体 |  |
| yaku | 24.0 | 0 | False | 几乎纯白 |  |  |
| info | 16 | 0 | True | 字体是白色；背景从左向右为线性灰度渐变：255 开始；到 15% 255；到 100% 204<br>倒数第二个像素行：255 开始；到 5% 255；到 30% 239；到 60% 250；到 80% 247；到 100% 246<br>最后一个像素行：255 开始；到 45% 156；到 100% 134 | 粗体 |  |
| furiten | 18.0 | 0 | False | 白色 |  |  |

## 文字目录

| OriginId | Label | 语义键 | 样式 / pt | 说明 | 核对 |
| ---: | --- | --- | --- | --- | --- |
| 403 | 1 | center.hand_one | center_black / 22 |  | 文案已确认 |
| 404 | 2 | center.hand_two | center_black / 22 |  | 文案已确认 |
| 405 | 3 | center.hand_three | center_black / 22 |  | 文案已确认 |
| 406 | 4 | center.hand_four | center_black / 22 |  | 文案已确认 |
| 407 | 東 | center.east | center_black / 22 |  | 文案已确认 |
| 408 | 南 | center.south | center_black / 22 |  | 文案已确认 |
| 409 | 西 | center.west | center_black / 22 |  | 文案已确认 |
| 410 | 北 | center.north | center_black / 22 |  | 文案已确认 |
| 411 | 局 | center.hand_unit | center_black / 22 |  | 文案已确认 |
| 412 | 東 | seat.neutral.east | seat_near_white / 30 |  | 文案已确认 |
| 413 | 南 | seat.neutral.south | seat_near_white / 30 |  | 文案已确认 |
| 414 | 西 | seat.neutral.west | seat_near_white / 30 |  | 文案已确认 |
| 415 | 北 | seat.neutral.north | seat_near_white / 30 |  | 文案已确认 |
| 416 | 東 | seat.self.east | seat_red_outline / 30 |  | 文案已确认 |
| 417 | 南 | seat.self.south | seat_red_outline / 30 |  | 文案已确认 |
| 418 | 西 | seat.self.west | seat_red_outline / 30 |  | 文案已确认 |
| 419 | 北 | seat.self.north | seat_red_outline / 30 |  | 文案已确认 |
| 420 | 東 | seat.black.east | seat_black / 30 |  | 文案已确认 |
| 421 | 南 | seat.black.south | seat_black / 30 |  | 文案已确认 |
| 422 | 西 | seat.black.west | seat_black / 30 |  | 文案已确认 |
| 423 | 北 | seat.black.north | seat_black / 30 |  | 文案已确认 |
| 424 | 自動理牌 | option.checked.auto_sort | option_checked / 18 |  | 文案已确认 |
| 425 | 鳴かなぃ | option.checked.no_call | option_checked / 18 |  | 文案已确认 |
| 426 | ツモ切り | option.checked.tsumogiri | option_checked / 18 |  | 文案已确认 |
| 427 | 自動和了 | option.checked.auto_win | option_checked / 18 |  | 文案已确认 |
| 428 | 流局聴牌 | option.checked.draw_tenpai | option_checked / 18 |  | 文案已确认 |
| 429 | 自動理牌 | option.unchecked.auto_sort | option_unchecked / 18 |  | 文案已确认 |
| 430 | 鳴かなぃ | option.unchecked.no_call | option_unchecked / 18 |  | 文案已确认 |
| 431 | ツモ切り | option.unchecked.tsumogiri | option_unchecked / 18 |  | 文案已确认 |
| 432 | 自動和了 | option.unchecked.auto_win | option_unchecked / 18 |  | 文案已确认 |
| 433 | 流局聴牌 | option.unchecked.draw_tenpai | option_unchecked / 18 |  | 文案已确认 |
| 435 | 満貫 | limit.mangan | limit / 22 |  | 文案已确认 |
| 436 | 跳満 | limit.haneman | limit / 22 |  | 文案已确认 |
| 437 | 倍満 | limit.baiman | limit / 22 |  | 文案已确认 |
| 438 | 三倍満 | limit.sanbaiman | limit / 22 |  | 文案已确认 |
| 439 | 役満 | limit.yakuman | limit / 22 |  | 文案已确认 |
| 441 | ポン | button.hover.pon | button_orange_outline / 24 |  | 文案已确认 |
| 442 | カン | button.hover.kan | button_orange_outline / 24 |  | 文案已确认 |
| 443 | チー | button.hover.chi | button_orange_outline / 24 |  | 文案已确认 |
| 444 | ロン | button.hover.ron | button_orange_outline / 24 |  | 文案已确认 |
| 445 | ツモ | button.hover.tsumo | button_orange_outline / 24 |  | 文案已确认 |
| 446 | リーチ | button.hover.riichi | button_orange_outline / 24 |  | 文案已确认 |
| 447 | 流局 | button.hover.abort_draw | button_orange_outline / 24 |  | 文案已确认 |
| 448 | 抜く | button.hover.nuki | button_orange_outline / 24 |  | 文案已确认 |
| 449 | パス | button.hover.pass | button_orange_outline / 24 |  | 文案已确认 |
| 450 | ポン | button.normal.pon | button_black_outline / 24 |  | 文案已确认 |
| 451 | カン | button.normal.kan | button_black_outline / 24 |  | 文案已确认 |
| 452 | チー | button.normal.chi | button_black_outline / 24 |  | 文案已确认 |
| 453 | ロン | button.normal.ron | button_black_outline / 24 |  | 文案已确认 |
| 454 | ツモ | button.normal.tsumo | button_black_outline / 24 |  | 文案已确认 |
| 455 | リーチ | button.normal.riichi | button_black_outline / 24 |  | 文案已确认 |
| 456 | 流局 | button.normal.abort_draw | button_black_outline / 24 |  | 文案已确认 |
| 457 | 抜く | button.normal.nuki | button_black_outline / 24 |  | 文案已确认 |
| 458 | パス | button.normal.pass | button_black_outline / 24 |  | 文案已确认 |
| 459 | チー（選択） | button.chi_selection | button_black_outline / 24 | 主字 24 pt＋右下角小字 12 pt；偏移待 P1 | 文案已确认 |
| 460 | 認定 | title.certification | title / 44 |  | 文案已确认 |
| 461 | 流局 | title.draw | title / 44 |  | 文案已确认 |
| 462 | 終局 | title.game_end | title / 44 |  | 文案已确认 |
| 463 | 対局 | title.match | title / 44 |  | 文案已确认 |
| 464 | 再開 | title.resume | title / 44 |  | 文案已确认 |
| 465 | 観戦 | title.spectator | title / 44 |  | 文案已确认 |
| 467 | 九種九牌 | draw.nine_terminals | draw_reason / 40 |  | 文案已确认 |
| 468 | 四家立直 | draw.four_riichi | draw_reason / 40 |  | 文案已确认 |
| 469 | 三家和了 | draw.triple_ron | draw_reason / 40 |  | 文案已确认 |
| 470 | 四槓散了 | draw.four_kans | draw_reason / 40 |  | 文案已确认 |
| 471 | 四風連打 | draw.four_winds | draw_reason / 40 |  | 文案已确认 |
| 472 | 流し満貫 | draw.nagashi_mangan | draw_reason / 40 |  | 文案已确认 |
| 473 | ポン | caption.pon | action_caption / 56 |  | 文案已确认 |
| 474 | カン | caption.kan | action_caption / 56 |  | 文案已确认 |
| 475 | チー | caption.chi | action_caption / 56 |  | 文案已确认 |
| 476 | ロン | caption.ron | action_caption / 56 |  | 文案已确认 |
| 477 | ツモ | caption.tsumo | action_caption / 56 |  | 文案已确认 |
| 478 | リーチ | caption.riichi | action_caption / 56 |  | 文案已确认 |
| 479 | パオ | caption.pao | action_caption / 56 |  | 文案已确认 |
| 480 | キタ | caption.kita | action_caption / 56 |  | 文案已确认 |
| 481 | 門前清自摸和 | yaku.riichi.menzen_tsumo | yaku / 24 | 已确认 24 pt，全部样式与通常役种一致 | 文案已确认 |
| 482 | 立直 | yaku.riichi.riichi | yaku / 24 |  | 文案已确认 |
| 483 | 一発 | yaku.riichi.ippatsu | yaku / 24 |  | 文案已确认 |
| 484 | 槍槓 | yaku.riichi.chankan | yaku / 24 |  | 文案已确认 |
| 485 | 嶺上開花 | yaku.riichi.rinshan_kaihou | yaku / 24 |  | 文案已确认 |
| 486 | 海底摸月 | yaku.riichi.haitei | yaku / 24 |  | 文案已确认 |
| 487 | 河底撈魚 | yaku.riichi.houtei | yaku / 24 |  | 文案已确认 |
| 488 | 平和 | yaku.riichi.pinfu | yaku / 24 |  | 文案已确认 |
| 489 | 断幺九 | yaku.riichi.tanyao | yaku / 24 |  | 文案已确认 |
| 490 | 一盃口 | yaku.riichi.iipeikou | yaku / 24 |  | 文案已确认 |
| 491 | 自風 東 | yaku.riichi.seat_east | yaku / 24 |  | 文案已确认 |
| 492 | 自風 南 | yaku.riichi.seat_south | yaku / 24 |  | 文案已确认 |
| 493 | 自風 西 | yaku.riichi.seat_west | yaku / 24 |  | 文案已确认 |
| 494 | 自風 北 | yaku.riichi.seat_north | yaku / 24 |  | 文案已确认 |
| 495 | 場風 東 | yaku.riichi.round_east | yaku / 24 |  | 文案已确认 |
| 496 | 場風 南 | yaku.riichi.round_south | yaku / 24 |  | 文案已确认 |
| 497 | 場風 西 | yaku.riichi.round_west | yaku / 24 |  | 文案已确认 |
| 498 | 場風 北 | yaku.riichi.round_north | yaku / 24 |  | 文案已确认 |
| 499 | 役牌 白 | yaku.riichi.white_dragon | yaku / 24 |  | 文案已确认 |
| 500 | 役牌 發 | yaku.riichi.green_dragon | yaku / 24 |  | 文案已确认 |
| 501 | 役牌 中 | yaku.riichi.red_dragon | yaku / 24 |  | 文案已确认 |
| 502 | 両立直 | yaku.riichi.double_riichi | yaku / 24 |  | 文案已确认 |
| 503 | 七対子 | yaku.riichi.chiitoitsu | yaku / 24 |  | 文案已确认 |
| 504 | 混全帯幺九 | yaku.riichi.chanta | yaku / 24 |  | 文案已确认 |
| 505 | 一気通貫 | yaku.riichi.ittsuu | yaku / 24 |  | 文案已确认 |
| 506 | 三色同順 | yaku.riichi.sanshoku_doujun | yaku / 24 |  | 文案已确认 |
| 507 | 三色同刻 | yaku.riichi.sanshoku_doukou | yaku / 24 |  | 文案已确认 |
| 508 | 三槓子 | yaku.riichi.sankantsu | yaku / 24 |  | 文案已确认 |
| 509 | 対々和 | yaku.riichi.toitoi | yaku / 24 |  | 文案已确认 |
| 510 | 三暗刻 | yaku.riichi.sanankou | yaku / 24 |  | 文案已确认 |
| 511 | 小三元 | yaku.riichi.shousangen | yaku / 24 |  | 文案已确认 |
| 512 | 混老頭 | yaku.riichi.honroutou | yaku / 24 |  | 文案已确认 |
| 513 | 二盃口 | yaku.riichi.ryanpeikou | yaku / 24 |  | 文案已确认 |
| 514 | 純全帯幺九 | yaku.riichi.junchan | yaku / 24 |  | 文案已确认 |
| 515 | 混一色 | yaku.riichi.honitsu | yaku / 24 |  | 文案已确认 |
| 516 | 清一色 | yaku.riichi.chinitsu | yaku / 24 |  | 文案已确认 |
| 517 | 人和 | yaku.riichi.renhou | yaku / 24 |  | 文案已确认 |
| 518 | 天和 | yaku.riichi.tenhou | yaku / 24 |  | 文案已确认 |
| 519 | 地和 | yaku.riichi.chiihou | yaku / 24 |  | 文案已确认 |
| 520 | 大三元 | yaku.riichi.daisangen | yaku / 24 |  | 文案已确认 |
| 521 | 四暗刻 | yaku.riichi.suuankou | yaku / 24 |  | 文案已确认 |
| 522 | 四暗刻単騎 | yaku.riichi.suuankou_tanki | yaku / 24 |  | 文案已确认 |
| 523 | 字一色 | yaku.riichi.tsuuiisou | yaku / 24 |  | 文案已确认 |
| 524 | 緑一色 | yaku.riichi.ryuuiisou | yaku / 24 |  | 文案已确认 |
| 525 | 清老頭 | yaku.riichi.chinroutou | yaku / 24 |  | 文案已确认 |
| 526 | 九蓮宝燈 | yaku.riichi.chuuren | yaku / 24 |  | 文案已确认 |
| 527 | 純正九蓮宝燈 | yaku.riichi.pure_chuuren | yaku / 24 |  | 文案已确认 |
| 528 | 国士無双 | yaku.riichi.kokushi | yaku / 24 |  | 文案已确认 |
| 529 | 国士無双13面 | yaku.riichi.kokushi_thirteen_wait | yaku / 24 |  | 文案已确认 |
| 530 | 大四喜 | yaku.riichi.daisuushii | yaku / 24 |  | 文案已确认 |
| 531 | 小四喜 | yaku.riichi.shousuushii | yaku / 24 |  | 文案已确认 |
| 532 | 四槓子 | yaku.riichi.suukantsu | yaku / 24 |  | 文案已确认 |
| 533 | ドラ | yaku.riichi.dora | yaku / 24 |  | 文案已确认 |
| 534 | 裏ドラ | yaku.riichi.ura_dora | yaku / 24 |  | 文案已确认 |
| 535 | 赤ドラ | yaku.riichi.aka_dora | yaku / 24 |  | 文案已确认 |
| 536 | 親 | yaku.extra.dealer | yaku / 24 |  | 文案已确认 |
| 537 | 門前 | yaku.extra.closed | yaku / 24 |  | 文案已确认 |
| 538 | 栄和 | yaku.extra.ron | yaku / 24 |  | 文案已确认 |
| 539 | 本場 | yaku.extra.honba | yaku / 24 |  | 文案已确认 |
| 540 | 明槓 | yaku.extra.open_kan | yaku / 24 |  | 文案已确认 |
| 541 | 暗槓 | yaku.extra.closed_kan | yaku / 24 |  | 文案已确认 |
| 542 | 五 | yaku.extra.five | yaku / 24 |  | 文案已确认 |
| 543 | ガリ | yaku.extra.gari | yaku / 24 |  | 文案已确认 |
| 544 | セツト | yaku.extra.set | yaku / 24 |  | 文案已确认 |
| 545 | カラス | yaku.extra.karasu | yaku / 24 |  | 文案已确认 |
| 546 | 全ガリ | yaku.extra.all_gari | yaku / 24 |  | 文案已确认 |
| 547 | パンパン | yaku.extra.panpan | yaku / 24 |  | 文案已确认 |
| 548 | オープン立直 | yaku.extra.open_riichi | yaku / 24 |  | 文案已确认 |
| 549 | 四枚七対子 | yaku.extra.four_tile_seven_pairs | yaku / 24 |  | 文案已确认 |
| 550 | オールスター | yaku.extra.all_star | yaku / 24 |  | 文案已确认 |
| 551 | 大車輪 | yaku.extra.daisharin | yaku / 24 |  | 文案已确认 |
| 552 | 0 | number.zero | yaku / 24 |  | 文案已确认 |
| 553 | 1 | number.one | yaku / 24 |  | 文案已确认 |
| 554 | 2 | number.two | yaku / 24 |  | 文案已确认 |
| 555 | 3 | number.three | yaku / 24 |  | 文案已确认 |
| 556 | 4 | number.four | yaku / 24 |  | 文案已确认 |
| 557 | 5 | number.five | yaku / 24 |  | 文案已确认 |
| 558 | 6 | number.six | yaku / 24 |  | 文案已确认 |
| 559 | 7 | number.seven | yaku / 24 |  | 文案已确认 |
| 560 | 8 | number.eight | yaku / 24 |  | 文案已确认 |
| 561 | 9 | number.nine | yaku / 24 |  | 文案已确认 |
| 562 | 飜 | unit.han | yaku / 24 |  | 文案已确认 |
| 563 | 符 | unit.fu | yaku / 24 |  | 文案已确认 |
| 564 | 点 | unit.points | yaku / 24 |  | 文案已确认 |
| 565 | 枚 | unit.tiles | yaku / 24 |  | 文案已确认 |
| 566 | 一般 | info.general | info / 16 |  | 文案已确认 |
| 567 | 上級 | info.advanced | info / 16 | 继承 566 | 文案已确认 |
| 568 | 特上 | info.expert | info / 16 | 继承 566 | 文案已确认 |
| 569 | 鳳凰 | info.phoenix | info / 16 | 继承 566 | 文案已确认 |
| 570 | 東風 | info.east | info / 16 | 继承 566 | 文案已确认 |
| 571 | 速 東風 | info.fast_east | info / 16 | 继承 566 | 文案已确认 |
| 572 | 東南戦 | info.east_south | info / 16 | 继承 566 | 文案已确认 |
| 573 | 速 東南戦 | info.fast_east_south | info / 16 | 继承 566 | 文案已确认 |
| 574 | 喰断アリ | info.open_tanyao | info / 16 | 继承 566 | 文案已确认 |
| 575 | 喰断ナシ | info.no_open_tanyao | info / 16 | 继承 566 | 文案已确认 |
| 576 | 赤アリ | info.red | info / 16 | 继承 566 | 文案已确认 |
| 577 | 赤ナシ | info.no_red | info / 16 | 继承 566 | 文案已确认 |
| 578 | オーラス | info.all_last | info / 16 | 继承 566 | 文案已确认 |
| 579 | 若葉 | info.wakaba | info / 16 | 继承 566 | 文案已确认 |
| 580 | 銀 | info.silver | info / 16 | 继承 566 | 文案已确认 |
| 581 | 琥珀 | info.amber | info / 16 | 继承 566 | 文案已确认 |
| 582 | 孔雀 | info.peacock | info / 16 | 继承 566 | 文案已确认 |
| 583 | 祝ナシ | info.no_bonus | info / 16 | 继承 566 | 文案已确认 |
| 584 | 祝2000 | info.bonus_2000 | info / 16 | 继承 566 | 文案已确认 |
| 585 | 祝5000 | info.bonus_5000 | info / 16 | 继承 566 | 文案已确认 |
| 586 | 場300 | info.honba_300 | info / 16 | 继承 566 | 文案已确认 |
| 587 | 場1500 | info.honba_1500 | info / 16 | 继承 566 | 文案已确认 |
| 588 | 東天紅 | info.toutenkou | info / 16 | 继承 566 | 文案已确认 |
| 590 | フリテン | status.furiten | furiten / 18 |  | 文案已确认 |
| 594 | カンリョウ | caption.complete | action_caption / 56 |  | 文案已确认 |
| 595 | 交換 | button.hover.exchange | button_orange_outline / 24 |  | 文案已确认 |
| 596 | 萬 | button.hover.man | button_orange_outline / 24 |  | 文案已确认 |
| 597 | 筒 | button.hover.pin | button_orange_outline / 24 |  | 文案已确认 |
| 598 | 索 | button.hover.sou | button_orange_outline / 24 |  | 文案已确认 |
| 599 | 交換 | button.normal.exchange | button_black_outline / 24 |  | 文案已确认 |
| 600 | 萬 | button.normal.man | button_black_outline / 24 |  | 文案已确认 |
| 601 | 筒 | button.normal.pin | button_black_outline / 24 |  | 文案已确认 |
| 602 | 索 | button.normal.sou | button_black_outline / 24 |  | 文案已确认 |
| 603 | 底和 | yaku.sichuan.base | yaku / 24 |  | 文案已确认 |
| 604 | 清一色 | yaku.sichuan.full_flush | yaku / 24 |  | 文案已确认 |
| 605 | 七対 | yaku.sichuan.seven_pairs | yaku / 24 |  | 文案已确认 |
| 606 | 清七対 | yaku.sichuan.pure_seven_pairs | yaku / 24 |  | 文案已确认 |
| 607 | 竜七対 | yaku.sichuan.dragon_seven_pairs | yaku / 24 |  | 文案已确认 |
| 608 | 清竜七対 | yaku.sichuan.pure_dragon_seven_pairs | yaku / 24 |  | 文案已确认 |
| 609 | 対々和 | yaku.sichuan.all_pungs | yaku / 24 |  | 文案已确认 |
| 610 | 清対 | yaku.sichuan.pure_all_pungs | yaku / 24 |  | 文案已确认 |
| 611 | 帯幺九 | yaku.sichuan.outside_hand | yaku / 24 |  | 文案已确认 |
| 612 | 清幺九 | yaku.sichuan.pure_terminals | yaku / 24 |  | 文案已确认 |
| 613 | 将対 | yaku.sichuan.jiang_pungs | yaku / 24 |  | 文案已确认 |
| 614 | 金鉤釣 | yaku.sichuan.golden_hook | yaku / 24 |  | 文案已确认 |
| 615 | 清金鉤釣 | yaku.sichuan.pure_golden_hook | yaku / 24 |  | 文案已确认 |
| 616 | 天和 | yaku.sichuan.heavenly | yaku / 24 |  | 文案已确认 |
| 617 | 地和 | yaku.sichuan.earthly | yaku / 24 |  | 文案已确认 |
| 618 | 槓上花 | yaku.sichuan.kong_bloom | yaku / 24 |  | 文案已确认 |
| 619 | 搶槓 | yaku.sichuan.robbing_kong | yaku / 24 |  | 文案已确认 |
| 620 | 槓上炮 | yaku.sichuan.kong_discard | yaku / 24 |  | 文案已确认 |
| 621 | 根 | yaku.sichuan.root | yaku / 24 |  | 文案已确认 |
| 622 | 大四喜 | yaku.guobiao.big_four_winds | yaku / 24 |  | 文案已确认 |
| 623 | 大三元 | yaku.guobiao.big_three_dragons | yaku / 24 |  | 文案已确认 |
| 624 | 緑一色 | yaku.guobiao.all_green | yaku / 24 |  | 文案已确认 |
| 625 | 九蓮宝燈 | yaku.guobiao.nine_gates | yaku / 24 |  | 文案已确认 |
| 626 | 四槓 | yaku.guobiao.four_kongs | yaku / 24 |  | 文案已确认 |
| 627 | 連七対 | yaku.guobiao.seven_shifted_pairs | yaku / 24 |  | 文案已确认 |
| 628 | 十三幺 | yaku.guobiao.thirteen_orphans | yaku / 24 |  | 文案已确认 |
| 629 | 清幺九 | yaku.guobiao.all_terminals | yaku / 24 |  | 文案已确认 |
| 630 | 小四喜 | yaku.guobiao.little_four_winds | yaku / 24 |  | 文案已确认 |
| 631 | 小三元 | yaku.guobiao.little_three_dragons | yaku / 24 |  | 文案已确认 |
| 632 | 字一色 | yaku.guobiao.all_honors | yaku / 24 |  | 文案已确认 |
| 633 | 四暗刻 | yaku.guobiao.four_concealed_pungs | yaku / 24 |  | 文案已确认 |
| 634 | 一色双竜会 | yaku.guobiao.pure_terminal_chows | yaku / 24 |  | 文案已确认 |
| 635 | 一色四同順 | yaku.guobiao.quadruple_chow | yaku / 24 |  | 文案已确认 |
| 636 | 一色四節高 | yaku.guobiao.four_pure_shifted_pungs | yaku / 24 |  | 文案已确认 |
| 637 | 一色四歩高 | yaku.guobiao.four_pure_shifted_chows | yaku / 24 |  | 文案已确认 |
| 638 | 三槓 | yaku.guobiao.three_kongs | yaku / 24 |  | 文案已确认 |
| 639 | 混幺九 | yaku.guobiao.terminals_and_honors | yaku / 24 |  | 文案已确认 |
| 640 | 七対 | yaku.guobiao.seven_pairs | yaku / 24 |  | 文案已确认 |
| 641 | 七星不靠 | yaku.guobiao.greater_honors_knitted | yaku / 24 |  | 文案已确认 |
| 642 | 全双刻 | yaku.guobiao.all_even_pungs | yaku / 24 |  | 文案已确认 |
| 643 | 清一色 | yaku.guobiao.full_flush | yaku / 24 |  | 文案已确认 |
| 644 | 一色三同順 | yaku.guobiao.pure_triple_chow | yaku / 24 |  | 文案已确认 |
| 645 | 一色三節高 | yaku.guobiao.pure_shifted_pungs | yaku / 24 |  | 文案已确认 |
| 646 | 全大 | yaku.guobiao.upper_tiles | yaku / 24 |  | 文案已确认 |
| 647 | 全中 | yaku.guobiao.middle_tiles | yaku / 24 |  | 文案已确认 |
| 648 | 全小 | yaku.guobiao.lower_tiles | yaku / 24 |  | 文案已确认 |
| 649 | 清竜 | yaku.guobiao.pure_straight | yaku / 24 |  | 文案已确认 |
| 650 | 三色双竜会 | yaku.guobiao.three_suited_terminal_chows | yaku / 24 |  | 文案已确认 |
| 651 | 一色三歩高 | yaku.guobiao.pure_shifted_chows | yaku / 24 |  | 文案已确认 |
| 652 | 全帯五 | yaku.guobiao.all_fives | yaku / 24 |  | 文案已确认 |
| 653 | 三同刻 | yaku.guobiao.triple_pung | yaku / 24 |  | 文案已确认 |
| 654 | 三暗刻 | yaku.guobiao.three_concealed_pungs | yaku / 24 |  | 文案已确认 |
| 655 | 全不靠 | yaku.guobiao.lesser_honors_knitted | yaku / 24 |  | 文案已确认 |
| 656 | 組合竜 | yaku.guobiao.knitted_straight | yaku / 24 |  | 文案已确认 |
| 657 | 大於五 | yaku.guobiao.upper_four | yaku / 24 |  | 文案已确认 |
| 658 | 小於五 | yaku.guobiao.lower_four | yaku / 24 |  | 文案已确认 |
| 659 | 三風刻 | yaku.guobiao.big_three_winds | yaku / 24 |  | 文案已确认 |
| 660 | 花竜 | yaku.guobiao.mixed_straight | yaku / 24 |  | 文案已确认 |
| 661 | 推不倒 | yaku.guobiao.reversible_tiles | yaku / 24 |  | 文案已确认 |
| 662 | 三色三同順 | yaku.guobiao.mixed_triple_chow | yaku / 24 |  | 文案已确认 |
| 663 | 三色三節高 | yaku.guobiao.mixed_shifted_pungs | yaku / 24 |  | 文案已确认 |
| 664 | 無飜和 | yaku.guobiao.chicken_hand | yaku / 24 |  | 文案已确认 |
| 665 | 妙手回春 | yaku.guobiao.last_tile_draw | yaku / 24 |  | 文案已确认 |
| 666 | 海底撈月 | yaku.guobiao.last_tile_claim | yaku / 24 |  | 文案已确认 |
| 667 | 槓上開花 | yaku.guobiao.replacement_tile | yaku / 24 |  | 文案已确认 |
| 668 | 搶槓和 | yaku.guobiao.robbing_kong | yaku / 24 |  | 文案已确认 |
| 669 | 対々和 | yaku.guobiao.all_pungs | yaku / 24 |  | 文案已确认 |
| 670 | 混一色 | yaku.guobiao.half_flush | yaku / 24 |  | 文案已确认 |
| 671 | 三色三歩高 | yaku.guobiao.mixed_shifted_chows | yaku / 24 |  | 文案已确认 |
| 672 | 五門斉 | yaku.guobiao.all_types | yaku / 24 |  | 文案已确认 |
| 673 | 全求人 | yaku.guobiao.melded_hand | yaku / 24 |  | 文案已确认 |
| 674 | 双暗槓 | yaku.guobiao.two_concealed_kongs | yaku / 24 |  | 文案已确认 |
| 675 | 双箭刻 | yaku.guobiao.two_dragon_pungs | yaku / 24 |  | 文案已确认 |
| 676 | 全帯幺 | yaku.guobiao.outside_hand | yaku / 24 |  | 文案已确认 |
| 677 | 不求人 | yaku.guobiao.fully_concealed | yaku / 24 |  | 文案已确认 |
| 678 | 双明槓 | yaku.guobiao.two_melded_kongs | yaku / 24 |  | 文案已确认 |
| 679 | 和絶張 | yaku.guobiao.last_of_kind | yaku / 24 |  | 文案已确认 |
| 680 | 箭刻 | yaku.guobiao.dragon_pung | yaku / 24 |  | 文案已确认 |
| 681 | 圏風刻 | yaku.guobiao.prevalent_wind | yaku / 24 |  | 文案已确认 |
| 682 | 門風刻 | yaku.guobiao.seat_wind | yaku / 24 |  | 文案已确认 |
| 683 | 門前清 | yaku.guobiao.concealed_hand | yaku / 24 |  | 文案已确认 |
| 684 | 平和 | yaku.guobiao.all_chows | yaku / 24 |  | 文案已确认 |
| 685 | 四帰一 | yaku.guobiao.tile_hog | yaku / 24 |  | 文案已确认 |
| 686 | 双同刻 | yaku.guobiao.double_pung | yaku / 24 |  | 文案已确认 |
| 687 | 双暗刻 | yaku.guobiao.two_concealed_pungs | yaku / 24 |  | 文案已确认 |
| 688 | 暗槓 | yaku.guobiao.concealed_kong | yaku / 24 |  | 文案已确认 |
| 689 | 断幺 | yaku.guobiao.all_simples | yaku / 24 |  | 文案已确认 |
| 690 | 一般高 | yaku.guobiao.pure_double_chow | yaku / 24 |  | 文案已确认 |
| 691 | 喜相逢 | yaku.guobiao.mixed_double_chow | yaku / 24 |  | 文案已确认 |
| 692 | 連六 | yaku.guobiao.short_straight | yaku / 24 |  | 文案已确认 |
| 693 | 老少副 | yaku.guobiao.two_terminal_chows | yaku / 24 |  | 文案已确认 |
| 694 | 幺九刻 | yaku.guobiao.terminal_honor_pung | yaku / 24 |  | 文案已确认 |
| 695 | 明槓 | yaku.guobiao.melded_kong | yaku / 24 |  | 文案已确认 |
| 696 | 缺一門 | yaku.guobiao.one_voided_suit | yaku / 24 |  | 文案已确认 |
| 697 | 無字 | yaku.guobiao.no_honors | yaku / 24 |  | 文案已确认 |
| 698 | 辺張 | yaku.guobiao.edge_wait | yaku / 24 |  | 文案已确认 |
| 699 | 坎張 | yaku.guobiao.closed_wait | yaku / 24 |  | 文案已确认 |
| 700 | 単釣将 | yaku.guobiao.single_wait | yaku / 24 |  | 文案已确认 |
| 701 | 自摸 | yaku.guobiao.self_draw | yaku / 24 |  | 文案已确认 |
| 702 | 花牌 | yaku.guobiao.flower | yaku / 24 |  | 文案已确认 |
| 703 | 明暗槓 | yaku.guobiao.mixed_kongs | yaku / 24 |  | 文案已确认 |
| 704 | 赤羽 | info.sichuan | info / 16 | 继承 566 | 文案已确认 |
| 705 | 国標 | info.guobiao | info / 16 | 继承 566 | 文案已确认 |

## 保留图像

| OriginId | 目录键 | 核对 |
| ---: | --- | --- |
| 401 | legacy_image_401 | |
| 402 | legacy_image_402 | |
| 440 | legacy_image_440 | |
| 466 | legacy_image_466 | |
| 589 | legacy_image_589 | |
| 591 | legacy_image_591 | |
| 592 | legacy_image_592 | |

## 规则映射

| 玩法 | 服务端名 | 显示名 | 缺省分值 | 核对 |
| --- | --- | --- | ---: | --- |
| guobiao | 大四喜 | 大四喜 | 88 |  |
| guobiao | 大三元 | 大三元 | 88 |  |
| guobiao | 绿一色 | 緑一色 | 88 |  |
| guobiao | 九莲宝灯 | 九蓮宝燈 | 88 |  |
| guobiao | 四杠 | 四槓 | 88 |  |
| guobiao | 连七对 | 連七対 | 88 |  |
| guobiao | 十三幺 | 十三幺 | 88 |  |
| guobiao | 清幺九 | 清幺九 | 64 |  |
| guobiao | 小四喜 | 小四喜 | 64 |  |
| guobiao | 小三元 | 小三元 | 64 |  |
| guobiao | 字一色 | 字一色 | 64 |  |
| guobiao | 四暗刻 | 四暗刻 | 64 |  |
| guobiao | 一色双龙会 | 一色双竜会 | 64 |  |
| guobiao | 一色四同顺 | 一色四同順 | 48 |  |
| guobiao | 一色四节高 | 一色四節高 | 48 |  |
| guobiao | 一色四步高 | 一色四歩高 | 32 |  |
| guobiao | 三杠 | 三槓 | 32 |  |
| guobiao | 混幺九 | 混幺九 | 32 |  |
| guobiao | 七对 | 七対 | 24 |  |
| guobiao | 七星不靠 | 七星不靠 | 24 |  |
| guobiao | 全双刻 | 全双刻 | 24 |  |
| guobiao | 清一色 | 清一色 | 24 |  |
| guobiao | 一色三同顺 | 一色三同順 | 24 |  |
| guobiao | 一色三节高 | 一色三節高 | 24 |  |
| guobiao | 全大 | 全大 | 24 |  |
| guobiao | 全中 | 全中 | 24 |  |
| guobiao | 全小 | 全小 | 24 |  |
| guobiao | 清龙 | 清竜 | 16 |  |
| guobiao | 三色双龙会 | 三色双竜会 | 16 |  |
| guobiao | 一色三步高 | 一色三歩高 | 16 |  |
| guobiao | 全带五 | 全帯五 | 16 |  |
| guobiao | 三同刻 | 三同刻 | 16 |  |
| guobiao | 三暗刻 | 三暗刻 | 16 |  |
| guobiao | 全不靠 | 全不靠 | 12 |  |
| guobiao | 组合龙 | 組合竜 | 12 |  |
| guobiao | 大于五 | 大於五 | 12 |  |
| guobiao | 小于五 | 小於五 | 12 |  |
| guobiao | 三风刻 | 三風刻 | 12 |  |
| guobiao | 花龙 | 花竜 | 8 |  |
| guobiao | 推不倒 | 推不倒 | 8 |  |
| guobiao | 三色三同顺 | 三色三同順 | 8 |  |
| guobiao | 三色三节高 | 三色三節高 | 8 |  |
| guobiao | 无番和 | 無飜和 | 8 |  |
| guobiao | 妙手回春 | 妙手回春 | 8 |  |
| guobiao | 海底捞月 | 海底撈月 | 8 |  |
| guobiao | 杠上开花 | 槓上開花 | 8 |  |
| guobiao | 抢杠和 | 搶槓和 | 8 |  |
| guobiao | 碰碰和 | 対々和 | 6 |  |
| guobiao | 混一色 | 混一色 | 6 |  |
| guobiao | 三色三步高 | 三色三歩高 | 6 |  |
| guobiao | 五门齐 | 五門斉 | 6 |  |
| guobiao | 全求人 | 全求人 | 6 |  |
| guobiao | 双暗杠 | 双暗槓 | 6 |  |
| guobiao | 双箭刻 | 双箭刻 | 6 |  |
| guobiao | 全带幺 | 全帯幺 | 4 |  |
| guobiao | 不求人 | 不求人 | 4 |  |
| guobiao | 双明杠 | 双明槓 | 4 |  |
| guobiao | 和绝张 | 和絶張 | 4 |  |
| guobiao | 箭刻 | 箭刻 | 2 |  |
| guobiao | 圈风刻 | 圏風刻 | 2 |  |
| guobiao | 门风刻 | 門風刻 | 2 |  |
| guobiao | 门前清 | 門前清 | 2 |  |
| guobiao | 平和 | 平和 | 2 |  |
| guobiao | 四归一 | 四帰一 | 2 |  |
| guobiao | 双同刻 | 双同刻 | 2 |  |
| guobiao | 双暗刻 | 双暗刻 | 2 |  |
| guobiao | 暗杠 | 暗槓 | 2 |  |
| guobiao | 断幺 | 断幺 | 2 |  |
| guobiao | 一般高 | 一般高 | 1 |  |
| guobiao | 喜相逢 | 喜相逢 | 1 |  |
| guobiao | 连六 | 連六 | 1 |  |
| guobiao | 老少副 | 老少副 | 1 |  |
| guobiao | 幺九刻 | 幺九刻 | 1 |  |
| guobiao | 明杠 | 明槓 | 1 |  |
| guobiao | 缺一门 | 缺一門 | 1 |  |
| guobiao | 无字 | 無字 | 1 |  |
| guobiao | 边张 | 辺張 | 1 |  |
| guobiao | 坎张 | 坎張 | 1 |  |
| guobiao | 单钓将 | 単釣将 | 1 |  |
| guobiao | 自摸 | 自摸 | 1 |  |
| guobiao | 花牌 | 花牌 | 1 |  |
| guobiao | 明暗杠 | 明暗槓 | 5 |  |
| sichuan | 底和 | 底和 | 0 |  |
| sichuan | 清一色 | 清一色 | 0 |  |
| sichuan | 七对 | 七対 | 0 |  |
| sichuan | 清七对 | 清七対 | 0 |  |
| sichuan | 龙七对 | 竜七対 | 0 |  |
| sichuan | 清龙七对 | 清竜七対 | 0 |  |
| sichuan | 对对和 | 対々和 | 0 |  |
| sichuan | 清对 | 清対 | 0 |  |
| sichuan | 带幺九 | 帯幺九 | 0 |  |
| sichuan | 清幺九 | 清幺九 | 0 |  |
| sichuan | 将对 | 将対 | 0 |  |
| sichuan | 金钩钓 | 金鉤釣 | 0 |  |
| sichuan | 清金钩钓 | 清金鉤釣 | 0 |  |
| sichuan | 天和 | 天和 | 0 |  |
| sichuan | 地和 | 地和 | 0 |  |
| sichuan | 杠上花 | 槓上花 | 0 |  |
| sichuan | 抢杠 | 搶槓 | 0 |  |
| sichuan | 杠上炮 | 槓上炮 | 0 |  |
| sichuan | 根 | 根 | 0 |  |

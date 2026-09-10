#pragma once

#include <dswros/radar_preferences.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace dswros {

enum class RadarTooltipId : std::uint8_t {
    Treasure, Boss, Assault, MiniGames,
    AreaQuests, BirdEggs, Clock, SceneTreasure,
    SceneAreaQuests, SceneMiniGames, HeightTreasure, HeightAreaQuests,
    HeightMole, HeightBoss, HeightAssault, AreaQuestAvailable,
    AreaQuestAll, AssaultAvailable, AssaultAll, SceneRange,
    SceneLimit, DistanceOff, DistanceAim, DistanceAuto,
    DistanceAll, Language, ModStatus, EnableDisable,
    RestoreDefaults, BugReport, Close, Endorse, AllRadar, AllMap, Count,
};

inline constexpr std::size_t kRadarTooltipCount =
    static_cast<std::size_t>(RadarTooltipId::Count);

struct RadarLocalizedText {
    const wchar_t* language_name{};
    const wchar_t* title{};
    const wchar_t* language{};
    const wchar_t* automatic{};
    const wchar_t* marker_visibility{};
    const wchar_t* radar{};
    const wchar_t* map{};
    const wchar_t* scene{};
    std::array<const wchar_t*, 7> marker_categories{};
    const wchar_t* height_indicators{};
    const wchar_t* radar_only{};
    std::array<const wchar_t*, static_cast<std::size_t>(
        HeightIndicatorCategory::Count)> height_categories{};
    const wchar_t* filter_modes{};
    const wchar_t* available{};
    const wchar_t* all{};
    const wchar_t* close{};
    const wchar_t* status{};
    const wchar_t* status_off{};
    const wchar_t* status_on{};
    const wchar_t* status_fault{};
    const wchar_t* enable_mod{};
    const wchar_t* disable_mod{};
    const wchar_t* retry_mod{};
    const wchar_t* bug_report{};
    const wchar_t* scene_settings{};
    const wchar_t* scene_range{};
    const wchar_t* scene_limit{};
    const wchar_t* scene_distance{};
    std::array<const wchar_t*, 4> scene_distance_modes{};
    const wchar_t* restore_defaults{};
    const wchar_t* all_markers{};
    std::array<const wchar_t*, kRadarTooltipCount> tooltips{};
};

inline constexpr std::array<RadarLocalizedText, kRadarUiLanguageCount>
    kRadarLocalizedText{{
        {
            L"English", L"Radar settings", L"Language", L"Use game language",
            L"Marker visibility", L"Radar", L"Map", L"Scene",
            {L"Clock", L"Treasure", L"Bosses", L"Sudden missions",
             L"Mini-games", L"Area quests", L"Bird eggs"},
            L"Height indicators", L"Radar only",
            {L"Treasure", L"Area quests", L"Mini-games", L"Bosses", L"Sudden missions"},
            L"Filter modes", L"Available", L"All", L"Close",
            L"Status", L"Off", L"On", L"Error", L"Enable",
            L"Disable", L"Retry", L"Feedback",
            L"In-world markers", L"Range", L"Marker limit", L"Distance labels",
            {L"Off", L"Aim focus", L"Auto focus", L"All"},
            L"Reset to defaults", L"All",
            {
                L"Show unopened chests. White: ordinary; green: mini-game rewards; orange: treasure maps or legendary; blue: puzzles.",
                L"Show the known locations of world bosses.",
                L"Show sudden mission locations using the selected filter.",
                L"Show the known locations of supported mini-games.",
                L"Show area quest locations using the selected filter.",
                L"Show nearby collectible bird eggs loaded by the game.",
                L"Show the game clock on the minimap.",
                L"Show chest symbols directly in the game view.",
                L"Show gray diamonds for area quests in the game view.",
                L"Show purple flags for mini-games in the game view.",
                L"Indicate whether chests are above or below you.",
                L"Indicate whether area quests are above or below you.",
                L"Indicate whether mini-games are above or below you.",
                L"Indicate whether boss locations are above or below you.",
                L"Indicate whether sudden mission locations are above or below you.",
                L"Show unfinished area quests whose prerequisites are met.",
                L"Show all unfinished area quests, regardless of prerequisites.",
                L"Show sudden missions that are currently available.",
                L"Show all known sudden mission locations, including unavailable ones.",
                L"Set the shared range for enabled in-world markers. Zero hides them.",
                L"Limit all enabled in-world markers. Zero hides them; higher limits may affect performance.",
                L"Hide distance labels while keeping in-world markers visible.",
                L"Aim near a marker and pause briefly to show its distance.",
                L"Show the distance to the marker nearest screen center, without frequent switching.",
                L"Show distances for every visible in-world marker.",
                L"Choose the interface language, or use the game language.",
                L"Shows whether Radar is enabled, disabled or has encountered an error.",
                L"Turn Radar on or off, or retry after an error.",
                L"Restore default display settings and use the game language.",
                L"Open Nexus Mods to share suggestions or report a problem.",
                L"Close settings and keep your changes.",
                L"Open Nexus Mods to vote for this mod in the monthly selection.",
                L"Show or hide every radar category, including the clock and bird eggs.",
                L"Show or hide every supported map category.",
            },
        },
        {
            L"日本語", L"レーダー設定", L"言語", L"ゲーム言語を使用",
            L"マーカー表示", L"レーダー", L"マップ", L"シーン",
            {L"時計", L"宝箱", L"ボス", L"突発ミッション", L"ミニゲーム",
             L"エリアクエスト", L"鳥の卵"},
            L"高低差表示", L"レーダーのみ",
            {L"宝箱", L"エリアクエスト", L"ミニゲーム", L"ボス", L"突発ミッション"},
            L"絞り込み", L"参加可能", L"すべて", L"閉じる",
            L"状態", L"オフ", L"オン", L"異常", L"有効",
            L"無効", L"再試行", L"ご意見・不具合",
            L"画面内マーカー", L"表示距離", L"表示数上限", L"距離表示",
            {L"非表示", L"照準で表示", L"自動フォーカス", L"すべて表示"},
            L"初期設定に戻す", L"すべて",
            {
                L"未開封の宝箱を表示します。白は通常、緑はミニゲーム報酬、橙は宝の地図・伝説、青は謎解きです。",
                L"既知のワールドボスの出現場所を表示します。",
                L"選択したフィルターに従って突発ミッションの場所を表示します。",
                L"対応しているミニゲームの場所を表示します。",
                L"選択したフィルターに従ってエリアクエストの場所を表示します。",
                L"周囲に読み込まれた、拾える鳥の卵を表示します。",
                L"ミニマップにゲーム内の時計を表示します。",
                L"ゲーム画面に宝箱のアイコンを表示します。",
                L"ゲーム画面に灰色の菱形でエリアクエストを表示します。",
                L"ゲーム画面に紫の旗でミニゲームを表示します。",
                L"宝箱が自分より上か下かを示します。",
                L"エリアクエストが自分より上か下かを示します。",
                L"ミニゲームが自分より上か下かを示します。",
                L"ボスの場所が自分より上か下かを示します。",
                L"突発ミッションの場所が自分より上か下かを示します。",
                L"前提条件を満たした未完了のエリアクエストを表示します。",
                L"前提条件にかかわらず、未完了のエリアクエストをすべて表示します。",
                L"現在参加できる突発ミッションを表示します。",
                L"現在参加できないものも含め、既知の突発ミッションの場所をすべて表示します。",
                L"有効にした画面内マーカーで共通の表示距離です。0にすると非表示になります。",
                L"画面内マーカーの合計表示数です。0で非表示になり、増やすと動作が重くなる場合があります。",
                L"画面内マーカーを残し、距離の文字だけを隠します。",
                L"マーカー付近を狙って少し待つと、その距離を表示します。",
                L"画面中央に最も近いマーカーの距離を自動表示し、頻繁な切り替えを抑えます。",
                L"表示中のすべての画面内マーカーに距離を表示します。",
                L"表示言語を選ぶか、ゲームの言語に合わせます。",
                L"レーダーがオン・オフ・エラーのどの状態かを示します。",
                L"レーダーをオン・オフにするか、エラー後に再試行します。",
                L"表示設定を初期値に戻し、言語をゲームに合わせます。",
                L"Nexus Modsを開いて、ご意見や不具合を投稿します。",
                L"変更を保存して設定を閉じます。",
                L"今月のModに投票するため、Nexus Modsのページを開きます。",
                L"時計と鳥の卵を含む、すべてのレーダー項目を切り替えます。",
                L"マップ上の対応項目をまとめて切り替えます。",
            },
        },
        {
            L"한국어", L"레이더 설정", L"언어", L"게임 언어 사용",
            L"마커 표시", L"레이더", L"지도", L"화면",
            {L"시계", L"보물상자", L"보스", L"돌발 임무", L"미니게임",
             L"지역 퀘스트", L"새알"},
            L"높이 표시", L"레이더 전용",
            {L"보물상자", L"지역 퀘스트", L"미니게임", L"보스", L"돌발 임무"},
            L"필터 모드", L"이용 가능", L"전체", L"닫기",
            L"상태", L"꺼짐", L"켜짐", L"오류", L"켜기",
            L"끄기", L"재시도", L"의견 / 문제",
            L"화면 내 마커", L"표시 거리", L"최대 표시 개수", L"거리 표시",
            {L"숨김", L"조준 표시", L"자동 초점", L"모두 표시"},
            L"기본값 복원", L"전체",
            {
                L"미개봉 상자를 표시합니다. 흰색은 일반, 초록은 미니게임 보상, 주황은 보물지도·전설, 파랑은 퍼즐입니다.",
                L"알려진 월드 보스의 위치를 표시합니다.",
                L"선택한 필터에 따라 돌발 임무의 위치를 표시합니다.",
                L"지원되는 미니게임의 위치를 표시합니다.",
                L"선택한 필터에 따라 지역 퀘스트의 위치를 표시합니다.",
                L"주변에서 게임이 불러온 채집 가능한 새알을 표시합니다.",
                L"미니맵에 게임 내 시계를 표시합니다.",
                L"게임 화면에 상자 아이콘을 표시합니다.",
                L"게임 화면에 회색 마름모로 지역 퀘스트를 표시합니다.",
                L"게임 화면에 보라색 깃발로 미니게임을 표시합니다.",
                L"상자가 나보다 위에 있는지 아래에 있는지 표시합니다.",
                L"지역 퀘스트가 나보다 위에 있는지 아래에 있는지 표시합니다.",
                L"미니게임이 나보다 위에 있는지 아래에 있는지 표시합니다.",
                L"보스 위치가 나보다 위에 있는지 아래에 있는지 표시합니다.",
                L"돌발 임무 위치가 나보다 위에 있는지 아래에 있는지 표시합니다.",
                L"선행 조건을 충족한 미완료 지역 퀘스트를 표시합니다.",
                L"선행 조건과 관계없이 미완료 지역 퀘스트를 모두 표시합니다.",
                L"지금 참여할 수 있는 돌발 임무를 표시합니다.",
                L"현재 참여할 수 없는 곳을 포함해 알려진 돌발 임무 위치를 모두 표시합니다.",
                L"활성화한 화면 내 마커에 공통으로 적용되는 표시 거리입니다. 0이면 마커를 숨깁니다.",
                L"화면 내 마커의 최대 표시 개수입니다. 0이면 숨기며, 늘리면 성능에 영향을 줄 수 있습니다.",
                L"화면 마커는 유지하고 거리 표시만 숨깁니다.",
                L"마커 근처를 조준하고 잠시 멈추면 거리를 표시합니다.",
                L"화면 중앙에 가장 가까운 마커의 거리를 자동으로 표시하며 잦은 대상 전환을 줄입니다.",
                L"보이는 모든 화면 마커에 거리를 표시합니다.",
                L"표시 언어를 선택하거나 게임 언어를 사용합니다.",
                L"레이더의 켜짐, 꺼짐 또는 오류 상태를 보여 줍니다.",
                L"레이더를 켜거나 끄고, 오류가 나면 다시 시도합니다.",
                L"표시 설정을 기본값으로 되돌리고 게임 언어를 사용합니다.",
                L"Nexus Mods를 열어 의견이나 문제를 남깁니다.",
                L"변경 사항을 저장하고 설정을 닫습니다.",
                L"이 모드를 이달의 모드로 뽑을 수 있는 Nexus Mods 페이지를 엽니다.",
                L"시계와 새알을 포함한 모든 레이더 항목을 켜거나 끕니다.",
                L"지원되는 모든 지도 항목을 켜거나 끕니다.",
            },
        },
        {
            L"简体中文", L"雷达设置", L"语言", L"跟随游戏语言",
            L"标记显示", L"雷达", L"地图", L"场景",
            {L"时钟", L"宝箱", L"世界首领", L"突发任务", L"小游戏",
             L"区域任务", L"鸟蛋"},
            L"高度提示", L"仅雷达",
            {L"宝箱", L"区域任务", L"小游戏", L"世界首领", L"突发任务"},
            L"筛选模式", L"可用", L"全部", L"关闭",
            L"模组状态", L"未开启", L"已开启", L"故障", L"开启",
            L"关闭", L"重试", L"建议/问题汇报",
            L"场景标记", L"显示范围", L"数量上限", L"距离显示",
            {L"不显示", L"瞄准显示", L"自动聚焦", L"全部显示"},
            L"恢复默认", L"全部",
            {
                L"显示未开启的宝箱。白色为普通，绿色为小游戏奖励，橙色为藏宝图或传说，蓝色为解谜。",
                L"显示已知的世界首领位置。",
                L"按所选筛选模式显示突发任务位置。",
                L"显示已知的小游戏位置。",
                L"按所选筛选模式显示区域任务位置。",
                L"显示附近已加载、可以采集的鸟蛋。",
                L"在小地图上显示游戏内时钟。",
                L"在游戏画面中显示宝箱图标。",
                L"在游戏画面中用灰色菱形标出区域任务。",
                L"在游戏画面中用紫色旗帜标出小游戏。",
                L"提示宝箱在你的上方还是下方。",
                L"提示区域任务在你的上方还是下方。",
                L"提示小游戏在你的上方还是下方。",
                L"提示世界首领位置在你的上方还是下方。",
                L"提示突发任务位置在你的上方还是下方。",
                L"只显示已满足前置条件、尚未完成的区域任务。",
                L"显示所有未完成的区域任务，不限前置条件。",
                L"只显示当前可以参与的突发任务。",
                L"显示全部已知突发任务位置，包括暂不可参与的任务。",
                L"已开启的场景标记共用此范围；设为0时隐藏标记。",
                L"所有场景标记共用此数量上限；0为隐藏，数量越高越可能影响性能。",
                L"隐藏距离文字，保留场景标记。",
                L"瞄准标记附近，稍停片刻便显示距离。",
                L"自动显示最靠近屏幕中心的标记距离，避免频繁切换目标。",
                L"为所有可见的场景标记显示距离。",
                L"选择界面语言，也可跟随游戏语言。",
                L"查看雷达当前的开启、关闭或故障状态。",
                L"开启或关闭雷达，也可在故障后重试。",
                L"恢复默认显示设置，并让界面语言跟随游戏。",
                L"打开 Nexus Mods，提交建议或问题。",
                L"保存当前更改并关闭设置。",
                L"打开 Nexus Mods，在月度模组评选中为它投票。",
                L"统一开关雷达上的所有类别，包括时钟和鸟蛋。",
                L"统一开关地图上支持的所有类别。",
            },
        },
        {
            L"繁體中文", L"雷達設定", L"語言", L"跟隨遊戲語言",
            L"標記顯示", L"雷達", L"地圖", L"場景",
            {L"時鐘", L"寶箱", L"世界首領", L"突發任務", L"小遊戲",
             L"區域任務", L"鳥蛋"},
            L"高度提示", L"僅雷達",
            {L"寶箱", L"區域任務", L"小遊戲", L"世界首領", L"突發任務"},
            L"篩選模式", L"可用", L"全部", L"關閉",
            L"模組狀態", L"未啟用", L"已啟用", L"故障", L"啟用",
            L"停用", L"重試", L"建議/問題回報",
            L"場景標記", L"顯示範圍", L"數量上限", L"距離顯示",
            {L"不顯示", L"瞄準顯示", L"自動聚焦", L"全部顯示"},
            L"恢復預設值", L"全部",
            {
                L"顯示未開啟的寶箱。白色為普通，綠色為小遊戲獎勵，橙色為藏寶圖或傳說，藍色為解謎。",
                L"顯示已知的世界首領位置。",
                L"依所選篩選模式顯示突發任務位置。",
                L"顯示已知的小遊戲位置。",
                L"依所選篩選模式顯示區域任務位置。",
                L"顯示附近已載入、可以採集的鳥蛋。",
                L"在小地圖上顯示遊戲內時鐘。",
                L"在遊戲畫面中顯示寶箱圖示。",
                L"在遊戲畫面中用灰色菱形標出區域任務。",
                L"在遊戲畫面中用紫色旗幟標出小遊戲。",
                L"提示寶箱在你的上方還是下方。",
                L"提示區域任務在你的上方還是下方。",
                L"提示小遊戲在你的上方還是下方。",
                L"提示世界首領位置在你的上方還是下方。",
                L"提示突發任務位置在你的上方還是下方。",
                L"只顯示已滿足前置條件、尚未完成的區域任務。",
                L"顯示所有未完成的區域任務，不限前置條件。",
                L"只顯示目前可以參與的突發任務。",
                L"顯示全部已知突發任務位置，包括暫時無法參與的任務。",
                L"已開啟的場景標記共用此範圍；設為0時隱藏標記。",
                L"所有場景標記共用此數量上限；0為隱藏，數量越高越可能影響效能。",
                L"隱藏距離文字，保留場景標記。",
                L"瞄準標記附近，稍停片刻便顯示距離。",
                L"自動顯示最靠近畫面中央的標記距離，避免頻繁切換目標。",
                L"為所有可見的場景標記顯示距離。",
                L"選擇介面語言，也可跟隨遊戲語言。",
                L"查看雷達目前的啟用、停用或故障狀態。",
                L"啟用或停用雷達，也可在故障後重試。",
                L"恢復預設顯示設定，並讓介面語言跟隨遊戲。",
                L"開啟 Nexus Mods，提交建議或問題。",
                L"儲存目前的變更並關閉設定。",
                L"開啟 Nexus Mods，在每月模組評選中為它投票。",
                L"統一開關雷達上的所有類別，包括時鐘和鳥蛋。",
                L"統一開關地圖上支援的所有類別。",
            },
        },
        {
            L"Français", L"Paramètres du radar", L"Langue", L"Langue du jeu",
            L"Visibilité des marqueurs", L"Radar", L"Carte", L"Scène",
            {L"Horloge", L"Trésors", L"Boss", L"Missions imprévues", L"Mini-jeux",
             L"Quêtes de zone", L"Œufs d'oiseau"},
            L"Indicateurs de hauteur", L"Radar uniquement",
            {L"Trésors", L"Quêtes de zone", L"Mini-jeux", L"Boss", L"Missions imprévues"},
            L"Modes de filtrage", L"Disponibles", L"Toutes", L"Fermer",
            L"État", L"Arrêt", L"Actif", L"Erreur", L"Activer",
            L"Désactiver", L"Réessayer", L"Avis / problèmes",
            L"Marqueurs en jeu", L"Portée", L"Nombre max.", L"Distances",
            {L"Masquer", L"Visée", L"Autofocus", L"Toutes"},
            L"Réinitialiser", L"Tout",
            {
                L"Afficher les coffres fermés. Blanc : ordinaires ; vert : récompenses de mini-jeux ; orange : cartes au trésor ou légendaires ; bleu : énigmes.",
                L"Afficher les emplacements connus des boss du monde.",
                L"Afficher les missions imprévues selon le filtre choisi.",
                L"Afficher les emplacements des mini-jeux pris en charge.",
                L"Afficher les quêtes de zone selon le filtre choisi.",
                L"Afficher les œufs d'oiseau à ramasser déjà chargés à proximité.",
                L"Afficher l'horloge du jeu sur la mini-carte.",
                L"Afficher les icônes de coffres directement dans le monde du jeu.",
                L"Signaler les quêtes de zone par des losanges gris dans le monde du jeu.",
                L"Signaler les mini-jeux par des drapeaux violets dans le monde du jeu.",
                L"Indiquer si les coffres sont au-dessus ou au-dessous de vous.",
                L"Indiquer si les quêtes de zone sont au-dessus ou au-dessous de vous.",
                L"Indiquer si les mini-jeux sont au-dessus ou au-dessous de vous.",
                L"Indiquer si les emplacements des boss sont au-dessus ou au-dessous de vous.",
                L"Indiquer si les missions imprévues sont au-dessus ou au-dessous de vous.",
                L"Afficher les quêtes de zone inachevées dont les prérequis sont remplis.",
                L"Afficher toutes les quêtes de zone inachevées, quels que soient leurs prérequis.",
                L"Afficher les missions imprévues actuellement disponibles.",
                L"Afficher tous les lieux de missions imprévues, même celles qui sont indisponibles.",
                L"Portée commune aux marqueurs en jeu activés. À 0, ils sont masqués.",
                L"Limite totale des marqueurs en jeu. À 0, ils sont masqués ; l'augmenter peut réduire les performances.",
                L"Masquer les distances tout en gardant les marqueurs en jeu.",
                L"Visez près d'un marqueur un court instant pour afficher sa distance.",
                L"Afficher la distance du marqueur le plus proche du centre de l'écran, sans changements incessants.",
                L"Afficher la distance de chaque marqueur visible dans le monde du jeu.",
                L"Choisir la langue de l'interface ou utiliser celle du jeu.",
                L"Indique si le radar est actif, désactivé ou en erreur.",
                L"Activer ou désactiver le radar, ou réessayer après une erreur.",
                L"Rétablir l'affichage par défaut et reprendre la langue du jeu.",
                L"Ouvrir Nexus Mods pour proposer une idée ou signaler un problème.",
                L"Enregistrer les changements et fermer les paramètres.",
                L"Ouvrir Nexus Mods pour voter pour ce mod comme mod du mois.",
                L"Afficher ou masquer toutes les catégories du radar, y compris l’horloge et les œufs.",
                L"Afficher ou masquer toutes les catégories prises en charge sur la carte.",
            },
        },
        {
            L"Deutsch", L"Radar-Einstellungen", L"Sprache", L"Spielsprache",
            L"Markierungsanzeige", L"Radar", L"Karte", L"Szene",
            {L"Uhr", L"Schätze", L"Bosse", L"Spontanmissionen", L"Minispiele",
             L"Gebietsquests", L"Vogeleier"},
            L"Höhenanzeigen", L"Nur Radar",
            {L"Schätze", L"Gebietsquests", L"Minispiele", L"Bosse", L"Spontanmissionen"},
            L"Filtermodi", L"Verfügbar", L"Alle", L"Schließen",
            L"Status", L"Aus", L"An", L"Fehler", L"Aktivieren",
            L"Deaktivieren", L"Wiederholen", L"Feedback",
            L"Markierungen im Spiel", L"Reichweite", L"Max. Markierungen", L"Entfernungen",
            {L"Aus", L"Zielfokus", L"Auto-Fokus", L"Alle"},
            L"Zurücksetzen", L"Alle",
            {
                L"Ungeöffnete Truhen anzeigen. Weiß: gewöhnlich; Grün: Minispiel-Belohnungen; Orange: Schatzkarten oder legendär; Blau: Rätsel.",
                L"Bekannte Standorte von Weltbossen anzeigen.",
                L"Spontanmissionen entsprechend dem gewählten Filter anzeigen.",
                L"Standorte der unterstützten Minispiele anzeigen.",
                L"Gebietsquests entsprechend dem gewählten Filter anzeigen.",
                L"Vogeleier in der Nähe anzeigen, die geladen und einsammelbar sind.",
                L"Die Uhrzeit im Spiel auf der Minikarte anzeigen.",
                L"Truhensymbole direkt in der Spielansicht anzeigen.",
                L"Gebietsquests mit grauen Rauten in der Spielansicht markieren.",
                L"Minispiele mit violetten Fahnen in der Spielansicht markieren.",
                L"Anzeigen, ob Truhen über oder unter dir liegen.",
                L"Anzeigen, ob Gebietsquests über oder unter dir liegen.",
                L"Anzeigen, ob Minispiele über oder unter dir liegen.",
                L"Anzeigen, ob Bossstandorte über oder unter dir liegen.",
                L"Anzeigen, ob Spontanmissionen über oder unter dir liegen.",
                L"Unabgeschlossene Gebietsquests mit erfüllten Voraussetzungen anzeigen.",
                L"Alle unabgeschlossenen Gebietsquests anzeigen, unabhängig von ihren Voraussetzungen.",
                L"Aktuell verfügbare Spontanmissionen anzeigen.",
                L"Alle bekannten Orte von Spontanmissionen anzeigen, auch derzeit nicht verfügbare.",
                L"Gemeinsame Reichweite der aktiven Markierungen in der Spielwelt. 0 blendet sie aus.",
                L"Gesamtlimit für Markierungen in der Spielwelt. 0 blendet sie aus; höhere Werte können die Leistung beeinträchtigen.",
                L"Entfernungen ausblenden und die Markierungen in der Spielwelt beibehalten.",
                L"Kurz auf eine Markierung zielen, um ihre Entfernung anzuzeigen.",
                L"Die Entfernung zur Markierung am nächsten an der Bildschirmmitte anzeigen, ohne ständige Zielwechsel.",
                L"Entfernungen für alle sichtbaren Markierungen in der Spielwelt anzeigen.",
                L"Die Anzeigesprache wählen oder die Spielsprache übernehmen.",
                L"Zeigt an, ob das Radar an, aus oder im Fehlerzustand ist.",
                L"Radar ein- oder ausschalten oder nach einem Fehler erneut versuchen.",
                L"Anzeige auf Standardwerte zurücksetzen und die Spielsprache übernehmen.",
                L"Nexus Mods für Vorschläge oder Problemmeldungen öffnen.",
                L"Änderungen speichern und die Einstellungen schließen.",
                L"Die Nexus-Mods-Seite öffnen, um für diesen Mod als Mod des Monats abzustimmen.",
                L"Alle Radar-Kategorien einschließlich Uhr und Vogeleiern ein- oder ausblenden.",
                L"Alle unterstützten Karten-Kategorien ein- oder ausblenden.",
            },
        },
        {
            L"Español (España)", L"Ajustes del radar", L"Idioma",
            L"Idioma del juego", L"Visibilidad de marcadores", L"Radar", L"Mapa", L"Escena",
            {L"Reloj", L"Tesoros", L"Jefes", L"Misiones repentinas", L"Minijuegos",
             L"Misiones de zona", L"Huevos de ave"},
            L"Indicadores de altura", L"Solo radar",
            {L"Tesoros", L"Misiones de zona", L"Minijuegos", L"Jefes", L"Misiones repentinas"},
            L"Modos de filtro", L"Disponibles", L"Todas", L"Cerrar",
            L"Estado", L"Apagado", L"Activo", L"Error", L"Activar",
            L"Desactivar", L"Reintentar", L"Sugerencias",
            L"Marcadores en el mundo", L"Alcance", L"Límite total", L"Distancias",
            {L"Ocultar", L"Al apuntar", L"Autoenfoque", L"Todas"},
            L"Restablecer", L"Todo",
            {
                L"Mostrar cofres sin abrir. Blanco: normales; verde: recompensas de minijuegos; naranja: mapas del tesoro o legendarios; azul: puzles.",
                L"Mostrar las ubicaciones conocidas de los jefes del mundo.",
                L"Mostrar misiones repentinas según el filtro elegido.",
                L"Mostrar las ubicaciones de los minijuegos compatibles.",
                L"Mostrar misiones de zona según el filtro elegido.",
                L"Mostrar huevos de ave cercanos ya cargados que se puedan recoger.",
                L"Mostrar el reloj del juego en el minimapa.",
                L"Mostrar iconos de cofres en el mundo del juego.",
                L"Marcar las misiones de zona con rombos grises en el mundo del juego.",
                L"Marcar los minijuegos con banderas moradas en el mundo del juego.",
                L"Indicar si los cofres están por encima o por debajo de ti.",
                L"Indicar si las misiones de zona están por encima o por debajo de ti.",
                L"Indicar si los minijuegos están por encima o por debajo de ti.",
                L"Indicar si las ubicaciones de los jefes están por encima o por debajo de ti.",
                L"Indicar si las misiones repentinas están por encima o por debajo de ti.",
                L"Mostrar misiones de zona pendientes cuyos requisitos se cumplan.",
                L"Mostrar todas las misiones de zona pendientes, sin filtrar por requisitos.",
                L"Mostrar las misiones repentinas disponibles en este momento.",
                L"Mostrar todas las ubicaciones de misiones repentinas, incluidas las no disponibles.",
                L"Alcance común de los marcadores activados en el mundo. A 0, se ocultan.",
                L"Límite total de marcadores en el mundo. 0 los oculta; aumentarlo puede afectar al rendimiento.",
                L"Ocultar las distancias sin ocultar los marcadores del mundo.",
                L"Apunta cerca de un marcador un momento para ver su distancia.",
                L"Mostrar la distancia del marcador más cercano al centro de la pantalla, sin cambios constantes de objetivo.",
                L"Mostrar distancias para todos los marcadores visibles en el mundo.",
                L"Elegir el idioma de la interfaz o usar el del juego.",
                L"Indica si el radar está activo, apagado o en error.",
                L"Activar o desactivar el radar, o reintentar tras un error.",
                L"Restablecer la visualización y usar el idioma del juego.",
                L"Abrir Nexus Mods para enviar sugerencias o informar de problemas.",
                L"Guardar los cambios y cerrar los ajustes.",
                L"Abrir Nexus Mods para votar por este mod como mod del mes.",
                L"Mostrar u ocultar todas las categorías del radar, incluidos el reloj y los huevos.",
                L"Mostrar u ocultar todas las categorías compatibles del mapa.",
            },
        },
        {
            L"Русский", L"Настройки радара", L"Язык", L"Язык игры",
            L"Отображение меток", L"Радар", L"Карта", L"Сцена",
            {L"Часы", L"Сокровища", L"Боссы", L"Внезапные задания",
             L"Мини-игры", L"Задания области", L"Птичьи яйца"},
            L"Указатели высоты", L"Только радар",
            {L"Сокровища", L"Задания области", L"Мини-игры", L"Боссы", L"Внезапные задания"},
            L"Режимы фильтра", L"Доступные", L"Все", L"Закрыть",
            L"Статус", L"Выкл", L"Вкл", L"Ошибка", L"Включить",
            L"Выключить", L"Повторить", L"Отзывы",
            L"Метки в мире", L"Дальность", L"Лимит меток", L"Расстояния",
            {L"Скрыть", L"Прицел", L"Автофокус", L"Все"},
            L"Сбросить", L"Всё",
            {
                L"Показывать закрытые сундуки. Белые — обычные, зелёные — награды мини-игр, оранжевые — карты сокровищ или легендарные, синие — головоломки.",
                L"Показывать известные места появления мировых боссов.",
                L"Показывать внезапные задания согласно выбранному фильтру.",
                L"Показывать места поддерживаемых мини-игр.",
                L"Показывать задания области согласно выбранному фильтру.",
                L"Показывать загруженные поблизости птичьи яйца, доступные для сбора.",
                L"Показывать игровые часы на мини-карте.",
                L"Показывать значки сундуков в игровом мире.",
                L"Отмечать задания области серыми ромбами в игровом мире.",
                L"Отмечать мини-игры фиолетовыми флагами в игровом мире.",
                L"Показывать, находятся ли сундуки выше или ниже вас.",
                L"Показывать, находятся ли задания области выше или ниже вас.",
                L"Показывать, находятся ли мини-игры выше или ниже вас.",
                L"Показывать, находятся ли места появления боссов выше или ниже вас.",
                L"Показывать, находятся ли внезапные задания выше или ниже вас.",
                L"Показывать незавершённые задания области с выполненными условиями.",
                L"Показывать все незавершённые задания области независимо от условий.",
                L"Показывать внезапные задания, доступные сейчас.",
                L"Показывать все известные места внезапных заданий, включая пока недоступные.",
                L"Общая дальность включённых меток в мире. При 0 они скрыты.",
                L"Общий лимит меток в мире. 0 скрывает их; увеличение может снизить производительность.",
                L"Скрывать расстояния, оставляя метки в мире.",
                L"Наведите прицел на метку и немного задержите, чтобы увидеть расстояние.",
                L"Автоматически показывать расстояние до ближайшей к центру экрана метки, без частой смены цели.",
                L"Показывать расстояния для всех видимых меток в мире.",
                L"Выбрать язык интерфейса или использовать язык игры.",
                L"Показывает состояние радара: включён, выключен или ошибка.",
                L"Включить или выключить радар либо повторить попытку после ошибки.",
                L"Вернуть стандартные настройки отображения и использовать язык игры.",
                L"Открыть Nexus Mods для предложений или сообщений о проблемах.",
                L"Сохранить изменения и закрыть настройки.",
                L"Открыть Nexus Mods, чтобы проголосовать за этот мод в конкурсе «Мод месяца».",
                L"Показать или скрыть все категории радара, включая часы и птичьи яйца.",
                L"Показать или скрыть все поддерживаемые категории на карте.",
            },
        },
        {
            L"ไทย", L"ตั้งค่าเรดาร์", L"ภาษา", L"ใช้ภาษาของเกม",
            L"การแสดงเครื่องหมาย", L"เรดาร์", L"แผนที่", L"ฉาก",
            {L"นาฬิกา", L"สมบัติ", L"บอส", L"ภารกิจกะทันหัน",
             L"มินิเกม", L"เควสต์พื้นที่", L"ไข่นก"},
            L"ตัวบอกระดับความสูง", L"เฉพาะเรดาร์",
            {L"สมบัติ", L"เควสต์พื้นที่", L"มินิเกม", L"บอส", L"ภารกิจกะทันหัน"},
            L"โหมดตัวกรอง", L"พร้อมใช้งาน", L"ทั้งหมด", L"ปิด",
            L"สถานะ", L"ปิด", L"เปิด", L"ขัดข้อง", L"เปิดใช้",
            L"ปิดใช้", L"ลองใหม่", L"เสนอแนะ",
            L"เครื่องหมายในฉาก", L"ระยะแสดง", L"จำนวนสูงสุด", L"แสดงระยะทาง",
            {L"ไม่แสดง", L"เล็งเพื่อแสดง", L"โฟกัสอัตโนมัติ", L"แสดงทั้งหมด"},
            L"คืนค่าเริ่มต้น", L"ทั้งหมด",
            {
                L"แสดงหีบที่ยังไม่เปิด สีขาวคือทั่วไป เขียวคือรางวัลมินิเกม ส้มคือแผนที่สมบัติหรือตำนาน น้ำเงินคือปริศนา",
                L"แสดงตำแหน่งเวิลด์บอสที่ทราบ",
                L"แสดงตำแหน่งภารกิจกะทันหันตามตัวกรองที่เลือก",
                L"แสดงตำแหน่งมินิเกมที่รองรับ",
                L"แสดงตำแหน่งเควสต์พื้นที่ตามตัวกรองที่เลือก",
                L"แสดงไข่นกใกล้ตัวที่เกมโหลดแล้วและเก็บได้",
                L"แสดงนาฬิกาในเกมบนมินิแมป",
                L"แสดงสัญลักษณ์หีบในฉากเกม",
                L"แสดงเควสต์พื้นที่ด้วยรูปสี่เหลี่ยมข้าวหลามตัดสีเทาในฉาก",
                L"แสดงมินิเกมด้วยธงสีม่วงในฉาก",
                L"บอกว่าหีบอยู่สูงหรือต่ำกว่าคุณ",
                L"บอกว่าเควสต์พื้นที่อยู่สูงหรือต่ำกว่าคุณ",
                L"บอกว่ามินิเกมอยู่สูงหรือต่ำกว่าคุณ",
                L"บอกว่าตำแหน่งบอสอยู่สูงหรือต่ำกว่าคุณ",
                L"บอกว่าภารกิจกะทันหันอยู่สูงหรือต่ำกว่าคุณ",
                L"แสดงเควสต์พื้นที่ที่ยังไม่เสร็จและผ่านเงื่อนไขแล้ว",
                L"แสดงเควสต์พื้นที่ที่ยังไม่เสร็จทั้งหมด โดยไม่กรองเงื่อนไข",
                L"แสดงภารกิจกะทันหันที่เข้าร่วมได้ในขณะนี้",
                L"แสดงตำแหน่งภารกิจกะทันหันทั้งหมด รวมถึงที่ยังไม่พร้อม",
                L"ระยะร่วมกันของเครื่องหมายในฉากที่เปิดใช้ ตั้งเป็น 0 เพื่อซ่อน",
                L"จำนวนเครื่องหมายในฉากสูงสุด ตั้งเป็น 0 เพื่อซ่อน การเพิ่มจำนวนอาจกระทบประสิทธิภาพ",
                L"ซ่อนระยะทางแต่ยังแสดงเครื่องหมายในฉาก",
                L"เล็งใกล้เครื่องหมายแล้วหยุดสักครู่เพื่อดูระยะทาง",
                L"แสดงระยะของเครื่องหมายที่ใกล้กลางจอที่สุดอัตโนมัติ โดยไม่สลับเป้าหมายบ่อย",
                L"แสดงระยะทางของทุกเครื่องหมายในฉากที่มองเห็น",
                L"เลือกภาษาเมนู หรือใช้ภาษาเดียวกับเกม",
                L"แสดงสถานะเรดาร์ว่าเปิด ปิด หรือขัดข้อง",
                L"เปิดหรือปิดเรดาร์ หรือลองใหม่หลังเกิดข้อผิดพลาด",
                L"คืนค่าการแสดงผลเป็นค่าเริ่มต้นและใช้ภาษาเกม",
                L"เปิด Nexus Mods เพื่อเสนอแนะหรือแจ้งปัญหา",
                L"บันทึกการเปลี่ยนแปลงแล้วปิดการตั้งค่า",
                L"เปิด Nexus Mods เพื่อโหวตให้ม็อดนี้เป็นม็อดประจำเดือน",
                L"แสดงหรือซ่อนทุกหมวดบนเรดาร์ รวมถึงนาฬิกาและไข่นก",
                L"แสดงหรือซ่อนทุกหมวดที่รองรับบนแผนที่",
            },
        },
        {
            L"Português (Brasil)", L"Configurações do radar", L"Idioma",
            L"Idioma do jogo", L"Visibilidade dos marcadores", L"Radar", L"Mapa", L"Cena",
            {L"Relógio", L"Tesouros", L"Chefes", L"Eventos aleatórios", L"Minijogos",
             L"Missões de área", L"Ovos de pássaro"},
            L"Indicadores de altura", L"Apenas radar",
            {L"Tesouros", L"Missões de área", L"Minijogos", L"Chefes", L"Eventos aleatórios"},
            L"Modos de filtro", L"Disponíveis", L"Todos", L"Fechar",
            L"Status", L"Desligado", L"Ativo", L"Falha", L"Ativar",
            L"Desativar", L"Tentar de novo", L"Sugestões",
            L"Marcadores no mundo", L"Alcance", L"Limite total", L"Distâncias",
            {L"Ocultar", L"Ao mirar", L"Foco auto", L"Todas"},
            L"Restaurar padrões", L"Tudo",
            {
                L"Mostrar baús fechados. Branco: comuns; verde: recompensas de minijogos; laranja: mapas do tesouro ou lendários; azul: quebra-cabeças.",
                L"Mostrar os locais conhecidos dos chefes mundiais.",
                L"Mostrar eventos aleatórios conforme o filtro escolhido.",
                L"Mostrar os locais dos minijogos compatíveis.",
                L"Mostrar missões de área conforme o filtro escolhido.",
                L"Mostrar ovos coletáveis próximos que o jogo já carregou.",
                L"Mostrar o relógio do jogo no minimapa.",
                L"Mostrar ícones de baús no mundo do jogo.",
                L"Marcar missões de área com losangos cinza no mundo do jogo.",
                L"Marcar minijogos com bandeiras roxas no mundo do jogo.",
                L"Indicar se os baús estão acima ou abaixo de você.",
                L"Indicar se as missões de área estão acima ou abaixo de você.",
                L"Indicar se os minijogos estão acima ou abaixo de você.",
                L"Indicar se os locais dos chefes estão acima ou abaixo de você.",
                L"Indicar se os eventos aleatórios estão acima ou abaixo de você.",
                L"Mostrar missões de área pendentes com os pré-requisitos cumpridos.",
                L"Mostrar todas as missões de área pendentes, independentemente dos pré-requisitos.",
                L"Mostrar os eventos aleatórios disponíveis no momento.",
                L"Mostrar todos os locais de eventos aleatórios, incluindo os indisponíveis.",
                L"Alcance comum dos marcadores ativos no mundo. Em 0, eles ficam ocultos.",
                L"Limite total de marcadores no mundo. 0 os oculta; aumentar pode afetar o desempenho.",
                L"Ocultar distâncias e manter os marcadores no mundo.",
                L"Mire perto de um marcador por um instante para ver a distância.",
                L"Mostrar a distância do marcador mais próximo do centro da tela, sem trocar de alvo constantemente.",
                L"Mostrar distâncias para todos os marcadores visíveis no mundo.",
                L"Escolher o idioma da interface ou usar o idioma do jogo.",
                L"Mostra se o radar está ativo, desligado ou com erro.",
                L"Ativar ou desativar o radar, ou tentar novamente após um erro.",
                L"Restaurar os padrões de exibição e usar o idioma do jogo.",
                L"Abrir o Nexus Mods para enviar sugestões ou relatar problemas.",
                L"Salvar as alterações e fechar as configurações.",
                L"Abrir o Nexus Mods para votar neste mod como mod do mês.",
                L"Mostrar ou ocultar todas as categorias do radar, incluindo relógio e ovos.",
                L"Mostrar ou ocultar todas as categorias compatíveis do mapa.",
            },
        },
    }};

static_assert(kRadarLocalizedText.size() == kRadarUiLanguageCount);

[[nodiscard]] constexpr bool radar_localized_text_complete(
    const RadarLocalizedText& text) noexcept {
    const auto present = [](const wchar_t* value) constexpr noexcept {
        return value != nullptr && value[0] != L'\0';
    };
    if (!present(text.language_name) || !present(text.title)
        || !present(text.language) || !present(text.automatic)
        || !present(text.marker_visibility) || !present(text.radar)
        || !present(text.map) || !present(text.scene)
        || !present(text.height_indicators)
        || !present(text.radar_only) || !present(text.filter_modes)
        || !present(text.available) || !present(text.all)
        || !present(text.close) || !present(text.status)
        || !present(text.status_off) || !present(text.status_on)
        || !present(text.status_fault) || !present(text.enable_mod)
        || !present(text.disable_mod) || !present(text.retry_mod)
        || !present(text.bug_report) || !present(text.scene_settings)
        || !present(text.scene_range) || !present(text.scene_limit)
        || !present(text.scene_distance) || !present(text.restore_defaults)) {
        return false;
    }
    for (const wchar_t* value : text.marker_categories) {
        if (!present(value)) {
            return false;
        }
    }
    for (const wchar_t* value : text.scene_distance_modes) {
        if (!present(value)) return false;
    }
    for (const wchar_t* value : text.height_categories) {
        if (!present(value)) {
            return false;
        }
    }
    for (const wchar_t* value : text.tooltips) {
        if (!present(value)) return false;
    }
    return true;
}

[[nodiscard]] constexpr bool radar_localizations_complete() noexcept {
    for (const auto& text : kRadarLocalizedText) {
        if (!radar_localized_text_complete(text)) {
            return false;
        }
    }
    return true;
}

static_assert(radar_localizations_complete());

[[nodiscard]] constexpr const RadarLocalizedText& radar_localized_text(
    RadarUiLanguage language) noexcept {
    const std::size_t index = static_cast<std::size_t>(language);
    return kRadarLocalizedText[
        index < kRadarLocalizedText.size() ? index : 0U];
}

} // namespace dswros

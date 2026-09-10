#pragma once

#include <dswros/radar_preferences.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace dswros {

enum class RadarGuideRadarKind : std::uint8_t {
    Fly, Mole, Wave, Boss, Assault, AreaQuest, BirdEgg, Count,
};

inline constexpr std::size_t kRadarGuideRadarCount =
    static_cast<std::size_t>(RadarGuideRadarKind::Count);
inline constexpr std::size_t kRadarGuideStringCount = 44;

struct RadarGuideLocalizedText {
    const wchar_t* guide_button{};
    const wchar_t* settings_button{};
    const wchar_t* title{};
    const wchar_t* treasure_title{};
    // Ordinary, mini-game reward, treasure-map/legendary, puzzle.
    std::array<const wchar_t*, 4> treasure_labels{};
    const wchar_t* radar_title{};
    const wchar_t* radar_column{};
    const wchar_t* map_column{};
    // Keep in RadarGuideRadarKind order; renderer supplies each column's glyph.
    std::array<const wchar_t*, kRadarGuideRadarCount> radar_labels{};
    const wchar_t* clock_body{};
    const wchar_t* height_title{};
    std::array<const wchar_t*, 4> height_states{};
    std::array<const wchar_t*, 5> height_labels{};
    const wchar_t* height_body{};
    const wchar_t* scene_title{};
    // Treasure, Area Quest, Mini-game.
    std::array<const wchar_t*, 3> scene_labels{};
    const wchar_t* distance_title{};
    std::array<const wchar_t*, 4> distance_labels{};
    std::array<const wchar_t*, 4> distance_descriptions{};
    const wchar_t* scene_pointer_body{};
};

inline constexpr std::array<RadarGuideLocalizedText, kRadarUiLanguageCount>
    kRadarGuideLocalizedText{{
        {
            L"Guide",
            L"Settings",
            L"Guide",
            L"Chests",
            {L"Ordinary chest", L"Mini-game reward chest", L"Treasure map / legendary", L"Puzzle chest"},
            L"Activities and items",
            L"Radar",
            L"Map",
            {L"Flying mini-game", L"Marmot mini-game", L"Wave mini-game", L"World boss", L"Sudden mission", L"Area quest", L"Bird eggs"},
            L"Clock · In-game time; sun and moon show the time of day.",
            L"Height · Radar only",
            {L"Above", L"Near level", L"Below", L"Unknown"},
            {L"Chest", L"Mini-games", L"Area quest", L"World boss", L"Sudden mission"},
            L"Only the nearest chest and mini-game get height hints; other categories show each visible marker. Mini-game arrows use their activity color. Turning height hints off keeps normal icons.",
            L"In-world markers",
            {L"Chest", L"Area quest", L"Mini-game"},
            L"Distance labels",
            {L"Off", L"Aim focus", L"Auto focus", L"All"},
            {L"Hide distances.", L"Aim near a marker and pause to show its distance.", L"Keep one visible marker near screen center selected.", L"Show distances for all visible markers."},
            L"The small v belongs to the raised icon; it is not a height arrow.",
        },
        {
            L"ガイド",
            L"設定",
            L"ガイド",
            L"宝箱",
            {L"通常の宝箱", L"ミニゲーム報酬の宝箱", L"宝の地図・伝説の宝箱", L"謎解きの宝箱"},
            L"アクティビティとアイテム",
            L"レーダー",
            L"マップ",
            {L"飛行ミニゲーム", L"マーモットのミニゲーム", L"波のミニゲーム", L"ワールドボス", L"突発ミッション", L"エリアクエスト", L"鳥の卵"},
            L"時計 · ゲーム内時刻。太陽と月は時間帯を示します。",
            L"高低差 · レーダーのみ",
            {L"上", L"ほぼ同じ高さ", L"下", L"不明"},
            {L"宝箱", L"ミニゲーム", L"エリアクエスト", L"ワールドボス", L"突発ミッション"},
            L"宝箱・ミニゲームは最寄り、ほかは各マーカー。三角は種目の色。高低差表示をオフにすると通常のアイコンです。",
            L"画面内マーカー",
            {L"宝箱", L"エリアクエスト", L"ミニゲーム"},
            L"距離表示",
            {L"非表示", L"照準で表示", L"自動選択", L"すべて"},
            {L"距離を表示しません。", L"マーカー付近に照準を合わせ、少し待つと表示します。", L"画面中央に近い、表示中のマーカーを1つ選びます。", L"表示中のすべてのマーカーに距離を表示します。"},
            L"小さな v は浮かせたアイコンの一部で、高低差の矢印ではありません。",
        },
        {
            L"가이드",
            L"설정",
            L"가이드",
            L"보물상자",
            {L"일반 상자", L"미니게임 보상 상자", L"보물지도 · 전설 상자", L"퍼즐 상자"},
            L"활동과 아이템",
            L"레이더",
            L"지도",
            {L"비행 미니게임", L"마멋 미니게임", L"파도 미니게임", L"월드 보스", L"돌발 임무", L"지역 퀘스트", L"새알"},
            L"시계 · 게임 내 시간입니다. 해와 달은 시간대를 나타냅니다.",
            L"높이 · 레이더 전용",
            {L"위", L"비슷한 높이", L"아래", L"알 수 없음"},
            {L"보물상자", L"미니게임", L"지역 퀘스트", L"월드 보스", L"돌발 임무"},
            L"상자·미니게임은 가장 가까운 하나, 나머지는 각 마커에 표시합니다. 삼각형은 활동별 색을 따릅니다. 높이 표시를 끄면 기본 아이콘으로 표시됩니다.",
            L"화면 내 마커",
            {L"보물상자", L"지역 퀘스트", L"미니게임"},
            L"거리 표시",
            {L"숨김", L"조준 시 표시", L"자동 선택", L"모두"},
            {L"거리를 숨깁니다.", L"마커 근처를 조준하고 잠시 멈추면 표시합니다.", L"화면 중앙에 가까운 마커 하나를 선택합니다.", L"보이는 모든 마커에 거리를 표시합니다."},
            L"작은 v는 위로 띄운 아이콘의 일부이며, 높이 차이를 나타내지 않습니다.",
        },
        {
            L"指南",
            L"设置",
            L"指南",
            L"宝箱",
            {L"普通宝箱", L"小游戏奖励宝箱", L"藏宝图 / 传说宝箱", L"解谜宝箱"},
            L"活动与物品",
            L"雷达",
            L"地图",
            {L"飞行小游戏", L"土拨鼠小游戏", L"波浪小游戏", L"世界首领", L"突发任务", L"区域任务", L"鸟蛋"},
            L"时钟 · 游戏内时间；太阳和月亮表示时段。",
            L"高度提示 · 仅雷达",
            {L"上方", L"高度接近", L"下方", L"未知"},
            {L"宝箱", L"小游戏", L"区域任务", L"世界首领", L"突发任务"},
            L"宝箱、小游戏只提示最近一个；其余提示每个可见标记。三角颜色随小游戏种类变化；关闭高度提示时保留普通图标。",
            L"场景标记",
            {L"宝箱", L"区域任务", L"小游戏"},
            L"距离显示",
            {L"不显示", L"瞄准显示", L"自动聚焦", L"全部显示"},
            {L"隐藏距离。", L"对准标记附近并稍作停留，显示其距离。", L"持续选择靠近画面中央的一个可见标记。", L"显示所有可见标记的距离。"},
            L"小 v 是抬高图标的一部分，不表示高低差。",
        },
        {
            L"指南",
            L"設定",
            L"指南",
            L"寶箱",
            {L"普通寶箱", L"小遊戲獎勵寶箱", L"藏寶圖 / 傳說寶箱", L"解謎寶箱"},
            L"活動與物品",
            L"雷達",
            L"地圖",
            {L"飛行小遊戲", L"土撥鼠小遊戲", L"波浪小遊戲", L"世界首領", L"突發任務", L"區域任務", L"鳥蛋"},
            L"時鐘 · 遊戲內時間；太陽和月亮表示時段。",
            L"高度提示 · 僅雷達",
            {L"上方", L"高度接近", L"下方", L"未知"},
            {L"寶箱", L"小遊戲", L"區域任務", L"世界首領", L"突發任務"},
            L"寶箱、小遊戲只提示最近一個；其餘提示每個可見標記。三角顏色隨小遊戲種類變化；關閉高度提示時保留普通圖示。",
            L"場景標記",
            {L"寶箱", L"區域任務", L"小遊戲"},
            L"距離顯示",
            {L"不顯示", L"瞄準顯示", L"自動聚焦", L"全部顯示"},
            {L"隱藏距離。", L"對準標記附近並稍作停留，顯示其距離。", L"持續選擇靠近畫面中央的一個可見標記。", L"顯示所有可見標記的距離。"},
            L"小 v 是抬高圖示的一部分，不表示高低差。",
        },
        {
            L"Guide",
            L"Réglages",
            L"Guide",
            L"Coffres",
            {L"Coffre ordinaire", L"Récompense de mini-jeu", L"Carte au trésor / légendaire", L"Coffre d'énigme"},
            L"Activités et objets",
            L"Radar",
            L"Carte",
            {L"Mini-jeu de vol", L"Mini-jeu de marmottes", L"Mini-jeu de vagues", L"Boss mondial", L"Mission soudaine", L"Quête de zone", L"Œufs d'oiseaux"},
            L"Horloge · Heure du jeu ; le soleil et la lune indiquent le moment de la journée.",
            L"Hauteur · Radar uniquement",
            {L"Au-dessus", L"Même hauteur", L"En dessous", L"Inconnue"},
            {L"Coffre", L"Mini-jeux", L"Quête de zone", L"Boss mondial", L"Mission soudaine"},
            L"Coffre et mini-jeu les plus proches ; chaque autre repère. Triangle de la couleur du mini-jeu. Hauteur désactivée : icônes normales.",
            L"Repères dans le monde",
            {L"Coffre", L"Quête de zone", L"Mini-jeu"},
            L"Distances",
            {L"Masquées", L"Visée", L"Auto", L"Toutes"},
            {L"Masquer les distances.", L"Visez près d'un repère et marquez une courte pause.", L"Garder un repère visible proche du centre sélectionné.", L"Afficher la distance de chaque repère visible."},
            L"Le petit v fait partie de l'icône surélevée ; il n'indique pas la hauteur.",
        },
        {
            L"Hilfe",
            L"Einstellungen",
            L"Hilfe",
            L"Truhen",
            {L"Normale Truhe", L"Minispiel-Belohnung", L"Schatzkarte / legendär", L"Rätseltruhe"},
            L"Aktivitäten und Gegenstände",
            L"Radar",
            L"Karte",
            {L"Flug-Minispiel", L"Murmeltier-Minispiel", L"Wellen-Minispiel", L"Weltboss", L"Spontane Mission", L"Gebietsquest", L"Vogeleier"},
            L"Uhr · Spielzeit; Sonne und Mond zeigen die Tageszeit.",
            L"Höhe · Nur Radar",
            {L"Oberhalb", L"Ähnliche Höhe", L"Unterhalb", L"Unbekannt"},
            {L"Truhe", L"Minispiele", L"Gebietsquest", L"Weltboss", L"Spontane Mission"},
            L"Truhen/Minispiele: nächstes Ziel; sonst jede Markierung. Dreiecke in Aktivitätsfarbe. Höhenanzeige aus: normale Symbole.",
            L"Markierungen in der Welt",
            {L"Truhe", L"Gebietsquest", L"Minispiel"},
            L"Entfernungen",
            {L"Aus", L"Beim Zielen", L"Automatisch", L"Alle"},
            {L"Entfernungen ausblenden.", L"Kurz auf die Nähe einer Markierung zielen.", L"Eine sichtbare Markierung nahe der Bildmitte auswählen.", L"Entfernungen aller sichtbaren Markierungen anzeigen."},
            L"Das kleine v gehört zum angehobenen Symbol und zeigt keinen Höhenunterschied.",
        },
        {
            L"Guía",
            L"Ajustes",
            L"Guía",
            L"Cofres",
            {L"Cofre normal", L"Recompensa de minijuego", L"Mapa del tesoro / legendario", L"Cofre de puzle"},
            L"Actividades y objetos",
            L"Radar",
            L"Mapa",
            {L"Minijuego de vuelo", L"Minijuego de marmotas", L"Minijuego de olas", L"Jefe de mundo", L"Misión repentina", L"Misión de zona", L"Huevos de ave"},
            L"Reloj · Hora del juego; el sol y la luna indican el momento del día.",
            L"Altura · Solo radar",
            {L"Encima", L"Altura similar", L"Debajo", L"Desconocida"},
            {L"Cofre", L"Minijuegos", L"Misión de zona", L"Jefe de mundo", L"Misión repentina"},
            L"Solo el cofre/minijuego más cercano; los demás, cada marcador. Triángulo del color del minijuego. Altura desactivada: iconos normales.",
            L"Marcadores del mundo",
            {L"Cofre", L"Misión de zona", L"Minijuego"},
            L"Distancias",
            {L"Ocultas", L"Al apuntar", L"Auto", L"Todas"},
            {L"Ocultar las distancias.", L"Apunta cerca de un marcador y espera un instante.", L"Mantener seleccionado un marcador cercano al centro.", L"Mostrar la distancia de todos los marcadores visibles."},
            L"La v pequeña forma parte del icono elevado; no indica la altura.",
        },
        {
            L"Справка",
            L"Настройки",
            L"Справка",
            L"Сундуки",
            {L"Обычный сундук", L"Награда за мини-игру", L"Карта сокровищ / легендарный", L"Сундук с головоломкой"},
            L"Активности и предметы",
            L"Радар",
            L"Карта",
            {L"Мини-игра с полётами", L"Мини-игра с сурками", L"Мини-игра с волнами", L"Мировой босс", L"Внезапное задание", L"Задание области", L"Птичьи яйца"},
            L"Часы · Игровое время; солнце и луна обозначают время суток.",
            L"Высота · Только радар",
            {L"Выше", L"Близкая высота", L"Ниже", L"Неизвестно"},
            {L"Сундук", L"Мини-игры", L"Задание области", L"Мировой босс", L"Внезапное задание"},
            L"Ближайший сундук/мини-игра; остальные — каждая метка. Треугольник в цвет мини-игры. Высота выключена: обычные значки.",
            L"Метки в мире",
            {L"Сундук", L"Задание области", L"Мини-игра"},
            L"Расстояния",
            {L"Скрыть", L"При наведении", L"Авто", L"Все"},
            {L"Скрывать расстояния.", L"Наведите прицел рядом с меткой и ненадолго задержитесь.", L"Выбирать видимую метку вблизи центра экрана.", L"Показывать расстояния у всех видимых меток."},
            L"Маленькая v относится к приподнятому значку и не обозначает высоту.",
        },
        {
            L"คู่มือ",
            L"การตั้งค่า",
            L"คู่มือ",
            L"หีบสมบัติ",
            {L"หีบทั่วไป", L"หีบรางวัลมินิเกม", L"แผนที่สมบัติ / หีบตำนาน", L"หีบปริศนา"},
            L"กิจกรรมและไอเทม",
            L"เรดาร์",
            L"แผนที่",
            {L"มินิเกมการบิน", L"มินิเกมมาร์มอต", L"มินิเกมคลื่น", L"เวิลด์บอส", L"ภารกิจกะทันหัน", L"เควสต์พื้นที่", L"ไข่นก"},
            L"นาฬิกา · เวลาในเกม ดวงอาทิตย์และดวงจันทร์บอกช่วงเวลา",
            L"ความสูง · เฉพาะเรดาร์",
            {L"สูงกว่า", L"ใกล้เคียงกัน", L"ต่ำกว่า", L"ไม่ทราบ"},
            {L"หีบ", L"มินิเกม", L"เควสต์พื้นที่", L"เวิลด์บอส", L"ภารกิจกะทันหัน"},
            L"หีบและมินิเกมแสดงจุดที่ใกล้ที่สุด ส่วนอื่นแสดงทุกจุด สามเหลี่ยมใช้สีตามมินิเกม ปิดความสูงแล้วใช้ไอคอนปกติ",
            L"เครื่องหมายในฉาก",
            {L"หีบ", L"เควสต์พื้นที่", L"มินิเกม"},
            L"ระยะทาง",
            {L"ไม่แสดง", L"เมื่อเล็ง", L"อัตโนมัติ", L"ทั้งหมด"},
            {L"ซ่อนระยะทาง", L"เล็งใกล้เครื่องหมายแล้วหยุดสักครู่เพื่อแสดงระยะ", L"เลือกเครื่องหมายที่มองเห็นใกล้กลางจอหนึ่งจุด", L"แสดงระยะของทุกเครื่องหมายที่มองเห็น"},
            L"ตัว v เล็กเป็นส่วนหนึ่งของไอคอนที่ยกสูงขึ้น ไม่ได้บอกความต่างของความสูง",
        },
        {
            L"Guia",
            L"Ajustes",
            L"Guia",
            L"Baús",
            {L"Baú comum", L"Recompensa de minijogo", L"Mapa do tesouro / lendário", L"Baú de quebra-cabeça"},
            L"Atividades e itens",
            L"Radar",
            L"Mapa",
            {L"Minijogo de voo", L"Minijogo de marmotas", L"Minijogo de ondas", L"Chefe mundial", L"Evento aleatório", L"Missão de área", L"Ovos de pássaro"},
            L"Relógio · Hora do jogo; o sol e a lua indicam o período do dia.",
            L"Altura · Apenas radar",
            {L"Acima", L"Altura próxima", L"Abaixo", L"Desconhecida"},
            {L"Baú", L"Minijogos", L"Missão de área", L"Chefe mundial", L"Evento aleatório"},
            L"Baú/minijogo mais próximo; os demais, cada marcador. Triângulo na cor do minijogo. Altura desativada: ícones normais.",
            L"Marcadores no mundo",
            {L"Baú", L"Missão de área", L"Minijogo"},
            L"Distâncias",
            {L"Ocultar", L"Ao mirar", L"Auto", L"Todas"},
            {L"Ocultar as distâncias.", L"Mire perto de um marcador e pare por um instante.", L"Manter um marcador visível próximo ao centro selecionado.", L"Mostrar a distância de todos os marcadores visíveis."},
            L"O pequeno v faz parte do ícone elevado; ele não indica a altura.",
        },
    }};

[[nodiscard]] constexpr bool radar_guide_localizations_complete() noexcept {
    const auto present = [](const wchar_t* value) constexpr noexcept {
        return value != nullptr && value[0] != L'\0';
    };
    for (const auto& text : kRadarGuideLocalizedText) {
        if (!present(text.guide_button) || !present(text.settings_button)
            || !present(text.title) || !present(text.treasure_title)
            || !present(text.radar_title) || !present(text.radar_column)
            || !present(text.map_column) || !present(text.clock_body)
            || !present(text.height_title)
            || !present(text.height_body) || !present(text.scene_title)
            || !present(text.distance_title)
            || !present(text.scene_pointer_body)) return false;
        for (const auto* value : text.treasure_labels) if (!present(value)) return false;
        for (const auto* value : text.radar_labels) if (!present(value)) return false;
        for (const auto* value : text.scene_labels) if (!present(value)) return false;
        for (const auto* value : text.height_states) if (!present(value)) return false;
        for (const auto* value : text.height_labels) if (!present(value)) return false;
        for (const auto* value : text.distance_labels) if (!present(value)) return false;
        for (const auto* value : text.distance_descriptions) if (!present(value)) return false;
    }
    return true;
}

static_assert(kRadarGuideRadarCount == 7);
static_assert(kRadarGuideLocalizedText.size() == kRadarUiLanguageCount);
static_assert(radar_guide_localizations_complete());

[[nodiscard]] constexpr const RadarGuideLocalizedText&
radar_guide_text(RadarUiLanguage language) noexcept {
    const auto index = static_cast<std::size_t>(language);
    return kRadarGuideLocalizedText[index < kRadarUiLanguageCount ? index : 0];
}

} // namespace dswros

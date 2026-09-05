#pragma once

#include <dswros/radar_preferences.hpp>

#include <array>
#include <cstddef>

namespace dswros {

struct RadarLocalizedText {
    const wchar_t* language_name{};
    const wchar_t* title{};
    const wchar_t* language{};
    const wchar_t* automatic{};
    const wchar_t* marker_visibility{};
    const wchar_t* radar{};
    const wchar_t* map{};
    std::array<const wchar_t*, 7> marker_categories{};
    const wchar_t* height_indicators{};
    const wchar_t* radar_only{};
    std::array<const wchar_t*, 3> height_categories{};
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
};

inline constexpr std::array<RadarLocalizedText, kRadarUiLanguageCount>
    kRadarLocalizedText{{
        {
            L"English", L"RADAR SETTINGS", L"LANGUAGE", L"USE GAME LANGUAGE",
            L"MARKER VISIBILITY", L"RADAR", L"MAP",
            {L"CLOCK", L"TREASURE", L"BOSS", L"SUDDEN MISSIONS",
             L"MINI-GAMES", L"AREA QUESTS", L"BIRD EGGS"},
            L"HEIGHT INDICATORS", L"RADAR ONLY",
            {L"TREASURE", L"AREA QUEST", L"MOLE GAME"},
            L"FILTER MODES", L"AVAILABLE", L"ALL", L"CLOSE",
            L"STATUS", L"OFF", L"ON", L"FAULT", L"ENABLE",
            L"DISABLE", L"RETRY", L"BUG REPORT",
        },
        {
            L"日本語", L"レーダー設定", L"言語", L"ゲーム言語を使用",
            L"マーカー表示", L"レーダー", L"マップ",
            {L"時計", L"宝箱", L"ボス", L"突発ミッション", L"ミニゲーム",
             L"エリアクエスト", L"鳥の卵"},
            L"高さインジケーター", L"レーダーのみ",
            {L"宝箱", L"エリアクエスト", L"モグラゲーム"},
            L"表示モード", L"利用可能", L"すべて", L"閉じる",
            L"状態", L"オフ", L"オン", L"異常", L"有効",
            L"無効", L"再試行", L"不具合報告",
        },
        {
            L"한국어", L"레이더 설정", L"언어", L"게임 언어 사용",
            L"마커 표시", L"레이더", L"지도",
            {L"시계", L"보물", L"보스", L"돌발 임무", L"미니게임",
             L"지역 퀘스트", L"새알"},
            L"높이 표시", L"레이더 전용",
            {L"보물", L"지역 퀘스트", L"두더지 게임"},
            L"필터 모드", L"이용 가능", L"전체", L"닫기",
            L"상태", L"꺼짐", L"켜짐", L"오류", L"켜기",
            L"끄기", L"재시도", L"버그 신고",
        },
        {
            L"简体中文", L"雷达设置", L"语言", L"跟随游戏语言",
            L"标记显示", L"雷达", L"地图",
            {L"时钟", L"宝箱", L"世界BOSS", L"突发任务", L"小游戏",
             L"区域任务", L"鸟蛋"},
            L"高度指示器", L"仅雷达",
            {L"宝箱", L"区域任务", L"土拨鼠游戏"},
            L"筛选模式", L"可用", L"全部", L"关闭",
            L"模组状态", L"未开启", L"已开启", L"故障", L"开启",
            L"关闭", L"重试", L"问题反馈",
        },
        {
            L"繁體中文", L"雷達設定", L"語言", L"跟隨遊戲語言",
            L"標記顯示", L"雷達", L"地圖",
            {L"時鐘", L"寶箱", L"首領", L"突發任務", L"小遊戲",
             L"區域任務", L"鳥蛋"},
            L"高度指示器", L"僅雷達",
            {L"寶箱", L"區域任務", L"土撥鼠遊戲"},
            L"篩選模式", L"可用", L"全部", L"關閉",
            L"模組狀態", L"未啟用", L"已啟用", L"故障", L"啟用",
            L"停用", L"重試", L"問題回報",
        },
        {
            L"Français", L"PARAMÈTRES DU RADAR", L"LANGUE", L"LANGUE DU JEU",
            L"VISIBILITÉ DES MARQUEURS", L"RADAR", L"CARTE",
            {L"HORLOGE", L"TRÉSORS", L"BOSS", L"MISSIONS IMPRÉVUES", L"MINI-JEUX",
             L"QUÊTES DE ZONE", L"ŒUFS D'OISEAU"},
            L"INDICATEURS DE HAUTEUR", L"RADAR UNIQUEMENT",
            {L"TRÉSOR", L"QUÊTE DE ZONE", L"JEU DE TAUPE"},
            L"MODES DE FILTRAGE", L"DISPONIBLES", L"TOUS", L"FERMER",
            L"ÉTAT", L"ARRÊT", L"ACTIF", L"ERREUR", L"ACTIVER",
            L"DÉSACT.", L"RÉESSAYER", L"SIGNALER",
        },
        {
            L"Deutsch", L"RADAR-EINSTELLUNGEN", L"SPRACHE", L"SPIELSPRACHE",
            L"MARKIERUNGSANZEIGE", L"RADAR", L"KARTE",
            {L"UHR", L"SCHÄTZE", L"BOSSE", L"SPONTANMISSIONEN", L"MINISPIELE",
             L"GEBIETSQUESTS", L"VOGELEIER"},
            L"HÖHENANZEIGEN", L"NUR RADAR",
            {L"SCHATZ", L"GEBIETSQUEST", L"MAULWURFSPIEL"},
            L"FILTERMODI", L"VERFÜGBAR", L"ALLE", L"SCHLIESSEN",
            L"STATUS", L"AUS", L"AN", L"FEHLER", L"AKTIVIEREN",
            L"DEAKTIV.", L"NEU VERSUCH", L"MELDEN",
        },
        {
            L"Español (España)", L"AJUSTES DEL RADAR", L"IDIOMA",
            L"IDIOMA DEL JUEGO", L"VISIBILIDAD DE MARCADORES", L"RADAR", L"MAPA",
            {L"RELOJ", L"TESOROS", L"JEFES", L"MISIÓN REPENTINA", L"MINIJUEGOS",
             L"MISIONES DE ZONA", L"HUEVOS DE AVE"},
            L"INDICADORES DE ALTURA", L"SOLO RADAR",
            {L"TESORO", L"MISIÓN DE ZONA", L"JUEGO DEL TOPO"},
            L"MODOS DE FILTRO", L"DISPONIBLES", L"TODOS", L"CERRAR",
            L"ESTADO", L"APAGADO", L"ACTIVO", L"ERROR", L"ACTIVAR",
            L"DESACTIVAR", L"REINTENTAR", L"INFORMAR",
        },
        {
            L"Русский", L"НАСТРОЙКИ РАДАРА", L"ЯЗЫК", L"ЯЗЫК ИГРЫ",
            L"ОТОБРАЖЕНИЕ МЕТОК", L"РАДАР", L"КАРТА",
            {L"ЧАСЫ", L"СОКРОВИЩА", L"БОССЫ", L"ВНЕЗАПНЫЕ ЗАДАНИЯ",
             L"МИНИ-ИГРЫ", L"ЗАДАНИЯ ОБЛАСТИ", L"ПТИЧЬИ ЯЙЦА"},
            L"УКАЗАТЕЛИ ВЫСОТЫ", L"ТОЛЬКО РАДАР",
            {L"СОКРОВИЩЕ", L"ЗАДАНИЕ ОБЛАСТИ", L"ИГРА С КРОТОМ"},
            L"РЕЖИМЫ ФИЛЬТРА", L"ДОСТУПНЫЕ", L"ВСЕ", L"ЗАКРЫТЬ",
            L"СТАТУС", L"ВЫКЛ", L"ВКЛ", L"ОШИБКА", L"ВКЛЮЧИТЬ",
            L"ВЫКЛЮЧИТЬ", L"ПОВТОРИТЬ", L"СООБЩИТЬ",
        },
        {
            L"ไทย", L"ตั้งค่าเรดาร์", L"ภาษา", L"ใช้ภาษาของเกม",
            L"การแสดงเครื่องหมาย", L"เรดาร์", L"แผนที่",
            {L"นาฬิกา", L"สมบัติ", L"บอส", L"ภารกิจกะทันหัน",
             L"มินิเกม", L"เควสต์พื้นที่", L"ไข่นก"},
            L"ตัวบอกระดับความสูง", L"เฉพาะเรดาร์",
            {L"สมบัติ", L"เควสต์พื้นที่", L"เกมตัวตุ่น"},
            L"โหมดตัวกรอง", L"พร้อมใช้งาน", L"ทั้งหมด", L"ปิด",
            L"สถานะ", L"ปิด", L"เปิด", L"ขัดข้อง", L"เปิดใช้",
            L"ปิดใช้", L"ลองใหม่", L"รายงานบั๊ก",
        },
        {
            L"Português (Brasil)", L"CONFIGURAÇÕES DO RADAR", L"IDIOMA",
            L"IDIOMA DO JOGO", L"VISIBILIDADE DOS MARCADORES", L"RADAR", L"MAPA",
            {L"RELÓGIO", L"TESOUROS", L"CHEFES", L"EVENTOS ALEATÓRIOS", L"MINIJOGOS",
             L"MISSÕES DE ÁREA", L"OVOS DE PÁSSARO"},
            L"INDICADORES DE ALTURA", L"APENAS RADAR",
            {L"TESOURO", L"MISSÃO DE ÁREA", L"JOGO DA TOUPEIRA"},
            L"MODOS DE FILTRO", L"DISPONÍVEIS", L"TODOS", L"FECHAR",
            L"STATUS", L"DESLIGADO", L"ATIVO", L"FALHA", L"ATIVAR",
            L"DESATIVAR", L"TENTAR", L"RELATAR",
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
        || !present(text.map) || !present(text.height_indicators)
        || !present(text.radar_only) || !present(text.filter_modes)
        || !present(text.available) || !present(text.all)
        || !present(text.close) || !present(text.status)
        || !present(text.status_off) || !present(text.status_on)
        || !present(text.status_fault) || !present(text.enable_mod)
        || !present(text.disable_mod) || !present(text.retry_mod)
        || !present(text.bug_report)) {
        return false;
    }
    for (const wchar_t* value : text.marker_categories) {
        if (!present(value)) {
            return false;
        }
    }
    for (const wchar_t* value : text.height_categories) {
        if (!present(value)) {
            return false;
        }
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

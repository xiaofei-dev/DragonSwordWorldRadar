#pragma once

#include <dswros/radar_preferences.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

namespace dswros {

enum class RadarConfirmationTextId : std::uint8_t {
    Endorse, Title, EndorseBody, FeedbackBody, RestoreBody, Yes, No, Count,
};

inline constexpr std::size_t kRadarConfirmationTextCount = 7;
static_assert(static_cast<std::size_t>(RadarConfirmationTextId::Count) ==
              kRadarConfirmationTextCount);

inline constexpr std::array<std::array<const wchar_t*, kRadarConfirmationTextCount>,
                            kRadarUiLanguageCount> kRadarConfirmationText{{
    {{L"Vote for this mod", L"Confirm",
      L"Open Nexus Mods to vote for this mod?",
      L"Open Nexus Mods to send feedback or report an issue?",
      L"Reset display settings to defaults?",
      L"Yes", L"No"}},
    {{L"このModに投票", L"確認",
      L"このModに投票するため、Nexus Modsを開きますか？",
      L"ご意見や不具合を投稿するため、Nexus Modsを開きますか？",
      L"表示設定を初期値に戻しますか？",
      L"はい", L"いいえ"}},
    {{L"이 모드에 투표", L"확인",
      L"이 모드에 투표하려면 Nexus Mods를 열까요?",
      L"의견이나 문제를 남기려면 Nexus Mods를 열까요?",
      L"표시 설정을 기본값으로 되돌릴까요?",
      L"예", L"아니요"}},
    {{L"为此模组投票", L"确认",
      L"打开 Nexus Mods，为此模组投票？",
      L"打开 Nexus Mods，提交建议或问题？",
      L"恢复默认显示设置？",
      L"是", L"否"}},
    {{L"為此模組投票", L"確認",
      L"開啟 Nexus Mods，為此模組投票？",
      L"開啟 Nexus Mods，提交建議或問題？",
      L"恢復預設顯示設定？",
      L"是", L"否"}},
    {{L"Voter pour ce mod", L"Confirmation",
      L"Ouvrir Nexus Mods pour voter pour ce mod ?",
      L"Ouvrir Nexus Mods pour proposer une idée ou signaler un problème ?",
      L"Rétablir les paramètres d'affichage par défaut ?",
      L"Oui", L"Non"}},
    {{L"Für diesen Mod abstimmen", L"Bestätigung",
      L"Nexus Mods öffnen, um für diesen Mod abzustimmen?",
      L"Nexus Mods für Vorschläge oder Problemmeldungen öffnen?",
      L"Anzeigeeinstellungen auf Standardwerte zurücksetzen?",
      L"Ja", L"Nein"}},
    {{L"Votar por este mod", L"Confirmación",
      L"¿Abrir Nexus Mods para votar por este mod?",
      L"¿Abrir Nexus Mods para enviar sugerencias o informar de un problema?",
      L"¿Restablecer los ajustes de visualización predeterminados?",
      L"Sí", L"No"}},
    {{L"Голосовать за мод", L"Подтверждение",
      L"Открыть Nexus Mods, чтобы проголосовать за этот мод?",
      L"Открыть Nexus Mods для отзывов и сообщений о проблемах?",
      L"Восстановить настройки отображения по умолчанию?",
      L"Да", L"Нет"}},
    {{L"โหวตให้ม็อดนี้", L"ยืนยัน",
      L"เปิด Nexus Mods เพื่อโหวตให้ม็อดนี้หรือไม่?",
      L"เปิด Nexus Mods เพื่อเสนอแนะหรือแจ้งปัญหาหรือไม่?",
      L"คืนค่าการแสดงผลเป็นค่าเริ่มต้นหรือไม่?",
      L"ใช่", L"ไม่"}},
    {{L"Votar neste mod", L"Confirmação",
      L"Abrir o Nexus Mods para votar neste mod?",
      L"Abrir o Nexus Mods para enviar sugestões ou relatar um problema?",
      L"Restaurar as configurações padrão de exibição?",
      L"Sim", L"Não"}},
}};

[[nodiscard]] inline constexpr const std::array<const wchar_t*, kRadarConfirmationTextCount>&
radar_confirmation_text(RadarUiLanguage language) noexcept {
    const auto index = static_cast<std::size_t>(language);
    return kRadarConfirmationText[index < kRadarUiLanguageCount ? index : 0];
}

} // namespace dswros

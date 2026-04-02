#include "tvweather/Localization.h"

namespace tvweather {

namespace {

const UiStrings kEnglishStrings {
    "~W~eather",
    "~P~ick Location...",
    "Active ~W~arnings",
    "E~x~it",
    "~O~ptions",
    "~P~references...",
    "~Ctrl-Q~ Exit",
    "Options",
    "Language",
    "~E~nglish",
    "~P~ortuguese",
    "~O~K",
    "Pick Location",
    "~O~pen Forecast",
    "~C~ancel",
    "Date",
    "Min",
    "Max",
    "Rain",
    "Wind",
    "Conditions",
    "Active Warnings",
    "No active warnings.",
    "From:",
    "To:"
};

const UiStrings kPortugueseStrings {
    "~M~eteorologia",
    "~E~scolher Localidade...",
    "Avisos ~A~tivos",
    "Sai~r~",
    "~O~pcoes",
    "~P~referencias...",
    "~Ctrl-Q~ Sair",
    "Opcoes",
    "Idioma",
    "~I~ngles",
    "~P~ortugues",
    "~O~K",
    "Escolher Localidade",
    "~A~brir Previsao",
    "~C~ancelar",
    "Data",
    "Min",
    "Max",
    "Chuva",
    "Vento",
    "Condicoes",
    "Avisos Ativos",
    "Sem avisos ativos.",
    "De:",
    "Ate:"
};

} // namespace

const UiStrings& uiStrings(Language language) noexcept
{
    return language == Language::Portuguese
        ? kPortugueseStrings
        : kEnglishStrings;
}

std::string forecastWindowTitle(const std::string& localName, Language language)
{
    const char* suffix = language == Language::Portuguese
        ? " - Previsao de 5 dias"
        : " - 5-Day Forecast";
    return localName + suffix;
}

std::string weatherDescriptionForType(const std::vector<IpmaWeatherType>& types,
                                      int id,
                                      Language language)
{
    for (const auto& type : types) {
        if (type.id != id)
            continue;

        if (language == Language::Portuguese) {
            if (!type.descPT.empty())
                return type.descPT;
            if (!type.descEN.empty())
                return type.descEN;
        } else {
            if (!type.descEN.empty())
                return type.descEN;
            if (!type.descPT.empty())
                return type.descPT;
        }
        break;
    }
    return "-";
}

const char* languageCode(Language language) noexcept
{
    return language == Language::Portuguese ? "pt" : "en";
}

Language languageFromCodeOrDefault(const std::string& code) noexcept
{
    return code == "pt" ? Language::Portuguese : Language::English;
}

} // namespace tvweather

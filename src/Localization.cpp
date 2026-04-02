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
    "Overview",
    "Details",
    "Temperature",
    "Rain risk",
    "Wind strength",
    "Navigate",
    "No forecast data available.",
    "Active Warnings",
    "No active warnings.",
    "From:",
    "To:",
    "Could not fetch data. Check your internet connection.",
    "~F2~ Refresh",
    "~U~V Index...",
    "UV Index",
    "No UV data available.",
    "Sea ~C~onditions...",
    " - Sea Conditions",
    "No sea forecast data available.",
    "Wave",
    "Period",
    "Dir",
    "SST"
};

const UiStrings kPortugueseStrings {
    "~M~eteorologia",
    "~E~scolher Localidade...",
    "Avisos ~A~tivos",
    "Sai~r~",
    "~O~pções",
    "~P~referências...",
    "~Ctrl-Q~ Sair",
    "Opções",
    "Idioma",
    "~I~nglês",
    "~P~ortuguês",
    "~O~K",
    "Escolher Localidade",
    "~A~brir Previsão",
    "~C~ancelar",
    "Data",
    "Min",
    "Max",
    "Chuva",
    "Vento",
    "Condições",
    "Resumo",
    "Detalhes",
    "Temperatura",
    "Risco de chuva",
    "Intensidade do vento",
    "Navegar",
    "Sem dados de previsão.",
    "Avisos Ativos",
    "Sem avisos ativos.",
    "De:",
    "Até:",
    "Não foi possível obter dados. Verifique a sua ligação à internet.",
    "~F2~ Actualizar",
    "Índice ~U~V...",
    "Índice UV",
    "Sem dados de índice UV.",
    "~C~ondições do Mar...",
    " - Condições do Mar",
    "Sem previsão marítima.",
    "Ondas",
    "Período",
    "Dir",
    "TSM"
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
        ? " - Previsão de 5 dias"
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

const char* rainRiskLabel(int precipitaProb, Language language) noexcept
{
    if (precipitaProb >= 70)
        return language == Language::Portuguese ? "Alto" : "High";
    if (precipitaProb >= 30)
        return language == Language::Portuguese ? "Moderado" : "Moderate";
    return language == Language::Portuguese ? "Baixo" : "Low";
}

const char* windStrengthLabel(int classWindSpeed, Language language) noexcept
{
    if (classWindSpeed >= 4)
        return language == Language::Portuguese ? "Forte" : "Strong";
    if (classWindSpeed >= 2)
        return language == Language::Portuguese ? "Moderado" : "Moderate";
    return language == Language::Portuguese ? "Fraco" : "Light";
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

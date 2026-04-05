#include "tvweather/Localization.h"

namespace tvweather {

namespace {

const UiStrings kEnglishStrings {
    "~W~eather",
    "~F~orecast...",
    "~C~lear Location",
    "Active ~W~arnings",
    "E~x~it",
    "~O~ptions",
    "~P~references...",
    "~C~olors...",
    "~Ctrl-Q~ Exit",
    "Options",
    "~F8~ Clear Location",
    "Location: ",
    "Location: none selected",
    "Language",
    "~E~nglish",
    "~P~ortuguese",
    "~O~K",
    "~R~eset",
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
    "Feels like",
    "Humidity",
    "Rain risk",
    "Wind strength",
    "Wind speed",
    "Direction",
    "Hour",
    "Hourly",
    "Selected hour",
    "Updated",
    "Hours shown in UTC.",
    "Warnings",
    "No active warnings for this location.",
    "UV",
    "Sea state",
    "Wave dir",
    "Wave period",
    "Peak period",
    "Water temp",
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
    "SST",
    "Sea conditions are shown for the closest coastal location.",
    "Location",
    "Distance from selected location"
};

const UiStrings kPortugueseStrings {
    "~M~eteorologia",
    "~P~revisão...",
    "~L~impar Localidade",
    "Avisos ~A~tivos",
    "Sai~r~",
    "~O~pções",
    "~P~referências...",
    "~C~ores...",
    "~Ctrl-Q~ Sair",
    "Opções",
    "~F8~ Limpar Localidade",
    "Localidade: ",
    "Localidade: nenhuma seleccionada",
    "Idioma",
    "~I~nglês",
    "~P~ortuguês",
    "~O~K",
    "~R~epor",
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
    "Sensação térmica",
    "Humidade",
    "Risco de chuva",
    "Intensidade do vento",
    "Velocidade do vento",
    "Direção",
    "Hora",
    "Horária",
    "Hora selecionada",
    "Atualizado",
    "Horas apresentadas em UTC.",
    "Avisos",
    "Sem avisos ativos para esta localidade.",
    "UV",
    "Estado do mar",
    "Dir. ondulação",
    "Período ondulação",
    "Período de pico",
    "Temp. água",
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
    "TSM",
    "As condições do mar são mostradas para a localidade costeira mais próxima.",
    "Localidade",
    "Distância desde a localidade seleccionada"
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
        ? " - Previsão horária"
        : " - Hourly Forecast";
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

std::string windTypeDescriptionForId(const std::vector<IpmaWindType>& types,
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

    return windStrengthLabel(id, language);
}

std::string windDirectionLabelForCode(const std::vector<IpmaWindDirectionType>& types,
                                      const std::string& code,
                                      Language language)
{
    for (const auto& type : types) {
        if (type.id != code)
            continue;

        if (language == Language::Portuguese) {
            if (!type.siglaPT.empty())
                return type.siglaPT;
            if (!type.labelPT.empty())
                return type.labelPT;
        } else {
            if (!type.siglaEN.empty())
                return type.siglaEN;
            if (!type.labelEN.empty())
                return type.labelEN;
        }
        break;
    }

    return code.empty() ? "-" : code;
}

std::string windDirectionDescriptionForCode(
    const std::vector<IpmaWindDirectionType>& types,
    const std::string& code,
    Language language)
{
    for (const auto& type : types) {
        if (type.id != code)
            continue;

        if (language == Language::Portuguese) {
            if (!type.descPT.empty())
                return type.descPT;
            if (!type.labelPT.empty())
                return type.labelPT;
        } else {
            if (!type.descEN.empty())
                return type.descEN;
            if (!type.labelEN.empty())
                return type.labelEN;
        }
        break;
    }

    return code.empty() ? "-" : code;
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

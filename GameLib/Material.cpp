/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2018-01-21
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Material.h"

#include "Utils.h"

#include <sstream>

std::unique_ptr<Material> Material::Create(picojson::object const & materialJson)
{
	std::string name = Utils::GetOptionalJsonMember<std::string>(materialJson, "name", "Unspecified");

    picojson::object massJson = Utils::GetMandatoryJsonObject(materialJson, "mass");
    float mass = static_cast<float>(Utils::GetMandatoryJsonMember<double>(massJson, "nominal_mass"))
               * static_cast<float>(Utils::GetOptionalJsonMember<double>(massJson, "density", 1.0));

    float strength = static_cast<float>(Utils::GetOptionalJsonMember<double>(materialJson, "strength", 1.0));
    float stiffness = static_cast<float>(Utils::GetOptionalJsonMember<double>(materialJson, "stiffness", 1.0));
	
    std::array<uint8_t, 3u> structuralColourRgb = Hex2RgbColour(Utils::GetMandatoryJsonMember<std::string>(materialJson, "structural_colour"));
    std::array<uint8_t, 3u> renderColourRgb = Hex2RgbColour(Utils::GetMandatoryJsonMember<std::string>(materialJson, "render_colour"));
	bool isHull = Utils::GetMandatoryJsonMember<bool>(materialJson, "is_hull");
    bool isRope = Utils::GetOptionalJsonMember<bool>(materialJson, "is_rope", false);

	std::optional<ElectricalProperties> electricalProperties;
	std::optional<picojson::object> electricalPropertiesJson = Utils::GetOptionalJsonObject(materialJson, "electrical_properties");
	if (!!electricalPropertiesJson)
	{
		std::string elementTypeStr = Utils::GetMandatoryJsonMember<std::string>(*electricalPropertiesJson, "element_type");
        ElectricalProperties::ElectricalElementType elementType = ElectricalProperties::StrToElectricalElementType(elementTypeStr);

		float resistance = static_cast<float>(Utils::GetOptionalJsonMember<double>(*electricalPropertiesJson, "resistance", 0.0));
		float generatedVoltage = static_cast<float>(Utils::GetOptionalJsonMember<double>(*electricalPropertiesJson, "generated_voltage", 0.0));

		electricalProperties.emplace(
			elementType,
			resistance,
			generatedVoltage);
	}

    std::optional<SoundProperties> soundProperties;
    std::optional<picojson::object> soundPropertiesJson = Utils::GetOptionalJsonObject(materialJson, "sound_properties");
    if (!!soundPropertiesJson)
    {        
        std::string elementTypeStr = Utils::GetMandatoryJsonMember<std::string>(*soundPropertiesJson, "element_type");
        SoundProperties::SoundElementType elementType = SoundProperties::StrToSoundElementType(elementTypeStr);

        soundProperties.emplace(
            elementType);
    }

	return std::unique_ptr<Material>(
		new Material(
			name,
			strength,
			mass,
            stiffness,
            structuralColourRgb,
            renderColourRgb,
			isHull,
            isRope,
			electricalProperties,
            soundProperties));
}

uint8_t Material::Hex2Byte(std::string const & str)
{
    std::stringstream ss;
    ss << std::hex << str;
    int x;
    ss >> x;

    return static_cast<uint8_t>(x);
}

std::array<uint8_t, 3u> Material::Hex2RgbColour(std::string str)    //  e.g. "#00FF00";
{
    if (str[0] == '#')
        str = str.substr(1);

    if (str.length() != 6)
        throw GameException("Error: badly formed hex colour value \"" + str + "\"");

    std::array<uint8_t, 3u> rgbColour;
    rgbColour[0] = Hex2Byte(str.substr(0, 2));
    rgbColour[1] = Hex2Byte(str.substr(2, 2));
    rgbColour[2] = Hex2Byte(str.substr(4, 2));

    return rgbColour;
}

vec3f Material::RgbToVec(std::array<uint8_t, 3u> const & rgbColour)    
{
    return vec3f(
        rgbColour[0] / 255.f,
        rgbColour[1] / 255.f,
        rgbColour[2] / 255.f);
}

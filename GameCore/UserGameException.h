/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-10-08
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameException.h"

#include <string>
#include <vector>

class UserGameException : public GameException
{
public:

	enum class MessageIdType
	{
		UnrecognizedShipFile = 1,
		InvalidShipFile,
		UnsupportedShipFile,
		LoadShipMaterialNotFoundLaterVersion,
		LoadShipMaterialNotFoundSameVersion
	};

	MessageIdType const MessageId;
	std::vector<std::string> const Parameters;

	UserGameException(MessageIdType messageId)
		: UserGameException(
			messageId,
			{})
	{}

	UserGameException(
		MessageIdType messageId,
		std::vector<std::string> && parameters)
		: GameException("MESSAGE ID " + std::to_string(static_cast<int>(messageId)))
		, MessageId(messageId)
		, Parameters(std::move(parameters))
	{}
};
/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IUserInterface.h"
#include "Model.h"
#include "ShipBuilderTypes.h"
#include "View.h"

#include <memory>

namespace ShipBuilder {

/*
 * This class implements operations on the model.
 */
class ModelController
{
public:

    static std::unique_ptr<ModelController> CreateNew(
        WorkSpaceSize const & workSpaceSize,
        IUserInterface & userInterface,
        View & view);

    WorkSpaceSize const & GetWorkSpaceSize() const
    {
        return mModel.GetWorkSpaceSize();
    }

private:

    ModelController(
        WorkSpaceSize const & workSpaceSize,
        IUserInterface & userInterface,
        View & view);

private:

    IUserInterface & mUserInterface;
    View & mView;

    Model mModel;
};

}
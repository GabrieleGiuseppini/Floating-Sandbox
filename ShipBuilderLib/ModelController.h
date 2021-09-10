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

#include <filesystem>
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
        View & view,
        IUserInterface & userInterface);

    static std::unique_ptr<ModelController> CreateFromLoad(
        std::filesystem::path const & shipFilePath,
        View & view,
        IUserInterface & userInterface);

    void NewStructuralLayer();
    void SetStructuralLayer(/*TODO*/);

    void NewElectricalLayer();
    void SetElectricalLayer(/*TODO*/);
    void RemoveElectricalLayer();

    void NewRopesLayer();
    void SetRopesLayer(/*TODO*/);
    void RemoveRopesLayer();

    void NewTextureLayer();
    void SetTextureLayer(/*TODO*/);
    void RemoveTextureLayer();

    Model const & GetModel() const
    {
        return mModel;
    }

    WorkSpaceSize const & GetWorkSpaceSize() const
    {
        return mModel.GetWorkSpaceSize();
    }

private:

    ModelController(
        WorkSpaceSize const & workSpaceSize,
        View & view,
        IUserInterface & userInterface);

private:

    View & mView;
    IUserInterface & mUserInterface;

    Model mModel;
};

}
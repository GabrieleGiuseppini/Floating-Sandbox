/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-03
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameParameters.h"

#include <GameCore/GameTypes.h>
#include <GameCore/Vectors.h>

/*
 * The interface presented by the Ship to its subordinate elements.
 */
struct IShipPhysicsHandler
{
    enum class ElectricalElementDestroySpecializationType
    {
        None,
        Lamp,
        LampExplosion,
        LampImplosion,
        SilentRemoval
    };

    //
    // Structure
    //

    /*
     * Invoked whenever a point is detached.
     *
     * The handler is invoked right before the point is modified for the detachment. However,
     * other elements connected to the soon-to-be-detached point might already have been
     * deleted.
     *
     * The handler is not re-entrant: detaching other points from it is not supported
     * and leads to undefined behavior.
     */
    virtual void HandlePointDetach(
        ElementIndex pointElementIndex,
        bool generateDebris,
        bool fireDestroyEvent,
        float currentSimulationTime,
        GameParameters const & gameParameters) = 0;

    /*
     * Invoked whenever a point is irrevocably modified, including when it is being detached,
     * but also in other situations.
     *
     * The dual of this is assumed to be handled by HandlePointRestored().
     */
    virtual void HandlePointDamaged(ElementIndex pointElementIndex) = 0;

    /*
     * Invoked whenever an ephemeral particle is destroyed.
     *
     * The handler is invoked right before the particle is modified for the destroy.
     *
     * The handler is not re-entrant: destroying other ephemeral particles from it is not supported
     * and leads to undefined behavior.
     */
    virtual void HandleEphemeralParticleDestroy(
        ElementIndex pointElementIndex) = 0;

    /*
     * Invoked whenever a point is restored.
     *
     * The handler is invoked right after the point is modified for the restore.
     *
     * The repair tool will invoke this only after connected springs and triangles
     * have also been restored.
     */
    virtual void HandlePointRestore(ElementIndex pointElementIndex) = 0;

    /*
     * Invoked whenever a spring is destroyed.
     *
     * The handler is invoked right before the spring is marked as deleted. However,
     * other elements connected to the soon-to-be-deleted spring might already have been
     * deleted.
     *
     * The handler is not re-entrant: destroying other springs from it is not supported
     * and leads to undefined behavior.
     */
    virtual void HandleSpringDestroy(
        ElementIndex springElementIndex,
        bool destroyAllTriangles,
        GameParameters const & gameParameters) = 0;

    /*
     * Invoked whenever a spring is restored.
     *
     * The handler is invoked right after the spring is unmarked as deleted. However,
     * other elements connected to the soon-to-be-deleted spring might not yet have been
     * restored.
     *
     * The handler is not re-entrant: restoring other springs from it is not supported
     * and leads to undefined behavior.
     */
    virtual void HandleSpringRestore(
        ElementIndex springElementIndex,
        GameParameters const & gameParameters) = 0;

    /*
     * Invoked whenever a triangle is destroyed.
     *
     * The handler is invoked right before the triangle is marked as deleted. However,
     * other elements connected to the soon-to-be-deleted triangle might already have been
     * deleted.
     *
     * The handler is not re-entrant: destroying other triangles from it is not supported
     * and leads to undefined behavior.
     */
    virtual void HandleTriangleDestroy(ElementIndex triangleElementIndex) = 0;

    /*
     * Invoked whenever a triangle is restored.
     *
     * The handler is invoked right after the triangle is modified to be restored. However,
     * other elements connected to the soon-to-be-restored triangle might not have been
     * restored yet.
     *
     * The handler is not re-entrant: restoring other triangles from it is not supported
     * and leads to undefined behavior.
     */
    virtual void HandleTriangleRestore(ElementIndex triangleElementIndex) = 0;

    /*
     * Invoked whenever an electrical element is destroyed.
     *
     * The handler is invoked right before the electrical element is marked as deleted. However,
     * other elements connected to the soon-to-be-deleted electrical element might already have been
     * deleted.
     *
     * The handler is not re-entrant: destroying other electrical elements from it is not supported
     * and leads to undefined behavior.
     */
    virtual void HandleElectricalElementDestroy(
        ElementIndex electricalElementIndex,
        ElementIndex pointIndex,
        ElectricalElementDestroySpecializationType specialization,
        float currentSimulationTime,
        GameParameters const & gameParameters) = 0;

    /*
     * Invoked whenever an electrical element is restored.
     *
     * The handler is invoked right after the element is modified to be restored. However,
     * other elements connected to the soon-to-be-restored element might not have been
     * restored yet.
     *
     * The handler is not re-entrant: restoring other elements from it is not supported
     * and leads to undefined behavior.
     */
    virtual void HandleElectricalElementRestore(ElementIndex electricalElementIndex) = 0;

    //
    // Misc
    //

    virtual void StartExplosion(
        float currentSimulationTime,
        PlaneId planeId,
        vec2f const & centerPosition,
        float blastRadius, // m
        float blastForce, // N
        float blastHeat, // KJ
        float renderRadiusOffset, // On top of blast radius
        ExplosionType explosionType,
        GameParameters const & gameParameters) = 0;

    virtual void DoAntiMatterBombPreimplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        float radius,
        GameParameters const & gameParameters) = 0;

    virtual void DoAntiMatterBombImplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        GameParameters const & gameParameters) = 0;

    virtual void DoAntiMatterBombExplosion(
        vec2f const & centerPosition,
        float sequenceProgress,
        GameParameters const & gameParameters) = 0;

    virtual void HandleWatertightDoorUpdated(
        ElementIndex pointElementIndex,
        bool isOpen) = 0;

    virtual void HandleElectricSpark(
        ElementIndex pointElementIndex,
        float strength,
        float currentSimulationTime,
        GameParameters const & gameParameters) = 0;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogInteractionLab, Log, All);

/**
 * Project trace channel for interaction focus and line-of-sight checks (Project Settings > Collision).
 * Defined by the game, not the plugin, because which GameTraceChannel slot is free is project-specific.
 */
#define ECC_Interaction ECC_GameTraceChannel2
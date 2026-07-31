#pragma once

#include "GpuTypes.h"

struct HALCYON_API GPU_ALIGN(16) Particle
{
	GPU_ALIGN(16) float3 position;
	GPU_ALIGN(4) uint seed;
	GPU_ALIGN(16) float3 scale;
	GPU_ALIGN(4) float liveTime;
	GPU_ALIGN(16) float3 rotation;
	GPU_ALIGN(4) uint emiterIndex;
	GPU_ALIGN(16) float4 color;
}; // optimise to hold less storage

struct HALCYON_API GPU_ALIGN(16) EmiterData
{
	GPU_ALIGN(4) bool active;
	GPU_ALIGN(4) uint spawnCount;
	GPU_ALIGN(16) float3 initialPosition;
	GPU_ALIGN(16) float3 directionalVector;
	GPU_ALIGN(8) float2 spawnRadius; // 1-min, 2-max
	GPU_ALIGN(8) float2 timeToLive; // 1-min, 2-max
	GPU_ALIGN(8) float2 velocity; // 1-min, 2-max
	GPU_ALIGN(8) float2 scale; // 1-min, 2-max
	GPU_ALIGN(16) float4 colorStart;
	GPU_ALIGN(16) float4 colorEnd;
};

struct HALCYON_API GPU_ALIGN(16) ParticlesMetadata
{
	GPU_ALIGN(4) uint bottomOfStack;
	GPU_ALIGN(4) uint maxNumberOfPatricles;
	GPU_ALIGN(4) uint numberOfEmiters;
};
# UE5 RDG Render Plugin

## Overview

This is a UE5.5 render plugin built with:

- Render Dependency Graph (RDG)
- Global Shader
- Scene View Extension
- Slate runtime debug panel

The plugin was developed as a focused graphics-learning project and implements a small but complete screen-space post-process pipeline.

## Implemented Features

- Debug views:
  - Depth
  - Normal
  - Roughness
  - SceneColor
- Screen-space filters:
  - Gaussian Blur
  - Bilateral Blur
  - Normal-aware Bilateral Blur
- SSAO pipeline:
  - Raw SSAO
  - Denoised SSAO
  - AO Composite
- Runtime tuning window:
  - mode switching
  - blur parameters
  - SSAO parameters
  - AO composite parameters

## Main Tech Points

- Plugin-based render feature integration in UE5
- RDG pass scheduling and intermediate texture flow
- GBuffer / SceneDepth sampling
- depth reconstruction
- hemisphere kernel generation
- screen-space occlusion estimation
- bilateral denoising
- post-process AO compositing

## Project Structure

- `RDGStarter.uplugin`: plugin descriptor
- `Source/`: C++ module, shader parameter setup, RDG pass orchestration, Slate debug UI
- `Shaders/`: HLSL shader files
- `docs/`: learning notes and implementation walkthroughs

## Notes

- This repository is intended to showcase the render plugin itself, not the full host game project.
- Build outputs such as `Binaries/` and `Intermediate/` are intentionally excluded.

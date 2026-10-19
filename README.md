# spectral-ray-tracer
A minimalist C++ spectral path tracer based on RTIOW, featuring Cauchy dispersion and CUDA-ready structs.
基于 RTIOW 改造的极简 C++ 光谱路径追踪器，包含柯西色散与面向CUDA的struct设计。
## preview/预览
<img width="1260" height="709" alt="preview2" src="https://github.com/user-attachments/assets/763a6817-6614-4723-9095-3a94720bdec5" />
<img width="1260" height="709" alt="preview1" src="https://github.com/user-attachments/assets/3cf967f3-9939-4b4d-9591-3e34022cc87d" />
<img width="1260" height="709" alt="ecbe21d283776bfcf69a70ad25159540" src="https://github.com/user-attachments/assets/25f15ae4-aeee-4907-bbfc-85a6d821ba36" />

## deepwiki
[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/mocatante/spectral-ray-tracer)

## Features/特性
- **Spectral Rendering**:95 channels (360nm–830nm).
  光谱渲染：95 个通道( 360nm–830nm )
- **Cauchy Dispersion**: Accurate glass refraction.
  利用柯西色散实现色散效果
- **CUDA Ready**: All primitives and materials use `struct`.
  面向 CUDA, 使用struct设计, 方便未来迁移至 GPU

## Build & Run / 编译运行
Requires C++17 and OpenMP.Or you can just delete the line at camera.h
```
#pragma omp parallel for schedule(dynamic, 1)
```
需要 C++17 及 OpenMP 支持。


## Design Choice/设计选择

### No Complex Optimizations/未做复杂优化
Diffuse surfaces use simple uniform hemisphere sampling instead of cosine-weighted importance sampling. 
No Russian Roulette.
To keep the codebase logically simple and straightforward, which makes it much easier to port to CUDA in the future. 
Performance is not the primary goal here.
为了保持代码逻辑简单直观, 方便以后直接迁移到 CUDA
这个项目的重点在于验证光谱渲染的可行性, 性能不是第一位

### The kB Matrix/关于 kB 矩阵
Converting 3 RGB values to a 95-channel spectrum is a highly underdetermined problem.
Instead of manually solving linear equations, I wrote a quick PyTorch script (rgbToLambda.py) and let gradient descent figure it out.
It balances the conflicting constraints of albedo [0,1] and emission >1. 
This introduces a slight cyan tint, but it's good enough for visualization.
将 3 个 RGB 值转换为 95 个波长的光谱是典型的欠定问题
理论上应该算方程组可行解, 在挑选最优解. 但我选择用梯度下降的方式快速训练一个.
为了同时兼容反照率 [0,1] 和发光度 >1, 模型做了妥协, 导致画面微微泛青, 但是渲染效果看够用

## Credits/致谢
This project is heavily inspired by the book series *Ray Tracing in One Weekend* by Peter Shirley.
Peter Shirley, Trevor David Black, Steve Hollasch.
*Ray Tracing in One Weekend*.
Version 4.0.2, 2025-04-25.
https://raytracing.github.io/books/RayTracingInOneWeekend.html
